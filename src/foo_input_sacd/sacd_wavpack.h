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

#ifndef _SACD_WAVPACK_H_INCLUDED
#define _SACD_WAVPACK_H_INCLUDED

#include "sacd_config.h"
#include "endianess.h"
#include "scarletbook.h"
#include "sacd_reader.h"
#include "sacd_dsd.h"
#include "wavpack.h"
#include "id3_tagger.h"

#include <../helpers/cue_parser.h>

using cue_parser::embeddedcue_metadata_manager;

bool g_dsd_in_wavpack(const char* file);

class file_wavpack_t {
	sacd_media_t*   m_file;
	int64_t         m_alt_header_offset;
	vector<uint8_t> m_alt_header_value;
	int64_t         m_alt_trailer_offset;
	vector<uint8_t> m_alt_trailer_value;
public:
	file_wavpack_t();
	~file_wavpack_t();
	void attach(sacd_media_t* p_file);
	void detach();
	vector<uint8_t>& get_alt_header();
	vector<uint8_t>& get_alt_trailer();
	void load_alt_header_and_trailer();
	void save_alt_header_and_trailer();
private:
	bool check_wavpack_header(WavpackHeader& wph);
	bool select_metadata(int metadata_id, vector<uint8_t>& metadata_value);
	bool update_metadata(int metadata_id, const vector<uint8_t>& metadata_value);
	bool create_alt_trailer_block();
};

class sacd_wavpack_t : public sacd_reader_t {
	sacd_media_t*   m_file;
	uint32_t        m_mode;
	WavpackContext* m_wpc;
	int             m_version;
	uint32_t        m_samplerate;
	int             m_channel_count;
	int             m_channel_mask;
	int             m_loudspeaker_config;
	int             m_framerate;
	uint64_t        m_samples;
	double          m_duration;
	vector<int32_t> m_data;
	uint32_t        m_track_number;
	uint64_t        m_track_start;
	uint64_t        m_track_end;
	uint64_t        m_track_position;
	tracklist_t     m_tracklist;
	id3_tagger_t    m_id3_tagger;
	bool            m_has_id3;
	file_wavpack_t  m_wv;
	embeddedcue_metadata_manager m_cuesheet;
public:
	sacd_wavpack_t();
	~sacd_wavpack_t();
	uint32_t get_track_count(uint32_t mode);
	uint32_t get_track_number(uint32_t track_index);
	int get_channels(uint32_t track_number);
	int get_loudspeaker_config(uint32_t track_number);
	int get_samplerate(uint32_t track_number);
	int get_framerate(uint32_t track_number);
	double get_duration(uint32_t track_number);
	void set_mode(uint32_t mode);
	bool open(sacd_media_t* p_file);
	bool close();
	bool select_track(uint32_t track_number, uint32_t offset);
	bool read_frame(uint8_t* frame_data, size_t* frame_size, frame_type_e* frame_type);
	bool seek(double seconds);
	void get_info(uint32_t track_number, file_info& info);
	void set_info(uint32_t track_number, const file_info& info);
	void commit();
private:
	tuple<double, double>get_track_times(uint32_t track_number);
	void import_metainfo();
	void import_cuesheet();
	void update_tags(vector<uint8_t>& metadata);
	void update_size(vector<uint8_t>& metadata, int64_t delta);
};

#endif
