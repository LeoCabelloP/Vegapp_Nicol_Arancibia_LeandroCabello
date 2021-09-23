/*
	Audio watermark detector

	Audio watermark detector is free software; you can redistribute it and/or
	modify it under the terms of the GNU Lesser General Public License
	as published by the Free Software Foundation; either version 2.1 of the
	License, or (at your option) any later version.

	Audio watermark detector is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
	Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public
	License along with Audio watermark detector; if not, write to the Free Software
	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/

#ifndef _PACKET_H
#define _PACKET_H

#include "common.h"
#include "detector.h"
#include "sequence.h"

class packet_location_t {
public:
	int      rank_check_interval;
	int      rank_check_index;
	int      rank_2_sum;
	int      rank_mul_sum;
	int      recalculate;
	int      min_packet_offset;
	int      max_packet_offset;
	int      next_rank_offset;
	real_t   avg_bit_length_1;
	real_t   avg_bit_length_2;
	int      rank_check_offset;
	int      min_packet_length;
	int      max_packet_length;
	uint8_t* p_hop_sequence;
};

class packet_t {
	int8_t*            p_corr_rank;
	int                run_block;
	int                packet_header_length;
	int                packet_length;
	sequence_t*        p_sequence;
	int                hop_sequence_length;
	int                avg_delay_index;
	int                nr_locations;
	packet_location_t* p_location;
public:
	packet_t(wm_band_t wm_band, real_t samplerate, real_t avg_delay_sec, int run_length, real_t run_blocks_per_sec, int mheader_hop_sequence_length, uint8_t* p_mheader_hop_sequence, int wm_hop_sequence_length, uint8_t* p_wm_hop_sequence, int packet_payload_length, int nr_locations, int8_t* p_corr_rank, sequence_t* p_sequence);
	~packet_t();
	void run();
};

#endif
