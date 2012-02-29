/*
 * decoder - CEC message decoding
 *
 * Copyright (c) 2010-2011 Pete Batard <pete@akeo.ie>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __LIBCEC_DECODER_H__
#define __LIBCEC_DECODER_H__

#include <stdint.h>

/*
 * As of v1.3a, this is the maximum number of bytes a
 * CEC command can have (including the address and opcode)
 */
#define CEC_MAX_COMMAND_SIZE 16

/*
 * Typedefs for operands
 */
typedef uint16_t cec_op_physical_address;
typedef uint8_t  cec_op_deck_control_mode;
typedef uint8_t  cec_op_deck_info;
typedef uint8_t  cec_op_status_request;
typedef uint8_t  cec_op_play_mode;
typedef uint8_t  cec_op_cec_version;
typedef uint8_t  cec_op_menu_request_type;
typedef uint8_t  cec_op_menu_state;
typedef uint8_t  cec_op_ui_command;
typedef uint8_t  cec_op_power_status;
typedef uint8_t  cec_op_audio_status;
typedef uint8_t  cec_op_system_audio_status;
typedef uint8_t  cec_op_audio_rate;

typedef uint8_t  cec_op_vendor_id[3];
typedef uint8_t  cec_op_vendor_specific[15];	/* also used for rc_code */
/* The ASCII structures below are padded with an extra char for NUL termination */
typedef char     cec_op_osd_name[15];
typedef char     cec_op_program_title[15];
typedef char     cec_op_menu_language[4];

typedef struct {
	cec_op_physical_address original_address;
	cec_op_physical_address new_address;
} cec_op_routing_change_addresses;

typedef struct {
	uint16_t transport_stream_id;
	uint16_t service_id;
	uint16_t original_network_id;
} cec_op_arib_dvb_data;

typedef struct {
	uint16_t transport_stream_id;
	uint16_t program_number;
	uint16_t reserved;
} cec_op_atsc_data;

typedef struct {
	uint16_t channel_number_high;
	uint16_t channel_number_low;
} cec_op_channel_data;

typedef struct {
	uint8_t method_and_broadcast;
	union {
		cec_op_arib_dvb_data	arib;
		cec_op_atsc_data		atsc;
		cec_op_arib_dvb_data	dvb;
		cec_op_channel_data		channel;
	} service;
} cec_op_digital_service;

typedef struct {
	uint8_t  broadcast_type;
	uint16_t frequency;
	uint8_t  broadcast_system;
} cec_op_analogue_service;

typedef struct {
	uint8_t record_source_type;
	union {
		cec_op_digital_service	digital;
		cec_op_analogue_service	analogue;
		uint8_t					plug;
		uint16_t				physical_address;
	} source;
} cec_op_record_source;

typedef struct {
	uint8_t day;
	uint8_t month;
	struct {
		uint8_t hour;
		uint8_t minute;
	} start_time;
	struct {
		uint8_t hours;
		uint8_t minutes;
	} duration;
	uint8_t recording_sequence;
	cec_op_analogue_service analogue;
} cec_op_analogue_timer;

typedef struct {
	uint8_t day;
	uint8_t month;
	struct {
		uint8_t hour;
		uint8_t minute;
	} start_time;
	struct {
		uint8_t hours;
		uint8_t minutes;
	} duration;
	uint8_t recording_sequence;
	cec_op_digital_service digital;
} cec_op_digital_timer;

typedef struct {
	uint8_t day;
	uint8_t month;
	struct {
		uint8_t hour;
		uint8_t minute;
	} start_time;
	struct {
		uint8_t hours;
		uint8_t minutes;
	} duration;
	uint8_t recording_sequence;
	uint8_t source_specifier;
	union {
		uint8_t  plug;
		uint16_t physical_address;
	} source;
} cec_op_external_timer;

typedef struct {
	uint8_t info;
	uint16_t duration_available;
} cec_op_timer_status_data;

