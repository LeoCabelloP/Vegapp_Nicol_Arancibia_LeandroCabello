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

#include "corhoplv.h"

/*
	Look for high frequency watermark bits:

	p_corr_accum stores the last corr_accum_length correlations for each delay
	p_corr_sign stores correlation average signs where each word contains signs for the all delays (up to 16)
	Advance run pointer on run_length after call
	Increment correlation pointer after call
	Fs = 44100:
		3/1 bit watermark:
			input_length      = 252..inf
			run_length        = 126
			nr_delays         = 16
			corr_delay        = 80, 79, 78, 69, 68, 67, 57, 56, 46, 45, 44, 35, 34, 33, 23, 22
			corr_accum_length = 7
			corr_sign_length  = 256
	Fs = 48000:
		3/1 bit watermark:
			input_length      = 264..inf
			run_length        = 132
			nr_delays         = 16
			corr_delay        = 87, 86, 85, 75, 74, 73, 62, 61, 50, 49, 48, 38, 37, 36, 25, 24 
			corr_accum_length = 7
			corr_sign_length  = 256
*/
corhoplv_t::corhoplv_t(real_t _samplerate, int _nr_delays, real_t* _p_delay_coeffs, int _run_length, real_t _run_blocks_per_sec, buf_real_t& _input_buffer) {
	real_t wm_bit_length = _samplerate / _run_length / _run_blocks_per_sec;
	int wm_bit_blocks = (int)wm_bit_length;  // watermark bit length expressed in full correlation blocks
	int wm_header_blocks = round((HEADER_LENGTH - 1) * wm_bit_length * 1.035) + 2; // length of the header in run blocks
	input_buffer    = _input_buffer;
	run_offset        = 0;
	nr_delays         = _nr_delays;
	p_corr_delay      = new int32_t[nr_delays];
	for (int delind = 0; delind < nr_delays; delind++)
		p_corr_delay[delind] = round(_samplerate * _p_delay_coeffs[delind]);
	corr_accum_length = (int)wm_bit_length;
	run_length        = _run_length;
	p_corr_accum      = new real_t[nr_delays * corr_accum_length];
	for (int i = 0; i < nr_delays * corr_accum_length; i++)
		p_corr_accum[i] = 0.0;
	corr_accum_index  = 0;
	corr_sign_length  = 1 << ROUND_LOG2(wm_header_blocks);
	corr_sign_index   = 0;
	p_corr_sign       = new uint16_t[corr_sign_length];
	for (int i = 0; i < corr_sign_length; i++)
		p_corr_sign[i] = 0;
}

corhoplv_t::~corhoplv_t() {
	delete[] p_corr_delay;
	delete[] p_corr_accum;
	delete[] p_corr_sign;
}

void corhoplv_t::run() {
	uint16_t corr_sign = 0;
	for (int delind = 0; delind < nr_delays; delind++) {
		int corr_offset = run_offset - p_corr_delay[delind];
		if (corr_offset < 0) corr_offset += input_buffer.get_size();
		real_t corr_sum = 0.0;
		for (int j = 0; j < run_length; j++)
			corr_sum += input_buffer[corr_offset + j] * input_buffer[run_offset + j];
		p_corr_accum[corr_accum_length * delind + corr_accum_index] = corr_sum;
		real_t corr_avg_sum = 0.0;
		for (int j = 0; j < corr_accum_length; j++)
			corr_avg_sum += p_corr_accum[corr_accum_length * delind + j];
		int corr_bit = (corr_avg_sum < 0.0) ? 0 : 1;
		corr_sign = (corr_sign << 1) | corr_bit;
	}
	p_corr_sign[corr_sign_index] = corr_sign;
	run_offset = (run_offset + run_length) % input_buffer.get_size();
	++corr_accum_index %= corr_accum_length;
	++corr_sign_index  %= corr_sign_length;
}
