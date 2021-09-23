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

#ifndef _DETECTOR_H
#define _DETECTOR_H

#include "common.h"

#define ROUND_LOG2(x) ((int)ceil(log((real_t)x)/log((real_t)2.0)))

#define CORRELATION_BLOCKS 3

#define SCANNERS               5
#define ASSEMBLERS_PER_SCANNER 3

#define HEADER_LENGTH      32
#define HEADER_PATTERN_INT 0x59f1ba84
#define HEADER_MISMATCHES  6

#define ECHO_DELAY      0.0007709
#define LF_CORRELATIONS 8
#define HF_DELAYS       16

#define LF_WM_PER_SECOND 75.0
#define HF_WM_PER_SECOND 50.0

#define WTM_LENGTH_3BIT            253
#define WTM_BLOCK_LENGTH_3BIT      3
#define WTM_BLOCKS_3BIT            42
#define WTM_BLOCKS_3BIT_MISMATCHES 8

#define WTM_BLOCK_LENGTH_8BIT      8
#define WTM_BLOCKS_8BIT            15
#define WTM_BLOCKS_8BIT_MISMATCHES 5
#define WTM_BLOCKS_8BIT_CORRECTION -7

#define WTM_LENGTH_1BIT            154
#define WTM_BLOCK_LENGTH_1BIT      1
#define WTM_BLOCKS_1BIT            154
#define WTM_BLOCKS_1BIT_FREE       56
#define WTM_BLOCKS_1BIT_MISMATCHES 35
#define WTM_BLOCKS_1BIT_CORRECTION -21

extern real_t  BIT_INTERVAL_COEFF_AVG[SCANNERS];
extern real_t  BIT_INTERVAL_COEFF_MIN[SCANNERS];
extern real_t  BIT_INTERVAL_COEFF_MAX[SCANNERS];
extern int8_t  NUM_DIFF_BITS_TABLE[256];
extern uint8_t HEADER_PATTERN[HEADER_LENGTH];
extern uint8_t LF_WTM_HOP_SEQUENCE[16];
extern real_t  HF_DELAY_COEFFS[HF_DELAYS];
extern uint8_t HF_HOP_SEQUENCE_3BIT[32];
extern uint8_t MHEADER_HOP_SEQUENCE_3BIT[16];
extern uint8_t MHEADER_HOP_SEQUENCE_1BIT[16];
extern uint8_t WTM_PATTERN_3BIT[WTM_LENGTH_3BIT];
extern uint8_t WTM_PATTERN_1BIT[WTM_LENGTH_1BIT];

class wm_vector_t {
	int  n;
	int* v;
public:
	wm_vector_t(int length) {
		n = length;
		v = new int[n];
		for (int i = 0; i < n; i++) v[i] = 0;
	}
	~wm_vector_t() {
	 delete[] v;
	}
	int length() {
		return n;
	}
	void reset() {
		for (int i = 0; i < n; i++) v[i] = 0;
	}
	int get(int index) {
		if (index < 0 || index >= n)
			return 0;
		return v[index];
	}
	void set(int index, int value) {
		if (index < 0 || index >= n)
			return;
		v[index] = value;
	}
	void add(int index, int value) {
		if (index < 0 || index >= n)
			return;
		v[index] += value;
	}
};

#endif