typedef struct {
	uint16_t physical_address;
	uint8_t device_type;
} cec_op_physical_address_report;

typedef struct {
	uint8_t tuner_info;
	union {
		cec_op_digital_service	digital;
		cec_op_analogue_service	analogue;
	} source;
} cec_op_tuner_device_info;

typedef struct {
	cec_op_vendor_id id;;
	cec_op_vendor_specific data;
} cec_op_vendor_command_with_id;

typedef struct {
	uint8_t display_control;
	uint8_t osd_string[14];
} cec_op_osd_string;

typedef struct {
	uint8_t feature_opcode;
	uint8_t abort_reason;
} cec_op_abort;

/*
 * Feature Opcode / CEC commands
 */
#define CEC_OP_FEATURE_ABORT				0x00
#define CEC_OP_IMAGE_VIEW_ON				0x04
#define CEC_OP_TUNER_STEP_INCREMENT			0x05
#define CEC_OP_TUNER_STEP_DECREMENT			0x06
#define CEC_OP_TUNER_DEVICE_STATUS			0x07
#define CEC_OP_GIVE_TUNER_DEVICE_STATUS		0x08
#define CEC_OP_RECORD_ON					0x09
#define CEC_OP_RECORD_STATUS				0x0A
#define CEC_OP_RECORD_OFF					0x0B
#define CEC_OP_TEXT_VIEW_ON					0x0D
#define CEC_OP_RECORD_TV_SCREEN				0x0F
#define CEC_OP_GIVE_DECK_STATUS				0x1A
#define CEC_OP_DECK_STATUS					0x1B
#define CEC_OP_SET_MENU_LANGUAGE			0x32
#define CEC_OP_CLEAR_ANALOGUE_TIMER			0x33
#define CEC_OP_SET_ANALOGUE_TIMER			0x34
#define CEC_OP_TIMER_STATUS					0x35
#define CEC_OP_STANDBY						0x36
#define CEC_OP_PLAY							0x41
#define CEC_OP_DECK_CONTROL					0x42
#define CEC_OP_TIMER_CLEARED_STATUS			0x43
#define CEC_OP_USER_CONTROL_PRESSED			0x44
#define CEC_OP_USER_CONTROL_RELEASED		0x45
#define CEC_OP_GIVE_OSD_NAME				0x46
#define CEC_OP_SET_OSD_NAME					0x47
#define CEC_OP_SET_OSD_STRING				0x64
#define CEC_OP_SET_TIMER_PROGRAM_TITLE		0x67
#define CEC_OP_SYSTEM_AUDIO_MODE_REQUEST	0x70
#define CEC_OP_GIVE_AUDIO_STATUS			0x71
#define CEC_OP_SET_SYSTEM_AUDIO_MODE		0x72
#define CEC_OP_REPORT_AUDIO_STATUS			0x7A
#define CEC_OP_GIVE_SYSTEM_AUDIO_MODE_STATUS 0x7D
#define CEC_OP_SYSTEM_AUDIO_MODE_STATUS		0x7E
#define CEC_OP_ROUTING_CHANGE				0x80
#define CEC_OP_ROUTING_INFORMATION			0x81
#define CEC_OP_ACTIVE_SOURCE				0x82
#define CEC_OP_GIVE_PHYSICAL_ADDRESS		0x83
#define CEC_OP_REPORT_PHYSICAL_ADDRESS		0x84
#define CEC_OP_REQUEST_ACTIVE_SOURCE		0x85
#define CEC_OP_SET_STREAM_PATH				0x86
#define CEC_OP_DEVICE_VENDOR_ID				0x87
#define CEC_OP_VENDOR_COMMAND				0x89
#define CEC_OP_VENDOR_REMOTE_BUTTON_DOWN	0x8A
#define CEC_OP_VENDOR_REMOTE_BUTTON_UP		0x8B
#define CEC_OP_GIVE_DEVICE_VENDOR_ID		0x8C
#define CEC_OP_MENU_REQUEST					0x8D
#define CEC_OP_MENU_STATUS					0x8E
#define CEC_OP_GIVE_DEVICE_POWER_STATUS		0x8F
#define CEC_OP_REPORT_POWER_STATUS			0x90
#define CEC_OP_GET_MENU_LANGUAGE			0x91
#define CEC_OP_SELECT_ANALOGUE_SERVICE		0x92
#define CEC_OP_SELECT_DIGITAL_SERVICE		0x93
#define CEC_OP_SET_DIGITAL_TIMER			0x97
#define CEC_OP_CLEAR_DIGITAL_TIMER			0x99
#define CEC_OP_SET_AUDIO_RATE				0x9A
#define CEC_OP_INACTIVE_SOURCE				0x9D
#define CEC_OP_CEC_VERSION					0x9E
#define CEC_OP_GET_CEC_VERSION				0x9F
#define CEC_OP_VENDOR_COMMAND_WITH_ID		0xA0
#define CEC_OP_CLEAR_EXTERNAL_TIMER			0xA1
#define CEC_OP_SET_EXTERNAL_TIMER			0xA2
#define CEC_OP_ABORT						0xFF

