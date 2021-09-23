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

#ifndef _SACD_CORE_H_INCLUDED
#define _SACD_CORE_H_INCLUDED

#include "sacd_config.h"
#include "sacd_reader.h"
#include "sacd_disc.h"
#include "sacd_metabase.h"
#include "sacd_dsdiff.h"
#include "sacd_dsf.h"
#include "sacd_wavpack.h"
#include "sacd_setup.h"
#include "sacd_version.h"

class sacd_core_t {
protected:
	media_type_e                media_type;
	uint32_t                    access_mode;
	unique_ptr<sacd_media_t>    sacd_media;
	unique_ptr<sacd_reader_t>   sacd_reader;
	unique_ptr<sacd_metabase_t> sacd_metabase;
public:
	static bool g_is_our_content_type(const char* p_type);
	static bool g_is_our_path(const char* p_path, const char* p_ext);
	sacd_core_t();
	void open(file_ptr p_filehint, const char* p_path, t_input_open_reason p_reason, abort_callback& p_abort);
};

#endif
