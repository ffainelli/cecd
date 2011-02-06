/*
 * libcec - core functions
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
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "libceci.h"

#if defined(LINUX_REALTEK_SOC)
const _ceci_backend* const ceci_backend = &linux_realtek_soc_backend;
#else
#error "Unsupported CEC backend"
#endif

const libcec_version libcec_version_internal = {
	LIBCEC_VERSION_MAJOR, LIBCEC_VERSION_MINOR,
	LIBCEC_VERSION_MICRO, LIBCEC_VERSION_NANO };

FILE* ceci_logger = NULL;
int ceci_global_log_level = LIBCEC_LOG_LEVEL_INFO;

/*
 * Set the logging level and destination.
 * If the stream is NULL, stderr will be used.
 * The stream must be open by the caller.
 * This function can be called outside of init/exit
 */
DEFAULT_VISIBILITY
void libcec_set_logging(int level, FILE* stream)
{
	ceci_global_log_level = level;
	if (stream == NULL) {
		ceci_logger = stderr;
	} else {
		ceci_logger = stream;
	}
}

DEFAULT_VISIBILITY
int libcec_init(void)
{
	if (ceci_logger == NULL) {
		ceci_logger = stderr;
	}
	return ceci_backend->init();
}

DEFAULT_VISIBILITY
int libcec_exit(void)
{
	return ceci_backend->exit();
}

DEFAULT_VISIBILITY
int libcec_open(char* device_name, libcec_device_handle** handle)
{
	size_t priv_size = ceci_backend->device_handle_priv_size;
	struct libcec_device_handle *_handle;
	int r;
	ceci_dbg("open %s", device_name);

	_handle = malloc(sizeof(*_handle) + priv_size);
	if (!_handle) {
		return LIBCEC_ERROR_RESOURCE;
	}

	// TODO: mutex?
	memset(&_handle->priv, 0, priv_size);

	r = ceci_backend->open(device_name, _handle);
	if (r < 0) {
		free(_handle);
		return r;
	}

	// TODO: add handles to a list and free on exit?
	*handle = _handle;

	return LIBCEC_SUCCESS;
}

DEFAULT_VISIBILITY
int libcec_close(libcec_device_handle* handle)
{
	int r;

	if (handle == NULL) {
		return LIBCEC_ERROR_INVALID_PARAM;
	}

	r = ceci_backend->close(handle);
	free(handle);
	return r;
}

DEFAULT_VISIBILITY
int libcec_read_edid(libcec_device_handle* handle, uint8_t* buffer, size_t length)
{
	if ((handle == NULL) || (buffer == NULL) || (length < 256)) {
		return LIBCEC_ERROR_INVALID_PARAM;
	}
	memset(buffer, 0, length);
	return ceci_backend->read_edid(handle, buffer, length);
}