/* Abort Reason */
#define CEC_ABORT_UNRECOGNIZED		0x00
#define CEC_ABORT_INCORRECT_MODE	0x01
#define CEC_ABORT_SOURCE_UNAVAIL	0x02
#define CEC_ABORT_INVALID_OPERAND	0x03
#define CEC_ABORT_REFUSED			0x04
#define CEC_ABORT_UNABLE_TO_DETERMINE	0x05

/* Analogue Broadcast Type */
#define CEC_ANALOGTYPE_CABLE		0x00
#define CEC_ANALOGTYPE_SATELLITE	0x01
#define CEC_ANALOGTYPE_TERRESTRIAL	0x02

/* Audio Rate */
#define CEC_AUDIORATE_CONTROL_OFF	0x00
#define CEC_AUDIORATE_WIDE_STD		0x01
#define CEC_AUDIORATE_WIDE_FAST		0x02
#define CEC_AUDIORATE_WIDE_SLOW		0x03
#define CEC_AUDIORATE_NARROW_STD	0x04
#define CEC_AUDIORATE_NARROW_FAST	0x05
#define CEC_AUDIORATE_NARROW_SLOW	0x06

/* Audio Status */
#define CEC_AUDIOSTATUS_MUTE_MASK	0x80
#define CEC_AUDIOSTATUS_VOL_MASK	0x7F
#define CEC_AUDIOSTATUS_VOL_UNKNOWN	0x7F

/* Bolean */
#define CEC_BOOL_FALSE				0x00
#define CEC_BOOL_TRUE				0x01

/* Broadcast System */
#define CEC_BCASTSYSTEM_PAL_BG		0x00
#define CEC_BCASTSYSTEM_SECAM_LPRIME 0x01
#define CEC_BCASTSYSTEM_PAL_M		0x02
#define CEC_BCASTSYSTEM_NTSC_M		0x03
#define CEC_BCASTSYSTEM_PAL_I		0x04
#define CEC_BCASTSYSTEM_SECAM_DK	0x05
#define CEC_BCASTSYSTEM_SECAM_BG	0x06
#define CEC_BCASTSYSTEM_SECAM_L		0x07
#define CEC_BCASTSYSTEM_PAL_DK		0x08
#define CEC_BCASTSYSTEM_OTHER		0x1F

/* CEC Version */
#define CEC_VERSION_V1_1			0x00
#define CEC_VERSION_V1_2			0x01
#define CEC_VERSION_V1_2A			0x02
#define CEC_VERSION_V1_3			0x03
#define CEC_VERSION_V1_3A			0x04
#define CEC_VERSION_V1_4			0x05

