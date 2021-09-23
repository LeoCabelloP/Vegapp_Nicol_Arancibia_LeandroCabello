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

#ifndef _AUDIO_STREAM_INFO_H_INCLUDED
#define _AUDIO_STREAM_INFO_H_INCLUDED

#include <stdint.h>
#include "dvda_block.h"

enum stream_id_t {UNK_STREAM_ID = 0, PCM_STREAM_ID = 0xa0, MLP_STREAM_ID = 0xa1};
enum stream_type_t {STREAM_TYPE_TRUEHD = 0xba, STREAM_TYPE_MLP = 0xbb};
enum chmode_t {CHMODE_BOTH = 0, CHMODE_TWOCH = 1, CHMODE_MULCH = 2};

typedef struct {
	uint32_t group1_channel_id[4];
	uint32_t group2_channel_id[4];
	char*    group1_channel_name[4];
	char*    group2_channel_name[4];
	int      group1_channels;
	int      group2_channels;
} MLPPCM_ASSIGNMENT;

typedef struct {
	uint32_t channel_id[2];
	char*    channel_name[2];
	int      channels;
} TRUEHD_ASSIGNMENT;

class audio_stream_info_t {
public:
	static const MLPPCM_ASSIGNMENT mlppcm_table[21];
	static const TRUEHD_ASSIGNMENT truehd_table[13];
	int stream_id;
	int stream_type;
	int channel_assignment;
	int group1_channels;
	int group1_bits;
	int group1_samplerate;
	int group2_channels;
	int group2_bits;
	int group2_samplerate;
	int bitrate;
	bool can_downmix;
	bool is_vbr;
	int sync_offset;
	audio_stream_info_t();
	const char* get_channel_name(int channel);
	uint32_t get_wfx_channels();
	double estimate_compression();
};

#endif
