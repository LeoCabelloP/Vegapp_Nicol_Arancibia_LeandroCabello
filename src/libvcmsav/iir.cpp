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

#include "iir.h"

/*
	Use 10 order Butterworth bandpass filters for watermark detection:
	LF watermark : 4.3-7.7 kHz
	HF watermark : 8.3-15.7 kHz
*/

static const int FLT_44100_LENGTH = 10 + 1;
static const real_t FLT_4300_7700_44100_B[FLT_44100_LENGTH] = {
	0.0004163025518237,
	0.0,
	-0.002081512759119,
	0.0,
	0.004163025518237,
	0.0,
	-0.004163025518237,
	0.0,
	0.002081512759119,
	0.0,
	-0.0004163025518237
};
static const real_t FLT_4300_7700_44100_A[FLT_44100_LENGTH] = {
	1.0,
	-5.703636393139,
	16.52982883408,
	-31.03245515879,
	41.4515481478,
	-40.90749785361,
	30.18121888965,
	-16.44510003959,
	6.372285803894,
	-1.599120231069,
	0.2045536253525
};
static const real_t FLT_8300_15700_44100_B[FLT_44100_LENGTH] = {
	0.01090482202178,
	0.0,
	-0.0545241101089,
	0.0,
	0.1090482202178,
	0.0,
	-0.1090482202178,
	0.0,
	0.0545241101089,
	0.0,
	-0.01090482202178
};
static const real_t FLT_8300_15700_44100_A[FLT_44100_LENGTH] = {
	1.0,
	1.061029232537,
	2.085709238093,
	1.643448240378,
	2.123236142965,
	1.217285392915,
	1.068444722412,
	0.4187826454186,
	0.2744346611499,
	0.05718013289234,
	0.02650010102885
};

static const int FLT_48000_LENGTH = 10 + 1;
static const real_t FLT_4300_7700_48000_B[FLT_48000_LENGTH] = {
	0.0002865588492895,
	0.0,
	-0.001432794246447,
	0.0,
	0.002865588492895,
	0,
	-0.002865588492895,
	0.0,
	0.001432794246447,
	0.0,
	-0.0002865588492895
};
static const real_t FLT_4300_7700_48000_A[FLT_48000_LENGTH] = {
	1.0,
	-6.208051380161,
	19.05969756673,
	-37.35232069317,
	51.40011290093,
	-51.6806785122,
	38.41514894615,
	-20.85943250948,
	7.951410994504,
	-1.934926986048,
	0.2333594148628
};
static const real_t FLT_8300_15700_48000_B[FLT_48000_LENGTH] = {
	0.007748366833266,
	0.0,
	-0.03874183416633,
	0.0,
	0.07748366833266,
	0.0,
	-0.07748366833266,
	0.0,
	0.03874183416633,
	0.0,
	-0.007748366833266
};
static const real_t FLT_8300_15700_48000_A[FLT_48000_LENGTH] = {
	1.0,
	1.33226762955e-015,
	1.893046609949,
	-3.330669073875e-016,
	1.899040522374,
	-1.165734175856e-015,
	1.021369436262,
	-7.632783294298e-016,
	0.3002058061654,
	-1.457167719821e-016,
	0.03688254366439
};

/*
	Use 3 order Elliptic lowpass filters (Fs = 88200Hz, Fpass = 3/16 * Fs, Apass = 1dB, Astop = 40dB) for sample rate conversion.
*/

static const int SRC_88200_96000_LENGTH = 3 + 1;
static const real_t SRC_88200_96000_B[SRC_88200_96000_LENGTH] = {
	0.08593663175118,
	0.1796307156959,
	0.1796307156959,
	0.08593663175118
};
static const real_t SRC_88200_96000_A[SRC_88200_96000_LENGTH] = {
	1.0,
	-1.110506450058,
	0.9562857674792,
	-0.314644622527
};

static const int SRC_176400_192000_LENGTH = 3 + 1;
static const real_t SRC_176400_192000_B[SRC_176400_192000_LENGTH] = {
	0.02503047314044,
	0.01620059189618,
	0.01620059189618,
	0.02503047314044
};
static const real_t SRC_176400_192000_A[SRC_176400_192000_LENGTH] = {
	1.0,
	-2.20093778027,
	1.846543057758,
	-0.5631431474152
};

iir_t::iir_t(wm_band_t _wm_band, int _samplerate) : filter_t(_wm_band, _samplerate) {
	filter_coef_A = filter_coef_B = NULL;
	filter_length = 0;
	filter_buffer = NULL;
	switch (wm_band) {
	case WM_LF:
		switch (samplerate) {
		case  44100:
		case  88200:
		case 176400:
			filter_length = FLT_44100_LENGTH;
			filter_coef_A = FLT_4300_7700_44100_A;
			filter_coef_B = FLT_4300_7700_44100_B;
			samplerate = 44100;
			break;
		case  48000:
		case  96000:
		case 192000:
			filter_length = FLT_48000_LENGTH;
			filter_coef_A = FLT_4300_7700_48000_A;
			filter_coef_B = FLT_4300_7700_48000_B;
			samplerate = 48000;
			break;
		}
		break;
	case WM_HF:
		switch (samplerate) {
		case  44100:
		case  88200:
		case 176400:
			filter_length = FLT_44100_LENGTH;
			filter_coef_A = FLT_8300_15700_44100_A;
			filter_coef_B = FLT_8300_15700_44100_B;
			samplerate = 44100;
			break;
		case  48000:
		case  96000:
		case 192000:
			filter_length = FLT_48000_LENGTH;
			filter_coef_A = FLT_8300_15700_48000_A;
			filter_coef_B = FLT_8300_15700_48000_B;
			samplerate = 48000;
			break;
		}
		break;
	case WM_DF:
		switch (samplerate) {
		case  88200:
		case  96000:
			filter_length = SRC_88200_96000_LENGTH;
			filter_coef_A = SRC_88200_96000_A;
			filter_coef_B = SRC_88200_96000_B;
			break;
		case 176400:
		case 192000:
			filter_length = SRC_176400_192000_LENGTH;
			filter_coef_A = SRC_176400_192000_A;
			filter_coef_B = SRC_176400_192000_B;
			break;
		}
		break;
	}
	filter_buffer = NULL;
	if (filter_length == 0)
		return;
	filter_buffer = new real_t[filter_length];
	for (int i = 0; i < filter_length; i++)
		filter_buffer[i] = 0.0;
}

iir_t::~iir_t() {
	if (filter_buffer)
		delete[] filter_buffer;
}

real_t iir_t::run(real_t x) {
	if (filter_length == 0)
		return x;
	real_t y = filter_coef_B[0] * x + filter_buffer[0];
	for (int j = 1; j < filter_length; j++)
		filter_buffer[j - 1] = filter_coef_B[j] * x - filter_coef_A[j] * y + filter_buffer[j];
	return y;
}
