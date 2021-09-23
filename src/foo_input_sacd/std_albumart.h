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

#ifndef _STD_ALBUMART_H_INCLUDED
#define _STD_ALBUMART_H_INCLUDED

#include "sacd_config.h"

class std_albumart_instance_t : public album_art_editor_instance {
	static constexpr char*           stream_ext = "*.WAV";
	file_ptr                         stream_ptr;
	album_art_extractor_instance_ptr extractor_ptr;
	album_art_editor_instance_ptr    editor_ptr;
public: 
	void create(t_input_open_reason p_reason, abort_callback& p_abort);
	void instantiate(file_ptr p_filehint, const char* p_path, t_input_open_reason p_reason, abort_callback& p_abort);
	void load(const vector<uint8_t>& p_data, abort_callback& p_abort);
	void save(vector<uint8_t>& p_data, abort_callback& p_abort);
	album_art_data_ptr query(const GUID& p_what, abort_callback& p_abort);
	void set(const GUID& p_what, album_art_data_ptr p_data, abort_callback& p_abort);
	void remove(const GUID& p_what);
	void commit(abort_callback& p_abort);
	void dump(const char* name);
};

#endif
