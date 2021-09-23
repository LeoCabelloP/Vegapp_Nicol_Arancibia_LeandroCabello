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

#include "sacd_setup.h"
#include "proxy_output.h"

constexpr GUID  proxy_output_guid = { 0xf4d74794, 0x2936, 0x4595, { 0x8d, 0x6a, 0xc0, 0xce, 0x7c, 0x5b, 0x35, 0x25 } };
constexpr char* proxy_output_name = "DSD";

constexpr GUID NULL_output_guid = { 0xEEEB07DE, 0xC2C8, 0x44C2, { 0x98, 0x5C, 0xC8, 0x58, 0x56, 0xD9, 0x6D, 0xA1 } };
constexpr GUID DS_output_guid   = { 0xD41D2423, 0xFBB0, 0x4635, { 0xB2, 0x33, 0x70, 0x54, 0xF7, 0x98, 0x14, 0xAB } };
constexpr GUID no_proxy_guids[] = { NULL_output_guid, DS_output_guid };

constexpr t_uint8 DoP_MARKER[2] = { 0x05, 0xFA };
constexpr int MAX_OUTPUT_WAIT_TIME_MS = 3000;

static GUID xor_guids(const GUID& p_guid1, const GUID& p_guid2) {
	GUID guid_xor;
	t_uint8* p_1 = (t_uint8*)&p_guid1;
	t_uint8* p_2 = (t_uint8*)&p_guid2;
	t_uint8* p_x = (t_uint8*)&guid_xor;
	for (int i = 0; i < sizeof(GUID); i++) {
		p_x[i] = p_1[i] ^ p_2[i];
	}
	return guid_xor;
}

bool proxy_output_t::g_advanced_settings_query() {
	return false;
}

bool proxy_output_t::g_needs_bitdepth_config() {
	return true;
}

bool proxy_output_t::g_needs_dither_config() {
	return true;
}

bool proxy_output_t::g_needs_device_list_prefixes() {
	return false;
}

bool proxy_output_t::g_supports_multiple_streams() {
	return true;
}

bool proxy_output_t::g_is_high_latency() {
	return true;
}

void proxy_output_t::g_enum_devices(output_device_enum_callback& p_callback) {
	class dsd_output_device_enum_callback : public output_device_enum_callback {
		output_device_enum_callback* m_callback;
		GUID m_guid;
		const char* m_name;
	public:
		dsd_output_device_enum_callback(output_device_enum_callback* p_callback, const GUID& p_guid, const char* p_name) {
			m_callback = p_callback;
			m_guid = p_guid;
			m_name = p_name;
		}
		void on_device(const GUID& p_guid, const char* p_name, unsigned p_name_length) {
			string8 name = m_name;
			name.get_length() > 0 ? name << " : " << p_name : name << p_name;
			auto guid_dev = xor_guids(m_guid, p_guid);
			m_callback->on_device(guid_dev, name, name.get_length());
		}
	};
	service_enum_t<output_entry> output_enum;
	service_ptr_t<output_entry> output_ptr;
	while (output_enum.next(output_ptr)) {
		auto guid = output_ptr->get_guid();
		auto name = output_ptr->get_name();
		if (guid != g_get_guid()) {
			bool skip_guid = false;
			for (auto no_proxy_guid : no_proxy_guids) {
				if (no_proxy_guid == guid) {
					skip_guid = true;
				}
			}
			if (!skip_guid) {
				dsd_output_device_enum_callback dsd_callback(&p_callback, guid, name);
				output_ptr->enum_devices(dsd_callback);
			}
		}
	}
}

GUID proxy_output_t::g_get_guid() {
	return proxy_output_guid;
}

const char* proxy_output_t::g_get_name() {
	return proxy_output_name;
}

void proxy_output_t::g_advanced_settings_popup(HWND p_parent, POINT p_menupoint) {
}

bool proxy_output_t::g_get_output(service_ptr_t<output_entry>& p_output, GUID& p_guid, string8& p_name) {
	bool ok = false;
	outputCoreConfig_t output_config;
	static_api_ptr_t<output_manager> output_manager_ptr;
	output_manager_ptr->getCoreConfig(output_config);
	service_enum_t<output_entry> output_enum;
	service_ptr_t<output_entry> output_ptr;
	while (output_enum.next(output_ptr)) {
		string8 output_name;
		if (output_ptr->get_guid() != g_get_guid()) {
			auto output_guid = xor_guids(output_ptr->get_guid(), output_config.m_device);
			if (output_ptr->get_device_name(output_guid, output_name)) {
				p_output = output_ptr;
 				p_guid = output_guid;
				p_name = output_name;
				ok = true;
				break;
			}
		}
	}
	return ok;
}