/* Channel Identifier */
#define CEC_CHANID_NUM_FORMAT_MASK	0xFC00
#define CEC_CHANID_MAJOR_CHAN_MASK	0x03FF
#define CEC_CHANID_1_PART_CHANNEL	0x0400
#define CEC_CHANID_2_PART_CHANNEL	0x0800

/* Deck Control Mode */
#define CEC_DECKCTRL_FORWARD		0x01
#define CEC_DECKCTRL_REVERSE		0x02
#define CEC_DECKCTRL_STOP			0x03
#define CEC_DECKCTRL_EJECT			0x04

/* Deck Info */
#define CEC_DECKINFO_PLAY			0x11
#define CEC_DECKINFO_RECORD			0x12
#define CEC_DECKINFO_PLAY_REVERSE	0x13
#define CEC_DECKINFO_STILL			0x14
#define CEC_DECKINFO_SLOW			0x15
#define CEC_DECKINFO_SLOW_REVERSE	0x16
#define CEC_DECKINFO_FAST_FORWARD	0x17
#define CEC_DECKINFO_FAST_REVERSE	0x18
#define CEC_DECKINFO_NO_MEDIA		0x19
#define CEC_DECKINFO_STOP			0x1A
#define CEC_DECKINFO_FORWARD		0x1B
#define CEC_DECKINFO_REVERSE		0x1C
#define CEC_DECKINFO_INDEX_FORWARD	0x1D
#define CEC_DECKINFO_INDEX_REVERSE	0x1E
#define CEC_DECKINFO_OTHER_STATUS	0x1F

/* Device Type */
#define CEC_DEVTYPE_TV				0x00
#define CEC_DEVTYPE_RECORDING		0x01
#define CEC_DEVTYPE_RESERVED		0x02
#define CEC_DEVTYPE_TUNER			0x03
#define CEC_DEVTYPE_PLAYBACK		0x04
#define CEC_DEVTYPE_AUDIO			0x05
#define CEC_DEVTYPE_FREEUSE			0x06

/* Digital Service Identification */
#define CEC_DSRVCID_METHOD_MASK		0x80
#define CEC_DSRVCID_METHOD_DID		0x00
#define CEC_DSRVCID_METHOD_CHANNEL	0x80
#define CEC_DSRVCID_BCAST_MASK		0x7F
#define CEC_DSRVCID_BCAST_ARIB_GEN	0x00
#define CEC_DSRVCID_BCAST_ATSC_GEN	0x01
#define CEC_DSRVCID_BCAST_DVB_GEN	0x02
#define CEC_DSRVCID_BCAST_ARIB_BS	0x08
#define CEC_DSRVCID_BCAST_ARIB_CS	0x09
#define CEC_DSRVCID_BCAST_ARIB_T	0x0A
#define CEC_DSRVCID_BCAST_ATSC_CABL 0x10
#define CEC_DSRVCID_BCAST_ATSC_SAT	0x11
#define CEC_DSRVCID_BCAST_ATSC_TER	0x12
#define CEC_DSRVCID_BCAST_DVB_C		0x18
#define CEC_DSRVCID_BCAST_DVB_S		0x19
#define CEC_DSRVCID_BCAST_DVB_S2	0x1A
#define CEC_DSRVCID_BCAST_DVB_T		0x1B

/* Display Control */
#define CEC_DISPLAY_DEFAULT_TIME	0x00
#define CEC_DISPLAY_UNTIL_CLEARED	0x01
#define CEC_DISPLAY_CLEAR_PREVIOUS	0x02
#define CEC_DISPLAY_RESERVED		0x03

/* External Source Specifier */
#define CEC_EXTSRC_PLUG				0x04
#define CEC_EXTSRC_PHYSICAL_ADDRESS	0x05

/* Menu Request Type */
#define CEC_MENUREQUEST_ACTIVATE	0x00
#define CEC_MENUREQUEST_DEACTIVATE	0x01
#define CEC_MENUREQUEST_QUERY		0x02

/* Menu State */
#define CEC_MENUSTATE_ACTIVATED		0x00
#define CEC_MENUSTATE_DEACTIVATED	0x01

