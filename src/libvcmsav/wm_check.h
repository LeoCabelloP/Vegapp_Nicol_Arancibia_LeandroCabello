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

#ifndef _WM_CHECK_H
#define _WM_CHECK_H

#include "common.h"
#include "detector.h"
#include "iir.h"
#include "queue.h"

class wm_check_t {
	int          track;
	int          channel;
	int          samplerate;
	int          lf_run_length;
	lf_queue_t*  lf_queue_3bit;
	lf_queue_t*  lf_queue_1bit;
	int          hf_run_length;
	hf_queue_t*  hf_queue_3bit;
	hf_queue_t*  hf_queue_1bit;
	watermarks_t watermarks;
	int          checked_samples;
	int          retime_to_sample;
	int          log_level;
public:
	wm_check_t();
	~wm_check_t();
	bool init(int channel, int samplerate, buf_real_t& lfi_buf, buf_real_t& hfi_buf, int log_level = INFO_LEVEL);
	void reset();
	void retime(double seek_time);
	int  run(int samples);
	int  test_slots(sequence_t* p_sequence);
	void show_watermark_tempo(sequence_t* p_sequence, int slot, char* str);
	void show_watermark(sequence_t* p_sequence, int slot);
	void set_track(int _track) {
		track = _track;
	}
};

extern void console_printf(const char* fmt, ...);

#endif
