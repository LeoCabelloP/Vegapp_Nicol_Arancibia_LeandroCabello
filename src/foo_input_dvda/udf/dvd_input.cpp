/*
* DVD-Audio Decoder plugin
* Copyright (c) 2009-2020 Maxim V.Anisiutkin <maxim.anisiutkin@gmail.com>
*
* DVD-Audio Decoder is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation; either
* version 2.1 of the License, or (at your option) any later version.
*
* DVD-Audio Decoder is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with FFmpeg; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/

#include "dvd_input.h"

dvd_input_t dvdinput_open(const char* path) {
  dvd_input_t dev = new dvda_file_t;
	if (!dev)
		return NULL;
	dev->open(path);
	if (!dev->is_open()) {
		delete dev;
		return NULL;
	}
  return dev;
}

int dvdinput_close(dvd_input_t dev) {
	if (dev)
		delete dev;
	return 0;
}

int dvdinput_seek(dvd_input_t dev, int block) {
	if (!dev->seek(2048 * block))
		return 0;
	return block;
}

int dvdinput_read(dvd_input_t dev, void *buffer, int blocks, int flags) {
	return dev->read(buffer, 2048 * blocks) / 2048;
}

extern "C" int strcasecmp(char* a, char* b) {
	return _stricmp(a, b);
}
