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

#ifndef _SACD_DSF_H_INCLUDED
#define _SACD_DSF_H_INCLUDED

#include "sacd_config.h"
#include "endianess.h"
#include "scarletbook.h"
#include "sacd_reader.h"
#include "sacd_dsd.h"
#include "id3_tagger.h"

class sacd_dsf_t : public sacd_reader_t {
	sacd_media_t*   m_file;
	uint32_t        m_mode;
	int             m_version;
	int             m_samplerate;
	int             m_framerate;
	int             m_channel_count;
	int             m_loudspeaker_config;
	int64_t         m_file_size;
	vector<uint8_t> m_block_data;
	int             m_block_size;
	int             m_sample_in_block;
	int             m_block_data_end;
	int64_t         m_sample_count;
	int64_t         m_data_offset;
	int64_t         m_data_size;
	int64_t         m_data_end_offset;
	int64_t         m_read_offset;
	bool            m_is_lsb;
	id3_tagger_t    m_id3_tagger;
	int64_t         m_id3_offset;
	vector<uint8_t> m_id3_data;
	uint8_t         swap_bits[256];
public:
	sacd_dsf_t();
	~sacd_dsf_t();
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
	void get_info(uint32_t subsong, file_info& info);
	void set_info(uint32_t subsong, const file_info& info);
	void get_albumart(uint32_t albumart_id, vector<t_uint8>& albumart_data);
	void set_albumart(uint32_t albumart_id, const vector<t_uint8>& albumart_data);
	void commit();
private:
	int64_t get_size();
	int64_t get_offset();
};

#endif
