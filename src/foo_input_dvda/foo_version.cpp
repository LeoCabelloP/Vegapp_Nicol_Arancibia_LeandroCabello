/*
* DVD-Audio Decoder plugin
* Copyright (c) 2009 Maxim V.Anisiutkin <maxim.anisiutkin@gmail.com>
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

#include "foo_version.h"

#ifdef FOO_VERSION_09X
#endif

#ifdef FOO_VERSION_08X

#include <ks.h>
#include <ksmedia.h>

t_size string_find_first(const char* p_string, const char* p_tofind, t_size p_start) {
	return (t_size)string8(p_string).find_first(p_tofind, (int)p_start);
}

static struct {
	DWORD m_wfx;
	unsigned m_native;
} const g_translation_table[] = {
	{SPEAKER_FRONT_LEFT,            audio_chunk_t::channel_front_left},
	{SPEAKER_FRONT_RIGHT,           audio_chunk_t::channel_front_right},
	{SPEAKER_FRONT_CENTER,          audio_chunk_t::channel_front_center},
	{SPEAKER_LOW_FREQUENCY,         audio_chunk_t::channel_lfe},
	{SPEAKER_BACK_LEFT,             audio_chunk_t::channel_back_left},
	{SPEAKER_BACK_RIGHT,            audio_chunk_t::channel_back_right},
	{SPEAKER_FRONT_LEFT_OF_CENTER,  audio_chunk_t::channel_front_center_left},
	{SPEAKER_FRONT_RIGHT_OF_CENTER, audio_chunk_t::channel_front_center_right},
	{SPEAKER_BACK_CENTER,           audio_chunk_t::channel_back_center},
	{SPEAKER_SIDE_LEFT,             audio_chunk_t::channel_side_left},
	{SPEAKER_SIDE_RIGHT,            audio_chunk_t::channel_side_right},
	{SPEAKER_TOP_CENTER,            audio_chunk_t::channel_top_center},
	{SPEAKER_TOP_FRONT_LEFT,        audio_chunk_t::channel_top_front_left},
	{SPEAKER_TOP_FRONT_CENTER,      audio_chunk_t::channel_top_front_center},
	{SPEAKER_TOP_FRONT_RIGHT,       audio_chunk_t::channel_top_front_right},
	{SPEAKER_TOP_BACK_LEFT,         audio_chunk_t::channel_top_back_left},
	{SPEAKER_TOP_BACK_CENTER,       audio_chunk_t::channel_top_back_center},
	{SPEAKER_TOP_BACK_RIGHT,        audio_chunk_t::channel_top_back_right},
};

static unsigned g_audio_channel_config_table[] = {
	0,
	audio_chunk_t::channel_config_mono,
	audio_chunk_t::channel_config_stereo,
	audio_chunk_t::channel_front_left | audio_chunk_t::channel_front_right | audio_chunk_t::channel_lfe,
	audio_chunk_t::channel_front_left | audio_chunk_t::channel_front_right | audio_chunk_t::channel_back_left | audio_chunk_t::channel_back_right,
	audio_chunk_t::channel_front_left | audio_chunk_t::channel_front_right | audio_chunk_t::channel_back_left | audio_chunk_t::channel_back_right | audio_chunk_t::channel_lfe,
	audio_chunk_t::channel_config_5point1,
	audio_chunk_t::channel_front_left | audio_chunk_t::channel_front_right | audio_chunk_t::channel_back_left | audio_chunk_t::channel_back_right | audio_chunk_t::channel_lfe | audio_chunk_t::channel_front_center_right | audio_chunk_t::channel_front_center_left,
	audio_chunk_t::channel_front_left | audio_chunk_t::channel_front_right | audio_chunk_t::channel_back_left | audio_chunk_t::channel_back_right | audio_chunk_t::channel_front_center | audio_chunk_t::channel_lfe | audio_chunk_t::channel_front_center_right | audio_chunk_t::channel_front_center_left,
};

unsigned audio_chunk_t::g_guess_channel_config(unsigned count) {
	if (count >= tabsize(g_audio_channel_config_table))
		return 0;
	return g_audio_channel_config_table[count];
}

unsigned audio_chunk_t::g_channel_config_from_wfx(DWORD p_wfx) {
	unsigned ret = 0;
	for(unsigned n = 0; n < tabsize(g_translation_table); n++) {
		if (p_wfx & g_translation_table[n].m_wfx)
			ret |= g_translation_table[n].m_native;
	}
	return ret;
}

void audio_chunk_t::set_data_fixedpoint(const void* ptr, t_size bytes, unsigned srate, unsigned nch, unsigned bps, unsigned channel_config) {
	m_chunk->set_data_fixedpoint(ptr, bytes, srate, nch, bps);
}

#endif