DEFAULT_VISIBILITY
int libcec_get_physical_address(libcec_device_handle* handle, uint16_t* phys_addr)
{
	uint8_t block_length, checksum = 0;
	const uint8_t edid_marker[] = {0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00};
	int i, r, block_start;

#ifdef EDID_TEST
	uint8_t edid[256] = {
	 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x3d, 0xcb, 0x72, 0x08, 0x00, 0x00, 0x00, 0x00,
	 0x00, 0x12, 0x01, 0x03, 0x80, 0x10, 0x09, 0x78, 0x0a, 0xee, 0x91, 0xa3, 0x54, 0x4c, 0x99, 0x26,
	 0x0f, 0x50, 0x54, 0x20, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x3a, 0x80, 0x18, 0x71, 0x38, 0x2d, 0x40, 0x58, 0x2c,
	 0x45, 0x00, 0xa0, 0x5a, 0x00, 0x00, 0x00, 0x1e, 0x01, 0x1d, 0x00, 0x72, 0x51, 0xd0, 0x1e, 0x20,
	 0x6e, 0x28, 0x55, 0x00, 0xa0, 0x5a, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0xfc, 0x00, 0x54,
	 0x58, 0x2d, 0x53, 0x52, 0x37, 0x30, 0x36, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0xfd,
	 0x00, 0x17, 0x3d, 0x1a, 0x44, 0x11, 0x00, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x01, 0x06,
	 0x02, 0x03, 0x45, 0x71, 0x4f, 0x90, 0x1f, 0x04, 0x13, 0x05, 0x14, 0x03, 0x12, 0x20, 0x21, 0x22,
	 0x0f, 0x1e, 0x24, 0x26, 0x38, 0x09, 0x7f, 0x07, 0x0f, 0x7f, 0x07, 0x17, 0x07, 0x50, 0x3f, 0x06,
	 0xc0, 0x4d, 0x02, 0x00, 0x57, 0x06, 0x00, 0x5f, 0x7e, 0x01, 0x67, 0x54, 0x00, 0x83, 0x4f, 0x00,
	 0x00, 0x6c, 0x03, 0x0c, 0x00, 0x12, 0x00, 0xb8, 0x2d, 0xc0, 0x00, 0x00, 0x00, 0x00, 0xe3, 0x05,
	 0x03, 0x01, 0xe2, 0x00, 0x0f, 0x01, 0x1d, 0x00, 0xbc, 0x52, 0xd0, 0x1e, 0x20, 0xb8, 0x28, 0x55,
	 0x40, 0xa0, 0x5a, 0x00, 0x00, 0x00, 0x1e, 0x01, 0x1d, 0x80, 0x18, 0x71, 0x1c, 0x16, 0x20, 0x58,
	 0x2c, 0x25, 0x00, 0xa0, 0x5a, 0x00, 0x00, 0x00, 0x9e, 0x01, 0x1d, 0x80, 0xd0, 0x72, 0x1c, 0x16,
	 0x20, 0x10, 0x2c, 0x25, 0x80, 0xa0, 0x5a, 0x00, 0x00, 0x00, 0x9e, 0x00, 0x00, 0x00, 0x00, 0x07,
	};
#else
	uint8_t edid[256];
#endif

	*phys_addr = 0xFFFF;	/* undetermined */
	if (handle == NULL) {
		return LIBCEC_ERROR_INVALID_PARAM;
	}
#ifndef EDID_TEST
	r = libcec_read_edid(handle, edid, sizeof(edid));
	if (r < 0) {
		return r;
	}
#endif

	/* Check the header for EDID signature */
	if (memcmp(edid, edid_marker, sizeof(edid_marker)) != 0) {
		ceci_error("invalid EDID header");
		return LIBCEC_ERROR_IO;
	}

	/* Validate the EDID checksum */
	for (i=0; i<0x80; i++) {
		checksum += edid[i];
	}
	if (checksum != 0) {
		ceci_error("invalid EDID checksum");
		return LIBCEC_ERROR_IO;
	}

	/* Confirm that we have an HDMI device */
	if (edid[0x7e] == 0) {
		ceci_error("display device does not appear to be HDMI");
		return LIBCEC_ERROR_NO_DEVICE;
	}

	/* Check that we have an E-EDID extension */
	if ((edid[0x7e] == 0) || (edid[0x80] != 0x02) || (edid[0x81] != 0x03)) {
		ceci_error("E-EDID marker not found");
		return LIBCEC_ERROR_NOT_FOUND;
	}

	for (block_start=0x84; block_start<256; ) {
		block_length = (edid[block_start] & 0x1f) + 1;
		switch((edid[block_start]>>5)&7) {
		case 3:	/* HDMI VSDB */
			if ( (edid[block_start+1] == 0x03) && (edid[block_start+2] == 0x0c)
				&& (edid[block_start+3] == 0x00) ) {
				/* HDMI IEEE Registration Identifier */
				*phys_addr = (edid[block_start+4]<<8) + edid[block_start+5];
				ceci_dbg("found physical address %04X", *phys_addr);
				return LIBCEC_SUCCESS;
			}
		default:
			block_start += block_length;
		}
	}

	return LIBCEC_ERROR_NOT_FOUND;
}


