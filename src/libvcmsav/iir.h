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

#ifndef _IIR_H
#define _IIR_H

#include "common.h"
#include "filter.h"

class iir_t : public filter_t {
	real_t*       filter_buffer;
public:
	int           filter_length;
	const real_t* filter_coef_A;
	const real_t* filter_coef_B;
public:
	iir_t(wm_band_t wm_band, int samplerate);
	~iir_t();
	real_t run(real_t x);
};

#endif
