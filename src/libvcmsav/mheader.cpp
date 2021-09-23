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

#include "mheader.h"

/*
	Look for high frequency watermark packet header
*/
mheader_t::mheader_t(real_t _samplerate, int _run_length, real_t _run_blocks_per_sec, int _mheader_hop_sequence_length, uint8_t* _p_mheader_hop_sequence, uint16_t* _p_corr_sign) {
	real_t wm_bit_length = _samplerate / _run_length / _run_blocks_per_sec;
	int wm_bit_blocks = (int)wm_bit_length;
	int wm_header_blocks = round((HEADER_LENGTH - 1) * wm_bit_length * 1.035) + 2;
	header_rest_length  = HEADER_LENGTH - ROUND_LOG2(wm_header_blocks);
	corr_max_rank       = HEADER_MISMATCHES; // permits up to 5 mismatches in 32 bit header
	header_length       = HEADER_LENGTH;
	p_corr_sign_seq     = HEADER_PATTERN; // header pattern (0x59f1ba84)
	hop_sequence_length = _mheader_hop_sequence_length;
	p_hop_sequence      = _p_mheader_hop_sequence;
	nr_delays           = SCANNERS;
	p_header_delay      = new int32_t[nr_delays];
	for (int delind = 0; delind < nr_delays; delind++) {
		real_t bit_interval = wm_bit_length * BIT_INTERVAL_COEFF_AVG[delind];
		int wm_header_interval = round(bit_interval * (header_length - 1) + 2.0);
		p_header_delay[delind] = round(bit_interval * (1 << header_rest_length));
	}
	p_corr_sign         = _p_corr_sign;
	header_interval     = 1 << header_rest_length;
	header_index        = header_interval / 2;
	p_corr_rank         = new int8_t[nr_delays];
	for (int delind = 0; delind < nr_delays; delind++) p_corr_rank[delind] = 0;
}

mheader_t::~mheader_t() {
	delete[] p_header_delay;
	delete[] p_corr_rank;
}

void mheader_t::run() {
	for (int delind = 0; delind < nr_delays; delind++) {
		uint8_t corr_sign_rank = corr_max_rank;
		uint32_t header_bits = header_index;
		int hop_sequence_index = hop_sequence_length;
		int num_diff_bits = 0;
		for (int j = 0; j < header_length; j++) {
			hop_sequence_index--;
			// Obtain correlation signs for delays from hop sequence table
			int corr_sign_index = header_bits >> header_rest_length;
			int corr_delay_index = p_hop_sequence[hop_sequence_index];
			uint16_t corr_bit = (p_corr_sign[corr_sign_index] >> corr_delay_index) & 1;
			// Calculate difference between correlation sign sequence and pattern (0x59f1ba84)
			if (corr_bit != p_corr_sign_seq[header_length - 1 - j]) {
				corr_sign_rank--;
				if (corr_sign_rank == 0)
					break;
			}
			header_bits -= p_header_delay[delind];
			if (hop_sequence_index == 0)
				hop_sequence_index = hop_sequence_length;
		}
		// Save corelation rank (rank > 0 is the packet header has been possibly found)
		p_corr_rank[delind] = corr_sign_rank;
	}
	header_index += header_interval;
}
