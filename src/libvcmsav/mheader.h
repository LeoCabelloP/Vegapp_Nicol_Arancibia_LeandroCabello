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

#ifndef _MHEADER_H
#define _MHEADER_H

#include "common.h"
#include "detector.h"

class mheader_t {
public:
	int8_t*   p_corr_rank;
	uint16_t* p_corr_sign;
	int       header_index;
	int       header_interval;
	int       header_rest_length;
	int       nr_delays;
	int32_t*  p_header_delay;
	uint8_t*  p_corr_sign_seq;
	int       hop_sequence_length;
	uint8_t*  p_hop_sequence;
	uint8_t   corr_max_rank;
	int       header_length;
public:
	mheader_t(real_t samplerate, int _run_length, real_t _run_blocks_per_sec, int _mheader_hop_sequence_length, uint8_t* _p_mheader_hop_sequence, uint16_t* p_corr_sign);
	~mheader_t();
	void run();
};

#endif
