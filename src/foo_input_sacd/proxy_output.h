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

#ifndef _PROXY_OUTPUT_H_INCLUDED
#define _PROXY_OUTPUT_H_INCLUDED

#include "dop_converter.h"
#include "dsd_processor_service.h"
#include "dsd_stream_service.h"

using pfc::eventHandle_t;

class proxy_output_t : public output_v4 {
	static constexpr t_size DSD_CHUNKS_TRESHOLD = 8;
	dop_converter_t m_dop_converter;
	static_api_ptr_t<dsd_stream_service> m_dsd_stream;
	bool m_dsd_playback;
	double m_volume_dB;
	bool m_paused;
	bool m_trace;
	t_samplespec m_inp_spec;
	t_samplespec m_out_spec;
	service_ptr_t<output> m_output;
	service_ptr_t<output_v2> m_output_v2;
	bool m_is_output_v2;
	service_ptr_t<output_v3> m_output_v3;
	bool m_is_output_v3;
	service_ptr_t<output_v4> m_output_v4;
	bool m_is_output_v4;
	bool m_track_marks;
	service_ptr_t<dsd_processor_service> m_dsddsp;
	t_samplespec m_dsddsp_out_spec;
public:
	static bool g_advanced_settings_query();
	static bool g_needs_bitdepth_config();
	static bool g_needs_dither_config();
	static bool g_needs_device_list_prefixes();
	static bool g_supports_multiple_streams();
	static bool g_is_high_latency();
	static void g_enum_devices(output_device_enum_callback& p_callback);
	static GUID g_get_guid();
	static const char* g_get_name();
	static void g_advanced_settings_popup(HWND p_parent, POINT p_menupoint);
	static bool g_get_output(service_ptr_t<output_entry>& p_output, GUID& p_guid, string8& p_name);
	proxy_output_t(const GUID& p_device, double p_buffer_length, bool p_dither, t_uint32 p_bitdepth);
	virtual ~proxy_output_t();
	virtual double get_latency();
	virtual void process_samples(const audio_chunk& p_chunk);
	virtual void update(bool& p_ready);
	virtual void pause(bool p_state);
	virtual void flush();
	virtual void flush_changing_track();
	virtual void force_play();
	virtual void volume_set(double p_val_dB);
	virtual bool want_track_marks();
	virtual void on_track_mark();
	virtual void enable_fading(bool p_state);
	virtual unsigned get_forced_sample_rate();
	virtual void get_injected_dsps(dsp_chain_config& p_dsps);
	virtual eventHandle_t get_trigger_event();
	virtual bool is_progressing();
	virtual size_t update_v2();

private:
	bool pick_dsd_processor(t_samplespec& p_dsd_spec, t_samplespec& p_pcm_spec);
	void volume_adjust();
	void check_dsd_stream(bool p_update);
};

#endif
