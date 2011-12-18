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

#include <config.h>

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <errno.h>

#include "libceci.h"
#include "linux_realtek_soc.h"


/* I2C definitions */
#define REALTEK_EDID_I2C_DEV	"/dev/i2c/0"
#define REALTEK_EDID_I2C_ADDR	0x50

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
		ceci_error("cannot open CEC device '%s' - errno: %d", device_name, errno);
		return LIBCEC_ERROR_NO_DEVICE;
	}

	ret_val = ioctl(handle_priv->cec_dev, CEC_ENABLE, 1);
	if (ret_val) {
		ceci_error("cannot enable CEC device '%s' - errno: %d", device_name, errno);
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
	int fd;
	size_t offset;
	struct i2c_msg i2c_message;
	struct i2c_rdwr_ioctl_data i2c_msgset = { &i2c_message, 1};
	const uint8_t edid_marker[] = {0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00};

	/* Only one HDMI port for RTD, and the DDC for EDID is at I2C address 0x50 */
	i2c_message.addr = REALTEK_EDID_I2C_ADDR;
	i2c_message.flags = I2C_M_RD;
	i2c_message.len = length;
	i2c_message.buf = buffer;

	fd = open(REALTEK_EDID_I2C_DEV, O_RDWR);
	if (fd < 0) {
		ceci_error("unable to open I2C device '%s' - errno: %d", REALTEK_EDID_I2C_DEV, errno);
		return LIBCEC_ERROR_ACCESS;
	}

	if (ioctl(fd, I2C_RDWR, &i2c_msgset) < 0) {
		ceci_error("unable to read EDID - errno: %d", errno);
		close(fd);
		return LIBCEC_ERROR_IO;
	}

	/* If you switch your HDMI connection around, parasitic bytes
	   may get inserted in the EDID => attempt to remedy that */
	if (memcmp(buffer, edid_marker, sizeof(edid_marker)) != 0) {
		/* EDID marker was not found at the zero position */
		for (offset=1; offset<length-sizeof(edid_marker); offset++) {
			if (memcmp(buffer+offset, edid_marker, sizeof(edid_marker)) == 0) {
				ceci_warn("found EDID marker at offset 0x%x - attempting to fix it", offset);
				memmove(buffer, buffer+offset, length-offset);
				break;
			}
		}
		if (offset >= length-sizeof(edid_marker)) {
			ceci_warn("could not find EDID marker in data - EDID seems invalid");
			close(fd);
			return LIBCEC_ERROR_IO;
		}
		/* Fill the missing bytes */
		i2c_message.len = offset;
		i2c_message.buf = buffer+length-offset-1;
		if (ioctl(fd, I2C_RDWR, &i2c_msgset) < 0) {
			ceci_error("failed to complete EDID readout - errno: %d", errno);
			close(fd);
			return LIBCEC_ERROR_IO;
		}
	}

	close(fd);

	return LIBCEC_SUCCESS;
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

int realtek_cec_write_message(libcec_device_handle* handle, uint8_t* buffer, size_t length)
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
		ceci_error("failed to send CEC message - errno: %d");
		return LIBCEC_ERROR_IO;
	}
	return LIBCEC_SUCCESS;
}

int realtek_cec_read_message(libcec_device_handle* handle, uint8_t* buffer, size_t length, int32_t timeout)
{
	realtek_device_handle_priv* handle_priv = __device_handle_priv(handle);
	int rcv_len;
	cec_msg msg;

	if (length > 255) {
		return LIBCEC_ERROR_INVALID_PARAM;
	}
	msg.timeout = (long)timeout;
	msg.buf = buffer;
	msg.len = (unsigned char)length;

	rcv_len = ioctl(handle_priv->cec_dev, CEC_RCV_MESSAGE, &msg);
	if (rcv_len <= 0) {
		if (errno == ETIME) {
			return LIBCEC_ERROR_TIMEOUT;
		}
		ceci_error("failed to receive CEC message - errno: %d", errno);
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
	realtek_cec_set_logical_address,
	realtek_i2c_read_edid,
	realtek_cec_read_message,
	realtek_cec_write_message,

	sizeof(realtek_device_handle_priv),
};
