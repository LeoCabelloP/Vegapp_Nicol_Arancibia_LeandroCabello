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

#include "sacd_core.h"
#include "dop_converter.h"
#include "dsd_stream_service.h"
#include "dst_decoder_mt.h"
#include "DSDPCMConverterEngine.h"
#include "std_wavpack.h"
#include <psapi.h>

constexpr int UPDATE_STATS_MS = 500;
constexpr int BITRATE_AVGS = 16;
constexpr audio_sample PCM_OVERLOAD_THRESHOLD = 1.0f;

enum {
	input_flag_dsd_extract = 1 << 16,
};

void console_printf(const char* fmt, ...) {
	va_list vl;
	va_start(vl, fmt);
	console::printfv(fmt, vl);
	va_end(vl);
}

void console_vprintf(const char* fmt, va_list vl) {
	console::printfv(fmt, vl);
}

int get_sacd_channel_map_from_loudspeaker_config(int loudspeaker_config) {
	int sacd_channel_map;
	switch (loudspeaker_config) {
	case 0:
		sacd_channel_map = audio_chunk::channel_front_left | audio_chunk::channel_front_right;
		break;
	case 1:
		sacd_channel_map = audio_chunk::channel_front_left | audio_chunk::channel_front_right | audio_chunk::channel_back_left | audio_chunk::channel_back_right;
		break;
	case 2:
		sacd_channel_map = audio_chunk::channel_front_left | audio_chunk::channel_front_right | audio_chunk::channel_front_center | audio_chunk::channel_lfe;
		break;
	case 3:
		sacd_channel_map = audio_chunk::channel_front_left | audio_chunk::channel_front_right | audio_chunk::channel_front_center | audio_chunk::channel_back_left | audio_chunk::channel_back_right;
		break;
	case 4:
		sacd_channel_map = audio_chunk::channel_front_left | audio_chunk::channel_front_right | audio_chunk::channel_front_center | audio_chunk::channel_lfe | audio_chunk::channel_back_left | audio_chunk::channel_back_right;
		break;
	case 5:
		sacd_channel_map = audio_chunk::channel_front_center;
		break;
	case 6:
		sacd_channel_map = audio_chunk::channel_front_left | audio_chunk::channel_front_right | audio_chunk::channel_front_center;
		break;
	default:
		sacd_channel_map = 0;
		break;
	}
	return sacd_channel_map;
}

int get_sacd_channel_map_from_channels(int channels) {
	int sacd_channel_map;
	switch (channels) {
	case 1:
		sacd_channel_map = audio_chunk::channel_front_center;
		break;
	case 2:
		sacd_channel_map = audio_chunk::channel_front_left | audio_chunk::channel_front_right;
		break;
	case 3:
		sacd_channel_map = audio_chunk::channel_front_left | audio_chunk::channel_front_right | audio_chunk::channel_front_center;
		break;
	case 4:
		sacd_channel_map = audio_chunk::channel_front_left | audio_chunk::channel_front_right | audio_chunk::channel_back_left | audio_chunk::channel_back_right;
		break;
	case 5:
		sacd_channel_map = audio_chunk::channel_front_left | audio_chunk::channel_front_right | audio_chunk::channel_front_center | audio_chunk::channel_back_left | audio_chunk::channel_back_right;
		break;
	case 6:
		sacd_channel_map = audio_chunk::channel_front_left | audio_chunk::channel_front_right | audio_chunk::channel_front_center | audio_chunk::channel_lfe | audio_chunk::channel_back_left | audio_chunk::channel_back_right;
		break;
	default:
		sacd_channel_map = audio_chunk::g_guess_channel_config(channels);
		break;
	}
	return sacd_channel_map;
}

conv_type_e get_converter_type() {
	auto conv_type = conv_type_e::DSDPCM_CONV_MULTISTAGE;
	switch (CSACDPreferences::get_converter_mode()) {
	case 0:
	case 1:
		conv_type = conv_type_e::DSDPCM_CONV_MULTISTAGE;
		break;
	case 2:
	case 3:
		conv_type = conv_type_e::DSDPCM_CONV_DIRECT;
		break;
	case 4:
	case 5:
		conv_type = conv_type_e::DSDPCM_CONV_USER;
		break;
	}
	return conv_type;
}

