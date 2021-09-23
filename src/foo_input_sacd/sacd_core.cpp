/*
* SACD Decoder plugin
* Copyright (c) 2011-2020 Maxim V.Anisiutkin <maxim.anisiutkin@gmail.com>
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

#include "sacd_core.h"

bool sacd_core_t::g_is_our_content_type(const char* p_type) {
	return false;
}

bool sacd_core_t::g_is_our_path(const char* p_path, const char* p_ext) {
	if ((stricmp_utf8(p_ext, "WV") == 0) && (stricmp_utf8(p_path, "!.WV") != 0) && g_dsd_in_wavpack(p_path)) {
		return true;
	}
	string_filename_ext filename_ext(p_path);
	return
		(stricmp_utf8(p_ext, "ISO") == 0 || stricmp_utf8(p_ext, "DAT") == 0) && sacd_disc_t::g_is_sacd(p_path) ||
		stricmp_utf8(p_ext, "DFF") == 0 ||
		stricmp_utf8(p_ext, "DSF") == 0 ||
		(stricmp_utf8(filename_ext, "") == 0 || stricmp_utf8(filename_ext, "MASTER1.TOC") == 0) && strlen_utf8(p_path) > 7 && sacd_disc_t::g_is_sacd(p_path[7]);
}

sacd_core_t::sacd_core_t() : media_type(media_type_e::INVALID), access_mode(ACCESS_MODE_NULL) {
}

void sacd_core_t::open(file_ptr p_filehint, const char* p_path, t_input_open_reason p_reason, abort_callback& p_abort) {
	string_filename_ext filename_ext(p_path);
	string_extension ext(p_path);
	auto is_sacd_disc = false;
	media_type = media_type_e::INVALID;
	if (stricmp_utf8(ext, "ISO") == 0) {
		media_type = media_type_e::ISO;
	}
	else if (stricmp_utf8(ext, "DAT") == 0) {
		media_type = media_type_e::ISO;
	}
	else if (stricmp_utf8(ext, "DFF") == 0) {
		media_type = media_type_e::DSDIFF;
	}
	else if (stricmp_utf8(ext, "DSF") == 0) {
		media_type = media_type_e::DSF;
	}
	else if (stricmp_utf8(ext, "WV") == 0) {
		media_type = media_type_e::WAVPACK;
	}
	else if ((stricmp_utf8(filename_ext, "") == 0 || stricmp_utf8(filename_ext, "MASTER1.TOC") == 0) && strlen_utf8(p_path) > 7 && sacd_disc_t::g_is_sacd(p_path[7])) {
		media_type = media_type_e::ISO;
		is_sacd_disc = true;
	}
	if (media_type == media_type_e::INVALID) {
		throw exception_io_unsupported_format();
	}
	if (is_sacd_disc) {
		sacd_media = make_unique<sacd_media_disc_t>();
		if (!sacd_media) {
			throw exception_overflow();
		}
	}
	else {
		sacd_media = make_unique<sacd_media_file_t>();
		if (!sacd_media) {
			throw exception_overflow();
		}
	}
	switch (media_type) {
	case media_type_e::ISO:
		sacd_reader = make_unique<sacd_disc_t>();
		if (!sacd_reader) {
			throw exception_overflow();
		}
		break;
	case media_type_e::DSDIFF:
		sacd_reader = make_unique<sacd_dsdiff_t>();
		if (!sacd_reader) {
			throw exception_overflow();
		}
		break;
	case media_type_e::DSF:
		sacd_reader = make_unique<sacd_dsf_t>();
		if (!sacd_reader) {
			throw exception_overflow();
		}
		break;
	case media_type_e::WAVPACK:
		sacd_reader = make_unique<sacd_wavpack_t>();
		if (!sacd_reader) {
			throw exception_overflow();
		}
		break;
	default:
		throw exception_io_data();
		break;
	}
	access_mode = ACCESS_MODE_NULL;
	switch (CSACDPreferences::get_area()) {
	case 0:
		access_mode |= ACCESS_MODE_TWOCH | ACCESS_MODE_MULCH;
		break;
	case 1:
		access_mode |= ACCESS_MODE_TWOCH;
		break;
	case 2:
		access_mode |= ACCESS_MODE_MULCH;
		break;
	}
	if (CSACDPreferences::get_emaster()) {
		access_mode |= ACCESS_MODE_FULL_PLAYBACK;
	}
	t_input_open_reason reason = (media_type == media_type_e::ISO && p_reason == input_open_info_write) ? input_open_info_read : p_reason;
	if (!sacd_media->open(p_filehint, p_path, reason)) {
		throw exception_io_data();
	}
	try {
		if (!sacd_reader->open(sacd_media.get())) {
			throw exception_io_data();
		}
	}
	catch (exception_io_unsupported_format) {
		throw;
	}
	sacd_reader->set_mode(access_mode);
	switch (media_type) {
	case media_type_e::ISO:
		const char* metafile_path = nullptr;
		string_replace_extension metafile_name(p_path, "xml");
		if (!is_sacd_disc && CSACDPreferences::get_store_tags_with_iso()) {
			metafile_path = metafile_name;
		}
		sacd_metabase = make_unique<sacd_metabase_t>(static_cast<sacd_disc_t*>(sacd_reader.get()), metafile_path);
	}
}
