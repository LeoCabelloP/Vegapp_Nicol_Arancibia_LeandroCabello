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

#ifndef _CORCONMN_H
#define _CORCONMN_H

#include "common.h"
#include "detector.h"

class corconmn_t {
public:
	buf_real_t input_buffer;
	uint16_t*  p_corr_sign;
	int        corr_delay;
	int        run_offset;
	int        nr_delays;
	int        run_length;
	int        corr_accum_length;
	real_t*    p_corr_accum;
	int        corr_accum_index;
	int        corr_avgs_length;
	real_t*    p_corr_avgs;
	int        corr_avgs_index;
	int        corr_sign_length;
	int        corr_sign_index;
public:
	corconmn_t(real_t samplerate, real_t avg_delay_sec, int run_length, real_t run_blocks_per_sec, buf_real_t& input_buffer);
	~corconmn_t();
	void run();
};

#endif