bool get_converter_fp64() {
	auto conv_fp64 = false;
	switch (CSACDPreferences::get_converter_mode()) {
	case 1:
	case 3:
	case 5:
		conv_fp64 = true;
		break;
	}
	return conv_fp64;
}

void adjust_replaygain(file_info& info, float dB_volume_adjust) {
	auto rg_info = info.get_replaygain();
	auto scale_adjust = audio_math::gain_to_scale(-dB_volume_adjust);
	if (rg_info.is_album_gain_present()) {
		rg_info.m_album_gain += dB_volume_adjust;
	}
	if (rg_info.is_album_peak_present()) {
		rg_info.m_album_peak *= scale_adjust;
	}
	if (rg_info.is_track_gain_present()) {
		rg_info.m_track_gain += dB_volume_adjust;
	}
	if (rg_info.is_track_peak_present()) {
		rg_info.m_track_peak *= scale_adjust;
	}
	info.set_replaygain(rg_info);
}

void fix_pcm_stream(bool is_end, audio_sample* m_pcm_data, int pcm_samples, int pcm_channels) {
	if (!is_end) {
		if (pcm_samples > 1) {
			for (int ch = 0; ch < pcm_channels; ch++) {
				m_pcm_data[0 * pcm_channels + ch] = m_pcm_data[1 * pcm_channels + ch];
			}
		}
	}
	else {
		if (pcm_samples > 1) {
			for (int ch = 0; ch < pcm_channels; ch++) {
				m_pcm_data[(pcm_samples - 1) * pcm_channels + ch] = m_pcm_data[(pcm_samples - 2) * pcm_channels + ch];
			}
		}
	}
}

int get_cpu_cores() {
	SYSTEM_INFO sysinfo;
	GetSystemInfo(&sysinfo);
	return sysinfo.dwNumberOfProcessors;
}

bool call_from_foo_converter(abort_callback& p_abort) {
	MODULEINFO module_info;
	memset(&module_info, 0, sizeof(module_info));
	if (GetModuleInformation(GetCurrentProcess(), GetModuleHandle(_T("foo_converter.dll")), &module_info, sizeof(module_info))) {
		BYTE* module_vtbl = reinterpret_cast<BYTE**>(&p_abort)[0];
		DWORD module_size = (DWORD)module_info.SizeOfImage;
		BYTE* module_base = (BYTE*)module_info.lpBaseOfDll;
		if (module_vtbl >= module_base && module_vtbl <= module_base + module_size) {
			return true;
		}
	}
	return false;
}

unique_ptr<DSDPCMConverterEngine> g_dsdpcm_playback;

class input_sacd_t : public sacd_core_t, public input_stubs {
	t_input_open_reason    open_reason;
	area_id_e              playable_area;
	unsigned               initialize_flags;
	int64_t                sacd_bitrate[BITRATE_AVGS];
	int                    sacd_bitrate_idx;
	int64_t                sacd_bitrate_sum;
	int64_t                sacd_read_bytes;
	array_t<uint8_t>       dsd_buf;
	size_t                 dsd_buf_size;
	array_t<uint8_t>       dst_buf;
	size_t                 dst_buf_size;
	array_t<audio_sample>  pcm_buf;
	int                    dst_threads;

	unique_ptr<dst_decoder_t>         dst_decoder;
	unique_ptr<DSDPCMConverterEngine> dsdpcm_convert;
	DSDPCMConverterEngine*            dsdpcm_decoder = nullptr;

	float                  dB_volume_adjust;
	float                  lfe_adjust_coef;
	bool                   log_overloads;
	string8                log_track_name;
	uint32_t               info_update_time_ms;
	int                    pcm_out_channels;
	unsigned int           pcm_out_channel_map;
	int                    pcm_out_samplerate;
	int                    pcm_out_bits_per_sample;
	int                    pcm_out_max_samples;
	float                  pcm_out_delay;
	int                    pcm_out_append_samples;
	int                    pcm_out_remove_samples;
	uint64_t               pcm_out_offset;
	int                    pcm_min_samplerate;
	
	bool                   use_dsd_path;
	bool                   use_pcm_path;
	bool                   use_dop_for_pcm;
	dop_converter_t        dop_converter;

