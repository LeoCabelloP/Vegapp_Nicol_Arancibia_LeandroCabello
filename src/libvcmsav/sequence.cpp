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

#include "sequence.h"

watermarks_t::watermarks_t(watermarks_t& _watermarks) {
	lf_wm_3bit = new wm_vector_t(_watermarks.lf_wm_3bit->length());
	lf_wm_1bit = new wm_vector_t(_watermarks.lf_wm_1bit->length());
	hf_wm_3bit = new wm_vector_t(_watermarks.hf_wm_3bit->length());
	hf_wm_1bit = new wm_vector_t(_watermarks.hf_wm_1bit->length());
	wm_checks  = 0;
	for (int i = 0; i < sizeof(wm_4C12bit); i++)
		wm_4C12bit[i] = '\0';
	for (int i = 0; i < 4 + 8; i++)
		wm_4C12bit[i] = 'X';
}
watermarks_t::watermarks_t() {
	lf_wm_3bit = 0;
	lf_wm_1bit = 0;
	hf_wm_3bit = 0;
	hf_wm_1bit = 0;
	wm_checks  = 0;
	for (int i = 0; i < sizeof(wm_4C12bit); i++)
		wm_4C12bit[i] = '\0';
	for (int i = 0; i < 4 + 8; i++)
		wm_4C12bit[i] = 'X';
}


watermarks_t::~watermarks_t() {
	if (lf_wm_3bit)
		delete lf_wm_3bit;
	if (lf_wm_1bit)
		delete lf_wm_1bit;
	if (hf_wm_3bit)
		delete hf_wm_3bit;
	if (hf_wm_1bit)
		delete hf_wm_1bit;
}

void watermarks_t::reset_4C12bit() {
	lf_wm_3bit->reset();
	lf_wm_1bit->reset();
	hf_wm_3bit->reset();
	hf_wm_1bit->reset();
	wm_checks = 0;
	for (int i = 0; i < 4 + 8; i++)
		wm_4C12bit[i] = 'X';
}

/*
CCI:
	C3,C2 = 00b - copying is permitted without restriction -- “Copy Freely”
	C3,C2 = 10b - copy one generation is permitted -- “Copy One Generation”
	C3,C2 = 11b - no more copies are permitted -- “No More Copies”
	C3,C2 = 01b - reserved for future use
SDMI Trigger Bit:
	C1 = 0b - trigger is not present
	C1 = 1b - trigger is present
Reserved bit:
	C0 = 0b - shall be set to “0b” until further notice
wm_4C12bit[0] - C3
wm_4C12bit[1] - C2
wm_4C12bit[2] - C1
wm_4C12bit[3] - C0
*/
int watermarks_t::set_4C12bit(wm_type_t _wm_type, int _wm_bits, int* _p_wm_bit) {
	switch (_wm_type) {
	case WM_3BIT:
		if (_wm_bits == 8) {
			// setup RSVD bits
			for (int i = 0; i < _wm_bits; i++)
				wm_4C12bit[4 + i] = '0' + _p_wm_bit[i];
		}
		else if ((wm_checks & 1) == 0) {
			// setup ?0?? or ?1?? CNTRL bits
			wm_checks |= 1;
			if ((wm_checks & 2) == 0)
				wm_4C12bit[1] = '0';
			else
				wm_4C12bit[1] = '1';
			wm_4C12bit[0] = '0' + _p_wm_bit[0];
			wm_4C12bit[2] = '0' + _p_wm_bit[1];
			wm_4C12bit[3] = '0' + _p_wm_bit[2];
		}
		break;
	case WM_1BIT:
		if ((wm_checks & 2) == 0) {
			// setup ?1XX CNTRL bits
			wm_checks |= 2;
			wm_4C12bit[1] = '1';
			if ((wm_checks & 1) == 0) {
				wm_4C12bit[0] = '0' + _p_wm_bit[0];
				wm_4C12bit[2] = 'X';
				wm_4C12bit[3] = 'X';
			}
		}
		break;
	}
	return 1;
}

