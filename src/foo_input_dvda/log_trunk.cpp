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

#include "log_trunk.h"
#include "dvda_config.h"

extern "C" {
#include "mlp_util.h"
}

void foo_av_log_callback(void* ptr, int level, const char* fmt, va_list vl) {
	#ifdef _DEBUG
	console_vprintf(fmt, vl);
	#else
	if (level != AV_LOG_DEBUG)
		console_vprintf(fmt, vl);
	#endif
}

void my_av_log_set_callback(void (*callback)(void*, int, const char*, va_list)) {
	av_log_set_callback(callback);
}

void my_av_log_set_default_callback(void) {
	av_log_set_callback(av_log_default_callback);
}

extern "C" void dprintf(int ptr, const char* fmt, ...) {
	/*
	va_list vl;
	va_start(vl, fmt);
	console_printfv(fmt, vl);
	va_end(vl);
	*/
}
