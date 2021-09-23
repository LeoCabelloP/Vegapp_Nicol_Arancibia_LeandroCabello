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

#include "sheader.h"

/*
	Look for low frequency watermark packet header
*/
sheader_t::sheader_t(real_t _samplerate, real_t _avg_delay_sec, int _run_length, real_t _run_blocks_per_sec, int _nr_locations, uint16_t* _p_corr_sign) {
	int avg_delay = round(_avg_delay_sec * _samplerate);
	int min_delay = round(0.9 * avg_delay);
	real_t wm_bit_length = _samplerate / _run_length / _run_blocks_per_sec;
	p_corr_sign      = _p_corr_sign;
	corr_sign_length = LF_CORRELATIONS; // save the last 8 correlations (1 correlation bit for each run block)
	corr_sign_index  = 0;
	corr_sign_seq    = HEADER_PATTERN_INT; // header pattern (0x59f1ba84)
	nr_locations     = _nr_locations;
	corr_max_rank    = HEADER_MISMATCHES; // permits up to 5 mismatches in 32 bit header
	p_corr_rank      = new int8_t[nr_locations];
	for (int delind = 0; delind < nr_locations; delind++) p_corr_rank[delind] = 0;
	p_location       = new sheader_location_t[nr_locations];
	for (int delind = 0; delind < nr_locations; delind++) {
		real_t wm_bit_length_for_delay = wm_bit_length * (min_delay + delind) / avg_delay;
		p_location[delind].wm_bit_length       = (int)ceil(wm_bit_length_for_delay);
		p_location[delind].p_corr_sign         = new uint32_t[p_location[delind].wm_bit_length];
		for (int i = 0; i < p_location[delind].wm_bit_length; i++) p_location[delind].p_corr_sign[i] = 0;
		p_location[delind].corr_sign_index     = 0;
		// Length correction for the header bit
		p_location[delind].correction_interval = 10000;
		p_location[delind].correction_offset   = round(10000.0 * (p_location[delind].wm_bit_length - wm_bit_length_for_delay) / wm_bit_length_for_delay);
		p_location[delind].correction_index    = 0;
	}
}

sheader_t::~sheader_t() {
	delete[] p_corr_rank;
	for (int delind = 0; delind < nr_locations; delind++)
		delete[] p_location[delind].p_corr_sign;
	delete[] p_location;
}

void sheader_t::run() {
	uint16_t corr_sign;
	// Each corr_sign word contains bit set for the all calculated delays (up to 16)
	corr_sign = p_corr_sign[corr_sign_index];
	corr_sign_index++; corr_sign_index %= corr_sign_length;
	// For each delay setup p_locations[delind] to the current p_corr_sign[corr_sign_index]
	// Each dword of p_locations[delind].p_corr_sign contains correlation bits for each particular delay (up to 32 bits for each delay)
	for (int delind = 0; delind < nr_locations; delind++) {
		// Repack corr_sign into location_t structure 
		p_location[delind].p_corr_sign[p_location[delind].corr_sign_index] = (p_location[delind].p_corr_sign[p_location[delind].corr_sign_index] << 1) | ((corr_sign >> delind) & 1);
		uint32_t corr_sign_xor = p_location[delind].p_corr_sign[p_location[delind].corr_sign_index] ^ corr_sign_seq;
		int num_diff_bits = 0;
		// Calculate difference between correlation sign sequence and pattern (0x59f1ba84)
		for (int i = 0; i < 4; i++) {
			int num_diff_bits_in_byte = NUM_DIFF_BITS_TABLE[corr_sign_xor & 0xff];
			corr_sign_xor >>= 8;
			num_diff_bits += num_diff_bits_in_byte;
		}
		int8_t corr_sign_rank = (num_diff_bits < (int8_t)corr_max_rank) ? (int8_t)corr_max_rank - num_diff_bits : 0;
		// Save corelation rank (rank > 0 is the packet header has been possibly found)
		p_corr_rank[delind] = corr_sign_rank;
		++p_location[delind].corr_sign_index %= p_location[delind].wm_bit_length;
		p_location[delind].correction_index += p_location[delind].correction_offset;
		// Save extra correlation bit (due to bit interval is not multiple of run_length)
		if (p_location[delind].correction_index >= p_location[delind].correction_interval) {
			p_location[delind].correction_index -= p_location[delind].correction_interval;
			p_location[delind].p_corr_sign[p_location[delind].corr_sign_index] = (p_location[delind].p_corr_sign[p_location[delind].corr_sign_index] << 1) | ((corr_sign >> delind) & 1);
			p_location[delind].corr_sign_index++; p_location[delind].corr_sign_index %= p_location[delind].wm_bit_length;
		}
	}
}
