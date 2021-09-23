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

#ifndef _DSD_STREAM_SERVICE_IMPL_H_INCLUDED
#define _DSD_STREAM_SERVICE_IMPL_H_INCLUDED

#include <mutex>
#include "dsd_stream_service.h"

using std::mutex;
using std::lock_guard;

class dsd_stream_service_impl : public dsd_stream_service {
	mutable mutex m_chunks_mutex;
	dsd_chunk_t m_null_chunk;
	chain_list_v2_t<dsd_chunk_t> m_chunks;
	t_size m_chunk_read_samples;
	t_size m_samples;
	t_samplespec m_last_read_spec;
	int m_streaming;
	bool m_accept_data;
public:
	dsd_stream_service_impl();
	virtual ~dsd_stream_service_impl();
	virtual bool is_streaming() const;
	virtual void set_streaming(bool p_streaming);
	virtual bool is_accept_data() const;
	virtual void set_accept_data(bool p_accept);
	virtual t_size get_chunk_count() const;
	virtual t_size get_sample_count() const;
	virtual t_samplespec get_spec() const;
	virtual bool is_spec_change() const;
	virtual void reset_spec_change();
	virtual const dsd_chunk_t& get_first_chunk() const;
	virtual void remove_first_chunk();
	virtual t_size read(t_uint8* p_buffer, t_size p_samples);
	virtual void write(const t_uint8* p_buffer, t_size p_samples, t_size p_channels, unsigned p_samplerate, unsigned p_channel_config);
	virtual void flush();
};

#endif
