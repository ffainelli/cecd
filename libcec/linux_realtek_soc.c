/*
 * libcec - Linux Realtek SoC CEC functions
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
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#include "libceci.h"
#include "linux_realtek_soc.h"

int realtek_cec_init(void)
{
	return LIBCEC_SUCCESS;
}

int realtek_cec_exit(void)
{
	return LIBCEC_SUCCESS;
}

int realtek_cec_open(char* device_name, libcec_device_handle* handle)
{
	int ret_val;

	realtek_device_handle_priv* handle_priv = __device_handle_priv(handle);
	handle_priv->cec_dev = open(device_name, 0);
	if (handle_priv->cec_dev < 0) {
		ceci_error("cannot open CEC device %s", device_name);
		return LIBCEC_ERROR_NO_DEVICE;
	}

	ret_val = ioctl(handle_priv->cec_dev, CEC_ENABLE, 1);
	if (ret_val) {
		ceci_error("cannot enable CEC device %s", device_name);
		close(handle_priv->cec_dev);
		return LIBCEC_ERROR_IO;
	}

	return LIBCEC_SUCCESS;
}

int realtek_cec_close(libcec_device_handle* handle)
{
	realtek_device_handle_priv* handle_priv = __device_handle_priv(handle);

	close(handle_priv->cec_dev);
	return LIBCEC_SUCCESS;
}

int realtek_i2c_read_edid(libcec_device_handle* handle, uint8_t* buffer, size_t length)
{
	return LIBCEC_ERROR_NOT_SUPPORTED;
}

int realtek_cec_set_logical_address(libcec_device_handle* handle, uint8_t logical_address)
{
	realtek_device_handle_priv* handle_priv = __device_handle_priv(handle);
	int ret_val;

	ret_val = ioctl(handle_priv->cec_dev, CEC_SET_LOGICAL_ADDRESS, logical_address);
	if (ret_val) {
		ceci_error("failed to set CEC logical address");
		return LIBCEC_ERROR_IO;
	}
	return LIBCEC_SUCCESS;
}

int realtek_cec_send_message(libcec_device_handle* handle, uint8_t* buffer, size_t length)
{
	realtek_device_handle_priv* handle_priv = __device_handle_priv(handle);
	int ret_val;
	cec_msg msg;

	if (length > 255) {
		return LIBCEC_ERROR_INVALID_PARAM;
	}
	msg.buf = buffer;
	msg.len = (unsigned char)length;

	ret_val = ioctl(handle_priv->cec_dev, CEC_SEND_MESSAGE, &msg);
	if (ret_val) {
		ceci_error("failed to send CEC message");
		return LIBCEC_ERROR_IO;
	}
	return LIBCEC_SUCCESS;
}

int realtek_cec_receive_message(libcec_device_handle* handle, uint8_t* buffer, size_t length)
{
	realtek_device_handle_priv* handle_priv = __device_handle_priv(handle);
	int rcv_len;
	cec_msg msg;

	if (length > 255) {
		return LIBCEC_ERROR_INVALID_PARAM;
	}
	msg.buf = buffer;
	msg.len = (unsigned char)length;

	rcv_len = ioctl(handle_priv->cec_dev, CEC_RCV_MESSAGE, &msg);
	if (rcv_len <= 0) {
		ceci_error("failed to receive CEC message");
		return LIBCEC_ERROR_IO;
	}
	return rcv_len;
}

const _ceci_backend linux_realtek_soc_backend = {
	"Linux Realtek SoC",
	realtek_cec_init,
	realtek_cec_exit,
	realtek_cec_open,
	realtek_cec_close,
	realtek_i2c_read_edid,
	realtek_cec_set_logical_address,
	realtek_cec_send_message,
	realtek_cec_receive_message,

	sizeof(realtek_device_handle_priv),
};
