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

#include "sacd_albumart.h"

albumart_id_e sacd_albumart_instance_t::g_get_albumart_id(const GUID& p_what) {
	albumart_id_e albumart_id = albumart_id_e::other;
	if (p_what == album_art_ids::cover_front) {
		albumart_id = albumart_id_e::front;
	}
	if (p_what == album_art_ids::cover_back) {
		albumart_id = albumart_id_e::back;
	}
	if (p_what == album_art_ids::disc) {
		albumart_id = albumart_id_e::disc;
	}
	if (p_what == album_art_ids::icon) {
		albumart_id = albumart_id_e::icon;
	}
	if (p_what == album_art_ids::artist) {
		albumart_id = albumart_id_e::artist;
	}
	return albumart_id;
}

sacd_albumart_instance_t::sacd_albumart_instance_t(file_ptr p_filehint, const char* p_path, t_input_open_reason p_reason, abort_callback& p_abort) {
	open(p_filehint, p_path, p_reason, p_abort);
	if (!sacd_metabase) {
		vector<uint8_t> id3_data;
		sacd_reader->get_albumart((uint32_t)albumart_id_e::rawid3, id3_data);
		std_albumart = new service_impl_t<std_albumart_instance_t>();
		if (std_albumart.is_valid()) {
			std_albumart->create(p_reason, p_abort);
			std_albumart->load(id3_data, p_abort);
			std_albumart->instantiate(p_filehint, p_path, p_reason, p_abort);
		}
	}
}

sacd_albumart_instance_t::~sacd_albumart_instance_t() {
}

album_art_data_ptr sacd_albumart_instance_t::query(const GUID& p_what, abort_callback& p_abort) {
	album_art_data_ptr data_ptr;
	if (std_albumart.is_valid()) {
		data_ptr = std_albumart->query(p_what, p_abort);
	}
	if (sacd_metabase) {
		vector<uint8_t> what_data;
		sacd_metabase->get_albumart((t_uint32)g_get_albumart_id(p_what), what_data);
		data_ptr = album_art_data_impl::g_create(what_data.data(), what_data.size());
	}
	return data_ptr;
}

void sacd_albumart_instance_t::set(const GUID& p_what, album_art_data_ptr p_data, abort_callback& p_abort) {
	if (CSACDPreferences::get_editable_tags()) {
		if (std_albumart.is_valid()) {
			std_albumart->set(p_what, p_data, p_abort);
		}
		if (sacd_metabase) {
			vector<uint8_t> what_data;
			if (p_data.is_valid()) {
				what_data.assign((const uint8_t*)p_data->get_ptr(), (const uint8_t*)p_data->get_ptr() + p_data->get_size());
			}
			else {
				what_data.clear();
			}
			sacd_metabase->set_albumart((t_uint32)g_get_albumart_id(p_what), what_data);
		}
	}
}

void sacd_albumart_instance_t::remove(const GUID& p_what) {
	if (CSACDPreferences::get_editable_tags()) {
		if (std_albumart.is_valid()) {
			std_albumart->remove(p_what);
		}
		if (sacd_metabase) {
			vector<uint8_t> what_data;
			sacd_metabase->set_albumart((t_uint32)g_get_albumart_id(p_what), what_data);
		}
	}
}

void sacd_albumart_instance_t::commit(abort_callback& p_abort) {
	if (CSACDPreferences::get_editable_tags()) {
		if (std_albumart.is_valid()) {
			vector<uint8_t> id3_data;
			std_albumart->commit(p_abort);
			std_albumart->save(id3_data, p_abort);
			sacd_reader->set_albumart((uint32_t)albumart_id_e::rawid3, id3_data);
			sacd_reader->commit();
		}
		if (sacd_metabase) {
			sacd_metabase->commit();
		}
	}
}


bool sacd_albumart_extractor_t::is_our_path(const char* p_path, const char* p_ext) {
	return (stricmp_utf8(p_ext, "WV") != 0) && sacd_core_t::g_is_our_path(p_path, p_ext);
}

album_art_extractor_instance_ptr sacd_albumart_extractor_t::open(file_ptr p_filehint, const char* p_path, abort_callback& p_abort) {
	return new service_impl_t<sacd_albumart_instance_t>(p_filehint, p_path, input_open_info_read, p_abort);
}


bool sacd_albumart_editor_t::is_our_path(const char* p_path, const char* p_ext) {
	return (stricmp_utf8(p_ext, "WV") != 0) && sacd_core_t::g_is_our_path(p_path, p_ext);
}

album_art_editor_instance_ptr sacd_albumart_editor_t::open(file_ptr p_filehint, const char* p_path, abort_callback& p_abort) {
	return new service_impl_t<sacd_albumart_instance_t>(p_filehint, p_path, input_open_info_write, p_abort);
}

static service_factory_single_t<sacd_albumart_extractor_t> g_sacd_albumart_extractor_factory;

static service_factory_single_t<sacd_albumart_editor_t> g_sacd_albumart_editor_factory;
