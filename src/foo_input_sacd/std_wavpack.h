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

#ifndef _STD_WAVPACK_H_INCLUDED
#define _STD_WAVPACK_H_INCLUDED

#include "sacd_config.h"

class std_wavpack_input_t : public input_info_writer {
	service_ptr_t<input_info_reader> input_info;
public:
	std_wavpack_input_t(file_ptr p_filehint, const char* p_path, t_input_open_reason p_reason, abort_callback& p_abort);
	~std_wavpack_input_t();
	t_uint32 get_subsong_count();
	t_uint32 get_subsong(t_uint32 p_index);
	void get_info(t_uint32 p_subsong, file_info& p_info, abort_callback& p_abort);
	t_filestats get_file_stats(abort_callback& p_abort);
	void set_info(t_uint32 p_subsong, const file_info& p_info, abort_callback& p_abort);
	void commit(abort_callback& p_abort);
};

#endif
