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

#include "packet.h"

packet_t::packet_t(wm_band_t _wm_band, real_t _samplerate, real_t _avg_delay_sec, int _run_length, real_t _run_blocks_per_sec, int _mheader_hop_sequence_length, uint8_t* _p_mheader_hop_sequence, int _wm_hop_sequence_length, uint8_t* _p_wm_hop_sequence, int _packet_payload_length, int _nr_locations, int8_t* _p_corr_rank, sequence_t* _p_sequence) {
	int avg_delay = round(_avg_delay_sec * _samplerate);
	int min_delay = round(0.9 * avg_delay);
	real_t wm_bit_length  = _samplerate / _run_length / _run_blocks_per_sec;
	packet_length         = _packet_payload_length + HEADER_LENGTH;
	p_corr_rank           = _p_corr_rank;
	run_block             = 0;
	packet_header_length  = HEADER_LENGTH;
	p_sequence            = _p_sequence;
	switch (_wm_band) {
	case WM_LF:
		hop_sequence_length = 1;
		avg_delay_index     = avg_delay - min_delay;
		break;
	case WM_HF:
		hop_sequence_length = _wm_hop_sequence_length;
		avg_delay_index     = 0;
		break;
	}
	nr_locations          = _nr_locations;
	p_location            = new packet_location_t[nr_locations];
	for (int delind = 0; delind < nr_locations; delind++) {
		real_t wm_bit_length_avg;
		real_t wm_bit_length_min;
		real_t wm_bit_length_max;
		int wm_bit_blocks;
		uint8_t* p_hop_sequence; 
		switch (_wm_band) {
		case WM_LF:
			wm_bit_length_avg = wm_bit_length * (min_delay + delind) / avg_delay;
			wm_bit_length_min = wm_bit_length * (min_delay + delind - 1) / avg_delay;
			wm_bit_length_max = wm_bit_length * (min_delay + delind + 1) / avg_delay;
			wm_bit_blocks = (int)(0.5 * wm_bit_length_avg + 4.0);
			p_hop_sequence = &LF_WTM_HOP_SEQUENCE[delind];
			break;
		case WM_HF:
			wm_bit_length_avg = wm_bit_length * BIT_INTERVAL_COEFF_AVG[delind];
			wm_bit_length_min = wm_bit_length * BIT_INTERVAL_COEFF_MIN[delind];
			wm_bit_length_max = wm_bit_length * BIT_INTERVAL_COEFF_MAX[delind];
			wm_bit_blocks = (int)(wm_bit_length_avg + 4.0);
			p_hop_sequence = _p_wm_hop_sequence;
			break;
		}
		p_location[delind].rank_check_interval = wm_bit_blocks;
		p_location[delind].rank_check_index    = 0;
		p_location[delind].rank_2_sum          = 0;
		p_location[delind].rank_mul_sum        = 0;
		p_location[delind].recalculate         = 0;
		p_location[delind].avg_bit_length_1    = wm_bit_length_avg;
		p_location[delind].avg_bit_length_2    = wm_bit_length_avg;
		p_location[delind].min_packet_length   = round(packet_length * wm_bit_length_min);
		p_location[delind].max_packet_length   = round(packet_length * wm_bit_length_max);
		p_location[delind].p_hop_sequence      = p_hop_sequence;
	}
}

packet_t::~packet_t() {
	delete[] p_location;
}

