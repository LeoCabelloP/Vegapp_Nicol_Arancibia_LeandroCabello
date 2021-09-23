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

#ifndef _SACD_MEDIA_H_INCLUDED
#define _SACD_MEDIA_H_INCLUDED

#include "sacd_config.h"

class sacd_media_t {
protected:
	abort_callback_impl media_abort;
public:
	virtual ~sacd_media_t() {};
	virtual bool open(file_ptr filehint, const char* path, t_input_open_reason reason) = 0;
	virtual bool close() = 0;
	virtual bool can_seek() = 0;
	virtual bool seek(int64_t position, file::t_seek_mode mode = file::seek_from_beginning) = 0;
	virtual file_ptr get_handle() = 0;
	virtual int64_t get_position() = 0;
	virtual int64_t get_size() = 0;
	virtual t_filestats get_stats() = 0;
	virtual t_input_open_reason get_reason() = 0;
	virtual size_t read(void* data, size_t size) = 0;
	virtual size_t write(const void* data, size_t size) = 0;
	virtual int64_t skip(int64_t bytes) = 0;
	virtual void truncate(int64_t position) = 0;
	virtual void on_idle() = 0;
};

class sacd_media_disc_t : public sacd_media_t {
	HANDLE  media_disc;
	int64_t file_position;
	t_input_open_reason open_reason;
public:
	sacd_media_disc_t();
	virtual ~sacd_media_disc_t();
	virtual bool open(file_ptr filehint, const char* path, t_input_open_reason reason);
	virtual bool close();
	virtual bool can_seek();
	virtual bool seek(int64_t position, file::t_seek_mode mode = file::seek_from_beginning);
	virtual file_ptr get_handle();
	virtual int64_t get_position();
	virtual int64_t get_size();
	virtual t_filestats get_stats();
	virtual t_input_open_reason get_reason();
	virtual size_t read(void* data, size_t size);
	virtual size_t write(const void* data, size_t size);
	virtual int64_t skip(int64_t bytes);
	virtual void truncate(int64_t position);
	virtual void on_idle();
};

class sacd_media_file_t : public sacd_media_t {
	file_ptr media_file;
	t_input_open_reason open_reason;
public:
	sacd_media_file_t();
	virtual ~sacd_media_file_t();
	virtual bool open(file_ptr filehint, const char* path, t_input_open_reason reason);
	virtual bool close();
	virtual bool can_seek();
	virtual bool seek(int64_t position, file::t_seek_mode mode = file::seek_from_beginning);
	virtual file_ptr get_handle();
	virtual int64_t get_position();
	virtual int64_t get_size();
	virtual t_filestats get_stats();
	virtual t_input_open_reason get_reason();
	virtual size_t read(void* data, size_t size);
	virtual size_t write(const void* data, size_t size);
	virtual int64_t skip(int64_t bytes);
	virtual void truncate(int64_t position);
	virtual void on_idle();
};

#endif
