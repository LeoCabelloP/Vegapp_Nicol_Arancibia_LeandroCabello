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

#include "common.h"

void print_time(real_t t) {
	real_t a = fabs(t);
	int h = (int)(a / 36000);
	int m = (int)(a / 60 - h * 60);
	int s = (int)(a - h * 3600 - m * 60);
	if (t < 0.0)
		printf("-%1d:%02d:%02d.%05d", h, m, s, (int)((a - floor(a)) * 100000));
	else
		printf("%02d:%02d:%02d.%05d", h, m, s, (int)((a - floor(a)) * 100000));
}

char* print_time(char* str, real_t t) {
	real_t a = fabs(t);
	int h = (int)(a / 36000);
	int m = (int)(a / 60 - h * 60);
	int s = (int)(a - h * 3600 - m * 60);
	if (t < 0.0)
		sprintf_s(str, 32, "-%01d:%02d:%02d.%05d", h, m, s, (int)((a - floor(a)) * 100000));
	else
		sprintf_s(str, 32, "%02d:%02d:%02d.%05d", h, m, s, (int)((a - floor(a)) * 100000));
	return str;
}
