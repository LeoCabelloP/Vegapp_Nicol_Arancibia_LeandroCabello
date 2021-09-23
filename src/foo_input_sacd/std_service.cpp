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

#include "std_service.h"

typedef foobar2000_client* _cdecl foobar2000_get_interface_t(foobar2000_api* p_api, HINSTANCE hIns);

std_service_t::std_service_t() : std_service_t("foo_input_std") {
}

std_service_t::std_service_t(const char* p_module) {
	auto hMod = ::GetModuleHandleA(p_module);
	if (hMod) {
		auto std_foobar2000_interface = reinterpret_cast<foobar2000_get_interface_t*>(GetProcAddress(hMod, "foobar2000_get_interface"));
		if (std_foobar2000_interface) {
			std_foobar2000_client = std_foobar2000_interface(g_foobar2000_api, hMod);
		}
	}
}

foobar2000_client* std_service_t::get_client() const {
	return std_foobar2000_client;
}