void packet_t::run() {
	for (int delind = 0; delind < nr_locations; delind++) {
		int8_t corr_rank_2 = p_corr_rank[delind] * p_corr_rank[delind];
		if (run_block == p_location[delind].next_rank_offset)
			p_location[delind].recalculate = 0;
		if (corr_rank_2 != 0 && p_location[delind].rank_check_index == 0)
			p_location[delind].rank_check_index = p_location[delind].rank_check_interval;
		if (p_location[delind].rank_check_index != 0) {
			p_location[delind].rank_check_index--;
			p_location[delind].rank_2_sum += corr_rank_2;
			// Check watermark if there is enough statistic after the first correlation hit (rank_check_interval subsequent hits summed)
			if (p_location[delind].rank_check_index == 0) {
				// Estimate the mean value of rank_check_index
				real_t rank_check_mean = (real_t)p_location[delind].rank_mul_sum / (real_t)p_location[delind].rank_2_sum;
				int rank_check_adjustment = round(rank_check_mean);
				int rank_check_offset = run_block - rank_check_adjustment;
				if (p_location[delind].recalculate)
					if (rank_check_offset - p_location[delind].min_packet_offset >= 0 && rank_check_offset - p_location[delind].max_packet_offset <= 0)
						p_location[delind].avg_bit_length_1 = (real_t)(rank_check_offset - p_location[delind].rank_check_offset) / (real_t)packet_length;
				real_t packet_offset_adjustment = 
					0.5 * (real_t)packet_header_length * (p_location[delind].avg_bit_length_1 - p_location[delind].avg_bit_length_2)
					+ p_location[delind].avg_bit_length_2 - rank_check_mean;
				if (packet_offset_adjustment > 0.0)
					packet_offset_adjustment += 0.5;
				else
					packet_offset_adjustment -= 0.5;
				int packet_adjusted_offset = run_block + (int)packet_offset_adjustment + 1;
				// Find a slot for the watermark packet
				int slot = p_sequence->p_scanner->find_slot(p_location[delind].rank_2_sum);
				if (p_sequence->p_scanner[slot].rank < p_location[delind].rank_2_sum) {
					// Allocate a new scanner for the watermark packet which has been possibly found
					p_sequence->p_scanner->alloc_slot(
						slot,
						delind,
						nr_locations,
						p_location[delind].avg_bit_length_1,
						packet_adjusted_offset,
						p_location[delind].p_hop_sequence,
						hop_sequence_length,
						p_location[delind].rank_2_sum
					);
					// Reset assemblers
					for (int i = 0; i < ASSEMBLERS_PER_SCANNER; i++) {
						assembler_t* p_assembler = p_sequence->p_scanner[slot].p_assembler[i];
						switch (p_sequence->wm_type) {
						case WM_3BIT:
							p_assembler->reset_wm_3bit(p_sequence->wm_band, p_sequence->wm_type, delind + 1);
							break;
						case WM_1BIT:
							p_assembler->reset_wm_1bit(p_sequence->wm_band, p_sequence->wm_type, delind + 1);
							break;
						}
					}
				}
				if (delind == avg_delay_index) {
					real_t delta = p_location[delind].avg_bit_length_1 / p_location[delind].avg_bit_length_2 - 1.0;
					if (fabs(delta) > 0.0003) {
						// Readjust the packet position
						real_t rank_check_adjustment = p_location[delind].avg_bit_length_1 - (float)p_location[delind].rank_mul_sum / (float)p_location[delind].rank_2_sum;
						int rank_check_offset = (int)(rank_check_adjustment + 0.5);
						int slot = p_sequence->p_scanner->find_slot(p_location[delind].rank_2_sum);
						if (p_sequence->p_scanner[slot].rank < p_location[delind].rank_2_sum) {
							int packet_adjusted_offset = run_block + rank_check_offset + 1;
							p_sequence->p_scanner->alloc_slot(
								slot,
								delind,
								nr_locations,
								p_location[delind].avg_bit_length_1,
								packet_adjusted_offset,
								p_location[delind].p_hop_sequence,
								hop_sequence_length,
								p_location[delind].rank_2_sum
							);
							for (int i = 0; i < ASSEMBLERS_PER_SCANNER; i++) {
								assembler_t* p_assembler = p_sequence->p_scanner[slot].p_assembler[i];
								switch (p_sequence->wm_type) {
								case WM_3BIT:
									p_assembler->reset_wm_3bit(p_sequence->wm_band, p_sequence->wm_type, 0);
									break;
								case WM_1BIT:
									p_assembler->reset_wm_1bit(p_sequence->wm_band, p_sequence->wm_type, 0);
									break;
								}
							}
						}
					}
				}
				p_location[delind].rank_mul_sum      = 0;
				p_location[delind].rank_2_sum        = 0;
				p_location[delind].recalculate       = 1;
				p_location[delind].min_packet_offset = run_block + p_location[delind].min_packet_length;
				p_location[delind].max_packet_offset = run_block + p_location[delind].max_packet_length;
				p_location[delind].rank_check_offset = rank_check_offset;
				p_location[delind].next_rank_offset  = run_block + p_location[delind].max_packet_length + p_location[delind].rank_check_interval;
			}
			else
				// The first hit has maximum weight rank_check_interval-1..1, calculate the mean value of rank_check_index
				p_location[delind].rank_mul_sum += p_location[delind].rank_check_index * corr_rank_2;
		}
	}
	run_block++;
}
