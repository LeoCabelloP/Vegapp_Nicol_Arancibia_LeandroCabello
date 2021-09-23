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

#ifndef _SACD_READER_H_INCLUDED
#define _SACD_READER_H_INCLUDED

#include "sacd_config.h"
#include "sacd_media.h"

enum track_e {
	TRACK_SELECTED = -1,
	TRACK_CUESHEET = 0
};

enum access_mode_e {
	ACCESS_MODE_NULL          = 0,
	ACCESS_MODE_TWOCH         = 1 << 0,
	ACCESS_MODE_MULCH         = 1 << 1,
	ACCESS_MODE_SINGLE_TRACK  = 1 << 2,
	ACCESS_MODE_FULL_PLAYBACK = 1 << 3
};

enum class media_type_e {
	INVALID = -1,
	ISO     = 0,
	DSDIFF  = 1,
	DSF     = 2,
	WAVPACK = 3
};

enum class frame_type_e {
	INVALID = -1,
	DSD     = 0,
	DST     = 1
};

enum area_id_e {
	AREA_NULL  = 0,
	AREA_TWOCH = 1 << 0,
	AREA_MULCH = 1 << 1,
	AREA_BOTH  = AREA_TWOCH | AREA_MULCH
};

class sacd_reader_t {
public:
	sacd_reader_t() {};
	virtual ~sacd_reader_t() {};
	virtual	uint32_t get_track_count(uint32_t mode = 0) = 0;
	virtual	uint32_t get_track_number(uint32_t track_index) = 0;
	virtual int get_channels(uint32_t track_number = TRACK_SELECTED) = 0;
	virtual int get_loudspeaker_config(uint32_t track_number = TRACK_SELECTED) = 0;
	virtual int get_samplerate(uint32_t track_number = TRACK_SELECTED) = 0;
	virtual int get_framerate(uint32_t track_number = TRACK_SELECTED) = 0;
	virtual double get_duration(uint32_t track_number = TRACK_SELECTED) = 0;
	virtual bool is_dst(uint32_t track_number = TRACK_SELECTED) { return false; };
	virtual void set_mode(uint32_t mode) = 0;
	virtual bool open(sacd_media_t* media) = 0;
	virtual bool close() = 0;
	virtual	bool select_track(uint32_t track_number, uint32_t offset = 0) = 0;
	virtual bool read_frame(uint8_t* frame_data, size_t* frame_size, frame_type_e* frame_type) = 0;
	virtual bool seek(double seconds) = 0;
	virtual void get_info(uint32_t track_number, file_info& info) = 0;
	virtual void set_info(uint32_t track_number, const file_info& info) {};
	virtual void get_albumart(uint32_t albumart_id, vector<t_uint8>& albumart_data) {};
	virtual void set_albumart(uint32_t albumart_id, const vector<t_uint8>& albumart_data) {};
	virtual void commit() {};
};

#endif