char* watermarks_t::get_4C12bit() {
	return wm_4C12bit;
}

assembler_t::assembler_t() {
	memset(this, 0, sizeof(*this));
}

assembler_t::~assembler_t() {
}

void assembler_t::reset_wm_3bit(wm_band_t _wm_band, wm_type_t _wm_type, int _delay_index) {
	wm3_bit_index   = 0;
	wm3_block_index = 0;
	wm8_bit_index   = 0;
	wm8_block_index = 0;
	wm_blocks       = 0;
	for (int i = 0; i < WTM_BLOCK_LENGTH_3BIT; i++)	wm3_bit_rank[i]  = 0;
	for (int i = 0; i < WTM_BLOCK_LENGTH_8BIT; i++)	wm8_bit_rank[i]  = 0;
	wm_index        = 0;
	delay_index     = _delay_index;
	wm_band         = _wm_band;
	wm_type         = _wm_type;
}

int assembler_t::check_wm_3bit(int _corr_bit) {
	int wm_hit = p_wm[wm_index] ^ _corr_bit;
	// Switch between looking for 3/8 bit part of watermark
	if (wm_blocks) {
		if (wm_hit)
			wm3_bit_rank[wm3_bit_index]++;
		wm3_bit_index++;
		if (wm3_bit_index == WTM_BLOCK_LENGTH_3BIT) {
			wm3_bit_index = 0;
			wm3_block_index++;
			if (wm3_block_index == WTM_BLOCKS_3BIT) {
				bool wm3_found, wm8_found;
				wm3_found = check_watermark(WTM_BLOCK_LENGTH_3BIT, wm3_bit_rank);
				if (wm3_found)
					wm8_found = check_watermark(WTM_BLOCK_LENGTH_8BIT, wm8_bit_rank);
				else
					wm8_found = false;
				return ((int)wm8_found << 2) | ((int)wm3_found << 1) | 1;
			}
		}
	}
	else {
		if (wm8_block_index < WTM_BLOCKS_8BIT) {
			if (wm_hit)
				wm8_bit_rank[wm8_bit_index]++;
			wm8_bit_index++;
			if (wm8_bit_index == WTM_BLOCK_LENGTH_8BIT) {
				wm8_bit_index = 0;
				wm8_block_index++;
			}
		}
	}
	wm_blocks = wm_blocks ^ 1;
	wm_index++;
	return 0;
}

void assembler_t::reset_wm_1bit(wm_band_t _wm_band, wm_type_t _wm_type, int _delay_index) {
	wm1_bit_index   = 0;
	wm1_block_index = 0;
	wm_blocks       = 0;
	for (int i = 0; i < WTM_BLOCK_LENGTH_1BIT; i++)	wm1_bit_rank[i]  = 0;
	wm_index        = 0;
	delay_index     = _delay_index;
	wm_band         = _wm_band;
	wm_type         = _wm_type;
}

int assembler_t::check_wm_1bit(int _corr_bit) {
	int wm_hit = p_wm[wm_index] ^ _corr_bit;
	if (wm_hit)
		wm1_bit_rank[wm1_bit_index]++;
	wm1_bit_index++;
	if (wm1_bit_index == WTM_BLOCK_LENGTH_1BIT) {
		wm1_bit_index = 0;
		wm1_block_index++;
		if (wm1_block_index == WTM_BLOCKS_1BIT) {
			bool wm1_found = check_watermark(WTM_BLOCK_LENGTH_1BIT, wm1_bit_rank);
			return ((int)wm1_found << 3) | 1;
		}
	}
	wm_index++;
	return 0;
}

