/*
* DVD-Audio Watermark Neutralizer
* Copyright (c) 2009-2020 Maxim V.Anisiutkin <maxim.anisiutkin@gmail.com>
*
* DVD-Audio Watermark Neutralizer is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation; either
* version 2.1 of the License, or (at your option) any later version.
*
* DVD-Audio Watermark Neutralizer is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with FFmpeg; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/

#ifndef _PLAYBACK_HANDLER_H_INCLUDED
#define _PLAYBACK_HANDLER_H_INCLUDED

#include <foobar2000.h>

class playback_handler_t : public play_callback_static {
	bool   m_seek_made;
	double m_seek_time;
public:
	playback_handler_t() {
		m_seek_made = false;
		m_seek_time = 0.0;
	}
	bool get_last_seek_and_reset(double& p_seek_time) {
		if (m_seek_made) {
			p_seek_time = m_seek_time;
			m_seek_time = 0.0;
			m_seek_made = false;
			return true;
		}
		else {
			p_seek_time = 0.0;
			return false;
		}
	}
	void on_playback_new_track(metadb_handle_ptr p_track) {}
	void on_playback_seek(double p_time) {
		m_seek_time = p_time;
		m_seek_made = true;
	}
	void on_playback_starting(play_control::t_track_command p_command, bool p_paused) {}
	void on_playback_stop(play_control::t_stop_reason p_reason) {}
	void on_playback_time(double p_time) {}
	void on_playback_pause(bool p_state) {}
	void on_playback_edited(metadb_handle_ptr p_track) {}
	void on_playback_dynamic_info(const file_info& p_info) {}
	void on_playback_dynamic_info_track(const file_info& p_info) {}
	void on_volume_change(float p_new_val) {}
	unsigned get_flags() {
		return flag_on_playback_seek;
	}
};

#endif
