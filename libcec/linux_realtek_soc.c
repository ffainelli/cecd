/*
 * libcec - Linux Realtek SoC CEC functions
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

#include "libceci.h"
#include "linux_realtek_soc.h"

int realtek_cec_init(void)
{
	return 0;
}

int realtek_cec_exit(void)
{
	return 0;
}

int realtek_cec_open(char* device_name, libcec_device_handle* handle)
{
	return 0;
}

int realtek_cec_close(libcec_device_handle* handle)
{
	return 0;
}

int realtek_i2c_read_edid(libcec_device_handle* handle, uint8_t buffer, size_t length)
{
	return 0;
}

int realtek_cec_set_logical_address(libcec_device_handle* handle, uint8_t logical_address)
{
	return 0;
}

int realtek_cec_send_message(libcec_device_handle* handle, uint8_t buffer, size_t length)
{
	return 0;
}

int realtek_cec_receive_message(libcec_device_handle* handle, uint8_t buffer, size_t length)
{
	return 0;
}

const struct _ceci_backend linux_realtek_soc_backend = {
	"Linux Realtek SoC",
	realtek_cec_init,
	realtek_cec_exit,
	realtek_cec_open,
	realtek_cec_close,
	realtek_i2c_read_edid,
	realtek_cec_set_logical_address,
	realtek_cec_send_message,
	realtek_cec_receive_message,

	sizeof(struct realtek_device_handle_priv),
};
