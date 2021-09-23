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

#ifndef _SEQUENCE_H
#define _SEQUENCE_H

#include "common.h"
#include "detector.h"

class watermarks_t {
public:
	wm_vector_t* lf_wm_3bit;
	wm_vector_t* lf_wm_1bit;
	wm_vector_t* hf_wm_3bit;
	wm_vector_t* hf_wm_1bit;
	uint32_t     wm_checks;
	char         wm_4C12bit[4 + 8 + 60 + 1];
public:
	watermarks_t();
	watermarks_t(watermarks_t& watermarks);
	~watermarks_t();
	void reset_4C12bit();
	int set_4C12bit(wm_type_t wm_type, int wm_bits, int* p_wm_bit);
	char* get_4C12bit();
};

class assembler_t {
public:
	uint8_t wm3_bit_rank[WTM_BLOCK_LENGTH_3BIT];
	int     wm3_bit_index;
	int     wm3_block_index;

	uint8_t wm8_bit_rank[WTM_BLOCK_LENGTH_8BIT];
	int     wm8_bit_index;
	int     wm8_block_index;

	uint8_t wm1_bit_rank[WTM_BLOCK_LENGTH_1BIT];
	int     wm1_bit_index;
	int     wm1_block_index;

	int     wm_blocks;

	uint8_t*      p_wm;
	int           wm_index;
	int           delay_index;
	wm_band_t     wm_band;
	wm_type_t     wm_type;
	watermarks_t* p_watermarks;
public:
	assembler_t();
	~assembler_t();
	void reset_wm_3bit(wm_band_t wm_band, wm_type_t wm_type, int delay_index);
	int  check_wm_3bit(int corr_bit);
	void reset_wm_1bit(wm_band_t wm_band, wm_type_t wm_type, int delay_index);
	int  check_wm_1bit(int corr_bit);
	bool check_watermark(int wm_bits, uint8_t* p_wm_rank);
};

class scanner_t {
public:
	int          delind;
	int          nr_delays;
	int          rank;
	int          wm_block_first;
	int          wm_block_next;
	real_t       wm_offset;
	real_t       wm_interval;
	uint8_t*     p_hop_sequence;
	int          hop_sequence_length;
	int          hop_sequence_index;
	assembler_t* p_assembler[ASSEMBLERS_PER_SCANNER];
	int          wm_check_code;
public:
	scanner_t();
	void alloc_slot(int slot, int delind, int nr_delays, real_t interval, int offset, uint8_t* p_hop_sequence, int hop_sequence_length, int rank);
	int  find_slot(int rank);
	bool is_active(int slot) {
		return this[slot].rank > 0;
	};
	void free(int slot) {
		this[slot].rank = 0;
	};
};

class sequence_t {
public:
	int        bit_input_length;
	uint16_t*  p_corr_sign;
	int        nr_scanners;
	scanner_t* p_scanner;
	int        run_block;
	wm_band_t  wm_band;
	wm_type_t  wm_type;
public:
	sequence_t(wm_band_t wm_band, wm_type_t wm_type, int bit_input_length, uint16_t* p_corr_sign, watermarks_t* p_watermarks, int scanners, int watermark_bits, int watermark_blocks, int wm8_block_index, int wm3_bit_index, uint8_t* p_wm);
	~sequence_t();
	void run();
};

#endif
