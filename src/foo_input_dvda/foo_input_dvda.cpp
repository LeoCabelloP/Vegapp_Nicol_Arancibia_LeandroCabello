/*
* DVD-Audio Decoder plugin
* Copyright (c) 2009-2020 Maxim V.Anisiutkin <maxim.anisiutkin@gmail.com>
*
* DVD-Audio Decoder is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation; either
* version 2.1 of the License, or (at your option) any later version.
*
* DVD-Audio Decoder is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with FFmpeg; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/

#include <foobar2000.h>
#include "resource.h"

#include "audio_stream.h"
#include "audio_track.h"
#include "dvda_decipher.h"
#include "dvda_disc_cache.h"
#include "stream_buffer.h"
#include "log_trunk.h"
#include "dvdawm.h"
#include "playback_handler.h"
#include "dvda_version.h"
#include "dvda_setup.h"

enum class media_type_e { UNK_TYPE = 0, IFO_TYPE = 1, ISO_TYPE = 2, MLP_TYPE = 3, AOB_TYPE = 4 };

static constexpr uint32_t START_OF_MASTER_TOC = 510;
static constexpr uint32_t UPDATE_STATS_MS = 500;
static constexpr double   SHORT_TRACK_SEC = 3.0;

static dvda_disc_cache_t g_dvda_disc_cache;
static bool              g_no_untagged_tracks;
static bool              g_watermarked;

void console_printf(const char* fmt, ...) {
	va_list vl;
	va_start(vl, fmt);
	console::printfv(fmt, vl);
	va_end(vl);
}

void console_vprintf(const char* fmt, va_list vl) {
	console::printfv(fmt, vl);
}

class input_dvda_t : public input_stubs {
	friend class dvda_input_pcm;
	dvda_filesystem_t*            dvda_filesystem;
	dvda_zone_t*                  dvda_zone;
	track_list_t*                 track_list;
	dvda_metabase_t*              dvda_metabase;
	stream_buffer_t<uint8_t, int> track_stream;
	t_filesize                    track_stream_bytes_aob;
	t_filesize                    track_stream_bytes_ps1;
	media_file_t                  media_file;
	media_type_e                  media_type;
	CPxMContext                   aob_cpxm_context;
	audio_stream_t*               audio_stream;
	audio_track_t                 audio_track;
	t_filesize                    stream_size;
	double                        stream_duration;
	int                           stream_titleset;
	sub_header_t                  stream_ps1_info;
	uint32_t                      stream_block_current;
	bool                          stream_needs_reinit;
	bool                          major_sync_0;
	byte_array_t                  pcm_out_buffer;
	t_size                        pcm_out_buffer_size;
	unsigned                      pcm_out_channel_map;
	DWORD                         info_update_time_ms;
	char*                         codec;
public:
	int                           pcm_out_channels;
	int                           pcm_out_samplerate;
	int                           pcm_out_bits;
	input_dvda_t() {
		audio_stream = nullptr;
		track_stream_bytes_aob = track_stream_bytes_ps1 = 0;
		info_update_time_ms = 0;
		my_av_log_set_callback(foo_av_log_callback);
		g_dvda_disc_cache.add_ref();
	}

	virtual ~input_dvda_t() {
		if (audio_stream) {
			delete audio_stream;
		}
		if (media_type == media_type_e::MLP_TYPE) {
			track_list->clear();
			delete track_list;
		}
		my_av_log_set_default_callback();
		g_dvda_disc_cache.release();
	}

	void open(media_file_t p_filehint, const char* p_path, t_input_open_reason p_reason, abort_callback& p_abort) {
		media_file = p_filehint;
		input_open_file_helper(media_file, p_path, p_reason == input_open_info_write ? input_open_info_read : p_reason, p_abort);
		pfc::string_filename_ext file_name_ext(p_path);
		pfc::string_extension file_ext(p_path);
		pcm_out_buffer_size = 192000;
		pcm_out_buffer.set_size(pcm_out_buffer_size);
		media_type = media_type_e::UNK_TYPE;
		if (stricmp_utf8(file_name_ext, "AUDIO_TS.IFO") == 0) {
			media_type = media_type_e::IFO_TYPE;
		}
		if (stricmp_utf8(file_ext, "ISO") == 0) {
			media_type = media_type_e::ISO_TYPE;
		}
		if (stricmp_utf8(file_ext, "MLP") == 0 || stricmp_utf8(file_ext, "TRUEHD") == 0) {
			media_type = media_type_e::MLP_TYPE;
		}
		if (stricmp_utf8(file_ext, "AOB") == 0) {
			media_type = media_type_e::AOB_TYPE;
		}
		if (media_type == media_type_e::IFO_TYPE || media_type == media_type_e::ISO_TYPE) {
			if (g_no_untagged_tracks != CDVDAPreferences::get_no_untagged_tracks()) {
				g_no_untagged_tracks = CDVDAPreferences::get_no_untagged_tracks();
				g_dvda_disc_cache.uncache(media_file->get_stats(p_abort));
			}
			else {
				if (g_dvda_disc_cache.cached(media_file->get_stats(p_abort), &dvda_filesystem, &dvda_zone, &track_list, &dvda_metabase)) {
					return;
				}
			}
		}
		dvda_filesystem = nullptr;
		dvda_zone = nullptr;
		track_list = new track_list_t;
		if (!track_list) {
			throw exception_io_unsupported_format();
		}
		dvda_metabase = nullptr;
		if (media_type == media_type_e::IFO_TYPE || media_type == media_type_e::ISO_TYPE) {
			console_printf("Reading DVD-Audio manager data");
			if (media_type == media_type_e::IFO_TYPE) {
				dvda_filesystem = new dir_dvda_filesystem_t;
				if (!dvda_filesystem) {
					throw exception_io_unsupported_format();
				}
				pfc::string_directory zone_dir(p_path);
				if (!dvda_filesystem->mount(zone_dir)) {
					throw exception_io_unsupported_format();
				}
			}
			if (media_type == media_type_e::ISO_TYPE) {
				dvda_filesystem = new iso_dvda_filesystem_t;
				if (!dvda_filesystem) {
					throw exception_io_unsupported_format();
				}
				if (!dvda_filesystem->mount(p_path)) {
					throw exception_io_unsupported_format();
				}
			}
			dvda_zone = new dvda_zone_t;
			if (!dvda_zone) {
				throw exception_io_unsupported_format();
			}
			if (!dvda_zone->open(dvda_filesystem)) {
				throw exception_io_unsupported_format();
			}
			if (!(dvda_zone->get_titlesets() > 0)) {
				throw exception_io_unsupported_format();
			}
			if (media_type == media_type_e::IFO_TYPE) {
				int prot = dvda_zone->get_cpxm_context()->media_type;
				if (prot >= 0) {
					console_printf("Media protection type: %s", prot == 0 ? "None" : prot == 1 ? "CSS/CPPM" : prot == 2 ? "CPRM" : "Unknown");
				}
			}
			const char* metafile_path = nullptr;
			string_replace_extension metafile_name(p_path, "xml");
			if (media_type == media_type_e::ISO_TYPE && CDVDAPreferences::get_store_tags_with_iso()) {
				metafile_path = metafile_name;
			}
			dvda_metabase = new dvda_metabase_t(dvda_filesystem, metafile_path);
			auto threshold_time = CDVDAPreferences::get_no_short_tracks() ? SHORT_TRACK_SEC : 0.0;
			track_list->init(dvda_zone, false, (chmode_t)CDVDAPreferences::get_chmode(), threshold_time, dvda_metabase, CDVDAPreferences::get_no_untagged_tracks());
			if (!CDVDAPreferences::get_no_downmixes()) {
				track_list->init(dvda_zone, true, (chmode_t)CDVDAPreferences::get_chmode(), threshold_time, dvda_metabase, CDVDAPreferences::get_no_untagged_tracks());
			}
			dvda_disc_t dvda_disc;
			dvda_disc.dvda_stats = media_file->get_stats(p_abort);
			dvda_disc.dvda_filesystem = dvda_filesystem;
			dvda_disc.dvda_zone = dvda_zone;
			dvda_disc.track_list = track_list;
			dvda_disc.dvda_metabase = dvda_metabase;
			g_dvda_disc_cache.add(dvda_disc);
		}
		if (media_type == media_type_e::MLP_TYPE) {
			media_file->reopen(p_abort);
			byte_array_t stream_buffer;
			stream_buffer.set_size(audio_stream_t::MAX_CHUNK_SIZE);
			int bytes_written = media_file->read(stream_buffer.get_ptr(), stream_buffer.get_size(), p_abort);
			audio_stream = new mlp_audio_stream_t;
			if (!audio_stream) {
				throw exception_io();
			}
			audio_stream_info_t* p_audio_stream_info = audio_stream->get_info(stream_buffer.get_ptr(), bytes_written);
			if (!p_audio_stream_info) {
				throw exception_io_unsupported_format();
			}
			audio_track_t audio_track;
			audio_track.track_number      = 1;
			audio_track.block_first       = 0;
			audio_track.block_last        = 0;
			audio_track.duration          = audio_math::samples_to_time(8 * media_file->get_size(p_abort), p_audio_stream_info->bitrate) * p_audio_stream_info->estimate_compression();
			audio_track.audio_stream_info = *p_audio_stream_info;
			audio_track.track_downmix     = false;
			audio_track.track_index       = track_list_t::get_track_index(0, 0, 0, audio_track.track_downmix);
			track_list->add(audio_track);
			if (!CDVDAPreferences::get_no_downmixes() && !(CDVDAPreferences::get_no_short_tracks() && audio_track.duration < SHORT_TRACK_SEC)) {
				if (audio_track.audio_stream_info.can_downmix) {
					audio_track.track_number    = 2;
					audio_track.track_downmix   = true;
					audio_track.track_index     = track_list_t::get_track_index(0, 0, 0, audio_track.track_downmix);
					track_list->add(audio_track);
				}
			}
		}
		if (media_type == media_type_e::AOB_TYPE) {
			aob_cpxm_context.media_type = -1;
			t_size drive = pfc::string_find_first(p_path, "://");
			string_ansi_from_utf8 a_file_name(drive != ~0 ? &p_path[drive + 3] : p_path);
			dvdcpxm_init(&aob_cpxm_context, (char*)a_file_name.get_ptr());
			media_file->reopen(p_abort);
			byte_array_t stream_buffer;
			stream_buffer.set_size(audio_stream_t::MAX_CHUNK_SIZE);
			int bytes_written = 0;
			sub_header_t ps1_info;
			int aob_bytes_written = media_file->read(stream_buffer.get_ptr(), stream_buffer.get_size(), p_abort);
			if (aob_cpxm_context.media_type > 0) {
				dvdcpxm_decrypt(&aob_cpxm_context, stream_buffer.get_ptr(), aob_bytes_written / DVD_BLOCK_SIZE, DVDCPXM_RESET_CCI);
			}
			dvda_block_t::get_ps1(stream_buffer.get_ptr(), aob_bytes_written / DVD_BLOCK_SIZE, stream_buffer.get_ptr(), &bytes_written, &ps1_info);
			audio_stream_info_t* p_audio_stream_info = NULL;
			switch (ps1_info.header.stream_id) {
			case MLP_STREAM_ID:
				audio_stream = new mlp_audio_stream_t;
				if (audio_stream) {
					p_audio_stream_info = audio_stream->get_info(stream_buffer.get_ptr(), bytes_written);
				}
				break;
			case PCM_STREAM_ID:
				audio_stream = new pcm_audio_stream_t;
				if (audio_stream) {
					p_audio_stream_info = audio_stream->get_info((uint8_t*)&ps1_info.extra_header, ps1_info.header.extra_header_length);
				}
				break;
			default:
				throw exception_io_unsupported_format();
				break;
			}
			if (!p_audio_stream_info) {
				throw exception_io_unsupported_format();
			}
			audio_track_t audio_track;
			audio_track.track_number      = 1;
			audio_track.block_first       = 0;
			audio_track.block_last        = media_file->get_size(p_abort) / DVD_BLOCK_SIZE - 1;
			audio_track.duration          = audio_math::samples_to_time(8 * media_file->get_size(p_abort), p_audio_stream_info->bitrate) * p_audio_stream_info->estimate_compression();
			audio_track.audio_stream_info = *p_audio_stream_info;
			audio_track.track_downmix     = false;
			audio_track.track_index       = track_list_t::get_track_index(0, 0, 0, audio_track.track_downmix);
			track_list->add(audio_track);
			if (!CDVDAPreferences::get_no_downmixes() && !(CDVDAPreferences::get_no_short_tracks() && audio_track.duration < SHORT_TRACK_SEC)) {
				audio_track.track_number      = 2;
				audio_track.track_downmix     = true;
				audio_track.track_index       = track_list_t::get_track_index(0, 0, 0, audio_track.track_downmix);
				track_list->add(audio_track);
			}
		}
		delete audio_stream;
		audio_stream = nullptr;
	}

	t_uint32 get_subsong_count() {
		return (t_uint32)track_list->size();
	}

	t_uint32 get_subsong(t_uint32 p_index) {
		return (t_uint32)(*track_list)[p_index].track_index;
	}

	void get_info(t_uint32 p_subsong, file_info& p_info, abort_callback& p_abort) {
		int i = track_list->get_track_index(p_subsong);
		if (i < 0) {
			console_printf("Track list error: entry %d does not exist", p_subsong);
			throw exception_io();
		}
		audio_track_t& audio_track = (*track_list)[i];
		p_info.set_length(audio_track.duration);
		p_info.info_set_int("samplerate", audio_track.audio_stream_info.group1_samplerate);
		p_info.info_set_int("channels", audio_track.audio_stream_info.group1_channels + audio_track.audio_stream_info.group2_channels);
		p_info.info_set_int("bitspersample", audio_track.audio_stream_info.group1_bits);
		codec = (audio_track.audio_stream_info.stream_id == MLP_STREAM_ID ? (audio_track.audio_stream_info.stream_type == STREAM_TYPE_MLP ? "MLP" : "TrueHD") : "PCM"); 
		p_info.info_set("codec", codec);
		p_info.info_set("encoding", "lossless");
		p_info.info_set_bitrate((audio_track.audio_stream_info.bitrate + 500) / 1000);
		pfc::string_formatter(track);
		track << audio_track.track_number;
		p_info.meta_set("tracknumber", track);
		pfc::string_formatter(title);
		for (int i = 0; i < audio_track.audio_stream_info.group1_channels; i++) {
			if (i > 0) {
				title << "-";
			}
			title << audio_track.audio_stream_info.get_channel_name(i);
		}
		title << " ";
		title << audio_track.audio_stream_info.group1_bits;
		title << "/";
		title << audio_track.audio_stream_info.group1_samplerate;
		if (audio_track.audio_stream_info.group2_channels > 0) {
			title << " + ";
			for (int i = 0; i < audio_track.audio_stream_info.group2_channels; i++) {
				if (i > 0) {
					title << "-";
				}
				title << audio_track.audio_stream_info.get_channel_name(audio_track.audio_stream_info.group1_channels + i);
			}
			title << " ";
			title << audio_track.audio_stream_info.group2_bits;
			title << "/";
			title << audio_track.audio_stream_info.group2_samplerate;
		}
		p_info.meta_set("title", title);
		pfc::string_formatter(album);
		if (dvda_filesystem) {
			char disc_name[256];
			if (dvda_filesystem->get_name(disc_name)) {
				album << disc_name;
				p_info.meta_set("album", album);
			}
		}
		if (media_type == media_type_e::IFO_TYPE || media_type == media_type_e::ISO_TYPE) {
			pfc::string_formatter(dvda_titleset);
			dvda_titleset << audio_track.dvda_titleset;
			p_info.meta_set("dvda_titleset", dvda_titleset);
			pfc::string_formatter(dvda_title);
			dvda_title << audio_track.dvda_title;
			p_info.meta_set("dvda_title", dvda_title);
			pfc::string_formatter(dvda_track);
			dvda_track << audio_track.dvda_track;
			p_info.meta_set("dvda_track", dvda_track);
			if (dvda_metabase) {
				dvda_metabase->get_track_info(p_subsong, p_info);
			}
		}
		if (audio_track.track_downmix) {
			pfc::string_formatter(title);
			title << p_info.meta_get("title", 0);
			title << " (stereo downmix)";
			p_info.meta_set("title", title);
		}
	}

	t_filestats get_file_stats(abort_callback& p_abort) {
		return media_file->get_stats(p_abort);
	}

	void decode_initialize(t_uint32 p_subsong, unsigned p_flags, abort_callback& p_abort) {
		int i = track_list->get_track_index(p_subsong);
		if (i < 0) {
			console_printf("Track list error: entry %d does not exist", p_subsong);
			throw exception_io();
		}
		audio_track = (*track_list)[i];
		track_stream.init(512 * DVD_BLOCK_SIZE, 4 * DVD_BLOCK_SIZE, 1 * DVD_BLOCK_SIZE);
		switch (media_type) {
		case media_type_e::IFO_TYPE:
		case media_type_e::ISO_TYPE:
			stream_titleset = ((p_subsong >> 16) & 0xff) - 1;
			stream_block_current = audio_track.block_first;
			stream_size = (audio_track.block_last + 1 - audio_track.block_first) * DVD_BLOCK_SIZE;
			stream_ps1_info.header.stream_id = UNK_STREAM_ID;
			break;
		case media_type_e::MLP_TYPE:
			media_file->reopen(p_abort);
			stream_size = media_file->get_size(p_abort);
			stream_ps1_info.header.stream_id = MLP_STREAM_ID;
			break;
		case media_type_e::AOB_TYPE:
			media_file->reopen(p_abort);
			stream_block_current = audio_track.block_first;
			stream_size = (audio_track.block_last + 1 - audio_track.block_first) * DVD_BLOCK_SIZE;
			stream_ps1_info.header.stream_id = UNK_STREAM_ID;
			break;
		}
		stream_duration = audio_track.duration;
		stream_needs_reinit = false;
		major_sync_0 = false;
		g_watermarked = false;
	}

	bool decode_run(audio_chunk_t& p_chunk, abort_callback& p_abort) {
		decode_run_read_stream_start:
		if (track_stream.is_ready_to_write() && !stream_needs_reinit) {
			stream_buffer_read(p_abort);
		}
		int data_size, bytes_decoded = 0;
		data_size = pcm_out_buffer.get_size();
		bytes_decoded = (audio_stream ? audio_stream->decode(pcm_out_buffer.get_ptr(), &data_size, track_stream.get_read_ptr(), track_stream.get_read_size()) : 0);
		if (bytes_decoded <= 0) {
			track_stream.move_read_ptr(0);
			if (bytes_decoded == audio_stream_t::RETCODE_EXCEPT) {
				console_printf("Error: Exception occured in DVD-Audio Decoder");
				return false;
			}
			bool decoder_needs_reinit = (bytes_decoded == audio_stream_t::RETCODE_REINIT);
			if (decoder_needs_reinit) {
				if (audio_stream) {
					delete audio_stream;
					audio_stream = nullptr;
				}
				console_printf("Reinitializing DVD-Audio Decoder: MLP/TrueHD");
				goto decode_run_read_stream_start;
			}
			if (track_stream.get_read_size() == 0) {
				if (stream_needs_reinit) {
					stream_needs_reinit = false;
					if (audio_stream) {
						delete audio_stream;
						audio_stream = nullptr;
					}
					stream_ps1_info.header.stream_id = UNK_STREAM_ID;
					console_printf("Reinitializing DVD-Audio Decoder: PCM");
					goto decode_run_read_stream_start;
				}
				else {
					return false;
				}
			}
			if (audio_stream) {
				int major_sync = audio_stream->resync(track_stream.get_read_ptr(), track_stream.get_read_size());
				if (major_sync == 0) {
					if (major_sync_0) {
						if (track_stream.get_read_size() > 4) {
							major_sync = audio_stream->resync(track_stream.get_read_ptr() + 1, track_stream.get_read_size() - 1);
						}
					}
					else {
						major_sync_0 = true;
					}
				}
				if (major_sync < 0) {
					if (stream_needs_reinit) {
						major_sync = track_stream.get_read_size();
					}
					else {
						major_sync = track_stream.get_read_size() > 4 ? track_stream.get_read_size() - 4 : 0;
					}
					if (major_sync <= 0) {
						return false;
					}
				}
				if (major_sync > 0) {
					track_stream.move_read_ptr(major_sync);
					console_printf("Error: DVD-Audio Decoder is out of sync: %d bytes skipped", major_sync);
				}
				goto decode_run_read_stream_start;
			}
			else {
				audio_stream = create_audio_stream(stream_ps1_info, track_stream.get_read_ptr(), track_stream.get_read_size(), audio_track.track_downmix);
				if (audio_stream) {
					if ((media_type == media_type_e::IFO_TYPE || media_type == media_type_e::ISO_TYPE) && audio_stream->get_downmix()) {
						audio_stream->set_downmix_coef(audio_track.LR_dmx_coef);
					}
					audio_stream->set_check(media_type == media_type_e::AOB_TYPE);
					track_stream.move_read_ptr(audio_stream->sync_offset);
				}
				else {
					track_stream.move_read_ptr(DVD_BLOCK_SIZE);
					stream_ps1_info.header.stream_id = UNK_STREAM_ID;
					console_printf("Error: DVD-Audio Decoder initialization failed");
				}
				goto decode_run_read_stream_start;
			}
			return false;
		}
		major_sync_0 = false;
		track_stream.move_read_ptr(bytes_decoded);
		p_chunk.set_data_fixedpoint(pcm_out_buffer.get_ptr(), data_size, pcm_out_samplerate, pcm_out_channels, pcm_out_bits, pcm_out_channel_map);
		return true;
	}

	void decode_seek(double p_seconds, abort_callback& p_abort) {
		track_stream.reinit();
		if (audio_stream) {
			delete audio_stream;
			audio_stream = nullptr;
		}
		switch (media_type) {
		case media_type_e::IFO_TYPE:
		case media_type_e::ISO_TYPE:
			{
				uint32_t offset = (uint32_t)((p_seconds / (audio_track.duration + 1.0)) * (double)(audio_track.block_last + 1 - audio_track.block_first));
				if (offset > audio_track.block_last - audio_track.block_first - 1) {
					offset = audio_track.block_last - audio_track.block_first - 1;
				}
				stream_block_current = audio_track.block_first + offset;
				stream_ps1_info.header.stream_id = UNK_STREAM_ID;
			}
			break;
		case media_type_e::MLP_TYPE:
			{
				media_file->ensure_seekable();
				t_filesize seek_position = (t_filesize)(p_seconds / (stream_duration + 1.0) * (double)stream_size);
				media_file->seek(seek_position, p_abort);
			}
			break;
		case media_type_e::AOB_TYPE:
			{
				media_file->ensure_seekable();
				uint32_t offset = (uint32_t)((p_seconds / (audio_track.duration + 1.0)) * (double)(audio_track.block_last + 1 - audio_track.block_first));
				if (offset > audio_track.block_last - audio_track.block_first - 1) {
					offset = audio_track.block_last - audio_track.block_first - 1;
				}
				stream_block_current = audio_track.block_first + offset;
				stream_ps1_info.header.stream_id = UNK_STREAM_ID;
				media_file->seek(stream_block_current * DVD_BLOCK_SIZE, p_abort);
			}
			break;
		}
		g_watermarked = false;
	}

	bool decode_can_seek() {
		switch (media_type) {
		case media_type_e::IFO_TYPE:
		case media_type_e::ISO_TYPE:
			return true;
		case media_type_e::MLP_TYPE:
		case media_type_e::AOB_TYPE:
			return media_file->can_seek();
		}
		return false;
	}

	bool decode_get_dynamic_info(file_info& p_info, double& p_timestamp_delta) {
		DWORD curr_time_ms = GetTickCount();
		if (info_update_time_ms + (DWORD)UPDATE_STATS_MS >= curr_time_ms) {
			return false;
		}
		info_update_time_ms = curr_time_ms;
		if (!audio_stream) {
			return false;
		}
		if (media_type == media_type_e::MLP_TYPE) {
			stream_duration = audio_math::samples_to_time(8 * stream_size, audio_stream->bitrate) * audio_stream->get_compression();
			p_info.set_length(stream_duration);
		}
		if (media_type == media_type_e::AOB_TYPE) {
			p_info.info_set_int("samplerate", audio_stream->group1_samplerate);
			p_info.info_set_int("channels", audio_stream->group1_channels + audio_stream->group2_channels);
			p_info.info_set_int("bitspersample", audio_stream->group1_bits);
			codec = (audio_stream->get_stream_id() == MLP_STREAM_ID ? "MLP" : "PCM"); 
			p_info.info_set_bitrate((audio_stream->bitrate + 500) / 1000);
			p_info.info_set_bitrate_vbr((audio_stream->bitrate + 500) / 1000);
			double stream_ratio = track_stream_bytes_aob > 0 ? (double)track_stream_bytes_ps1 / (double)track_stream_bytes_aob : 1.0;
			stream_duration = audio_math::samples_to_time(8 * stream_size * stream_ratio, audio_stream->bitrate) * audio_stream->get_compression();
			p_info.set_length(stream_duration);
		}
		if (audio_stream->is_vbr) {
			p_info.info_set_bitrate_vbr(((t_int64)audio_stream->get_instant_bitrate() + 500) / 1000);
		}
		if (audio_stream->get_downmix()) {
			p_info.info_set_int("channels", 2);
		}
		if (g_watermarked) {
			pfc::string_formatter(codec_wm);
			codec_wm << codec;
			codec_wm << "/Watermarked";
			p_info.info_set("codec", codec_wm);
		}
		return true;
	}
	
	bool decode_get_dynamic_info_track(file_info& p_info, double& p_timestamp_delta) {
		return false;
	}
	
	void decode_on_idle(abort_callback & p_abort) {
		media_file->on_idle(p_abort);
	}

	void retag_set_info(t_uint32 p_subsong, const file_info& p_info, abort_callback& p_abort) {
		if (dvda_metabase) {
			dvda_metabase->set_track_info(p_subsong, p_info);
		}
	}
	
	void retag_commit(abort_callback& p_abort)	{
		g_dvda_disc_cache.uncache(media_file->get_stats(p_abort));
		if (dvda_metabase) {
			dvda_metabase->commit();
		}
	}

	static const char* g_get_name() {
		return DVDA_NAME;
	}

	static const GUID g_get_guid() {
		return DVDA_GUID;
	}

	static const GUID g_get_preferences_guid() {
		return CDVDAPreferences::class_guid;
	}

	static bool g_is_our_content_type(const char* p_content_type) {
		return false;
	}
	
	static bool g_is_our_path(const char* p_path, const char* p_extension) {
		if (stricmp_utf8(p_extension, "ISO") == 0) {
			abort_callback_impl abort;
			service_ptr_t<file> file;
			char sacdmtoc[8];
			try {
				filesystem::g_open_read(file, p_path, abort);
				file->seek(START_OF_MASTER_TOC * DVD_BLOCK_SIZE, abort);
				if (file->read(sacdmtoc, 8, abort) == 8) {
					if (memcmp(sacdmtoc, "SACDMTOC", 8) == 0) {
						return false;
					}
				}
			}
			catch (...) {
				return false;
			}
		}
		return
			stricmp_utf8(p_extension, "AOB") == 0 ||
			stricmp_utf8(p_extension, "IFO") == 0 ||
			stricmp_utf8(p_extension, "ISO") == 0 ||
			stricmp_utf8(p_extension, "MLP") == 0 ||
			stricmp_utf8(p_extension, "TRUEHD") == 0;
	}

private:
	audio_stream_t* create_audio_stream(sub_header_t& p_ps1_info, uint8_t* p_buf, int p_buf_size, bool p_downmix) {
		audio_stream_t* audio_stream;
		int init_code = -1;
		switch (p_ps1_info.header.stream_id) {
		case MLP_STREAM_ID:
			audio_stream = new mlp_audio_stream_t;
			if (audio_stream) {
				init_code = audio_stream->init(p_buf, p_buf_size, p_downmix);
			}
			break;
		case PCM_STREAM_ID:
			audio_stream = new pcm_audio_stream_t;
			if (audio_stream) {
				init_code = audio_stream->init((uint8_t*)&p_ps1_info.extra_header, p_ps1_info.header.extra_header_length, p_downmix);
			}
			break;
		default:
			audio_stream = nullptr;
			break;
		}
		if (!audio_stream) {
			return nullptr;
		}
		if (init_code < 0) {
			console_printf("Error: DVD-Audio Decoder cannot initialize audio stream");
			delete audio_stream;
			return nullptr;
		}
		pcm_out_samplerate = audio_stream->group1_samplerate;
		pcm_out_bits = audio_stream->group1_bits > 16 ? 32 : 16;
		if (audio_stream->get_downmix()) {
			pcm_out_channels = 2;
			pcm_out_channel_map = audio_chunk_t::g_guess_channel_config(pcm_out_channels);
		}
		else {
			pcm_out_channels = audio_stream->group1_channels + audio_stream->group2_channels;
			pcm_out_channel_map = audio_chunk_t::g_channel_config_from_wfx(audio_stream->get_wfx_channels());
		}
		return audio_stream;
	}

	void stream_buffer_read(abort_callback& p_abort) {
		sub_header_t ps1_info;
		int blocks_to_read, bytes_written = 0;
		switch (media_type) {
		case media_type_e::IFO_TYPE:
		case media_type_e::ISO_TYPE:
			blocks_to_read = track_stream.get_write_size() / DVD_BLOCK_SIZE;
			if (stream_block_current <= audio_track.block_last) {
				if (stream_block_current + blocks_to_read > audio_track.block_last + 1) {
					blocks_to_read = audio_track.block_last + 1 - stream_block_current;
				}
				int blocks_read = dvda_zone->get_blocks(stream_titleset, stream_block_current, blocks_to_read, track_stream.get_write_ptr());
				dvda_block_t::get_ps1(track_stream.get_write_ptr(), blocks_read, track_stream.get_write_ptr(), &bytes_written, &ps1_info);
				if (stream_ps1_info.header.stream_id == UNK_STREAM_ID) {
					stream_ps1_info = ps1_info;
				}
				track_stream.move_write_ptr(bytes_written);
				if (blocks_read < blocks_to_read) {
					console_printf("Error: DVD-Audio Decoder cannot read track data: titleset = %d, block_number = %d, blocks_to_read = %d", stream_titleset, stream_block_current + blocks_read, blocks_to_read - blocks_read);
				}
				stream_block_current += blocks_to_read;
				if (stream_block_current > audio_track.block_last) {
					int blocks_after_last = dvda_zone->get_titleset(stream_titleset)->get_last() - audio_track.block_last;
					int blocks_to_sync = blocks_after_last < 8 ? blocks_after_last : 8;
					if (stream_block_current <= audio_track.block_last + blocks_to_sync) {
						if (stream_block_current + blocks_to_read > audio_track.block_last + 1 + blocks_to_sync) {
							blocks_to_read = audio_track.block_last + 1 + blocks_to_sync - stream_block_current;
						}
						int blocks_read = dvda_zone->get_blocks(stream_titleset, stream_block_current, blocks_to_read, track_stream.get_write_ptr());
						bytes_written = 0;
						dvda_block_t::get_ps1(track_stream.get_write_ptr(), blocks_read, track_stream.get_write_ptr(), &bytes_written, NULL);
						if (audio_stream) {
							int major_sync = audio_stream->resync(track_stream.get_write_ptr(), bytes_written);
							if (major_sync > 0) {
								track_stream.move_write_ptr(major_sync);
							}
						}
						if (blocks_read < blocks_to_read) {
							console_printf("Error: DVD-Audio Decoder cannot read track tail: titleset = %d, block_number = %d, blocks_to_read = %d", stream_titleset, stream_block_current + blocks_read, blocks_to_read - blocks_read);
						}
						stream_block_current += blocks_to_read;
					}
				}
			}
			break;
		case media_type_e::MLP_TYPE:
			bytes_written = media_file->read(track_stream.get_write_ptr(), track_stream.get_write_size(), p_abort);
			track_stream.move_write_ptr(bytes_written);
			break;
		case media_type_e::AOB_TYPE:
			blocks_to_read = track_stream.get_write_size() / DVD_BLOCK_SIZE;
			if (stream_block_current <= audio_track.block_last) {
				if (stream_block_current + blocks_to_read > audio_track.block_last + 1) {
					blocks_to_read = audio_track.block_last + 1 - stream_block_current;
				}
				int aob_bytes_written = media_file->read(track_stream.get_write_ptr(), blocks_to_read * DVD_BLOCK_SIZE, p_abort);
				int blocks_read = aob_bytes_written / DVD_BLOCK_SIZE;
				if (aob_cpxm_context.media_type > 0) {
					dvdcpxm_decrypt(&aob_cpxm_context, track_stream.get_write_ptr(), blocks_read, DVDCPXM_RESET_CCI);
				}
				for (int block = 0; block < blocks_read; block++) {
					int block_bytes_written = 0;
					ps1_info.header.stream_id = UNK_STREAM_ID;
					dvda_block_t::get_ps1(track_stream.get_write_ptr() + block * DVD_BLOCK_SIZE, track_stream.get_write_ptr() + bytes_written, &block_bytes_written, &ps1_info);
					if (ps1_info.header.stream_id != UNK_STREAM_ID) {
						if (stream_ps1_info.header.stream_id == UNK_STREAM_ID) {
							stream_ps1_info = ps1_info;
						}
						if (
							ps1_info.header.stream_id != stream_ps1_info.header.stream_id || (
								ps1_info.header.stream_id == PCM_STREAM_ID && stream_ps1_info.header.stream_id == PCM_STREAM_ID && (
									stream_ps1_info.extra_header.pcm.channel_assignment != ps1_info.extra_header.pcm.channel_assignment ||
									stream_ps1_info.extra_header.pcm.group1_bits != ps1_info.extra_header.pcm.group1_bits ||
									stream_ps1_info.extra_header.pcm.group1_samplerate != ps1_info.extra_header.pcm.group1_samplerate ||
									stream_ps1_info.extra_header.pcm.group2_bits != ps1_info.extra_header.pcm.group2_bits ||
									stream_ps1_info.extra_header.pcm.group2_samplerate != ps1_info.extra_header.pcm.group2_samplerate
								)
							)
						) {
							blocks_read = block;
							if (media_file->can_seek()) {
								media_file->seek((stream_block_current + blocks_read) * DVD_BLOCK_SIZE, p_abort);
							}
							stream_needs_reinit = true;
							break;
						}
						bytes_written += block_bytes_written;
					}
				}
				track_stream_bytes_aob += aob_bytes_written;
				track_stream_bytes_ps1 += bytes_written;
				track_stream.move_write_ptr(bytes_written);
				stream_block_current += blocks_read;
			}
			break;
		}
	}
};

static input_factory_t<input_dvda_t> g_input_dvda_factory;

class dvda_decryption_thread_t {
	class directory_dvda_callback_t : public directory_callback_impl {
	public:
		list_t<string8> dir_files;
		directory_dvda_callback_t(bool p_recur) : directory_callback_impl(p_recur) {
			dir_files.remove_all();
		}

		bool on_entry(filesystem* owner, abort_callback& p_abort, const char* url, bool is_subdirectory, const t_filestats& p_stats) {
			if (!is_subdirectory) {
				dir_files.add_item(url);
			}
			return true;
		}
	};
	CPxMContext cpxm_context;
	string8 source_dir;
	string8 target_dir;
	HANDLE h_thread;
	HWND hwnd_main;
	HWND hwnd_filename;
	HWND hwnd_progress;
	volatile bool running;
public:
	dvda_decryption_thread_t() {
		running = false;
		hwnd_main = NULL;
	}

	~dvda_decryption_thread_t() {
		if (running) {
			TerminateThread(h_thread, -1);
		}
		if (hwnd_main) {
			DestroyWindow(hwnd_main);
		}
	}

	bool run(const char* p_source_dir, const char* p_target_dir) {
		if (running) {
			popup_message::g_show("Previous decryption operation is still in progress, wait for completion", "Decrypt", popup_message::icon_information);
			return false;
		}
		source_dir = p_source_dir;
		target_dir = p_target_dir;
		dvda_remove_protection_window(core_api::get_my_instance(), core_api::get_main_window(), &hwnd_main, &hwnd_filename, &hwnd_progress);
		h_thread = CreateThread(NULL, 0, run_thread, (LPVOID)this, 0, NULL);
		if (h_thread == NULL) {
			console_printf("Error: DVD-Audio Decoder cannot create decryption thread");
		}
		return h_thread != NULL;
	}

private:
	static DWORD WINAPI run_thread(LPVOID p_parameter) {
		int status = 0;
		abort_callback_impl abort;
		dvda_decryption_thread_t* p_thread = (dvda_decryption_thread_t*)p_parameter;
		p_thread->running = true;
		p_thread->cpxm_context.media_type = -1;
		t_size drive = p_thread->source_dir.find_first("://");
		if (drive < p_thread->source_dir.get_length() - 3) {
			string_ansi_from_utf8 a_file_name(&p_thread->source_dir[drive + 3]);
			dvdcpxm_init(&p_thread->cpxm_context, (char*)a_file_name.get_ptr());
		}
		if (p_thread->cpxm_context.media_type > 0) {
			console_printf("Decrypting DVD-Audio files");
			directory_dvda_callback_t cb(false)	;
			filesystem::g_list_directory(p_thread->target_dir, cb, abort);
			for (t_size i = 0; i < cb.dir_files.get_count(); i++) {
				const char* fname = cb.dir_files[i];
				string_filename_ext file_name_ext(fname);
				if ((stricmp_utf8(file_name_ext, "DVDAUDIO.MKB") == 0 || stricmp_utf8(file_name_ext, "DVDAUDIO.BUP") == 0)) {
					try {
						filesystem::g_remove(fname, abort);
					}
					catch(...) {
					}
					continue;
				}
				string_extension file_ext(fname);
				if (stricmp_utf8(file_ext, "AOB") == 0 || stricmp_utf8(file_ext, "VOB") == 0) {
					t_size drive = string_find_first(fname, "://");
					if (drive < strlen_utf8(fname) - 3) {
						const char* file_name = fname + drive + 3;
						console_printf("Decrypting %s", file_name);
						status = dvda_remove_protection_from_file(p_thread->cpxm_context, string_os_from_utf8(file_name), p_thread->hwnd_main, p_thread->hwnd_filename, p_thread->hwnd_progress);
						if (status < 0) {
							popup_message::g_show("Decryption failed", "Decrypt", popup_message::icon_error);
						}
					}
				}
			}
			console_printf("DVD-Audio files decrypted");
			popup_message::g_show("DVD-Audio files decrypted", "Decrypt", popup_message::icon_information);
		}
		if (p_thread->hwnd_main) {
			DestroyWindow(p_thread->hwnd_main);
			p_thread->hwnd_main = NULL;
		}
		p_thread->running = false;
		return status;
	}
};

static dvda_decryption_thread_t g_dec_thread;

class dvda_file_operation_t : public file_operation_callback {
	void decrypt(const list_base_const_t<const char*>& p_from, const list_base_const_t<const char*>& p_to) {
		if (p_from.get_count() > 0) {
			string_directory source_dir(p_from[0]);
			string_directory target_dir(p_to[0]);
			g_dec_thread.run(source_dir, target_dir);
		}
	}

	void on_files_deleted_sorted(const list_base_const_t<const char*>& p_items) {
	}

	void on_files_moved_sorted(const list_base_const_t<const char*>& p_from, const list_base_const_t<const char*>& p_to) {
		decrypt(p_from, p_to);
	}

	void on_files_copied_sorted(const list_base_const_t<const char*>& p_from, const list_base_const_t<const char*>& p_to) {
		decrypt(p_from, p_to);
	}
};

static service_factory_t<dvda_file_operation_t>           g_dvda_file_operation_factory;
static play_callback_static_factory_t<playback_handler_t> g_playback_handler_factory;

class dsp_watermark_detector_t : public dsp_impl_base {
	int m_initialized;
	int m_channels;
	int m_samplerate;
	bool m_watermarked;
	bool m_track_ended;
	bool m_track_changed;
	dvdawm_t m_dvdawm;
public:
	dsp_watermark_detector_t() {
		m_initialized = 0;
		m_channels = 0;
		m_samplerate = 0;
		m_watermarked = false;
		m_track_ended = false;
		m_track_changed = false;
	}

	~dsp_watermark_detector_t() {
		m_dvdawm.free();
	}

	static GUID g_get_guid() {
		return DSP_WM_GUID;
	}

	static void g_get_name(pfc::string_base& p_out) {
		p_out = DSP_WM_NAME;
	}

	virtual void on_endoftrack(abort_callback& p_abort) {
		m_track_ended = true;
		m_track_changed = true;
	}

	virtual void on_endofplayback(abort_callback& p_abort) {
		m_track_ended = true;
		m_track_changed = true;
	}

	virtual bool on_chunk(audio_chunk_t* p_chunk, abort_callback& p_abort) {
		if (!(m_channels == p_chunk->get_channels() && m_samplerate == p_chunk->get_sample_rate())) {
			m_channels = p_chunk->get_channels();
			m_samplerate = p_chunk->get_sample_rate();
			if (m_initialized == DVDAWM_INITIALIZED_OK) {
				m_dvdawm.free();
				m_initialized = DVDAWM_NOT_INITIALIZED;
				g_watermarked = false;
			}
		}
		if (m_initialized == DVDAWM_NOT_INITIALIZED) {
			console_printf("Initializing DVD-Audio Watermark Detector: %dch %dHz", m_channels, m_samplerate);
			m_initialized = m_dvdawm.init(m_channels, m_samplerate);
			if (!check_init_code(m_initialized)) {
				return false;
			}
			print_source();
			m_track_changed = true;
		}
		if (m_initialized == DVDAWM_INITIALIZED_OK) {
			if (m_track_ended) {
				print_source();
				m_dvdawm.reset();
				g_watermarked = false;
				m_dvdawm.retime(get_latency());
				m_track_ended = false;
			}
			double seek_time;
			if (g_playback_handler_factory.get_static_instance().get_last_seek_and_reset(seek_time)) {
				m_dvdawm.reset();
				g_watermarked = false;
				m_dvdawm.retime(seek_time);
			}
			if (m_track_changed) {
				metadb_handle_ptr p_metadb;
				if (get_cur_file(p_metadb)) {
					file_info_impl p_info;
					const char* track = 0;
					if (p_metadb->get_info(p_info)) {
						track = p_info.meta_get("tracknumber", 0);
						if (track) {
							m_dvdawm.set_track(atoi(track));
						}
					}
				}
				m_track_changed = false;
			}
			if (m_dvdawm.run((audio_sample*)p_chunk->get_data(), p_chunk->get_sample_count()) > 0) {
				m_dvdawm.reset();
				g_watermarked = true;
			}
		}
		return true;
	}

	virtual void flush() {
	}

	virtual double get_latency() {
		return 0.0;
	}

	virtual bool need_track_change_mark() {
		return true;
	}

	bool check_init_code(int init_code) {
		switch (init_code) {
		case DVDAWM_INITIALIZED_OK:
			return true;
		case DVDAWM_OUT_OF_MEMORY:
			console_printf("Error: DVD-Audio Watermark Detector is out of memory");
			return false;
		case DVDAWM_UNSUPPORTED_SAMPLERATE:
			console_printf("Warning: DVD-Audio Watermark Detector does not support %dHz samplerate", m_samplerate);
			return false;
		default:
			console_printf("Warning: DVD-Audio Watermark Detector unknown error");
			return false;
		}
	}

	void print_source() {
		metadb_handle_ptr p_metadb;
		if (get_cur_file(p_metadb)) {
			file_info_impl p_info;
			const char* track = 0;
			const char* title = 0;
			if (p_metadb->get_info(p_info)) {
				track = p_info.meta_get("tracknumber", 0);
				title = p_info.meta_get("title", 0);
			}
			if (track && title) {
				console_printf("Running DVD-Audio Watermark Detector: %s \"%s\"", track, title);
			}
			else {
				console_printf("Running DVD-Audio Watermark Detector: %s / index: %d", p_metadb->get_path(), p_metadb->get_subsong_index());
			}
		}
	}

};

static dsp_factory_nopreset_t<dsp_watermark_detector_t> g_dsp_watermark_detector_factory;

class initquit_dvda_t : public initquit {
public:
	virtual void on_init() {
		g_no_untagged_tracks = CDVDAPreferences::get_no_untagged_tracks();
		g_watermarked = false;
	}
	
	virtual void on_quit() {
		g_dvda_disc_cache.flush();
	}
};

static initquit_factory_t<initquit_dvda_t> g_initquit_dvda_factory;

DECLARE_COMPONENT_VERSION(DVDA_NAME, DVDA_VERSION, DVDA_COPYRIGHT);
DECLARE_FILE_TYPE("DVD-Audio files", "*.AOB;AUDIO_TS.IFO;*.ISO;*.MLP;*.TRUEHD");
VALIDATE_COMPONENT_FILENAME(DVDA_FILE);
