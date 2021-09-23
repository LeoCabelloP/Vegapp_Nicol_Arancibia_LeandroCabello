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

#include "dvda_disc_cache.h"

static DWORD WINAPI wait_for_flush(LPVOID lpParameter) {
	dvda_disc_cache_t* p_cache = reinterpret_cast<dvda_disc_cache_t*>(lpParameter);
	Sleep(1000);
	if (p_cache->references <= 0) {
		Sleep(1000);
		if (p_cache->references <= 0) {
			Sleep(1000);
			if (p_cache->references <= 0) {
				p_cache->flush();
			}
		}
	}
	p_cache->thread_id = 0;
	return 0;
}

void dvda_disc_cache_t::delayed_flush() {
	if (!thread_id)
		CreateThread(NULL, 0, ::wait_for_flush, (LPVOID)this, 0, &thread_id);
}
