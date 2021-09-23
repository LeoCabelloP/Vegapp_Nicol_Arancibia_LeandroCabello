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

#ifndef _CORHOPLV_H
#define _CORHOPLV_H

#include "common.h"
#include "detector.h"

class corhoplv_t {
public:
	buf_real_t input_buffer;
	uint16_t*  p_corr_sign;
	int        run_offset;
	int        nr_delays;
	int32_t*   p_corr_delay;
	int        run_length;
	int        corr_accum_length;
	real_t*    p_corr_accum;
	int        corr_accum_index;
	int        corr_sign_length;
	int        corr_sign_index;
public:
	corhoplv_t(real_t samplerate, int nr_delays, real_t* p_delay_coeffs, int run_length, real_t run_blocks_per_sec, buf_real_t& input_buffer);
	~corhoplv_t();
	void run();
};

#endif