bool assembler_t::check_watermark(int _wm_bits, uint8_t* _p_wm_rank) {
	wm_vector_t* p_wm_vector;
	int p_wm_bit[8];
	int wm_0_max_level, wm_blocks_1 = _wm_bits, wm_blocks_2, wm_level_correction;
	bool check_average_rank = false;
	for (int i = 0; i < 8; i++) p_wm_bit[i] = 'X' - '0';
	switch (wm_type) {
	case WM_3BIT:	// setup 3 or 8 watermark bits
		switch (_wm_bits) {
		case 3:	// setup C3, C1, C0 CNTRL bits
			wm_blocks_1         = WTM_BLOCKS_3BIT;
			wm_blocks_2         = WTM_BLOCKS_8BIT;
			wm_0_max_level      = WTM_BLOCKS_3BIT_MISMATCHES;
			wm_level_correction = WTM_BLOCKS_8BIT_CORRECTION;
			switch (wm_band) {
			case WM_LF:
				p_wm_vector = p_watermarks->lf_wm_3bit;
				break;
			case WM_HF:
				p_wm_vector = p_watermarks->hf_wm_3bit;
				break;
			}
			check_average_rank  = true;
			break;
		case 8:	// setup RSVD bits
			wm_blocks_1         = WTM_BLOCKS_8BIT;
			wm_0_max_level      = WTM_BLOCKS_8BIT_MISMATCHES;
			break;
		}
		break;
	case WM_1BIT: // setup 1 watermark bit
		wm_blocks_1           = WTM_BLOCKS_1BIT;
		wm_blocks_2           = WTM_BLOCKS_1BIT_FREE;
		wm_0_max_level        = WTM_BLOCKS_1BIT_MISMATCHES;
		wm_level_correction   = WTM_BLOCKS_1BIT_CORRECTION;
		switch (wm_band) {
		case WM_LF:
			p_wm_vector = p_watermarks->lf_wm_1bit;
			break;
		case WM_HF:
			p_wm_vector = p_watermarks->hf_wm_1bit;
			break;
		}
		check_average_rank  = true;
		break;
	}
	int wm_1_min_level = wm_blocks_1 - wm_0_max_level;
	for (int i = 0; i < _wm_bits; i++) {
		uint8_t wm_rank = _p_wm_rank[i];
		if (wm_rank <= wm_0_max_level)
			p_wm_bit[i] = 0;
		else if (wm_rank >= wm_1_min_level)
			p_wm_bit[i] = 1;
		else {
			if (check_average_rank) {
				int wm_bits_offset = (_wm_bits + 1) * delay_index;
				for (int i = 0; i < _wm_bits; i++)
					p_wm_vector->add(wm_bits_offset + i, _p_wm_rank[i]);
				p_wm_vector->add(wm_bits_offset + _wm_bits, 1);
				int wm_checks = p_wm_vector->get(wm_bits_offset + _wm_bits);
				if (wm_checks > 1) {
					int wm_0_avg_max_level = wm_checks * wm_blocks_2 + wm_level_correction;
					int wm_1_avg_min_level = wm_checks * wm_blocks_1 - wm_0_avg_max_level;
					for (int i = 0; i < _wm_bits; i++) {
						int wm_rank = p_wm_vector->get(wm_bits_offset + i);
						if (wm_rank <= wm_0_avg_max_level)
							p_wm_bit[i] = 0;
						else if (wm_rank < wm_1_avg_min_level)
							return false;
					}
				}
				else
					return false;
			}
			else
				return false;
		}
	}
	// Watermark has been found
	if (check_average_rank)
		p_wm_vector->reset();
	p_watermarks->set_4C12bit(wm_type, _wm_bits, p_wm_bit);
	return true;
}

scanner_t::scanner_t() {
	memset(this, 0, sizeof(*this));
}

