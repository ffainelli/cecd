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
	uint8_t edid[256], block_length, checksum = 0;
	const uint8_t edid_marker[] = {0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00};
	int i, r, block_start;

	*phys_addr = 0xFFFF;	/* undetermined */
	if (handle == NULL) {
		return LIBCEC_ERROR_INVALID_PARAM;
	}
	r = libcec_read_edid(handle, edid, sizeof(edid));
	if (r < 0) {
		return r;
	}

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
int libcec_allocate_logical_address(libcec_device_handle* handle, uint8_t device_type, uint16_t* physical_address)
{
	const uint8_t logical_address_table[15] = {0, 1, 1, 3, 4, 5, 3, 3, 4, 1, 3, 4, 2, 2, 0};
	int r;
	uint8_t logical_address, polling_message;

	if (handle == NULL) {
		return LIBCEC_ERROR_INVALID_PARAM;
	}

	switch (device_type) {
		case 0:
		case 1:
		case 3:
		case 4:
		case 5:
			break;
		default:
			return LIBCEC_ERROR_INVALID_PARAM;
	}

	/* Set our logical address to unregistered during allocation */
	r = libcec_set_logical_address(handle, 15);
	if (r != LIBCEC_SUCCESS) return r;
	ceci_dbg("switched to unregistered logical address");

	r = libcec_get_physical_address(handle, physical_address);
	if (r != LIBCEC_SUCCESS) return r;
	ceci_info("physical address: %d.%d.%d.%d", (*physical_address>>12)&0xF,
		(*physical_address>>8)&0xF, (*physical_address>>4)&0xF, (*physical_address>>0)&0xF);
	if (*physical_address == 0xFFFF) {
		return LIBCEC_SUCCESS;	/* keep unregistered */
	}

	/* TV as root */
	if (*physical_address == 0x0000) {
		if (device_type != 0) {
			ceci_error("invalid device type for physical address 0.0.0.0 - must be TV");
			return LIBCEC_ERROR_INVALID_PARAM;
		}
		return libcec_set_logical_address(handle, 0);
	}

	for (logical_address = 1; logical_address < 15; logical_address++) {
		if (logical_address_table[logical_address] != device_type) {
			continue;
		}
		polling_message = 0xF0 | logical_address;
		ceci_dbg("querying logical address %d\n", logical_address);
		if (libcec_write_message(handle, &polling_message, 1) != LIBCEC_SUCCESS) {
			/* error on polling (no ACK) => assume address is free */
			r = libcec_set_logical_address(handle, logical_address);
			if (r != LIBCEC_SUCCESS) return r;
			ceci_dbg("using logical address %d\n", logical_address);
			return logical_address;
		}
		ceci_dbg("address already in use - trying next\n");
	}

	ceci_warn("exhausted all possible logical addresses - keeping unregistered (15)\n");
	return LIBCEC_SUCCESS;
}

DEFAULT_VISIBILITY
int libcec_write_message(libcec_device_handle* handle, uint8_t* buffer, size_t length)
{
	if ((handle == NULL) || (buffer == NULL) || (length == 0)) {
		return LIBCEC_ERROR_INVALID_PARAM;
	}
	return ceci_backend->write_message(handle, buffer, length);
}

DEFAULT_VISIBILITY
int libcec_read_message(libcec_device_handle* handle, uint8_t* buffer, size_t length, int32_t timeout)
{
	if ((handle == NULL) || (buffer == NULL) || (length == 0)) {
		return LIBCEC_ERROR_INVALID_PARAM;
	}
	return ceci_backend->read_message(handle, buffer, length, timeout);
}

void ceci_log_v(enum libcec_log_level level, const char *function,
				const char *format, va_list args)
{
	const char *prefix;
	struct timeval tv;
	struct tm *loc;

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

	gettimeofday(&tv, (struct timezone *)0);
	loc = localtime(&tv.tv_sec);
	fprintf(ceci_logger, "%04d.%02d.%02d %02d:%02d:%02d.%03ld libcec:%s [%s] ",
		loc->tm_year+1900, loc->tm_mon+1, loc->tm_mday, loc->tm_hour,
		loc->tm_min, loc->tm_sec, tv.tv_usec/1000, prefix, function);
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