/* Play Mode */
#define CEC_PLAYMODE_PLAY_FOWARD	0x24
#define CEC_PLAYMODE_PLAY_REVERSE	0x20
#define CEC_PLAYMODE_PLAY_STILL		0x25
#define CEC_PLAYMODE_FF_MIN_SPEED	0x05
#define CEC_PLAYMODE_FF_MED_SPEED	0x06
#define CEC_PLAYMODE_FF_MAX_SPEED	0x07
#define CEC_PLAYMODE_FR_MIN_SPEED	0x09
#define CEC_PLAYMODE_FR_MED_SPEED	0x0A
#define CEC_PLAYMODE_FR_MAX_SPEED	0x0B
#define CEC_PLAYMODE_SF_MIN_SPEED	0x15
#define CEC_PLAYMODE_SF_MED_SPEED	0x16
#define CEC_PLAYMODE_SF_MAX_SPEED	0x17
#define CEC_PLAYMODE_SR_MIN_SPEED	0x19
#define CEC_PLAYMODE_SR_MED_SPEED	0x1A
#define CEC_PLAYMODE_SR_MAX_SPEED	0x1B

/* Power Status */
#define CEC_POWERSTATUS_ON			0x00
#define CEC_POWERSTATUS_STANDBY		0x01
#define CEC_POWERSTATUS_STDBY_TO_ON	0x02
#define CEC_POWERSTATUS_ON_TO_STDBY	0x03

/* Record Source Type */
#define CEC_RECORDSRC_OWN_SOURCE	0x01
#define CEC_RECORDSRC_DIGITAL		0x02
#define CEC_RECORDSRC_ANALOGUE		0x03
#define CEC_RECORDSRC_EXT_PLUG		0x04
#define CEC_RECORDSRC_EXT_ADDRESS	0x05

/* Record Status Info */
#define CEC_RECORDING_CURRENT		0x01
#define CEC_RECORDING_DIGITAL		0x02
#define CEC_RECORDING_ANALOGUE		0x03
#define CEC_RECORDING_EXTERNAL		0x04
#define CEC_NO_RECORD_DIGITAL		0x05
#define CEC_NO_RECORD_ANALOGUE		0x06
#define CEC_NO_RECORD_CANT_SELECT	0x07
#define CEC_NO_RECORD_INVALID_PLUG	0x09
#define CEC_NO_RECORD_INVALID_ADDR	0x0A
#define CEC_NO_RECORD_UNSUPPORTED_CA 0x0B
#define CEC_NO_RECORD_CA_ENTITLEMENT 0x0C
#define CEC_NO_RECORD_NOT_ALLOWED	0x0D
#define CEC_NO_RECORD_NO_MORE_COPIES 0x0E
#define CEC_NO_RECORD_NO_MEDIA		0x10
#define CEC_NO_RECORD_PLAYING		0x11
#define CEC_NO_RECORD_ALREADY_REC	0x12
#define CEC_NO_RECORD_MEDIA_PROTECT	0x13
#define CEC_NO_RECORD_NO_SIGNAL		0x14
#define CEC_NO_RECORD_MEDIA_PROBLEM	0x15
#define CEC_NO_RECORD_NO_SPACE		0x16
#define CEC_NO_RECORD_PARENTAL_LOCK	0x17
#define CEC_RECORDING_TERMINATED_OK	0x1A
#define CEC_RECORDING_ALREADY_DONE	0x1B
#define CEC_NO_RECORD_OTHER			0x1F

/* Recording Sequences */
#define CEC_RECORDSEQ_ONCE_MASK		0x00
#define CEC_RECORDSEQ_SUN_MASK		0x01
#define CEC_RECORDSEQ_MON_MASK		0x02
#define CEC_RECORDSEQ_TUE_MASK		0x04
#define CEC_RECORDSEQ_WED_MASK		0x08
#define CEC_RECORDSEQ_THU_MASK		0x10
#define CEC_RECORDSEQ_FRI_MASK		0x20
#define CEC_RECORDSEQ_SAT_MASK		0x40

