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

#ifndef _DVDA_FILE_H_INCLUDED
#define _DVDA_FILE_H_INCLUDED

//#define _RTL_DVDA_FILE

#ifdef _RTL_DVDA_FILE

#include <stdint.h>
#include <string>
#include <sys/stat.h>

class dvda_file_t {
	std::string dvda_zone;
	FILE* stream;
public:
	dvda_file_t() {
		dvda_zone = ".";
		stream = NULL;
	}
	void set_path(const char* zone_path) {
		dvda_zone = zone_path;
		stream = NULL;
	}
	bool open(const char* file_name)  {
		std::string::size_type pos = dvda_zone.find("://");
		if (pos != std::string::npos)
			pos += 3;
		else
			pos = 0;
		stream = fopen((dvda_zone.substr(pos) + "\\" + file_name).c_str(), "rb");
		if (stream == NULL)
			return false;
		return (stream != NULL);
	}
	bool is_open() {
		return stream != NULL;
	}
	bool close() {
		if (stream != NULL) {
			int err = fclose(stream);
			stream = NULL;
			return err == 0;
		}
		return false;
	}
	int read(void* buffer, int count) {
		return fread(buffer, 1, count, stream);
	}
	bool seek(int64_t offset, int origin = SEEK_SET) {
		return _fseeki64(stream, offset, SEEK_SET) == 0;
	}
	int64_t size(const char* file_name) {
		std::string::size_type pos = dvda_zone.find("://");
		if (pos != std::string::npos)
			pos += 3;
		else
			pos = 0;
		struct _stati64 st;
		if (_stati64((dvda_zone.substr(pos) + "\\" + file_name).c_str(), &st) == 0)
			return st.st_size;
		return -1;
	}
};

#else

#include <sys/stat.h>
#include "dvda_config.h"

class dvda_file_t {
	abort_callback_impl abort;
	service_ptr_t<file> stream;
public:
	bool open(const char* file_path) {
		bool file_exists = false;
		try {
			file_exists = filesystem::g_exists(file_path, abort);
		}
		catch (...) {
		}
		if (file_exists) {
			filesystem::g_open_read(stream, file_path, abort);
			return true;
		}
		return false;
	}
	bool is_open() {
		return stream.is_valid();
	}
	bool close() {
		if (stream.is_valid()) {
			stream.release();
			return true;
		}
		return false;
	}
	int read(void* buffer, int count) {
		return (int)stream->read(buffer, count, abort);
	}
	bool seek(int64_t offset, int origin = SEEK_SET) {
		stream->seek(offset, abort);
		return true;
	}
	int64_t get_size(const char* file_path) {
		bool file_exists = false;
		try {
			file_exists = filesystem::g_exists(file_path, abort);
		}
		catch (...) {
		}
		if (file_exists) {
			t_filestats stats;
			bool is_writable;
			filesystem::g_get_stats(file_path, stats, is_writable, abort);
			return stats.m_size;
		}
		return -1;
	}
};

#endif

#endif
