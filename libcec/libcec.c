/*
 * libcec - core functions
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

#include <config.h>

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "libceci.h"

#if defined(LINUX_REALTEK_SOC)
const struct _ceci_backend * const ceci_backend = &linux_realtek_soc_backend;
#else
#error "Unsupported CEC backend"
#endif

const struct libcec_version libcec_version_internal = { LIBCEC_VERSION_MAJOR, LIBCEC_VERSION_MINOR, LIBCEC_VERSION_MICRO, LIBCEC_VERSION_NANO};
