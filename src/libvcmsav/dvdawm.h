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

#ifndef _DVDAWM_H
#define _DVDAWM_H

#include "iir.h"
#include "detector.h"
#include "wm_check.h"

#define DVDAWM_INITIALIZED_OK          0
#define DVDAWM_NOT_INITIALIZED         1
#define DVDAWM_OUT_OF_MEMORY          -1
#define DVDAWM_UNSUPPORTED_SAMPLERATE -2

class dvdawm_t {
	int run_length;
	int run_index;
	int downsample_index;
	int prev_run_index;
	int prev_downsample_index;
	template<class T> void out(T* data, int samples, buf_real_t* input_buffer);
public:
	int channels;
	int samplerate;
	int downsample_factor;
	iir_t** lf_iir;
	iir_t** hf_iir;
	iir_t** df_iir;
	buf_real_t* lf_filter_buffer;
	buf_real_t* hf_filter_buffer;
	wm_check_t* wm_check;
	dvdawm_t();
	int init(int channels, int samplerate);
	void free();
	void reset();
	void retime(double seek_time);
	template<class T> int run(T* data, int samples);
	void set_track(int track);
};

#endif
