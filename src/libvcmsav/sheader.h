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

#ifndef _SHEADER_H
#define _SHEADER_H

#include "common.h"
#include "detector.h"

class sheader_location_t {
public:
	int       wm_bit_length;
	uint32_t* p_corr_sign;
	int       corr_sign_index;
	int       correction_interval;
	int       correction_offset;
	int       correction_index;
};

class sheader_t {
public:
	int8_t*             p_corr_rank;
	int                 corr_sign_length;
	uint16_t*           p_corr_sign;
	int                 corr_sign_index;
	uint8_t             corr_max_rank;
	uint8_t             reserved[3];
	int                 corr_sign_seq;
	int                 nr_locations;
	sheader_location_t* p_location;
public:
	sheader_t(real_t samplerate, real_t avg_delay_sec, int run_length, real_t run_blocks_per_sec, int nr_locations, uint16_t* p_corr_sign);
	~sheader_t();
	void run();
};

#endif
