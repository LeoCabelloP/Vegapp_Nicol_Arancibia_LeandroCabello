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

#include "dop_converter.h"

constexpr t_uint8 DoP_MARKER[2] = { 0x05, 0xFA };

dop_converter_t::dop_converter_t() {
	m_dop_marker_n.set_size(0);
	m_dop_data.set_size(0);
}

void dop_converter_t::set_inp_spec(const t_samplespec& p_spec) {
	m_inp_spec = p_spec;
}

void dop_converter_t::set_out_spec(const t_samplespec& p_spec) {
	m_out_spec = p_spec;
}

void dop_converter_t::dsd_to_dop(const t_uint8* p_inp_data, t_size p_inp_samples, audio_chunk& p_out_chunk) {
	unsigned out_samplerate = m_inp_spec.m_sample_rate / 16;
	t_size inp_channels = m_inp_spec.m_channels;
	t_size out_channels = m_out_spec.m_channels;
	t_size out_samples = p_inp_samples / 2;
	t_size out_bytes = 3 * out_samples * out_channels;
	m_dop_data.set_size(out_bytes);
	t_uint8* dop_data = m_dop_data.get_ptr();
	t_size o = 0;
	if (m_dop_marker_n.get_size() != out_channels) {
		m_dop_marker_n.set_size(out_channels);
		for (t_size ch = 0; ch < m_dop_marker_n.get_size(); ch++) {
			m_dop_marker_n[ch] = 0;
		}
	}
	for (t_size sample = 0; sample < out_samples; sample++) {
		for (t_size ch = 0; ch < out_channels; ch++) {
			t_uint8 b_msb, b_lsb;
			b_msb = p_inp_data[(2 * sample + 0) * inp_channels + ch % inp_channels];
			b_lsb = p_inp_data[(2 * sample + 1) * inp_channels + ch % inp_channels];
			dop_data[o + 0] = b_lsb;
			dop_data[o + 1] = b_msb;
			dop_data[o + 2] = DoP_MARKER[m_dop_marker_n[ch]];
			o += 3;
			m_dop_marker_n[ch] = ++m_dop_marker_n[ch] & 1;
		}
	}
	p_out_chunk.set_data_fixedpoint(dop_data, out_bytes, out_samplerate, out_channels, 24, m_out_spec.m_channel_config);
}