	int                                  dsd_samplerate;
	static_api_ptr_t<dsd_stream_service> dsd_stream;
	
	int                    framerate;
	bool                   read_frame;

	service_ptr_t<std_wavpack_input_t> std_wavpack_input;
public:
	input_sacd_t() {
		open_reason = input_open_info_read;
		access_mode = ACCESS_MODE_NULL;
		dB_volume_adjust = 0;
		lfe_adjust_coef = 1.0f;
		info_update_time_ms = 0;
		use_dsd_path = false;
		use_pcm_path = true;
	}

	virtual ~input_sacd_t() {
		if (use_dsd_path) {
			dsd_stream->set_streaming(false);
		}
	}

	void open(file_ptr p_filehint, const char* p_path, t_input_open_reason p_reason, abort_callback& p_abort) {
		open_reason = p_reason;
		sacd_core_t::open(p_filehint, p_path, p_reason, p_abort);
		dB_volume_adjust = CSACDPreferences::get_volume_adjust();
		lfe_adjust_coef = pow(10.0f, CSACDPreferences::get_lfe_adjust() / 20.0f);
		log_overloads = CSACDPreferences::get_log_overloads();
		pcm_out_samplerate = CSACDPreferences::get_samplerate();
		pcm_out_bits_per_sample = 24;
		if ((media_type == media_type_e::WAVPACK) && CSACDPreferences::get_std_tags()) {
			switch (p_reason) {
			case input_open_info_read:
			case input_open_info_write:
				std_wavpack_input = new service_impl_t<std_wavpack_input_t>(sacd_media.get()->get_handle(), p_path, p_reason, p_abort);
				break;
			}
		}
	}

	t_uint32 get_subsong_count() {
		t_uint32 track_count = 0;
		switch (CSACDPreferences::get_area()) {
		case AREA_TWOCH:
			track_count = sacd_reader->get_track_count(AREA_TWOCH);
			if (track_count == 0) {
				sacd_reader->set_mode(access_mode | AREA_MULCH);
				track_count = sacd_reader->get_track_count(AREA_MULCH);
			}
			break;
		case AREA_MULCH:
			track_count = sacd_reader->get_track_count(AREA_MULCH);
			if (track_count == 0) {
				sacd_reader->set_mode(access_mode | AREA_TWOCH);
				track_count = sacd_reader->get_track_count(AREA_TWOCH);
			}
			break;
		default:
			track_count = sacd_reader->get_track_count(AREA_TWOCH) + sacd_reader->get_track_count(AREA_MULCH);
			break;
		}
		if ((media_type == media_type_e::WAVPACK) && CSACDPreferences::get_std_tags()) {
			if (std_wavpack_input.is_valid()) {
				track_count = std_wavpack_input->get_subsong_count();
			}
		}
		return track_count;
	}

	t_uint32 get_subsong(t_uint32 p_index) {
		auto subsong = sacd_reader->get_track_number(p_index);
		if ((media_type == media_type_e::WAVPACK) && CSACDPreferences::get_std_tags()) {
			if (std_wavpack_input.is_valid()) {
				subsong = std_wavpack_input->get_subsong(p_index);
			}
		}
		return subsong;
	}

	void get_info(t_uint32 p_subsong, file_info& p_info, abort_callback& p_abort) {
		sacd_reader->get_info(p_subsong, p_info);
		if (sacd_metabase) {
			sacd_metabase->get_track_info(p_subsong, p_info);
		}
		if ((media_type == media_type_e::WAVPACK) && CSACDPreferences::get_std_tags()) {
			if (std_wavpack_input.is_valid()) {
				std_wavpack_input->get_info(p_subsong, p_info, p_abort);
			}
		}
		p_info.set_length(sacd_reader->get_duration(p_subsong));
		p_info.info_set_int("channels", sacd_reader->get_channels(p_subsong));
		p_info.info_set_bitrate(((t_int64)(sacd_reader->get_samplerate(p_subsong) * sacd_reader->get_channels(p_subsong)) + 500) / 1000);
		if (open_reason == input_open_decode) {
			p_info.info_set_int("samplerate", use_dsd_path ? sacd_reader->get_samplerate() : pcm_out_samplerate);
			p_info.info_set_int("bitspersample", (use_dsd_path && !call_from_foo_converter(p_abort)) ? 1 : pcm_out_bits_per_sample);
		}
		else {
			p_info.info_set_int("samplerate", sacd_reader->get_samplerate());
			p_info.info_set_int("bitspersample", 1);
		}
		string_formatter(codec);
		codec << (sacd_reader->is_dst(p_subsong) ? "DST" : "DSD");
		codec << sacd_reader->get_samplerate(p_subsong) / 44100;
		p_info.info_set("codec", codec);
		p_info.info_set("encoding", "lossless");
		adjust_replaygain(p_info, -dB_volume_adjust);
		if (log_overloads) {
			auto track_title = p_info.meta_get("title", 0);
			log_track_name = track_title ? track_title : "Untitled";
		}
	}

