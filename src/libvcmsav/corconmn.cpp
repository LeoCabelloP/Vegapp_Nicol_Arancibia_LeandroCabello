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

#include "corconmn.h"

/*
	Look for low frequency watermark bits:

	Watermark bit rate is 75 bps
	Average delay time is 2 * 0.7709 ms for 3 bit watermark and 0.7709 ms for 1 bit watermark
	input_length must be multiple of run_length
	run_length must be even and is actually correlation length
	p_corr_accum stores the last corr_accum_length correlations for each delay
	p_corr_avgs stores the last corr_avgs_length correlations sums for each delay
	p_corr_sign stores correlation average signs where each word contains signs for the all delays (up to 16)
	Advance run pointer on run_length after call
	Increment correlation pointer after call
	Fs = 44100:
		3 bit watermark:
			input_length      = 126..inf
			run_length        = 42
			nr_delays         = 15
			corr_delay        = 75..61
			corr_accum_length = 6
			corr_avgs_length  = 7
			corr_sign_length  = 8
		1 bit watermark:
			input_length      = 126..inf
			run_length        = 42
			nr_delays         = 9
			corr_delay        = 37..31
			corr_accum_length = 6
			corr_avgs_length  = 7
			corr_sign_length  = 8
	Fs = 48000:
		3 bit watermark:
			input_length      = 32..inf
			run_length        = 44
			nr_delays         = 15
			corr_delay        = 81..67
			corr_accum_length = 6
			corr_avgs_length  = 7
			corr_sign_length  = 8
		1 bit watermark:
			input_length      = 132..inf
			run_length        = 44
			nr_delays         = 7
			corr_delay        = 41..33
			corr_accum_length = 6
			corr_avgs_length  = 7
			corr_sign_length  = 8
*/
corconmn_t::corconmn_t(real_t _samplerate, real_t _avg_delay_sec, int _run_length, real_t _run_blocks_per_sec, buf_real_t& _input_buffer) {
	// Calculate +-10% boundaries
	int avg_delay = round(_avg_delay_sec * _samplerate);
	int min_delay = round(0.9 * avg_delay);
	int max_delay = round(1.1 * avg_delay);
	// How many run_length correlation blocks contains one watermark bit (i.e. echo repeats in consequent run blocks)
	// 14.0 blocks for Fs = 44100, 14,55 blocks for Fs = 48000
	// Each second contains 75 correlation watermark bits (echoes)
	real_t wm_bit_length = _samplerate / _run_length / _run_blocks_per_sec;
	input_buffer      = _input_buffer; // output buffer of band filter
	corr_delay        = max_delay;
	run_offset        = 0;
	nr_delays         = max_delay - min_delay + 1; // length of 20% interval in samples
	run_length        = _run_length;
	corr_accum_length = round(0.5 * wm_bit_length) - 1;  // do average for the half of repeat length
	p_corr_accum      = new real_t[corr_accum_length * nr_delays];
	for (int i = 0; i < corr_accum_length * nr_delays; i++)
		p_corr_accum[i] = 0.0;
	corr_accum_index  = 0;
	corr_avgs_length  = round(0.5 * wm_bit_length);
	p_corr_avgs       = new real_t[corr_avgs_length * nr_delays];
	for (int i = 0; i < corr_avgs_length * nr_delays; i++)
		p_corr_avgs[i] = 0.0;
	corr_avgs_index   = 0;
	corr_sign_length  = LF_CORRELATIONS;
	p_corr_sign       = new uint16_t[corr_sign_length];
	for (int i = 0; i < corr_sign_length; i++)
		p_corr_sign[i] = 0;
	corr_sign_index   = 0;
}

corconmn_t::~corconmn_t() {
	delete[] p_corr_accum;
	delete[] p_corr_avgs;
	delete[] p_corr_sign;
}

void corconmn_t::run() {
	uint16_t corr_sign = 0;
	for (int delind = 0; delind < nr_delays; delind++) {
		int corr_offset = run_offset - (corr_delay - delind);
		if (corr_offset < 0) corr_offset += input_buffer.get_size();
		real_t corr_sum = 0.0;
		// Use each 2'nd sample for calculation
		for (int j = 0; j < run_length; j += 2)
			corr_sum += input_buffer[corr_offset + j] * input_buffer[run_offset + j];
		real_t corr_avg_sum = corr_sum;
		// Calculate correlation average sum for each delay of 20% range:
		for (int j = 0; j < corr_accum_length; j++)
			corr_avg_sum += p_corr_accum[corr_accum_length * delind + j];
		// Save current correlation at corr_accum_index
		p_corr_accum[corr_accum_length * delind + corr_accum_index] = corr_sum;
		// Calculate corr_bit as 1 if current correlation average < last correlation average  
		int corr_bit = (corr_avg_sum < p_corr_avgs[corr_avgs_length * delind + corr_avgs_index]) ? 1 : 0;
		// Save current correlation average at corr_avgs_index
		p_corr_avgs[corr_avgs_length * delind + corr_avgs_index] = corr_avg_sum;
		// corr_sign contains 1 bit for each delay (up to 16 delays)
		corr_sign = (corr_sign << 1) | corr_bit;
	}
	p_corr_sign[corr_sign_index] = corr_sign;
	run_offset = (run_offset + run_length) % input_buffer.get_size();
	++corr_accum_index %= corr_accum_length;
	++corr_avgs_index  %= corr_avgs_length;
	++corr_sign_index  %= corr_sign_length;
}
