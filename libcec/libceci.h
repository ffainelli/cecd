/*
 * libcec - internal header
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

#ifndef __LIBCECI_H__
#define __LIBCECI_H__

#include <config.h>

#include <stddef.h>
#include <stdint.h>
#include <time.h>

#include <libcec.h>
#include "libcec_version.h"

#define MIN(a, b)	((a) < (b) ? (a) : (b))
#define MAX(a, b)	((a) > (b) ? (a) : (b))

void ceci_log(enum libcec_log_level level, const char *function, const char *format, ...);

#if defined (ENABLE_LOGGING)
#define _ceci_log(level, ...) ceci_log(level, __FUNCTION__, __VA_ARGS__)
#else
#define _ceci_log(level, ...)
#endif

#if defined(ENABLE_DEBUG_LOGGING)
#define ceci_dbg(...) _ceci_log(LIBCEC_LOG_LEVEL_DEBUG, __VA_ARGS__)
#else
#define ceci_dbg(...)
#endif

#define ceci_info(...)  _ceci_log(LIBCEC_LOG_LEVEL_INFO, __VA_ARGS__)
#define ceci_warn(...)  _ceci_log(LIBCEC_LOG_LEVEL_WARNING, __VA_ARGS__)
#define ceci_error(...) _ceci_log(LIBCEC_LOG_LEVEL_ERROR, __VA_ARGS__)

struct libcec_device_handle {
	unsigned char priv[0];
};

/* CEC implementation abstraction */
typedef struct {
	const char *name;
	int (*init)(void);
	int (*exit)(void);
	int (*open)(char* device_name, libcec_device_handle* handle);
	int (*close)(libcec_device_handle* handle);
	/* we need a call to read EDID from closest sink, to obtain our physical address */
	int (*read_edid)(libcec_device_handle* handle, uint8_t* buffer, size_t length);
	int (*set_logical_address)(libcec_device_handle* handle, uint8_t logical_address);
	int (*send_message)(libcec_device_handle* handle, uint8_t* buffer, size_t length);
	int (*receive_message)(libcec_device_handle* handle, uint8_t* buffer, size_t length);

	/* number of bytes to reserve for the device handle private backend data */
	size_t device_handle_priv_size;
} _ceci_backend ;

extern FILE* ceci_logger;
extern int ceci_global_log_level;
extern const _ceci_backend* const ceci_backend;
extern const _ceci_backend linux_realtek_soc_backend;

#endif
