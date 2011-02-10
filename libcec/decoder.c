/*
 * decoder - CEC message decoding
 *
 * Copyright (c) 2010 Pete Batard <pbatard@gmail.com>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "libceci.h"
#include "decoder.h"

static uint8_t msg_min_max[15][2] = {
	{ 0, 0},	// 0
	{ 1, 1},	// 1
	{ 2, 2},	// 2
	{ 3, 3},	// 3
	{ 4, 4},	// 4
	{ 5, 8},	// 5
	{ 1, 3},	// 6
	{ 7, 7},	// 7
	{ 4, 8},	// 8
	{ 9, 10},	// 9
	{ 2, 14},	// A
	{ 11, 11},	// B
	{ 1, 14},	// C
	{ 3, 17},	// D
	{ 14, 14},	// E
};

/*
 * bit[0-4] = msg_min_max index
 * bit[5] = directly addressed
 * bit[6] = broadcast
 * bit[7] = opcode not in use
 */
static uint8_t msg_props[256] = {
//	 0    1    2    3    4    5    6    7    8    9    A    B    C    D    E    F
	0x22,0xff,0xff,0xff,0x20,0x20,0x20,0x25,0x21,0x28,0x21,0x20,0xff,0x20,0xff,0x20,  //  0
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x21,0x21,0xff,0xff,0xff,0xff,  //  1
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,  //  2
	0xff,0xff,0x43,0x2b,0x2b,0x26,0x60,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,  //  3
	0xff,0x21,0x21,0x21,0x21,0x20,0x20,0x2c,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,  //  4
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,  //  5
	0xff,0xff,0xff,0xff,0x2a,0xff,0xff,0x2c,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,  //  6
	0x22,0x20,0x61,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x21,0xff,0xff,0x20,0x21,0xff,  //  7
	0x44,0x42,0x42,0x20,0x43,0x40,0x42,0x43,0xff,0x2c,0x6c,0x60,0x20,0x21,0x21,0x20,  //  8
	0x21,0x20,0x24,0x27,0xff,0xff,0xff,0x2e,0xff,0x2e,0x21,0xff,0xff,0x22,0x21,0x20,  //  9
	0x6d,0x29,0x29,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,  //  A
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,  //  B
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,  //  C
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,  //  D
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,  //  E
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x20,  //  F
};

// TODO: add an option to not include these messages
static char* msg_description[63] = {
	"*Unsupported Opcode*",			// N/A		0
	"Feature Abort",				// 0x00		1
	"Image View On",				// 0x04		2
	"Tuner Step Increment",			// 0x05		3
	"Tuner Step Decrement",			// 0x06		4
	"Tuner Device Status",			// 0x07		5
	"Give Tuner Device Status",		// 0x08		6
	"Record On",					// 0x09		7
	"Record Status",				// 0x0A		8
	"Record Off",					// 0x0B		9
	"Text View On",					// 0x0D		10
	"Record TV Screen",				// 0x0F		11
	"Give Deck Status",				// 0x1A		12
	"Deck Status",					// 0x1B		13
	"Set Menu Language",			// 0x32		14
	"Clear Analogue Timer",			// 0x33		15
	"Set Analogue Timer",			// 0x34		16
	"Timer Status",					// 0x35		17
	"Standby",						// 0x36		18
	"Play",							// 0x41		19
	"Deck Control",					// 0x42		20
	"Timer Cleared Status",			// 0x43		21
	"User Control Pressed",			// 0x44		22
	"User Control Released",		// 0x45		23
	"Give OSD Name",				// 0x46		24
	"Set OSD Name",					// 0x47		25
	"Set OSD String",				// 0x64		26
	"Set Timer Program Title",		// 0x67		27
	"System Audio Mode Request",	// 0x70		28
	"Give Audio Status",			// 0x71		29
	"Set System Audio Mode",		// 0x72		30
	"Report Audio Status",			// 0x7A		31
	"Give System Audio Mode Status",// 0x7D		32
	"System Audio Mode Status",		// 0x7E		33
	"Routing Change",				// 0x80		34
	"Routing Information",			// 0x81		35
	"Active Source",				// 0x82		36
	"Give Physical Address",		// 0x83		37
	"Report Physical Address",		// 0x84		38
	"Request Active Source",		// 0x85		39
	"Set Stream Path",				// 0x86		40
	"Device Vendor ID",				// 0x87		41
	"Vendor Command",				// 0x89		42
	"Vendor Remote Button Down",	// 0x8A		43
	"Vendor Remote Button Up",		// 0x8B		44
	"Give Device Vendor ID",		// 0x8C		45
	"Menu Request",					// 0x8D		46
	"Menu Status",					// 0x8E		47
	"Give Device Power Status",		// 0x8F		48
	"Report Power Status",			// 0x90		49
	"Get Menu Language",			// 0x91		50
	"Select Analogue Service",		// 0x92		51
	"Select Digital Service",		// 0x93		52
	"Set Digital Timer",			// 0x97		53
	"Clear Digital Timer",			// 0x99		54
	"Set Audio Rate",				// 0x9A		55
	"Inactive Source",				// 0x9D		56
	"CEC Version",					// 0x9E		57
	"Get CEC Version",				// 0x9F		58
	"Vendor Command With ID",		// 0xA0		59
	"Clear External Timer",			// 0xA1		60
	"Set External Timer",			// 0xA2		61
	"Abort",						// 0xFF		62
};

