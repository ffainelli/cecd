/*
 * decoder - CEC message decoding
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

#include <stdint.h>

typedef struct {
	uint16_t original_address;
	uint16_t new_address;
} cec_op_routing_change;

typedef struct {
	uint16_t transport_stream_id;
	uint16_t service_id;
	uint16_t original_network_id;
} cec_op_arib_dvb_data;

typedef struct {
	uint16_t transport_stream_id;
	uint16_t program_number;
	uint16_t reserved;
} cec_op_atsc_data;

typedef struct {
	uint16_t channel_number_high;
	uint16_t channel_number_low;
} cec_op_channel_data;

typedef struct {
	uint8_t record_source_type;					// 1
	union {
		struct {
			uint8_t method_and_broadcast;		// 1
			union {
				cec_op_arib_dvb_data arib;		// 6
				cec_op_atsc_data     atsc;		// 6
				cec_op_arib_dvb_data dvb;		// 6
				cec_op_channel_data  channel;	// 4
			} service;							// [4-6]
		} digital;								// [5-7]
		struct {
			uint8_t  broadcast_type;
			uint16_t frequency;
			uint8_t  broadcast_system;
		} analogue;								// 4
		union {
			uint8_t plug;
			uint16_t physical_address;
		} external;								// 3
	} source;									// [3-7]
} cec_op_record_source;							// [4-8]

typedef struct {
	uint8_t day;
	uint8_t month;
	struct {
		uint8_t hour;
		uint8_t minute;
	} start_time;
	struct {
		uint8_t hours;
		uint8_t minutes;
	} duration;
	uint8_t recording_sequence;
	uint8_t  broadcast_type;
	uint16_t frequency;
	uint8_t  broadcast_system;
} cec_op_analogue_timer;

typedef struct {
	uint8_t day;
	uint8_t month;
	struct {
		uint8_t hour;
		uint8_t minute;
	} start_time;
	struct {
		uint8_t hours;
		uint8_t minutes;
	} duration;
	uint8_t recording_sequence;
	struct {
		uint8_t method_and_broadcast;
		union {
			cec_op_arib_dvb_data arib;
			cec_op_atsc_data     atsc;
			cec_op_arib_dvb_data dvb;
			cec_op_channel_data  channel;
		} service;
	} digital;							// 7
} cec_op_digital_timer;

typedef struct {
	uint8_t day;
	uint8_t month;
	struct {
		uint8_t hour;
		uint8_t minute;
	} start_time;
	struct {
		uint8_t hours;
		uint8_t minutes;
	} duration;
	uint8_t recording_sequence;
	uint8_t source_specifier;
	union {
		uint8_t plug;
		uint16_t physical_address;
	} external;
} cec_op_external_timer;

typedef struct {
	uint8_t info;
	uint16_t duration_available;
} cec_op_timer_status_data;