	t_filestats get_file_stats(abort_callback& p_abort) {
		return sacd_media->get_stats();
	}

	void decode_initialize(t_uint32 p_subsong, unsigned p_flags, abort_callback& p_abort) {
		initialize_flags = p_flags;
		if (!sacd_reader->select_track(p_subsong)) {
			throw exception_io();
		}
		dsd_samplerate = sacd_reader->get_samplerate(p_subsong);
		framerate = sacd_reader->get_framerate(p_subsong);
		pcm_out_channels = sacd_reader->get_channels(p_subsong);
		dst_threads = get_cpu_cores();
		dst_buf_size = dsd_buf_size = dsd_samplerate / 8 / framerate * pcm_out_channels;
		dsd_buf.set_size(dst_threads * dsd_buf_size);
		dst_buf.set_size(dst_threads * dst_buf_size);
		pcm_out_channel_map = get_sacd_channel_map_from_loudspeaker_config(sacd_reader->get_loudspeaker_config(p_subsong));
		if (pcm_out_channel_map == 0) {
			pcm_out_channel_map = get_sacd_channel_map_from_channels(pcm_out_channels);
		}
		pcm_min_samplerate = 44100;
		while ((pcm_min_samplerate / framerate) * framerate != pcm_min_samplerate) {
			pcm_min_samplerate *= 2;
		}
		pcm_out_samplerate = max(pcm_min_samplerate, pcm_out_samplerate);
		pcm_out_max_samples = pcm_out_samplerate / framerate;
		pcm_buf.set_size(pcm_out_channels * pcm_out_max_samples);
		memset(sacd_bitrate, 0, sizeof(sacd_bitrate));
		sacd_bitrate_idx = 0;
		sacd_bitrate_sum = 0;
		sacd_read_bytes = 0;
		double* fir_data = nullptr;
		int fir_size = 0;
		if (get_converter_type() == conv_type_e::DSDPCM_CONV_USER) {
			fir_data = CSACDPreferences::get_user_fir().get_ptr();
			fir_size = CSACDPreferences::get_user_fir().get_size();
		}
		if (initialize_flags & input_flag_playback) {
			dsdpcm_decoder = g_dsdpcm_playback.get();
			use_dsd_path = CSACDPreferences::use_dsd_path();
			use_pcm_path = CSACDPreferences::use_pcm_path();
			use_dop_for_pcm = false;
		}
		else {
			dsdpcm_convert = make_unique<DSDPCMConverterEngine>();
			dsdpcm_decoder = dsdpcm_convert.get();
			use_dsd_path = false;
			use_pcm_path = true;
			use_dop_for_pcm = CSACDPreferences::get_dop_for_converter() && call_from_foo_converter(p_abort);
			use_dop_for_pcm = use_dop_for_pcm || (initialize_flags & input_flag_dsd_extract);
		}
		if (use_dsd_path) {
			dsd_stream->set_streaming(true);
		}
		if (use_pcm_path) {
			if (!use_dop_for_pcm) {
				dsdpcm_decoder->set_gain(dB_volume_adjust);
				int rv = dsdpcm_decoder->init(pcm_out_channels, framerate, dsd_samplerate, pcm_out_samplerate, get_converter_type(), get_converter_fp64(), fir_data, fir_size);
				if (rv < 0) {
					if (rv == -2) {
						popup_message::g_show("No installed FIR, continue with the default", "DSD2PCM", popup_message::icon_error);
					}
					int rv = dsdpcm_decoder->init(pcm_out_channels, framerate, dsd_samplerate, pcm_out_samplerate, conv_type_e::DSDPCM_CONV_DIRECT, get_converter_fp64(), nullptr, 0);
					if (rv < 0) {
						throw exception_io();
					}
				}
			}
			else {
				t_samplespec spec;
				spec.m_channels = pcm_out_channels;
				spec.m_channel_config = pcm_out_channel_map;
				spec.m_sample_rate = dsd_samplerate;
				dop_converter.set_inp_spec(spec);
				spec.m_sample_rate = dsd_samplerate / 16;
				dop_converter.set_out_spec(spec);
			}
		}
		pcm_out_delay = 0.0f;
		if (!(initialize_flags & input_flag_playback)) {
			pcm_out_delay = dsdpcm_decoder->get_delay();
		}
		auto pcm_out_delay_in_samples = (pcm_out_delay > 0.0) ? (int)(pcm_out_delay + 0.5f) : 0;
		if (pcm_out_delay_in_samples > pcm_out_max_samples - 1) {
			pcm_out_delay_in_samples = pcm_out_max_samples - 1;
		}
		pcm_out_append_samples = pcm_out_delay_in_samples;
		pcm_out_remove_samples = pcm_out_delay_in_samples;
		read_frame = true;
	}

