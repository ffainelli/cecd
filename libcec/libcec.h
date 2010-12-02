/*
 * libcec - HDMI-CEC library header
 *
 * Copyright (c) 2010 Pete Batard <pbatard@gmail.com>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
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
#include <time.h>
#include <limits.h>
#include <sys/time.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct libcec_version {
	uint16_t major;
	uint16_t minor;
	uint16_t micro;
	uint16_t nano;
};

/* Opaque type returned by open and used for CEC I/O */
struct libcec_device_handle;
typedef struct libcec_device_handle libcec_device_handle;

int libcec_open(char* device_name, libcec_device_handle** handle);
int libcec_close(libcec_device_handle* handle);

#ifdef __cplusplus
}
#endif

#endif
