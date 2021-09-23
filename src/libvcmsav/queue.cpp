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

#include "queue.h"

lf_queue_t::lf_queue_t(wm_type_t _wm_type, real_t _samplerate, real_t _avg_delay_sec, int _run_length, real_t _run_blocks_per_sec, int _packet_payload_length, buf_real_t& _input_buffer, watermarks_t* _p_watermarks, int _scanners, int _watermark_bits, int _watermark_blocks, int _wm8_block_index, int _wm3_bit_index, uint8_t* _p_wm) {
	corconmn = new corconmn_t(_samplerate, _avg_delay_sec, _run_length, _run_blocks_per_sec, _input_buffer);
	sheader  = new sheader_t(_samplerate, _avg_delay_sec, _run_length, _run_blocks_per_sec, corconmn->nr_delays, corconmn->p_corr_sign);
	sequence = new sequence_t(WM_LF, _wm_type, corconmn->corr_sign_length, corconmn->p_corr_sign, _p_watermarks, _scanners, _watermark_bits, _watermark_blocks, _wm8_block_index, _wm3_bit_index, _p_wm);
	packet   = new packet_t(WM_LF, _samplerate, _avg_delay_sec, _run_length, _run_blocks_per_sec, 0, 0, 0, 0, _packet_payload_length, corconmn->nr_delays, sheader->p_corr_rank, sequence);
	int vector_length = (corconmn->nr_delays + 1) * (_watermark_bits + 1);
	wm_vector_t* p_wm_vector = new wm_vector_t(vector_length);
	switch (_wm_type) {
	case WM_3BIT:
		_p_watermarks->lf_wm_3bit = p_wm_vector;
		break;
	case WM_1BIT:
		_p_watermarks->lf_wm_1bit = p_wm_vector;
		break;
	}
}

lf_queue_t::~lf_queue_t() {
	delete corconmn;
	delete sheader;
	delete packet;
	delete sequence;
}

void lf_queue_t::run() {
	corconmn->run();
	sheader->run();
	packet->run();
	sequence->run();
}

hf_queue_t::hf_queue_t(wm_type_t _wm_type, real_t _samplerate, int _nr_delays, real_t* _p_delay_coeffs, int _run_length, real_t _run_blocks_per_sec, int _mheader_hop_sequence_length, uint8_t* _p_mheader_hop_sequence, int _wm_hop_sequence_length, uint8_t* _p_wm_hop_sequence, int _packet_payload_length, buf_real_t& _input_buffer, watermarks_t* _p_watermarks, int _scanners, int _watermark_bits, int _watermark_blocks, int _wm8_block_index, int _wm3_bit_index, uint8_t* _p_wm) {
	corhoplv = new corhoplv_t(_samplerate, _nr_delays, _p_delay_coeffs, _run_length, _run_blocks_per_sec, _input_buffer);
	mheader  = new mheader_t(_samplerate, _run_length, _run_blocks_per_sec, _mheader_hop_sequence_length, _p_mheader_hop_sequence, corhoplv->p_corr_sign);
	sequence = new sequence_t(WM_HF, _wm_type, corhoplv->corr_sign_length, corhoplv->p_corr_sign, _p_watermarks, _scanners, _watermark_bits, _watermark_blocks, _wm8_block_index, _wm3_bit_index, _p_wm);
	packet   = new packet_t(WM_HF, _samplerate, 0.0, _run_length, _run_blocks_per_sec, _mheader_hop_sequence_length, _p_mheader_hop_sequence, _wm_hop_sequence_length, _p_wm_hop_sequence, _packet_payload_length, SCANNERS, mheader->p_corr_rank, sequence);
	int vector_length = (SCANNERS + 1) * (_watermark_bits + 1);
	wm_vector_t* p_wm_vector = new wm_vector_t(vector_length);
	switch (_wm_type) {
	case WM_3BIT:
		_p_watermarks->hf_wm_3bit = p_wm_vector;
		break;
	case WM_1BIT:
		_p_watermarks->hf_wm_1bit = p_wm_vector;
		break;
	}
}

hf_queue_t::~hf_queue_t() {
	delete corhoplv;
	delete mheader;
	delete packet;
	delete sequence;
}

void hf_queue_t::run() {
	corhoplv->run();
	mheader->run();
	packet->run();
	sequence->run();
}