	bool decode_run_internal(audio_chunk& p_chunk, mem_block_container* p_raw, abort_callback& p_abort) {
		uint8_t* dsd_data = nullptr;
		size_t dsd_size = 0;
		while (read_frame) {
			auto slot_nr = dst_decoder ? dst_decoder->get_slot_nr() : 0;
			dsd_data = dsd_buf.get_ptr() + dsd_buf_size * slot_nr;
			dsd_size = 0;
			uint8_t* frame_data = dst_buf.get_ptr() + dst_buf_size * slot_nr;
			size_t frame_size = dst_buf_size;
			frame_type_e frame_type;
			read_frame = sacd_reader->read_frame(frame_data, &frame_size, &frame_type);
			if (read_frame) {
				switch (frame_type) {
				case frame_type_e::DSD:
					dsd_data = frame_data;
					dsd_size = frame_size;
					break;
				case frame_type_e::DST:
					if (!dst_decoder) {
						dst_decoder = make_unique<dst_decoder_t>(dst_threads);
						if (!dst_decoder || dst_decoder->init(sacd_reader->get_channels(), sacd_reader->get_samplerate(), sacd_reader->get_framerate()) != 0) {
							return false;
						}
					}
					dst_decoder->decode(frame_data, frame_size, &dsd_data, &dsd_size);
					break;
				}
				sacd_bitrate_idx = (++sacd_bitrate_idx) % BITRATE_AVGS;
				sacd_bitrate_sum -= sacd_bitrate[sacd_bitrate_idx];
				sacd_bitrate[sacd_bitrate_idx] = (int64_t)8 * frame_size * framerate;
				sacd_bitrate_sum += sacd_bitrate[sacd_bitrate_idx];
			}
			if (dsd_size) {
				break;
			}
		}
		if (!dsd_size) {
 			if (dst_decoder) {
				dst_decoder->decode(nullptr, 0, &dsd_data, &dsd_size);
			}
		}
		if (dsd_size) {
			if (p_raw) {
				p_raw->set_size(dsd_size);
				memcpy(p_raw->get_ptr(), dsd_data, dsd_size);
			}
			if (use_dsd_path) {
				dsd_stream->write(dsd_data, dsd_size / pcm_out_channels, pcm_out_channels, dsd_samplerate, pcm_out_channel_map);
			}
			if (use_pcm_path) {
				if (use_dop_for_pcm) {
					dop_converter.dsd_to_dop(dsd_data, dsd_size / pcm_out_channels, p_chunk);
				}
				else {
					auto pcm_out_samples = dsdpcm_decoder->convert(dsd_data, dsd_size, pcm_buf.get_ptr()) / pcm_out_channels;
					if (pcm_out_remove_samples > 0) {
						fix_pcm_stream(false, pcm_buf.get_ptr() + pcm_out_channels * pcm_out_remove_samples, pcm_out_samples - pcm_out_remove_samples, pcm_out_channels);
					}
					adjust_lfe(pcm_buf.get_ptr() + pcm_out_channels * pcm_out_remove_samples, pcm_out_samples - pcm_out_remove_samples, pcm_out_channels, pcm_out_channel_map);
					p_chunk.set_data(pcm_buf.get_ptr() + pcm_out_channels * pcm_out_remove_samples, pcm_out_samples - pcm_out_remove_samples, pcm_out_channels, pcm_out_samplerate, pcm_out_channel_map);
					check_overloads(pcm_buf.get_ptr() + pcm_out_channels * pcm_out_remove_samples, pcm_out_samples - pcm_out_remove_samples);
					pcm_out_offset -= pcm_out_remove_samples;
					pcm_out_offset += pcm_out_samples;
					pcm_out_remove_samples = 0;
				}
			}
			else {
				p_chunk.set_sample_rate(pcm_min_samplerate);
				p_chunk.set_channels(pcm_out_channels, pcm_out_channel_map);
				p_chunk.set_silence(pcm_min_samplerate * (dsd_size / pcm_out_channels) / (dsd_samplerate / 8));
			}
		}
		else {
			if (pcm_out_append_samples > 0) {
				dsdpcm_decoder->convert(nullptr, 0, pcm_buf.get_ptr());
				fix_pcm_stream(true, pcm_buf.get_ptr(), pcm_out_append_samples, pcm_out_channels);
				adjust_lfe(pcm_buf.get_ptr(), pcm_out_append_samples, pcm_out_channels, pcm_out_channel_map);
				p_chunk.set_data(pcm_buf.get_ptr(), pcm_out_append_samples, pcm_out_channels, pcm_out_samplerate, pcm_out_channel_map);
				check_overloads(pcm_buf.get_ptr(), pcm_out_append_samples);
				pcm_out_append_samples = 0;
			}
			else {
				p_chunk.set_sample_count(0);
			}
		}
		sacd_read_bytes += dsd_size;
		return p_chunk.get_sample_count() > 0;
	}

