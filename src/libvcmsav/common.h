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

#pragma warning(disable : 4244 4305)

#ifndef _COMMON_H
#define _COMMON_H

#include <math.h>
#include <memory.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define INFO_LEVEL  0
#define TRACE_LEVEL 1

#define MSG_LENGTH 1024

enum wm_band_t {WM_LF = 1, WM_HF = 2, WM_DF = 3};
enum wm_type_t {WM_1BIT = 1, WM_3BIT = 3, WM_8BIT = 8};

#define round(x) ((int)(x + 0.5))

#if !defined min
	#define min(a, b) ((a < b) ? a : b)
#endif

#if !defined max
	#define max(a, b) ((a > b) ? a : b)
#endif

template <class T, class I> class buffer_t {
	I  size;
	I  mask;
	T* data;
public:
	static I round_size(I size) {
		for (int i = 0; i < 8 * sizeof(I); i++) {
			if (((I)1 << i) >= size)
				return (I)1 << i;
		}
		return (I)0;
	}
	buffer_t(void) {
		mask = size = (I)0;
		data = (T*)0;
	}
	~buffer_t(void) {
	}
	bool init(I min_size) {
		if (data != (T*)0)
			::free(data);
		size = round_size(min_size);
		data = (T*)calloc((size_t)size, sizeof(T));
		if (data == (T*)0)
			mask = size = (I)0;
		else
			mask = size - (I)1;
		return data != (T*)0;
	}
	void free(void) {
		if (data != (T*)0) {
			::free(data);
			data = (T*)0;
		}
		mask = size = (I)0;
	}
	I get_size(void) {
		return size;
	}
	I get_mask(void) {
		return mask;
	}
	T& operator[](I i) {
		return data[i & mask];
	}
};

typedef float real_t;
typedef uint16_t bits_t;

typedef buffer_t<real_t, uint32_t> buf_real_t;
typedef buffer_t<bits_t, uint32_t> buf_bits_t;

void print_time(real_t t);
char* print_time(char* str, real_t t);

#endif
