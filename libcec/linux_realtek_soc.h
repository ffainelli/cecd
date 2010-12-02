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

struct realtek_device_handle_priv {
	int cec_dev;
	int i2c_dev;
};

static inline struct realtek_device_handle_priv *__device_handle_priv(struct libcec_device_handle *handle)
{
	return (struct realtek_device_handle_priv *) handle->priv;
}
