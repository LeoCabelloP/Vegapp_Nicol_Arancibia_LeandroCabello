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

#include <memory.h>
#include <stdint.h>
#include "detector.h"
#include "dvdawm.h"

dvdawm_t::dvdawm_t() {
	memset(this, 0, sizeof(*this));
}

int dvdawm_t::init(int _channels, int _samplerate) {
	channels = _channels;
	samplerate = _samplerate;
	switch (samplerate) {
	case 44100:
	case 48000:
	case 88200:
	case 96000:
	case 176400:
	case 192000:
		break;
	default:
		return DVDAWM_UNSUPPORTED_SAMPLERATE;
		break;
 	}
	downsample_factor = 1;
	switch (samplerate) {
	case 88200:
	case 96000:
		downsample_factor = 2;
		break;
	case 176400:
	case 192000:
		downsample_factor = 4;
		break;
 	}
	int detector_samplerate = samplerate / downsample_factor;
	run_length = 0;
	switch (detector_samplerate) {
	case 44100:
		run_length = 3 * 42;
		break;
	case 48000:
		run_length = 3 * 44;
		break;
 	}
	int lf_filter_samples = detector_samplerate * (1.0 / LF_WM_PER_SECOND + 3.0 * ECHO_DELAY);
	int hf_filter_samples = detector_samplerate * (1.0 / HF_WM_PER_SECOND + 3.0 * ECHO_DELAY);
	lf_filter_buffer = new buf_real_t[channels];
	hf_filter_buffer = new buf_real_t[channels];
	if (!(lf_filter_buffer && hf_filter_buffer))
		return DVDAWM_OUT_OF_MEMORY;
	for (int channel = 0; channel < channels; channel++) {
		if (!lf_filter_buffer[channel].init(lf_filter_samples))
			return DVDAWM_OUT_OF_MEMORY;
		if (!hf_filter_buffer[channel].init(hf_filter_samples))
			return DVDAWM_OUT_OF_MEMORY;
	}
	lf_iir = new iir_t*[channels];
	hf_iir = new iir_t*[channels];
	df_iir = new iir_t*[channels];
	if (!(lf_iir && hf_iir && df_iir))
		return DVDAWM_OUT_OF_MEMORY;
	for (int channel = 0; channel < channels; channel++) {
		lf_iir[channel] = new iir_t(WM_LF, detector_samplerate);
		hf_iir[channel] = new iir_t(WM_HF, detector_samplerate);
		df_iir[channel] = new iir_t(WM_DF, samplerate);
	}
	for (int channel = 0; channel < channels; channel++) {
		if (!(lf_iir[channel] && hf_iir[channel] && df_iir[channel]))
			return DVDAWM_OUT_OF_MEMORY;
	}
	wm_check = new wm_check_t[channels];
	if (!wm_check)
		return DVDAWM_OUT_OF_MEMORY;
	for (int channel = 0; channel < channels; channel++) {
		if (!wm_check[channel].init(channel, detector_samplerate, lf_filter_buffer[channel], hf_filter_buffer[channel]))
			return DVDAWM_OUT_OF_MEMORY;
	}
	run_index = downsample_index = 0;
	return DVDAWM_INITIALIZED_OK;
}

void dvdawm_t::free() {
	for (int channel = 0; channel < channels; channel++) {
		if (lf_filter_buffer)
			lf_filter_buffer[channel].free();
		if (hf_filter_buffer)
			hf_filter_buffer[channel].free();
		if (lf_iir && lf_iir[channel])
			delete lf_iir[channel];
		if (hf_iir && hf_iir[channel])
			delete hf_iir[channel];
		if (df_iir && df_iir[channel])
			delete df_iir[channel];
	}
	if (lf_filter_buffer)
		delete[] lf_filter_buffer;
	if (hf_filter_buffer)
		delete[] hf_filter_buffer;
	if (lf_iir)
		delete[]lf_iir;
	if (hf_iir)
		delete[] hf_iir;
	if (df_iir)
		delete[] df_iir;
	if (wm_check)
		delete[] wm_check;
	memset(this, 0, sizeof(*this));
}

void dvdawm_t::reset() {
	for (int channel = 0; channel < channels; channel++) {
		wm_check[channel].reset();
	}
}

void dvdawm_t::retime(double seek_time) {
	for (int channel = 0; channel < channels; channel++) {
		wm_check[channel].retime(seek_time);
	}
}

template<class T> int dvdawm_t::run(T* data, int samples) {
	prev_run_index = run_index;
	prev_downsample_index = downsample_index;
	int watermarks = 0;
	for (int sample = 0; sample < samples; sample++) {
		for (int channel = 0; channel < channels; channel++) {
			real_t x = (real_t)data[sample * channels + channel];
			real_t x_df = df_iir[channel]->run(x);
			if (downsample_index == 0) {
				real_t x_lf = lf_iir[channel]->run(x_df);
				real_t x_hf = hf_iir[channel]->run(x_df);
				lf_filter_buffer[channel][run_index] = x_lf;
				hf_filter_buffer[channel][run_index] = x_hf;
			}
		}
		if (downsample_index == 0 && run_length && run_index && run_index % run_length == 0) {
			for (int channel = 0; channel < channels; channel++) {
				watermarks += wm_check[channel].run(run_length);
			}
		}
		downsample_index++;
		if (downsample_index == downsample_factor) {
			downsample_index = 0;
			run_index++;
		}
	}
	//out(data, samples, lf_filter_buffer); 
	return watermarks;
}
template int dvdawm_t::run<float>(float*, int);
template int dvdawm_t::run<double>(double*, int);

void dvdawm_t::set_track(int track) {
	for (int channel = 0; channel < channels; channel++) {
		wm_check[channel].set_track(track);
	}
}

template<class T> void dvdawm_t::out(T* data, int samples, buf_real_t* input_buffer) {
	uint32_t out_run_index = prev_run_index;
	uint32_t out_downsample_index = prev_downsample_index;
	for (int sample = 0; sample < samples; sample++) {
		for (int channel = 0; channel < channels; channel++) {
			float y = (float)input_buffer[channel][out_run_index];
			data[sample * channels + channel] = y;
		}
		out_downsample_index++;
		if (out_downsample_index == downsample_factor) {
			out_downsample_index = 0;
			out_run_index++;
		}
	}
}
template void dvdawm_t::out<float>(float*, int, buf_real_t*);
template void dvdawm_t::out<double>(double*, int, buf_real_t*);
