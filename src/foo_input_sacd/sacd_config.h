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

#ifndef _SACD_CONFIG_H_INCLUDED
#define _SACD_CONFIG_H_INCLUDED

#include <minmax.h>
#include <stdint.h>
#include <vector>
#include <foobar2000.h>

constexpr GUID SACD_GUID { 0xbd4e4810, 0xbdbf, 0x4418,{ 0xa8, 0x53, 0x2, 0xd3, 0xcb, 0x18, 0xe0, 0xe6 } };

using pfc::array_t;
using pfc::chain_list_v2_t;
using pfc::exception_overflow;
using pfc::format_float;
using pfc::string8;
using pfc::string_extension;
using pfc::string_formatter;
using pfc::string_filename_ext;
using pfc::string_printf;
using pfc::string_replace_extension;
using pfc::strlen_utf8;
using pfc::stringcvt::string_utf8_from_wide;
using pfc::stringcvt::string_wide_from_utf8;
using pfc::stringcvt::convert_utf8_to_ascii;
using pfc::stringcvt::convert_utf8_to_wide;
using pfc::throw_exception_with_message;

using std::string;
using std::wstring;
using std::vector;
using std::unique_ptr;
using std::make_unique;
using std::tuple;
using std::make_tuple;

#endif
