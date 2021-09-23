/*
* DVD-Audio Decoder plugin
* Copyright (c) 2009-2020 Maxim V.Anisiutkin <maxim.anisiutkin@gmail.com>
*
* DVD-Audio Decoder is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation; either
* version 2.1 of the License, or (at your option) any later version.
*
* DVD-Audio Decoder is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with FFmpeg; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/

#ifndef _DVDA_CONFIG_H_INCLUDED
#define _DVDA_CONFIG_H_INCLUDED

#include <vector>
#include <foobar2000.h>

constexpr GUID DVDA_GUID { 0x132cc739, 0x29b0, 0x4175, { 0x97, 0x84, 0x25, 0xc9, 0x51, 0x1, 0x54, 0xe1 } };

constexpr GUID  DSP_WM_GUID { 0x9cd44d0f, 0x290b, 0x4a74, { 0xb4, 0xea, 0x6d, 0xd5, 0x93, 0x4c, 0x9, 0x80 } };
constexpr char* DSP_WM_NAME = "DVD-Audio Watermark Detector";

typedef pfc::array_t<t_uint8> byte_array_t;
typedef pfc::map_t<t_size, file_info_impl> file_info_array_t;
typedef service_ptr_t<file> media_file_t;
typedef audio_chunk audio_chunk_t;

extern void console_printf(const char* fmt, ...);
extern void console_vprintf(const char* fmt, va_list vl);

using namespace pfc::stringcvt;

using pfc::array_t;
using pfc::list_t;
using pfc::list_base_const_t;
using pfc::string8;
using pfc::string_directory;
using pfc::string_printf;
using pfc::string_extension;
using pfc::string_replace_extension;
using pfc::string_formatter;
using pfc::string_filename_ext;
using pfc::string_find_first;
using pfc::strlen_utf8;

using std::vector;

#endif
