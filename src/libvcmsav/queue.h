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

#ifndef _QUEUE_H
#define _QUEUE_H

#include "common.h"
#include "detector.h"
#include "corconmn.h"
#include "sheader.h"
#include "corhoplv.h"
#include "mheader.h"
#include "packet.h"
#include "sequence.h"

class lf_queue_t {
public:
	corconmn_t* corconmn;
	sheader_t*  sheader;
	packet_t*   packet;
	sequence_t* sequence;
	lf_queue_t(wm_type_t wm_type, real_t samplerate, real_t avg_delay_sec, int run_length, real_t run_blocks_per_sec, int packet_payload_length, buf_real_t& input_buffer, watermarks_t* p_watermarks, int scanners, int watermark_bits, int watermark_blocks, int wm8_block_index, int wm3_bit_index, uint8_t* p_wm);
	~lf_queue_t();
	void run();
};

class hf_queue_t {
public:
	corhoplv_t* corhoplv;
	mheader_t*  mheader;
	packet_t*   packet;
	sequence_t* sequence;
	hf_queue_t(wm_type_t wm_type, real_t samplerate, int nr_delays, real_t* p_delay_coeffs, int run_length, real_t run_blocks_per_sec, int mheader_hop_sequence_length, uint8_t* p_mheader_hop_sequence, int wm_hop_sequence_length, uint8_t* p_wm_hop_sequence, int packet_payload_length, buf_real_t& input_buffer, watermarks_t* p_watermarks, int scanners, int watermark_bits, int watermark_blocks, int wm8_block_index, int wm3_bit_index, uint8_t* p_wm);
	~hf_queue_t();
	void run();
};

#endif