proxy_output_t::proxy_output_t(const GUID& p_device, double p_buffer_length, bool p_dither, t_uint32 p_bitdepth) {
	m_dsd_playback = false;
	m_volume_dB = 0;
	m_trace = CSACDPreferences::g_get_trace();
	m_paused = false;
	m_is_output_v2 = false;
	m_is_output_v3 = false;
	m_is_output_v4 = false;
	m_track_marks = false;
	service_ptr_t<output_entry> output_ptr;
	GUID output_guid;
	string8 output_name;
	if (g_get_output(output_ptr, output_guid, output_name)) {
		output_ptr->instantiate(m_output, output_guid, p_buffer_length, p_dither, p_bitdepth);
	}
	if (m_output.is_empty()) {
		throw_exception_with_message<exception_io>("proxy_output::proxy_output() => DSD output device is not defined");
	}
	m_is_output_v2 = m_output->service_query_t(m_output_v2);
	m_is_output_v3 = m_output->service_query_t(m_output_v3);
	m_is_output_v4 = m_output->service_query_t(m_output_v4);
	if (m_trace) {
		console::printf(
			"proxy_output::proxy_output() => output%s [device = \"%s : %s\", buffer_length = %s, dither = %s, bitdepth = %d]",
			m_is_output_v4 ? "_v4" : m_is_output_v3 ? "_v3" : m_is_output_v2 ? "_v2" : "",
			output_ptr->get_name(),
			output_name.c_str(),
			format_float(p_buffer_length, 0, 5).toString(),
			p_dither ? "true" : "false",
			p_bitdepth
		);
		check_dsd_stream(true);
	}
	service_enum_t<dsd_processor_service> dsddsp_enum;
	service_ptr_t<dsd_processor_service> dsddsp_temp;
	while (dsddsp_enum.next(dsddsp_temp)) {
		if (dsddsp_temp->get_guid() == CSACDPreferences::get_dsd_processor()) {
			m_dsddsp = dsddsp_temp;
			if (m_trace) {
				console::printf("proxy_output::proxy_output() => Use DSD Processor [name = \"%s\"]", m_dsddsp->get_name());
			}
			break;
		}
	}
	m_dsd_stream->set_accept_data(true);
}

proxy_output_t::~proxy_output_t() {
	m_dsd_stream->set_accept_data(false);
	m_dsd_stream->flush();
	if (m_dsddsp.is_valid()) {
		m_dsddsp->stop();
		if (m_trace) {
			console::printf("proxy_output::~proxy_output() => Stop DSD Processor");
		}
	}
	if (m_trace) {
		console::printf("proxy_output::~proxy_output()");
	}
}

double proxy_output_t::get_latency() {
	auto latency = m_output->get_latency();
	/*
	if (m_trace) {
		console::printf("proxy_output::get_latency() => %s", format_float(latency, 0, 5).toString());
	}
	*/
	return latency;
}

