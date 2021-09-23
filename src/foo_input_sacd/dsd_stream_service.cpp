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

#include "dsd_stream_service.h"

constexpr GUID dsd_stream_service::class_guid = { 0x18091b99, 0x3944, 0x4648,{ 0x96, 0xf5, 0x3, 0xa3, 0xa1, 0x3, 0x44, 0xc2 } };

dsd_chunk_t::dsd_chunk_t() {
}

dsd_chunk_t::dsd_chunk_t(const t_uint8* p_buffer, t_size p_samples, const t_samplespec& p_spec) {
	append_data(p_buffer, p_samples, p_spec);
}

t_samplespec dsd_chunk_t::get_spec() const {
	return m_spec;
}

t_size dsd_chunk_t::get_channels() const {
	return m_spec.m_channels;
}

t_size dsd_chunk_t::get_sample_count() const {
	return (m_spec.m_channels > 0) ? m_buffer.get_count() / m_spec.m_channels : 0;
}

t_size dsd_chunk_t::get_size() const {
	return m_buffer.get_count();
}

const array_t<t_uint8>& dsd_chunk_t::get_buffer() const {
	return m_buffer;
}

const t_uint8* dsd_chunk_t::get_data() const {
	return m_buffer.get_ptr();
}

void dsd_chunk_t::append_data(const t_uint8* p_buffer, t_size p_samples, const t_samplespec& p_spec) {
	m_spec = p_spec;
	m_buffer.append_fromptr(p_buffer, p_samples * m_spec.m_channels);
}
