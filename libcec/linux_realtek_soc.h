/*
 * libcec - Linux Realtek SoC CEC functions
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

/* from drivers/cec/core/cec_dev.h */
enum {
	CEC_ENABLE,
	CEC_SET_LOGICAL_ADDRESS,
	CEC_SET_POWER_STATUS,
	CEC_SEND_MESSAGE,
	CEC_RCV_MESSAGE,
};

typedef struct {
	unsigned char*		buf;
	unsigned char		len;
	long				timeout;
} cec_msg;

typedef struct {
	int cec_dev;
	int i2c_dev;
} realtek_device_handle_priv;

static inline realtek_device_handle_priv* __device_handle_priv(libcec_device_handle *handle)
{
	return (realtek_device_handle_priv*) handle->priv;
}
