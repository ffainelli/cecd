/*
 * libcec - HDMI-CEC library header
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

#ifndef __LIBCEC_H__
#define __LIBCEC_H__

#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <limits.h>
#include <sys/time.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	uint16_t major;
	uint16_t minor;
	uint16_t micro;
	uint16_t nano;
} libcec_version;

/*
 * Log level
 */
enum libcec_log_level {
	LIBCEC_LOG_LEVEL_DEBUG,
	LIBCEC_LOG_LEVEL_INFO,
	LIBCEC_LOG_LEVEL_WARNING,
	LIBCEC_LOG_LEVEL_ERROR,
	LIBCEC_LOG_LEVEL_NONE
};

/*
 * Error codes. Most libcec functions return 0 on success or one of these
 * codes on failure. You can use libcec_strerror() to retrieve a short
 * string description of a libcec_error enumeration value.
 */
enum libcec_error {
	/** Success (no error) */
	LIBCEC_SUCCESS = 0,
	/** Input/output error */
	LIBCEC_ERROR_IO = -1,
	/** Invalid parameter */
	LIBCEC_ERROR_INVALID_PARAM = -2,
	/** Access denied */
	LIBCEC_ERROR_ACCESS = -3,
	/** No such device */
	LIBCEC_ERROR_NO_DEVICE = -4,
	/** Entity not found */
	LIBCEC_ERROR_NOT_FOUND = -5,
	/** Resource busy */
	LIBCEC_ERROR_BUSY = -6,
	/** Operation timed out */
	LIBCEC_ERROR_TIMEOUT = -7,
	/** Overflow */
	LIBCEC_ERROR_OVERFLOW = -8,
	/** System call interrupted (perhaps due to signal) */
	LIBCEC_ERROR_INTERRUPTED = -10,
	/** Could not acquire resource (Insufficient memory, etc) */
	LIBCEC_ERROR_RESOURCE = -11,
	/** Operation not supported or unimplemented on this platform */
	LIBCEC_ERROR_NOT_SUPPORTED = -12,
	/** Other error */
	LIBCEC_ERROR_OTHER = -99
	/* IMPORTANT: when adding new values to this enum, remember to
	   update the LIBCEC_strerror() function implementation! */
};

/* Opaque type returned by open and used for CEC I/O */
struct libcec_device_handle;
typedef struct libcec_device_handle libcec_device_handle;

void libcec_set_logging(int level, FILE* stream);
const char* libcec_strerror(enum libcec_error error_code);
int libcec_init(void);
int libcec_exit(void);
int libcec_open(char* device_name, libcec_device_handle** handle);
int libcec_close(libcec_device_handle* handle);
int libcec_read_edid(libcec_device_handle* handle, uint8_t* buffer, size_t length);
int libcec_get_physical_address(libcec_device_handle* handle, uint16_t* phys_addr);
int libcec_set_logical_address(libcec_device_handle* handle, uint8_t logical_address);
int libcec_allocate_logical_address(libcec_device_handle* handle, uint8_t device_type, uint16_t* physical_address);
int libcec_write_message(libcec_device_handle* handle, uint8_t* buffer, size_t length);
/* timeout is in ms */
int libcec_read_message(libcec_device_handle* handle, uint8_t* buffer, size_t length, int32_t timeout);
int libcec_decode_message(uint8_t* message, size_t length);

#ifdef __cplusplus
}
#endif

#endif
