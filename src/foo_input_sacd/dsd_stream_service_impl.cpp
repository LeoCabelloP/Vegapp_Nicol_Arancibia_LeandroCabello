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

#include "dsd_stream_service_impl.h"

dsd_stream_service_impl::dsd_stream_service_impl() {
	m_chunk_read_samples = 0;
	m_samples = 0;
	m_streaming = 0;
	m_accept_data = false;
}

dsd_stream_service_impl::~dsd_stream_service_impl() {
	flush();
}

bool dsd_stream_service_impl::is_streaming() const {
	return (m_streaming > 0);
}

void dsd_stream_service_impl::set_streaming(bool p_streaming) {
	m_streaming += p_streaming ? +1 : -1;
}

bool dsd_stream_service_impl::is_accept_data() const {
	return m_accept_data;
}

void dsd_stream_service_impl::set_accept_data(bool p_accept) {
	m_accept_data = p_accept;
}

t_size dsd_stream_service_impl::get_chunk_count() const {
	return m_chunks.get_count();
}

t_size dsd_stream_service_impl::get_sample_count() const {
	return m_samples;
}

t_samplespec dsd_stream_service_impl::get_spec() const {
	lock_guard<mutex> lock(m_chunks_mutex);
	t_samplespec spec;
	auto chunk_iter = m_chunks.first();
	if (chunk_iter.is_valid()) {
		spec = chunk_iter->get_spec();
	}
	return spec;
}

bool dsd_stream_service_impl::is_spec_change() const {
	return m_last_read_spec != get_spec();
}

void dsd_stream_service_impl::reset_spec_change() {
	m_last_read_spec = get_spec();
}

const dsd_chunk_t& dsd_stream_service_impl::get_first_chunk() const {
	lock_guard<mutex> lock(m_chunks_mutex);
	auto chunk_iter = m_chunks.first();
	return chunk_iter.is_valid() ? *chunk_iter : m_null_chunk;
}

void dsd_stream_service_impl::remove_first_chunk() {
	lock_guard<mutex> lock(m_chunks_mutex);
	auto chunk_iter = m_chunks.first();
	if (chunk_iter.is_valid()) {
		m_samples -= chunk_iter->get_sample_count();
		m_chunks.remove(chunk_iter);
	}
}

t_size dsd_stream_service_impl::read(t_uint8* p_buffer, t_size p_samples) {
	lock_guard<mutex> lock(m_chunks_mutex);
	t_size read_samples = 0;
	while (m_chunks.get_count() > 0) {
		auto chunk_iter = m_chunks.first();
		t_samplespec chunk_spec = chunk_iter->get_spec();
		if (chunk_spec != m_last_read_spec) {
			break;
		}
		auto chunk_data = chunk_iter->get_data();
		t_size chunk_samples = chunk_iter->get_sample_count();
		unsigned chunk_channels = chunk_iter->get_channels();
		t_size samples_to_read = p_samples - read_samples;
		if (chunk_samples - m_chunk_read_samples > samples_to_read) {
			memcpy(p_buffer + read_samples * chunk_channels, chunk_data + m_chunk_read_samples * chunk_channels, samples_to_read * chunk_channels);
			m_chunk_read_samples += samples_to_read;
			read_samples += samples_to_read;
			m_samples -= samples_to_read;
		}
		else {
			memcpy(p_buffer + read_samples * chunk_channels, chunk_data + m_chunk_read_samples * chunk_channels, (chunk_samples - m_chunk_read_samples) * chunk_channels);
			m_chunks.remove(chunk_iter);
			read_samples += chunk_samples - m_chunk_read_samples;
			m_samples -= chunk_samples - m_chunk_read_samples;
			m_chunk_read_samples = 0;
		}
		if (p_samples <= read_samples) {
			break;
		}
	}
	return read_samples;
}

void dsd_stream_service_impl::write(const t_uint8* p_buffer, t_size p_samples, t_size p_channels, unsigned p_samplerate, unsigned p_channel_config) {
	lock_guard<mutex> lock(m_chunks_mutex);
	if (m_accept_data) {
		bool appended = false;
		t_samplespec write_spec;
		write_spec.m_channels = p_channels;
		write_spec.m_sample_rate = p_samplerate;
		write_spec.m_channel_config = p_channel_config;
		m_chunks.add_item(dsd_chunk_t());
		m_chunks.last()->append_data(p_buffer, p_samples, write_spec);
		m_samples += p_samples;
	}
}

void dsd_stream_service_impl::flush() {
	lock_guard<mutex> lock(m_chunks_mutex);
	m_chunks.remove_all();
	m_chunk_read_samples = 0;
	m_samples = 0;
}

static service_factory_single_t<dsd_stream_service_impl> g_dsd_stream_service_factory;
