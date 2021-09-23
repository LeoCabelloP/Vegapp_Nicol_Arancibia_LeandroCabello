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

#ifndef _DOP_CONVERTER_H_INCLUDED
#define _DOP_CONVERTER_H_INCLUDED

#include "sacd_config.h"

class dop_converter_t {
	t_samplespec     m_inp_spec;
	t_samplespec     m_out_spec;
	array_t<t_size>  m_dop_marker_n;
	array_t<t_uint8> m_dop_data;
public:
	dop_converter_t();
	void set_inp_spec(const t_samplespec& p_spec);
	void set_out_spec(const t_samplespec& p_spec);
	void dsd_to_dop(const t_uint8* p_inp_data, t_size p_inp_samples, audio_chunk& p_out_chunk);
};

#endif
