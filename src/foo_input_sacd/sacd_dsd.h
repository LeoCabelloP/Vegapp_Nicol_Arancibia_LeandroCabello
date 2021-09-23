/*
* SACD Decoder plugin
* Copyright (c) 2011-2019 Maxim V.Anisiutkin <maxim.anisiutkin@gmail.com>
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation; either
* version 2.1 of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with FFmpeg; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/

#ifndef _SACD_DSD_H_INCLUDED
#define _SACD_DSD_H_INCLUDED

#include "sacd_config.h"
#include "endianess.h"

#pragma pack(1)

class ID {
	char ckID[4];
public:
	bool operator==(const char* id) {
		return ckID[0] == id[0] && ckID[1] == id[1] && ckID[2] == id[2] && ckID[3] == id[3];
	}
	bool operator!=(const char* id) {
		return !(*this == id);
	}
	void set_id(const char* id) {
		ckID[0] = id[0];
		ckID[1] = id[1];
		ckID[2] = id[2];
		ckID[3] = id[3];
	}
};

class Chunk : public ID {
public:
	uint64_t ckDataSize;
	uint64_t get_size() {
		return hton64(ckDataSize);
	}
	void set_size(uint64_t size) {
		ckDataSize = hton64(size);
	}
};

class FormDSDChunk : public Chunk {
public:
	ID formType;
};

class DSTFrameIndex {
public:
	uint64_t offset;
	uint32_t length;
};

enum MarkType { TrackStart = 0, TrackStop = 1, ProgramStart = 2, Index = 4 };

class Marker {
public:
	uint16_t hours;
	uint8_t  minutes;
	uint8_t  seconds;
	uint32_t samples;
	int32_t  offset;
	uint16_t markType;
	uint16_t markChannel;
	uint16_t TrackFlags;
	uint32_t count;
};

class FmtDSFChunk : public Chunk {
public:
	uint32_t format_version;
	uint32_t format_id;
	uint32_t channel_type;
	uint32_t channel_count;
	uint32_t samplerate;
	uint32_t bits_per_sample;
	uint64_t sample_count;
	uint32_t block_size;
	uint32_t reserved;
};

#pragma pack()

class track_time_t {
public:
	double start_time;
	double stop_time;
};

typedef vector<track_time_t> tracklist_t;

double get_marker_time(const Marker& m, uint32_t samplerate);

#endif
