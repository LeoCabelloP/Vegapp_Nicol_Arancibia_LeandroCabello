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

#include <string>
#include "wm_check.h"

wm_check_t::wm_check_t() {
	memset(this, 0, sizeof(*this));
}

wm_check_t::~wm_check_t() {
	delete lf_queue_3bit;
	delete lf_queue_1bit;
	delete hf_queue_3bit;
	delete hf_queue_1bit;
}

bool wm_check_t::init(int _channel, int _samplerate, buf_real_t& _lfi_buf, buf_real_t& _hfi_buf, int _log_level) {
	log_level = _log_level;
	//log_level = TRACE_LEVEL;
	channel =_channel;
	samplerate = _samplerate;
	switch (samplerate) {
	case 44100:
		lf_run_length = 42;
		hf_run_length = 3 * 42;
		break;
	case 48000:
		lf_run_length = 44;
		hf_run_length = 3 * 44;
		break;
	default:
		return false;
		break;
	}
	// Set the controls for the heart of the sun
	lf_queue_3bit = new lf_queue_t(WM_3BIT, samplerate, ECHO_DELAY * 2.0,           lf_run_length, LF_WM_PER_SECOND,                                                               WTM_LENGTH_3BIT, _lfi_buf, &watermarks, SCANNERS, WTM_BLOCK_LENGTH_3BIT, WTM_BLOCKS_3BIT, 2, 1, WTM_PATTERN_3BIT);
	lf_queue_1bit = new lf_queue_t(WM_1BIT, samplerate, ECHO_DELAY * 1.0,           lf_run_length, LF_WM_PER_SECOND,                                                               WTM_LENGTH_1BIT, _lfi_buf, &watermarks, SCANNERS, WTM_BLOCK_LENGTH_1BIT, WTM_BLOCKS_1BIT, 1, 0, WTM_PATTERN_1BIT);
	hf_queue_3bit = new hf_queue_t(WM_3BIT, samplerate, HF_DELAYS, HF_DELAY_COEFFS, hf_run_length, HF_WM_PER_SECOND, 16, MHEADER_HOP_SEQUENCE_3BIT, 32, HF_HOP_SEQUENCE_3BIT,      WTM_LENGTH_3BIT, _hfi_buf, &watermarks, SCANNERS, WTM_BLOCK_LENGTH_3BIT, WTM_BLOCKS_3BIT, 2, 1, WTM_PATTERN_3BIT);
	hf_queue_1bit = new hf_queue_t(WM_1BIT, samplerate, HF_DELAYS, HF_DELAY_COEFFS, hf_run_length, HF_WM_PER_SECOND, 16, MHEADER_HOP_SEQUENCE_1BIT, 16, MHEADER_HOP_SEQUENCE_1BIT, WTM_LENGTH_1BIT, _hfi_buf, &watermarks, SCANNERS, WTM_BLOCK_LENGTH_1BIT, WTM_BLOCKS_1BIT, 1, 0, WTM_PATTERN_1BIT);
	checked_samples = 0;
	retime_to_sample = 0;
	return true;
}

void wm_check_t::reset() {
	watermarks.reset_4C12bit();
}

void wm_check_t::retime(double seek_time) {
	retime_to_sample = checked_samples - round(seek_time * samplerate);
}

int wm_check_t::run(int samples) {
	int nr_watermarks = 0;
	for (int i = 0; i < samples; i += lf_run_length) {
		lf_queue_3bit->run();
		nr_watermarks += test_slots(lf_queue_3bit->sequence);
		lf_queue_1bit->run();
		nr_watermarks += test_slots(lf_queue_1bit->sequence);
	}
	for (int i = 0; i < samples; i += hf_run_length) {
		hf_queue_3bit->run();
		nr_watermarks += test_slots(hf_queue_3bit->sequence);
		hf_queue_1bit->run();
		nr_watermarks += test_slots(hf_queue_1bit->sequence);
	}
	checked_samples += samples;
	return nr_watermarks;
}

int wm_check_t::test_slots(sequence_t* p_sequence) {
	int nr_watermarks = 0;
	for (int slot = 0; slot < p_sequence->nr_scanners; slot++) {
		if (p_sequence->p_scanner[slot].wm_check_code > 1) {
			p_sequence->p_scanner[slot].wm_check_code = 0;
			show_watermark(p_sequence, slot);
			nr_watermarks++;
		}
	}
	return nr_watermarks;
}

void wm_check_t::show_watermark_tempo(sequence_t* _p_sequence, int _slot, char* _str) {
	int avg_delind = 0;
	real_t tempo = 0.0;
	switch (_p_sequence->wm_band) {
	case WM_LF:
		avg_delind = (_p_sequence->p_scanner[_slot].nr_delays - 1) / 2;
		tempo = (real_t)(_p_sequence->p_scanner[_slot].delind - avg_delind) / (real_t)avg_delind * 0.1; 
		break;
	case WM_HF:
		tempo = BIT_INTERVAL_COEFF_AVG[_p_sequence->p_scanner[_slot].delind] - 1.0;
		break;
	}
	sprintf_s(_str, MSG_LENGTH, "Tempo %+06.3f", tempo);
}

