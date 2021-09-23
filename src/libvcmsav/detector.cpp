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

#include "detector.h"

real_t BIT_INTERVAL_COEFF_AVG[SCANNERS] = {1.000, 0.981, 1.019, 0.965, 1.035};
real_t BIT_INTERVAL_COEFF_MIN[SCANNERS] = {0.986, 0.969, 1.005, 0.960, 1.023};
real_t BIT_INTERVAL_COEFF_MAX[SCANNERS] = {1.014, 0.995, 1.031, 0.977, 1.040};

int8_t NUM_DIFF_BITS_TABLE[256] = {
	0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4,
	1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
	1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
	2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
	1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
	2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
	2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
	3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
	1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
	2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
	2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
	3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
	2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
	3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
	3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
	4,5,5,6,5,6,6,7,5,6,6,7,6,7,7,8,
};

uint8_t HEADER_PATTERN[HEADER_LENGTH] = {
  0,1,0,1, 1,0,0,1, 1,1,1,1, 0,0,0,1,	1,0,1,1, 1,0,1,0, 1,0,0,0, 0,1,0,0,
};

uint8_t LF_WTM_HOP_SEQUENCE[16] = {
	0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,
};

real_t HF_DELAY_COEFFS[HF_DELAYS] = {
	 0.0018141,
	 0.0017914,
	 0.0017687,
	 0.0015646,
	 0.0015420,
	 0.0015193,
	 0.0012926,
	 0.0012699,
	 0.0010431,
	 0.0010204,
	 0.0009977,
	 0.0007937,
	 0.0007710,
	 0.0007483,
	 0.0005215,
	 0.0004989,
};

uint8_t HF_HOP_SEQUENCE_3BIT[32] = {
	3,  7, 15, 11, 12,  9,  2,  6, 10,  2,  0,  8,  1,  0, 11, 10,
	6, 13,  5,  1,  9,  3,  4, 15, 14,  5,  7, 14, 13, 12,  8,  4,
};

uint8_t MHEADER_HOP_SEQUENCE_3BIT[16] = {
	7, 11,  9,  6,  2,  8,  0, 10, 13,  1,  3, 15,  5, 14, 12,  4
};

uint8_t MHEADER_HOP_SEQUENCE_1BIT[16] = {
	7,  9, 12,  4,  6, 11, 14,  5,  0, 15,  3, 13,  2,  8,  1, 10,
};

uint8_t WTM_PATTERN_3BIT[WTM_LENGTH_3BIT] = {
	0,0,0,1,0,1,1,0,0,0,1,0,1,1,0,0,
	0,0,1,1,0,0,0,1,0,1,1,0,1,0,1,1,
	0,1,0,0,0,1,0,1,0,0,0,0,0,1,0,0,
	1,0,0,0,0,0,0,1,1,1,0,0,1,1,0,0,
	0,0,1,1,1,1,0,1,1,1,0,0,0,0,0,1,
	0,1,1,0,0,1,0,1,0,0,1,1,1,1,0,1,
	0,1,0,1,0,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,0,1,0,1,1,0,0,0,0,0,
	1,1,1,0,0,1,0,1,0,0,0,1,1,0,1,1,
	1,0,0,1,1,0,1,0,0,1,0,1,1,1,0,1,
	0,0,0,0,0,0,0,0,1,0,0,1,1,1,0,0,
	1,1,0,1,0,1,0,1,0,0,0,1,0,0,0,1,
	0,1,1,1,1,1,1,0,1,1,0,0,1,1,1,1,
	0,0,1,0,1,0,1,1,0,1,1,0,0,0,1,0,
	0,1,1,0,0,0,1,1,0,1,0,1,1,1,1,1,
	1,1,1,1,0,0,1,1,0,1,1,0,0,
};

uint8_t WTM_PATTERN_1BIT[WTM_LENGTH_1BIT] = {
	1,1,1,0,1,1,1,1,0,0,0,0,1,1,0,0,
	0,0,1,1,1,1,1,1,0,1,0,0,1,1,1,0,
	1,1,0,0,0,0,1,0,0,1,1,0,0,0,0,1,
	1,0,0,1,0,1,1,1,1,0,0,0,1,0,0,0,
	0,1,1,0,1,1,0,1,1,0,1,1,0,1,1,1,
	0,0,0,0,0,1,1,1,1,0,1,0,1,0,1,1,
	0,0,1,0,1,1,1,1,1,0,1,1,1,1,1,0,
	1,1,1,1,0,1,0,0,1,1,1,1,1,0,1,1,
	1,1,1,1,0,0,1,1,0,1,1,0,1,0,1,1,
	1,0,1,1,1,0,1,1,0,0,
};
