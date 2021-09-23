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

#include "std_wavpack.h"
#include "std_service.h"

std_wavpack_input_t::std_wavpack_input_t(file_ptr p_filehint, const char* p_path, t_input_open_reason p_reason, abort_callback& p_abort) {
	auto std_service = new service_impl_t<std_service_t>();
	auto std_service_list = std_service->get_client()->get_service_list();
	service_ptr_t<input_entry> std_input;
	while (std_service_list) {
		if (std_service_list->get_class_guid() == input_entry::class_guid) {
			service_ptr_t<service_base> std_instance;
			std_service_list->instance_create(std_instance);
			if (std_instance->cast(std_input)) {
				if (std_input->is_our_path(p_path, "WV")) {
					break;
				}
			}
		}
		std_service_list = std_service_list->__internal__next;
	}
	if (!std_input.is_valid()) {
		return;
	}
	service_ptr_t<input_info_writer> writer;
	switch (p_reason) {
	case input_open_info_read:
		std_input->open_for_info_read(input_info, p_filehint, p_path, p_abort);
		break;
	case input_open_info_write:
		std_input->open_for_info_write(writer, p_filehint, p_path, p_abort);
		writer->cast(input_info);
		break;
	}
}

std_wavpack_input_t::~std_wavpack_input_t() {
}

t_uint32 std_wavpack_input_t::get_subsong_count() {
	return input_info->get_subsong_count();
}

t_uint32 std_wavpack_input_t::get_subsong(t_uint32 p_index) {
	return input_info->get_subsong(p_index);
}

void std_wavpack_input_t::get_info(t_uint32 p_subsong, file_info& p_info, abort_callback& p_abort) {
	input_info->get_info(p_subsong, p_info, p_abort);
}

t_filestats std_wavpack_input_t::get_file_stats(abort_callback& p_abort) {
	return input_info->get_file_stats(p_abort);
}

void std_wavpack_input_t::set_info(t_uint32 p_subsong, const file_info& p_info, abort_callback& p_abort) {
	service_ptr_t<input_info_writer> writer;
	if (input_info->cast(writer)) {
		writer->set_info(p_subsong, p_info, p_abort);
	}
}

void std_wavpack_input_t::commit(abort_callback& p_abort) {
	service_ptr_t<input_info_writer> writer;
	if (input_info->cast(writer)) {
		writer->commit(p_abort);
	}
}
