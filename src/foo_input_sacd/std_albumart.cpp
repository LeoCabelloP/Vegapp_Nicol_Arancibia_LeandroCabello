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

#include "std_albumart.h"

#pragma pack(1)

struct RiffChunk {
	uint8_t  id[4];
	uint32_t length;
};

struct RiffHeader {
	RiffChunk ck;
	uint8_t   type[4];
	RiffHeader() : ck{ {'R', 'I', 'F', 'F'} , 0 }, type{ 'W', 'A', 'V', 'E' } {}
};

struct WaveHeaderEx {
	RiffChunk     ck;
	PCMWAVEFORMAT wfx;
	WaveHeaderEx() : ck{ {'f', 'm', 't', ' '}, sizeof(WaveHeaderEx) }, wfx{ WAVE_FORMAT_PCM } {}
};

struct RiffData {
	RiffChunk ck;
	RiffData() : ck{ {'d', 'a', 't', 'a'} , 0 } {}
};

struct RiffId3 {
	RiffChunk ck;
	RiffId3() : ck{ {'i', 'd', '3', ' '} , 0 } {}
};

#pragma pack(1)

void std_albumart_instance_t::create(t_input_open_reason p_reason, abort_callback& p_abort) {
	filesystem::g_open_tempmem(stream_ptr, p_abort);
	RiffHeader riff;
	stream_ptr->write(&riff, sizeof(riff), p_abort);
	WaveHeaderEx wave;
	auto bps = 16;
	auto& wfx = wave.wfx;
	wfx.wf.wFormatTag = WAVE_FORMAT_PCM;
	wfx.wf.nChannels = 2;
	wfx.wf.nSamplesPerSec = 44100;
	wfx.wf.nAvgBytesPerSec = wfx.wf.nSamplesPerSec * wfx.wf.nChannels * (bps / 8);
	wfx.wf.nBlockAlign = wfx.wf.nChannels * (bps / 8);
	wfx.wBitsPerSample = bps;
	stream_ptr->write(&wave, sizeof(wave), p_abort);
	RiffData data;
	stream_ptr->write(&data, sizeof(data), p_abort);
	stream_ptr->seek(0, p_abort);
}

void std_albumart_instance_t::instantiate(file_ptr p_filehint, const char* p_path, t_input_open_reason p_reason, abort_callback& p_abort) {
	switch (p_reason) {
	case input_open_info_read:
		{
			service_ptr_t<album_art_extractor> std_service;
			album_art_extractor::g_get_interface(std_service, stream_ext);
			if (std_service.is_valid()) {
				extractor_ptr = std_service->open(stream_ptr, stream_ext, p_abort);
			}
		}
		break;
		case input_open_info_write:
		{
			service_ptr_t<album_art_editor> std_service;
			album_art_editor::g_get_interface(std_service, stream_ext);
			if (std_service.is_valid()) {
				editor_ptr = std_service->open(stream_ptr, stream_ext, p_abort);
			}
		}
		break;
	}
}

void std_albumart_instance_t::load(const vector<uint8_t>& p_data, abort_callback& p_abort) {
	stream_ptr->seek(stream_ptr->get_size(p_abort), p_abort);
	RiffId3 id3;
	id3.ck.length = p_data.size();
	stream_ptr->write(&id3, sizeof(id3), p_abort);
	stream_ptr->write(p_data.data(), p_data.size(), p_abort);
	stream_ptr->seek(0, p_abort);
	RiffHeader riff;
	riff.ck.length = uint32_t(stream_ptr->get_size(p_abort)) - 8;
	stream_ptr->write(&riff, sizeof(riff), p_abort);
	stream_ptr->seek(0, p_abort);
}

void std_albumart_instance_t::save(vector<uint8_t>& p_data, abort_callback& p_abort) {
	stream_ptr->seek(0, p_abort);
	RiffHeader riff;
	stream_ptr->read(&riff, sizeof(riff), p_abort);
	if (*(uint32_t*)riff.ck.id != *(uint32_t*)(RiffHeader().ck.id)) {
		return;
	}
	while (!stream_ptr->is_eof(p_abort)) {
		RiffChunk chunk;
		stream_ptr->read(&chunk, sizeof(chunk), p_abort);
		if (*(uint32_t*)chunk.id == *(uint32_t*)(RiffId3().ck.id)) {
			p_data.resize(chunk.length);
			stream_ptr->read(p_data.data(), p_data.size(), p_abort);
			break;
		}
		stream_ptr->skip(chunk.length, p_abort);
	}
	stream_ptr->seek(0, p_abort);
}

album_art_data_ptr std_albumart_instance_t::query(const GUID& p_what, abort_callback& p_abort) {
	return extractor_ptr->query(p_what, p_abort);
}

void std_albumart_instance_t::set(const GUID& p_what, album_art_data_ptr p_data, abort_callback& p_abort) {
	editor_ptr->set(p_what, p_data, p_abort);
}

void std_albumart_instance_t::remove(const GUID& p_what) {
	editor_ptr->remove(p_what);
}

void std_albumart_instance_t::commit(abort_callback& p_abort) {
	editor_ptr->commit(p_abort);
}

void std_albumart_instance_t::dump(const char* name) {
	vector<uint8_t> buffer;
	abort_callback_impl abort;
	auto pos = stream_ptr->get_position(abort);
	stream_ptr->seek(0, abort);
	buffer.resize((size_t)stream_ptr->get_size(abort));
	stream_ptr->read(buffer.data(), buffer.size(), abort);
	service_ptr_t<file> out;
	filesystem::g_open_write_new(out, name, abort);
	out->write(buffer.data(), buffer.size(), abort);
	stream_ptr->seek(pos, abort);
}