DEFAULT_VISIBILITY
int libcec_set_logical_address(libcec_device_handle* handle, uint8_t logical_address)
{
	if ((handle == NULL) || (logical_address > 15)) {
		return LIBCEC_ERROR_INVALID_PARAM;
	}
	return ceci_backend->set_logical_address(handle, logical_address);
}

DEFAULT_VISIBILITY
int libcec_send_message(libcec_device_handle* handle, uint8_t* buffer, size_t length)
{
	if ((handle == NULL) || (buffer == NULL) || (length == 0)) {
		return LIBCEC_ERROR_INVALID_PARAM;
	}
	return ceci_backend->send_message(handle, buffer, length);
}

DEFAULT_VISIBILITY
int libcec_receive_message(libcec_device_handle* handle, uint8_t* buffer, size_t length)
{
	if ((handle == NULL) || (buffer == NULL) || (length == 0)) {
		return LIBCEC_ERROR_INVALID_PARAM;
	}
	return ceci_backend->receive_message(handle, buffer, length);
}

void ceci_log_v(enum libcec_log_level level, const char *function,
				const char *format, va_list args)
{
	const char *prefix;

#ifndef ENABLE_DEBUG_LOGGING
	if (level < ceci_global_log_level)
		return;
#endif

	switch (level) {
	case LIBCEC_LOG_LEVEL_DEBUG:
		prefix = "debug";
		break;
	case LIBCEC_LOG_LEVEL_INFO:
		prefix = "info";
		break;
	case LIBCEC_LOG_LEVEL_WARNING:
		prefix = "warning";
		break;
	case LIBCEC_LOG_LEVEL_ERROR:
		prefix = "error";
		break;
	default:
		prefix = "unknown";
		break;
	}

	fprintf(ceci_logger, "libcec:%s [%s] ", prefix, function);
	vfprintf(ceci_logger, format, args);
	fprintf(ceci_logger, "\n");
	fflush(ceci_logger);
}

void ceci_log(enum libcec_log_level level, const char *function, const char *format, ...)
{
	va_list args;

	va_start (args, format);
	ceci_log_v(level, function, format, args);
	va_end (args);
}

/*
 * Returns a constant NULL-terminated string with an English short description
 * of the given error code. The caller should never free() the returned pointer
 * since it points to a constant string.
 * The returned string is encoded in ASCII form and always starts with a
 * capital letter and ends without any punctuation.
 *
 * \param errcode the error code whose description is desired
 * \returns a short description of the error code in English, or NULL if the
 * error descriptions are unavailable
 */
DEFAULT_VISIBILITY
const char* libcec_strerror(enum libcec_error error_code)
{
	switch (error_code) {
	case LIBCEC_SUCCESS:
		return "Success";
	case LIBCEC_ERROR_IO:
		return "Input/Output error";
	case LIBCEC_ERROR_INVALID_PARAM:
		return "Invalid parameter";
	case LIBCEC_ERROR_ACCESS:
		return "Access denied";
	case LIBCEC_ERROR_NO_DEVICE:
		return "No such device";
	case LIBCEC_ERROR_NOT_FOUND:
		return "Entity not found";
	case LIBCEC_ERROR_BUSY:
		return "Resource busy";
	case LIBCEC_ERROR_TIMEOUT:
		return "Operation timed out";
	case LIBCEC_ERROR_OVERFLOW:
		return "Overflow";
	case LIBCEC_ERROR_INTERRUPTED:
		return "System call interrupted";
	case LIBCEC_ERROR_RESOURCE:
		return "resource unavailable or insufficient memory";
	case LIBCEC_ERROR_NOT_SUPPORTED:
		return "Operation not supported or unimplemented in this version of the library";
	case LIBCEC_ERROR_OTHER:
		return "Other error";
	}
	return "Unknown error";
}