	const char* get_time_stamp(double t) {
		static char ts[13];
		int fraction = (int)((t - floor(t)) * 1000);
		int second = (int)floor(t);
		int minute = second / 60;
		int hour = minute / 60;
		ts[ 0] = '0' + (hour / 10) % 6;
		ts[ 1] = '0' + hour % 10;
		ts[ 2] = ':';
		ts[ 3] = '0' + (minute / 10) % 6;
		ts[ 4] = '0' + minute % 10;
		ts[ 5] = '.';
		ts[ 6] = '0' + (second / 10) % 6;
		ts[ 7] = '0' + second % 10;
		ts[ 8] = '.';
		ts[ 9] = '0' + (fraction / 100) % 10;
		ts[10] = '0' + (fraction / 10) % 10;
		ts[11] = '0' + fraction % 10;
		ts[12] = '\0';
		return ts;
	}

	void adjust_lfe(audio_sample* pcm_data, t_size pcm_samples, unsigned channels, unsigned channel_config) {
		if ((channels >= 4) && (channel_config & audio_chunk::channel_lfe) && (lfe_adjust_coef != 1.0f)) {
			for (t_size sample = 0; sample < pcm_samples; sample++) {
				pcm_data[sample * channels + 3] *= lfe_adjust_coef;
			}
		}
	}

	void check_overloads(const audio_sample* pcm_data, t_size pcm_samples) {
		if (log_overloads) {
			for (t_size sample = 0; sample < pcm_samples; sample++) {
				for (int ch = 0; ch < pcm_out_channels; ch++) {
					audio_sample v = pcm_data[sample * pcm_out_channels + ch];
					if ((v > +PCM_OVERLOAD_THRESHOLD) || (v < -PCM_OVERLOAD_THRESHOLD)) {
						double overload_time = (double)(pcm_out_offset + sample) / (double)pcm_out_samplerate;
						const char* track_name = log_track_name;
						const char* time_stamp = get_time_stamp(overload_time);
						console_printf("Overload at '%s' ch:%d [%s]", track_name, ch, time_stamp);
						break;
					}
				}
			}
		}
	}

	bool decode_run(audio_chunk& p_chunk, abort_callback& p_abort) {
		return decode_run_internal(p_chunk, nullptr, p_abort);
	}