static uint8_t msg_index[256] = {
//	 0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
	01,00,00,00,02,03,04,05,06,07, 8, 9,00,10,00,11,  //  0
	00,00,00,00,00,00,00,00,00,00,12,13,00,00,00,00,  //  1
	00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,  //  2
	00,00,14,15,16,17,18,00,00,00,00,00,00,00,00,00,  //  3
	00,19,20,21,22,23,24,25,00,00,00,00,00,00,00,00,  //  4
	00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,  //  5
	00,00,00,00,26,00,00,27,00,00,00,00,00,00,00,00,  //  6
	28,29,30,00,00,00,00,00,00,00,31,00,00,32,33,00,  //  7
	34,35,36,37,38,39,40,41,00,42,43,44,45,46,47,48,  //  8
	49,50,51,52,00,00,00,53,00,54,55,00,00,56,57,58,  //  9
	59,60,61,00,00,00,00,00,00,00,00,00,00,00,00,00,  //  A
	00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,  //  B
	00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,  //  C
	00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,  //  D
	00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,  //  E
	00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,62,  //  F
};

static void display_buffer_hex(const char *function, uint8_t *buffer, size_t length)
{
	size_t i;

	if (ceci_global_log_level > LIBCEC_LOG_LEVEL_INFO) {
		return;
	}

	for (i=0; i<length; i++) {
		if (!(i%0x10))
			fprintf(ceci_logger, "                        libcec:info [%s]    ", function);
		fprintf(ceci_logger, " %02X", buffer[i]);
	}
	fprintf(ceci_logger, "\n");
	fflush(ceci_logger);
}


/*
 * Display a human readable version of a message in the log
 */
DEFAULT_VISIBILITY
int libcec_decode_message(uint8_t* message, size_t length)
{
	uint8_t src, dst;

	if ((message == NULL) || (length < 2)) {
		return LIBCEC_ERROR_INVALID_PARAM;
	}

	if (msg_props[message[1]] & 0x80) {
		ceci_warn("unsupported Opcode: %02X", message[1]);
		return LIBCEC_ERROR_NOT_SUPPORTED;
	}
	if ( (length-2 < msg_min_max[msg_props[message[1]]&0x1F][0])
	  || (length-2 > msg_min_max[msg_props[message[1]]&0x1F][1]) ) {
		  ceci_warn("invalid payload length for opcode: %02X", message[1]);
		  return LIBCEC_ERROR_INVALID_PARAM;
	}
	ceci_info("  o %1X->%1X: <%s>", src = message[0] >> 4, dst = message[0] & 0x0F,
			  msg_description[msg_index[message[1]]]);
	display_buffer_hex(__FUNCTION__, message, length);

	return LIBCEC_SUCCESS;
}