/* Status Request */
#define CEC_STATUSREPORT_ON			0x01
#define CEC_STATUSREPORT_OFF		0x02
#define CEC_STATUSREPORT_ONCE		0x03

/* System Audio Status */
#define CEC_SYSAUDIO_OFF			0x00
#define CEC_SYSAUDIO_ON				0x01

/* Timer Cleared Status */
#define CEC_TIMERCLR_RECORDING		0x00
#define CEC_TIMERCLR_NO_MATCHING	0x01
#define CEC_TIMERCLR_NO_INFO		0x02
#define CEC_TIMERCLR_CLEARED		0x80

/* Timer Status Data (always 24 bits) */
#define CEC_TIMERSTAT_OVERLAP_MASK	0x800000
#define CEC_TIMERSTAT_NO_OVERLAP	0x000000
#define CEC_TIMERSTAT_BLOCK_OVERLAP	0x800000
#define CEC_TIMERSTAT_MEDIA_MASK	0x600000
#define CEC_TIMERSTAT_MEDIA_OK		0x000000
#define CEC_TIMERSTAT_MEDIA_PROTECT	0x200000
#define CEC_TIMERSTAT_MEDIA_NOMEDIA	0x400000
#define CEC_TIMERSTAT_MEDIA_FUTURE	0x600000
#define CEC_TIMERSTAT_PROGID_MASK	0x100000
#define CEC_TIMERSTAT_PROGID_NOPROG	0x000000
#define CEC_TIMERSTAT_PROGID_PROG	0x100000
#define CEC_TIMERSTAT_PROGINF_MASK	0x0F0000
#define CEC_TIMERSTAT_PROGINF_OK	0x080000
#define CEC_TIMERSTAT_PROGINF_NOSPC	0x090000
#define CEC_TIMERSTAT_PROGINF_Q_SPC	0x0B0000
#define CEC_TIMERSTAT_PROGINF_NOINF	0x0A0000
#define CEC_TIMERSTAT_NOPROG_TIMER	0x010000
#define CEC_TIMERSTAT_NOPROG_DATE	0x020000
#define CEC_TIMERSTAT_NOPROG_SEQ	0x030000
#define CEC_TIMERSTAT_NOPROG_PLUG	0x040000
#define CEC_TIMERSTAT_NOPROG_ADDR	0x050000
#define CEC_TIMERSTAT_NOPROG_CASYS	0x060000
#define CEC_TIMERSTAT_NOPROG_CAENT	0x070000
#define CEC_TIMERSTAT_NOPROG_RES	0x080000
#define CEC_TIMERSTAT_NOPROG_PARENT	0x090000
#define CEC_TIMERSTAT_NOPROG_CLOCK	0x0E0000

/* Tuner Device Info */
#define CEC_TUNINFO_RECFLAG_MASK	0x80
#define CEC_TUNINFO_DISPLAY_MASK	0x7F
#define CEC_TUNINFO_RECFLAG_UNUSED	0x00
#define CEC_TUNINFO_RECFLAG_USED	0x80
#define CEC_TUNINFO_DISPLAY_DTUNER	0x00
#define CEC_TUNINFO_DISPLAY_ATUNER	0x02
#define CEC_TUNINFO_DISPLAY_NOTUNER	0x01

