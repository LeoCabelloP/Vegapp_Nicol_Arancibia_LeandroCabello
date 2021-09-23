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

#ifndef _SACD_ALBUMART_H_INCLUDED
#define _SACD_ALBUMART_H_INCLUDED

#include "sacd_core.h"
#include "sacd_reader.h"
#include "sacd_metabase.h"
#include "sacd_setup.h"
#include "sacd_version.h"
#include "std_albumart.h"

enum class albumart_id_e {
	rawid3 = -1,
	other  = 0,
	icon   = 2,
	front  = 3,
	back   = 4,
	disc   = 6,
	artist = 8
};

class sacd_albumart_instance_t : public sacd_core_t, public album_art_editor_instance {
	service_ptr_t<album_art_data_impl>     data_ptr;
	service_ptr_t<std_albumart_instance_t> std_albumart;
public:
	static albumart_id_e g_get_albumart_id(const GUID& p_what);
	sacd_albumart_instance_t(file_ptr p_filehint, const char* p_path, t_input_open_reason p_reason, abort_callback& p_abort);
	~sacd_albumart_instance_t();
	album_art_data_ptr query(const GUID& p_what, abort_callback& p_abort);
	void set(const GUID& p_what, album_art_data_ptr p_data, abort_callback& p_abort);
	void remove(const GUID& p_what);
	void commit(abort_callback& p_abort);
};

class sacd_albumart_extractor_t : public album_art_extractor {
	static constexpr GUID sacd_albumart_extractor_guid{ 0x16f43975, 0xa229, 0x44a5, { 0x87, 0x1d, 0xbd, 0xab, 0x31, 0x9c, 0xdd, 0xd4 } };
public:
	bool is_our_path(const char* p_path, const char* p_ext);
	album_art_extractor_instance_ptr open(file_ptr p_filehint, const char* p_path, abort_callback& p_abort);
};

class sacd_albumart_editor_t : public album_art_editor {
	static constexpr GUID sacd_albumart_editor_guid{ 0x8d3b525c, 0x747, 0x4803, { 0x9c, 0xf8, 0x6f, 0xf7, 0xcf, 0x91, 0x59, 0x40 } };
public:
	bool is_our_path(const char* p_path, const char* p_ext);
	album_art_editor_instance_ptr open(file_ptr p_filehint, const char* p_path, abort_callback& p_abort);
};

#endif
