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

#ifndef _DSD_STREAM_SERVICE_H_INCLUDED
#define _DSD_STREAM_SERVICE_H_INCLUDED

#include "sacd_config.h"

class dsd_chunk_t {
	t_samplespec m_spec;
	array_t<t_uint8> m_buffer;
public:
	dsd_chunk_t();
	dsd_chunk_t(const t_uint8* p_buffer, t_size p_samples, const t_samplespec& p_spec);
	t_samplespec get_spec() const;
	t_size get_channels() const;
	t_size get_sample_count() const;
	t_size get_size() const;
	const array_t<t_uint8>& get_buffer() const;
	const t_uint8* get_data() const;
	void append_data(const t_uint8* p_buffer, t_size p_samples, const t_samplespec& p_spec);
};

class dsd_stream_service : public service_base {
public:
	virtual bool is_streaming() const = 0;
	virtual void set_streaming(bool p_streaming) = 0;
	virtual bool is_accept_data() const = 0;
	virtual void set_accept_data(bool p_accept) = 0;
	virtual t_size get_chunk_count() const = 0;
	virtual t_size get_sample_count() const = 0;
	virtual t_samplespec get_spec() const = 0;
	virtual bool is_spec_change() const = 0;
	virtual void reset_spec_change() = 0;
	virtual const dsd_chunk_t& get_first_chunk() const = 0;
	virtual void remove_first_chunk() = 0;
	virtual t_size read(t_uint8* p_buffer, t_size p_samples) = 0;
	virtual void write(const t_uint8* p_buffer, t_size p_samples, t_size p_channels, unsigned p_samplerate, unsigned p_channel_config) = 0;
	virtual void flush() = 0;
	FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(dsd_stream_service);
};

#endif