/* UI Command (Table 27) */
#define CEC_UI_SELECT				0x00
#define CEC_UI_UP					0x01
#define CEC_UI_DOWN					0x02
#define CEC_UI_LEFT					0x03
#define CEC_UI_RIGHT				0x04
#define CEC_UI_RIGHT_UP				0x05
#define CEC_UI_RIGHT_DOWN			0x06
#define CEC_UI_LEFT_UP				0x07
#define CEC_UI_LEFT_DOWN			0x08
#define CEC_UI_ROOT_MENU			0x09
#define CEC_UI_SETUP_MENU			0x0A
#define CEC_UI_CONTENT_MENU			0x0B
#define CEC_UI_FAVORITE_MENU		0x0C
#define CEC_UI_EXIT					0x0D
#define CEC_UI_NUMBER_0				0x20
#define CEC_UI_NUMBER_1				0x21
#define CEC_UI_NUMBER_2				0x22
#define CEC_UI_NUMBER_3				0x23
#define CEC_UI_NUMBER_4				0x24
#define CEC_UI_NUMBER_5				0x25
#define CEC_UI_NUMBER_6				0x26
#define CEC_UI_NUMBER_7				0x27
#define CEC_UI_NUMBER_8				0x28
#define CEC_UI_NUMBER_9				0x29
#define CEC_UI_DOT					0x2A
#define CEC_UI_ENTER				0x2B
#define CEC_UI_CLEAR				0x2C
#define CEC_UI_NEXT_FAVORITE		0x2F
#define CEC_UI_CHANNEL_UP			0x30
#define CEC_UI_CHANNEL_DOWN			0x31
#define CEC_UI_PREVIOUS_CHANNEL		0x32
#define CEC_UI_SOUND_SELECT			0x33
#define CEC_UI_INPUT_SELECT			0x34
#define CEC_UI_DISPLAY_INFORMATION	0x35
#define CEC_UI_HELP					0x36
#define CEC_UI_PAGE_UP				0x37
#define CEC_UI_PAGE_DOWN			0x38
#define CEC_UI_POWER				0x40
#define CEC_UI_VOLUME_UP			0x41
#define CEC_UI_VOLUME_DOWN			0x42
#define CEC_UI_MUTE					0x43
#define CEC_UI_PLAY					0x44
#define CEC_UI_STOP					0x45
#define CEC_UI_PAUSE				0x46
#define CEC_UI_RECORD				0x47
#define CEC_UI_REWIND				0x48
#define CEC_UI_FAST_FORWARD			0x49
#define CEC_UI_EJECT				0x4A
#define CEC_UI_FORWARD				0x4B
#define CEC_UI_BACKWARD				0x4C
#define CEC_UI_STOP_RECORD			0x4D
#define CEC_UI_PAUSE_RECORD			0x4E
#define CEC_UI_ANGLE				0x50
#define CEC_UI_SUB_PICTURE			0x51
#define CEC_UI_VIDEO_ON_DEMAND		0x52
#define CEC_UI_EPG					0x53
#define CEC_UI_TIMER_PROGRAMMING	0x54
#define CEC_UI_INITIAL_CONFIG		0x55
#define CEC_UI_PLAY_FUNC			0x60
#define CEC_UI_PAUSE_PLAY_FUNC		0x61
#define CEC_UI_RECORD_FUNC			0x62
#define CEC_UI_PAUSE_RECORD_FUNC	0x63
#define CEC_UI_STOP_FUNC			0x64
#define CEC_UI_MUTE_FUNC			0x65
#define CEC_UI_RESTORE_VOL_FUNC		0x66
#define CEC_UI_TUNE_FUNC			0x67
#define CEC_UI_SELECT_MEDIA_FUNC	0x68
#define CEC_UI_SELECT_AV_INPUT_FUNC	0x69
#define CEC_UI_SELECT_AUDIO_IN_FUNC	0x6A
#define CEC_UI_POWER_TOGGLE_FUNC	0x6B
#define CEC_UI_POWER_OFF_FUNC		0x6C
#define CEC_UI_POWER_ON_FUNC		0x6D
#define CEC_UI_F1_OR_BLUE			0x71
#define CEC_UI_F2_OR_RED			0x72
#define CEC_UI_F3_OR_GREEN			0x73
#define CEC_UI_F4_OR_YELLOW			0x74
#define CEC_UI_F5					0x75
#define CEC_UI_DATA					0x76

#endif