void scanner_t::alloc_slot(int _slot, int _delind, int _nr_delays, real_t _interval, int _offset, uint8_t* _p_hop_sequence, int _hop_sequence_length, int _rank) {
	this[_slot].delind              = _delind;
	this[_slot].nr_delays           = _nr_delays;
	this[_slot].wm_interval         = _interval;
	this[_slot].wm_block_first      = _offset;
	this[_slot].wm_block_next       = _offset;
	this[_slot].hop_sequence_length = _hop_sequence_length;
	this[_slot].p_hop_sequence      = _p_hop_sequence;
	this[_slot].rank                = _rank;
	this[_slot].wm_offset           = 0.0;
	this[_slot].hop_sequence_index  = 0;
	this[_slot].wm_check_code       = 0;
}

int scanner_t::find_slot(int rank) {
	int min_rank = rank;
	int slot = 0;
	for (int i = 0; i < SCANNERS; i++) {
		if (this[i].rank < min_rank) {
			min_rank = this[i].rank;
			slot = i;
			if (min_rank == 0)
				break;
		}
	}
	return slot;
}

sequence_t::sequence_t(wm_band_t _wm_band, wm_type_t _wm_type, int _bit_input_length, uint16_t* _p_corr_sign, watermarks_t* _p_watermarks, int _scanners, int _watermark_bits, int _watermark_blocks, int _wm8_block_index, int _wm3_bit_index, uint8_t* _p_wm) {
	bit_input_length = _bit_input_length;
	p_corr_sign      = _p_corr_sign;
	nr_scanners      = _scanners;
	p_scanner        = new scanner_t[nr_scanners];
	if (bit_input_length <= 12)
		run_block = -(bit_input_length - ASSEMBLERS_PER_SCANNER);
	else if (bit_input_length < 128)
		run_block      = -16;
	else
		run_block      = -(bit_input_length >> ASSEMBLERS_PER_SCANNER);
	wm_band          = _wm_band;
	wm_type          = _wm_type;
	for (int slot = 0; slot < nr_scanners; slot++) {
		for (int a = 0; a < ASSEMBLERS_PER_SCANNER; a++) {
			assembler_t* p_assembler = new assembler_t;
			p_scanner[slot].p_assembler[a] = p_assembler;
			p_assembler->p_watermarks = _p_watermarks;
			p_assembler->p_wm         = _p_wm;
			p_assembler->wm_band      = wm_band;
			p_assembler->wm_type      = wm_type;
		}
	}
}

sequence_t::~sequence_t() {
	delete[] p_scanner;
}

void sequence_t::run() {
	for (int slot = 0; slot < nr_scanners; slot++) {
		if (p_scanner->is_active(slot)) {
			if (run_block >= p_scanner[slot].wm_block_next) {
				// The first watermark bit has been reached
				int delay_index = p_scanner[slot].p_hop_sequence[p_scanner[slot].hop_sequence_index];
				for (int i = 0; i < ASSEMBLERS_PER_SCANNER; i++) {
					int corr_sign_index = (bit_input_length - 1) & (run_block - i);
					uint16_t corr_sign = p_corr_sign[corr_sign_index];
					int corr_bit = (corr_sign >> delay_index) & 1;
					assembler_t* p_assembler = p_scanner[slot].p_assembler[i];
					switch (p_assembler->wm_type) {
					case WM_3BIT:
						p_scanner[slot].wm_check_code |= p_assembler->check_wm_3bit(corr_bit);
						break;
					case WM_1BIT:
						p_scanner[slot].wm_check_code |= p_assembler->check_wm_1bit(corr_bit);
						break;
					}
				}
				if (p_scanner[slot].wm_check_code == 0) {
					// Advance to the next watermark bit (watermark test is in progress)
					p_scanner[slot].wm_offset += p_scanner[slot].wm_interval;
					p_scanner[slot].wm_block_next = p_scanner[slot].wm_block_first + round(p_scanner[slot].wm_offset);
					++p_scanner[slot].hop_sequence_index %= p_scanner[slot].hop_sequence_length;
				}
				else {
					// Release used slot at the end of analysis
					p_scanner->free(slot);
				}
			}
		}
	}
	run_block++;
}