void proxy_output_t::process_samples(const audio_chunk& p_chunk) {
	t_samplespec pcm_spec(p_chunk);
	audio_chunk_impl dop_chunk;
	if (!pcm_spec.is_valid()) {
		throw_exception_with_message<exception_io_data>("proxy_output::process_samples() => Invalid audio stream specifications");
	}
	if (m_dsd_stream->is_streaming() != m_dsd_playback) {
		m_dsd_playback = m_dsd_stream->is_streaming();
		if (m_trace) {
			check_dsd_stream(true);
			console::printf("proxy_output::process_samples() => Switch to %s playback", m_dsd_playback ? "DSD" : "PCM");
		}
	}
	if (m_trace) {
		check_dsd_stream(false);
	}
	if (m_dsd_playback) {
		// DSD playback
		while (m_dsd_stream->get_chunk_count() > 0) {
			auto dsd_chunk = m_dsd_stream->get_first_chunk();
			audio_chunk_impl out_chunk;
			if (pick_dsd_processor(dsd_chunk.get_spec(), pcm_spec)) {
				t_size inp_samples = dsd_chunk.get_sample_count();
				t_size out_samples;
				auto out_data = m_dsddsp->run(dsd_chunk.get_data(), inp_samples, &out_samples);
				m_dop_converter.set_inp_spec(m_dsddsp_out_spec);
				m_dop_converter.dsd_to_dop(out_data, out_samples, out_chunk);
			}
			else {
				m_dop_converter.set_inp_spec(dsd_chunk.get_spec());
				m_dop_converter.dsd_to_dop(dsd_chunk.get_data(), dsd_chunk.get_sample_count(), out_chunk);
			}
			m_dsd_stream->remove_first_chunk();
			if (dop_chunk.is_empty()) {
				dop_chunk = out_chunk;
				if (m_dsd_stream->get_chunk_count() <= DSD_CHUNKS_TRESHOLD) {
					break;
				}
			}
			else {
				if (dop_chunk.get_spec() != out_chunk.get_spec()) {
					break;
				}
				dop_chunk.set_data_size(dop_chunk.get_data_size() + out_chunk.get_data_size());
				memcpy(dop_chunk.get_data() + dop_chunk.get_channel_count() * dop_chunk.get_sample_count(), out_chunk.get_data(), out_chunk.get_channel_count() * out_chunk.get_sample_count() * sizeof(audio_sample));
				dop_chunk.set_sample_count(dop_chunk.get_sample_count() + out_chunk.get_sample_count());
		 	}
		}
		if (dop_chunk.get_sample_count() > 0) {
			m_output->process_samples(dop_chunk);
		}
	}
	else {
		// PCM playback
		static t_samplespec null_spec;
		if (pick_dsd_processor(null_spec, pcm_spec)) {
			t_size inp_samples = p_chunk.get_sample_count();
			t_size out_samples;
			auto out_data = m_dsddsp->run(p_chunk.get_data(), inp_samples, &out_samples);
			m_dop_converter.set_inp_spec(m_dsddsp_out_spec);
			m_dop_converter.dsd_to_dop(out_data, out_samples, dop_chunk);
			m_output->process_samples(dop_chunk);
		}
		else {
			m_output->process_samples(p_chunk);
		}
	}
	/*
	if (m_trace) {
		console::printf("proxy_output::process_samples(channels = %d, sample_rate = %d, channel_config = 0x%08x, chunk_duration = %s)",
			p_chunk.get_channels(), p_chunk.get_sample_rate(), pcm_spec.m_channel_config, format_float(p_chunk.get_duration(), 0, 5).toString()
		);
	}
	*/
}

void proxy_output_t::update(bool& p_ready) {
	m_output->update(p_ready);
	/*
	if (m_trace) {
		console::printf("proxy_output::update(%s)", p_ready ? "true" : "false");
	}
	*/
}

void proxy_output_t::pause(bool p_state) {
	m_output->pause(p_state);
	m_paused = p_state;
	if (m_trace) {
		check_dsd_stream(true);
		console::printf("proxy_output::pause(%s)", p_state ? "true" : "false");
	}
}

void proxy_output_t::flush() {
	m_dsd_stream->flush();
	m_output->flush();
	if (m_trace) {
		check_dsd_stream(true);
		console::printf("proxy_output::flush()");
	}
}

void proxy_output_t::flush_changing_track() {
	m_dsd_stream->flush();
	if (m_is_output_v2) {
		m_output_v2->flush_changing_track();
	}
	else {
		m_output->flush();
	}
	if (m_trace) {
		check_dsd_stream(true);
		console::printf("proxy_output::flush_changing_track()");
	}
}

void proxy_output_t::force_play() {
	m_output->force_play();
	if (m_trace) {
		check_dsd_stream(true);
		console::printf("proxy_output::force_play()");
	}
}

void proxy_output_t::volume_set(double p_val_dB) {
	m_volume_dB = p_val_dB;
	volume_adjust();
	if (m_trace) {
		console::printf("proxy_output::volume_set(%s)", format_float(p_val_dB, 0, 5).toString());
	}
}

bool proxy_output_t::want_track_marks() {
	m_track_marks = m_is_output_v2 ? m_output_v2->want_track_marks() : false;
	if (m_trace) {
		console::printf("proxy_output::want_track_marks() => %s", m_track_marks ? "true" : "false");
	}
	return true;
}

void proxy_output_t::on_track_mark() {
	if (m_track_marks) {
		m_output_v2->on_track_mark();
	}
	if (m_trace) {
		check_dsd_stream(true);
		console::printf("proxy_output::on_track_mark()");
	}
}

void proxy_output_t::enable_fading(bool p_state) {
	if (m_is_output_v2) {
		m_output_v2->enable_fading(p_state);
	}
	if (m_trace) {
		console::printf("proxy_output::enable_fading(%s)", p_state ? "true" : "false");
	}
}

unsigned proxy_output_t::get_forced_sample_rate() {
	unsigned forced_sample_rate;
	forced_sample_rate = m_is_output_v3 ? m_output_v3->get_forced_sample_rate() : 0;
	if (m_trace) {
		console::printf("proxy_output::get_forced_sample_rate() => %d", forced_sample_rate);
	}
	return m_is_output_v3 ? m_output_v3->get_forced_sample_rate() : 0;
}