	bool decode_run_raw(audio_chunk& p_chunk, mem_block_container& p_raw, abort_callback& p_abort) {
		return decode_run_internal(p_chunk, &p_raw, p_abort);
	}

	void decode_seek(double p_seconds, abort_callback& p_abort) {
		if (!sacd_reader->seek(p_seconds)) {
			throw exception_io();
		}
	}

	bool decode_can_seek() {
		return sacd_media->can_seek();
	}

	bool decode_get_dynamic_info(file_info& p_info, double& p_timestamp_delta) {
		DWORD curr_time_ms = GetTickCount();
		if (info_update_time_ms + (DWORD)UPDATE_STATS_MS >= curr_time_ms) {
			return false;
		}
		info_update_time_ms = curr_time_ms;
		p_info.info_set_bitrate_vbr((sacd_bitrate_sum / BITRATE_AVGS + 500) / 1000);
		return true;
	}
	
	bool decode_get_dynamic_info_track(file_info& p_info, double& p_timestamp_delta) {
		return false;
	}
	
	void decode_on_idle(abort_callback & p_abort) {
		sacd_media->on_idle();
	}

	void retag_set_info(t_uint32 p_subsong, const file_info& p_info, abort_callback& p_abort) {
		file_info_impl info = p_info;
		adjust_replaygain(info, +dB_volume_adjust);
		if (CSACDPreferences::get_editable_tags()) {
			if ((media_type == media_type_e::WAVPACK) && CSACDPreferences::get_std_tags()) {
				if (std_wavpack_input.is_valid() && (get_subsong_count() == 1)) {
					std_wavpack_input->set_info(p_subsong, info, p_abort);
				}
			}
			else {
				sacd_reader->set_info(p_subsong, info);
			}
			if (sacd_metabase) {
				sacd_metabase->set_track_info(p_subsong, info, false);
				if (CSACDPreferences::get_linked_tags()) {
					t_uint32 twoch_count = sacd_reader->get_track_count(ACCESS_MODE_TWOCH);
					t_uint32 mulch_count = sacd_reader->get_track_count(ACCESS_MODE_MULCH);
					if (twoch_count == mulch_count) {
						sacd_metabase->set_track_info((p_subsong <= twoch_count) ? p_subsong + twoch_count : p_subsong - twoch_count, info, true);
					}
				}
			}
		}
	}
	
	void retag_commit(abort_callback& p_abort) {
		if (CSACDPreferences::get_editable_tags()) {
			if ((media_type == media_type_e::WAVPACK) && CSACDPreferences::get_std_tags()) {
				if (std_wavpack_input.is_valid() && (get_subsong_count() == 1)) {
					std_wavpack_input->commit(p_abort);
				}
			}
			else {
				sacd_reader->commit();
			}
			if (sacd_metabase) {
				sacd_metabase->commit();
			}
		}
	}

	void set_logger(event_logger::ptr p_ptr) {
	}

	size_t extended_param(const GUID& p_type, size_t p_arg1, void* p_arg2, size_t p_arg2size) {
		return 0;
	}

	static const char* g_get_name() {
		return SACD_NAME;
	}

	static const GUID g_get_guid() {
		return SACD_GUID;
	}

	static const GUID g_get_preferences_guid() {
		return CSACDPreferences::class_guid;
	}
};

static input_factory_t<input_sacd_t> g_input_sacd_factory;

class initquit_sacd_t : public initquit {
public:
	virtual void on_init() {
#ifdef _DEBUG
		_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_CHECK_ALWAYS_DF | _CRTDBG_CHECK_CRT_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
		g_dsdpcm_playback = make_unique<DSDPCMConverterEngine>();
	}
	
	virtual void on_quit() {
		g_dsdpcm_playback->free();
	}
};

static initquit_factory_t<initquit_sacd_t> g_initquit_sacd_factory;

DECLARE_COMPONENT_VERSION(SACD_NAME, SACD_VERSION, SACD_COPYRIGHT);
DECLARE_FILE_TYPE("SACD files", "*.DAT;*.DFF;*.DSF;*.ISO;MASTER1.TOC");
VALIDATE_COMPONENT_FILENAME(SACD_FILE);