void wm_check_t::show_watermark(sequence_t* _p_sequence, int _slot) {
	std::string wm_s;
	real_t wm_time;
	char str[MSG_LENGTH];
	switch (_p_sequence->wm_band) {
	case WM_LF:
		wm_time = (real_t)_p_sequence->p_scanner[_slot].wm_block_first * (real_t)lf_run_length / (real_t)samplerate;
		wm_time -= (HEADER_LENGTH + 1.0) / LF_WM_PER_SECOND;
		break;
	case WM_HF:
		wm_time = (real_t)_p_sequence->p_scanner[_slot].wm_block_first * (real_t)hf_run_length / (real_t)samplerate;
		wm_time -= (HEADER_LENGTH + 1.0) / HF_WM_PER_SECOND;
		break;
	}
	wm_time -= (real_t)retime_to_sample / (real_t)samplerate;
	wm_s += "[";
	::print_time(str, wm_time);
	wm_s += str;
	wm_s += "]";
	if (track > 0)
		sprintf_s(str, MSG_LENGTH, " - Watermark on tr %d / ch %d - ", track, channel + 1);
	else
		sprintf_s(str, MSG_LENGTH, " - Watermark on ch %d - ", channel + 1);
	wm_s += str;
	switch (_p_sequence->wm_band) {
	case WM_LF:
		sprintf_s(str, MSG_LENGTH, "LF ");
		wm_s += str;
		break;
	case WM_HF:
		sprintf_s(str, MSG_LENGTH, "HF ");
		wm_s += str;
		break;
	}
	switch (_p_sequence->wm_type) {
	case WM_3BIT:
		sprintf_s(str, MSG_LENGTH, "3/8 BIT ");
		wm_s += str;
		break;
	case WM_1BIT:
		sprintf_s(str, MSG_LENGTH, "  1 BIT ");
		wm_s += str;
		break;
	}
	sprintf_s(str, MSG_LENGTH, "[%s]", _p_sequence->p_scanner[_slot].p_assembler[0]->p_watermarks->get_4C12bit());
	wm_s += str;
	sprintf_s(str, MSG_LENGTH, " - ");
	wm_s += str;
	show_watermark_tempo(_p_sequence, _slot, str);
	wm_s += str;
	if (log_level >= TRACE_LEVEL) {
		for (int a = 0; a  < ASSEMBLERS_PER_SCANNER; a++) {
			sprintf_s(str, MSG_LENGTH, "\n");
			wm_s += str;
			assembler_t* p_assembler = _p_sequence->p_scanner[_slot].p_assembler[a];
			sprintf_s(str, MSG_LENGTH, "  ASSEMBLER(%d): ", a);
			wm_s += str;
			sprintf_s(str, MSG_LENGTH, "[");
			wm_s += str;
			switch (p_assembler->wm_type) {
				int wm_0_max;
				int wm_x_avg;
				int wm_1_min;
			case WM_3BIT:
				wm_0_max = WTM_BLOCKS_3BIT_MISMATCHES;
				wm_x_avg = WTM_BLOCKS_3BIT / 2;
				wm_1_min = WTM_BLOCKS_3BIT - WTM_BLOCKS_3BIT_MISMATCHES;
				for (int i = 0; i < WTM_BLOCK_LENGTH_3BIT; i++)	{
					int bit_rank = p_assembler->wm3_bit_rank[i];
					int wm_base = bit_rank > wm_x_avg ? wm_1_min : wm_0_max;
					char wm_bit = bit_rank <= wm_0_max ? '0' : bit_rank >= wm_1_min ? '1' : 'X';
					sprintf_s(str, MSG_LENGTH, " %c:%+03d", wm_bit, (wm_base < wm_x_avg ? -1 : 1) * (bit_rank - wm_base));
					wm_s += str;
				}
				sprintf_s(str, MSG_LENGTH, " |");
				wm_s += str;
				wm_0_max = WTM_BLOCKS_8BIT_MISMATCHES;
				wm_x_avg = WTM_BLOCKS_8BIT / 2;
				wm_1_min = WTM_BLOCKS_8BIT - WTM_BLOCKS_8BIT_MISMATCHES;
				for (int i = 0; i < WTM_BLOCK_LENGTH_8BIT; i++)	{
					int bit_rank = p_assembler->wm8_bit_rank[i];
					int wm_base = bit_rank > wm_x_avg ? wm_1_min : wm_0_max;
					char wm_bit = bit_rank <= wm_0_max ? '0' : bit_rank >= wm_1_min ? '1' : 'X';
					sprintf_s(str, MSG_LENGTH, " %c:%+03d", wm_bit, (wm_base < wm_x_avg ? -1 : 1) * (bit_rank - wm_base));
					wm_s += str;
				}
				sprintf_s(str, MSG_LENGTH, " ]");
				wm_s += str;
				break;
			case WM_1BIT:
				wm_0_max = WTM_BLOCKS_1BIT_MISMATCHES;
				wm_x_avg = WTM_BLOCKS_1BIT / 2;
				wm_1_min = WTM_BLOCKS_1BIT - WTM_BLOCKS_1BIT_MISMATCHES;
				for (int i = 0; i < WTM_BLOCK_LENGTH_1BIT; i++)	{
					int bit_rank = p_assembler->wm1_bit_rank[i];
					int wm_base = bit_rank > wm_x_avg ? wm_1_min : wm_0_max;
					char wm_bit = bit_rank <= wm_0_max ? '0' : bit_rank >= wm_1_min ? '1' : 'X';
					sprintf_s(str, MSG_LENGTH, " %c:%+03d", wm_bit, (wm_base < wm_x_avg ? -1 : 1) * (bit_rank - wm_base));
					wm_s += str;
				}
				sprintf_s(str, MSG_LENGTH, " ]");
				wm_s += str;
				break;
			}
		}
	}
	console_printf(wm_s.c_str());
}