void proxy_output_t::get_injected_dsps(dsp_chain_config& p_dsps) {
	if (m_is_output_v3) {
		m_output_v3->get_injected_dsps(p_dsps);
	}
	if (m_trace) {
		console::printf("proxy_output::get_injected_dsps() => %d", p_dsps.get_count());
	}
}

eventHandle_t proxy_output_t::get_trigger_event() {
	auto evt{ pfc::eventInvalid };
	if (m_is_output_v4) {
		evt = m_output_v4->get_trigger_event();
	}
	if (m_trace) {
		console::printf("proxy_output::get_trigger_event() => %d", evt);
	}
	return evt;
}

bool proxy_output_t::is_progressing() {
	auto processing = true;
	eventHandle_t evt{ pfc::eventInvalid };
	if (m_is_output_v4) {
		processing = m_output_v4->is_progressing();
	}
	if (m_trace) {
		console::printf("proxy_output::is_progressing() => %s", processing ? "true" : "false");
	}
	return processing;
}

size_t proxy_output_t::update_v2() {
	auto samples = SIZE_MAX;
	if (m_is_output_v4) {
		samples = m_output_v4->update_v2();
	}
	if (m_trace) {
		console::printf("proxy_output::update_v2() => %d", samples);
	}
	return samples;
}

bool proxy_output_t::pick_dsd_processor(t_samplespec& p_dsd_spec, t_samplespec& p_pcm_spec) {
	t_samplespec inp_spec = p_dsd_spec;
	t_samplespec out_spec = p_pcm_spec;
	if (inp_spec.is_valid()) {
		out_spec.m_sample_rate = p_dsd_spec.m_sample_rate;
	}
	else {
		inp_spec = p_pcm_spec;
	}
	if (m_dsddsp.is_valid() && m_dsddsp->is_active()) {
		out_spec.m_sample_rate = m_dsddsp_out_spec.m_sample_rate;
	}
	if (m_dsddsp.is_valid()) {
		if (m_dsddsp->is_changed() || inp_spec != m_inp_spec || out_spec != m_out_spec) {
			m_dsddsp_out_spec = out_spec;
			bool ok = m_dsddsp->start(inp_spec.m_channels, inp_spec.m_sample_rate, inp_spec.m_channel_config, m_dsddsp_out_spec.m_channels, m_dsddsp_out_spec.m_sample_rate, m_dsddsp_out_spec.m_channel_config);
			if (m_trace) {
				console::printf("proxy_output::process_samples() => Start DSD Processor [channels: %d -> %d, samplerate: %d -> %d, channel_config: 0x%08u -> 0x%08u] %s",
					inp_spec.m_channels, m_dsddsp_out_spec.m_channels,
					inp_spec.m_sample_rate, m_dsddsp_out_spec.m_sample_rate,
					inp_spec.m_channel_config, m_dsddsp_out_spec.m_channel_config,
					ok ? "running" : "not running"
				);
				m_dsddsp->set_volume(m_volume_dB);
				if (m_dsddsp->is_active()) {
					out_spec.m_sample_rate = m_dsddsp_out_spec.m_sample_rate;
				}
			}
		}
	}
	if (inp_spec != m_inp_spec) {
		m_inp_spec = inp_spec;
		if (m_trace) {
			console::printf("proxy_output::process_samples() => Input stream [channels: %d, samplerate: %d, channel_config: 0x%08u]",
				inp_spec.m_channels, inp_spec.m_sample_rate, inp_spec.m_channel_config
			);
		}
	}
	if (out_spec != m_out_spec) {
		m_out_spec = out_spec;
		m_dop_converter.set_out_spec(m_out_spec);
		volume_adjust();
		if (m_trace) {
			console::printf("proxy_output::process_samples() => Output stream [channels: %d, samplerate: %d, channel_config: 0x%08u]",
				out_spec.m_channels, out_spec.m_sample_rate, out_spec.m_channel_config
			);
		}
	}
	return m_dsddsp.is_valid() && m_dsddsp->is_active();
}

void proxy_output_t::volume_adjust() {
	m_output->volume_set((m_out_spec.m_sample_rate < 2822400) ? m_volume_dB : 0);
	if (m_dsddsp.is_valid()) {
		m_dsddsp->set_volume(m_volume_dB);
	}
}

void proxy_output_t::check_dsd_stream(bool p_update) {
	static size_t max_chunks = 0;
	auto chunks = m_dsd_stream->get_chunk_count();
	if (p_update || (chunks > max_chunks)) {
		max_chunks = chunks;
		console::printf("proxy_output::check_dsd_stream(%s) => DSD stream contains %d chunks and %d samples", p_update ? "true" : "false", chunks, m_dsd_stream->get_sample_count());
	}
}

static output_factory_t<proxy_output_t> g_proxy_output_factory;
