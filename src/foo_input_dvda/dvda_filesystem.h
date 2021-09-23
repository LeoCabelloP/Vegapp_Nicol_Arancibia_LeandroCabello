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

#ifndef _DVDA_FILESYSTEM_H_INCLUDED
#define _DVDA_FILESYSTEM_H_INCLUDED

#include <stdint.h>
#include <string>
#include "dvda_file.h"
#include "udf/dvd_input.h"
#include "udf/dvd_udf.h"

class dvda_filesystem_t;
class dvda_fileobject_t;

class dvda_fileobject_t {
protected:
	CRITICAL_SECTION lock;
	int64_t size;
public:
	dvda_fileobject_t() {
		InitializeCriticalSection(&lock);
		size = 0;
	}
	~dvda_fileobject_t() {
		DeleteCriticalSection(&lock);
	}
	virtual int  read(void* buffer, int count) = 0;
	virtual bool seek(int64_t offset) = 0;
	virtual void close() = 0;
	virtual int64_t get_size() {
		return size;
	}
};

class dvda_filesystem_t {
protected:
	CRITICAL_SECTION lock;
	std::string path;
public:
	dvda_filesystem_t() {
		InitializeCriticalSection(&lock);
	}
	~dvda_filesystem_t() {
		DeleteCriticalSection(&lock);
	}
	const char* get_path() {
		const char* c_path;
		EnterCriticalSection(&lock);
		c_path = path.c_str();
		LeaveCriticalSection(&lock);
		return c_path;
	}
	virtual bool mount(const char* path) {
		this->path.assign(path);
		return true;
	};
	virtual void dismount() {
		path.clear();
	};
	virtual bool get_name(char* name) = 0;
	virtual dvda_fileobject_t* file_open(const char* name) = 0;
	virtual void file_close(dvda_fileobject_t* p_file) = 0;
};

class dir_dvda_fileobject_t : public dvda_fileobject_t {
	friend class dir_dvda_filesystem_t;
	dir_dvda_filesystem_t* fs;
	dvda_file_t fo;
public:
	virtual int  read(void* buffer, int count);
	virtual bool seek(int64_t offset);
	virtual void close();
};

class dir_dvda_filesystem_t : public dvda_filesystem_t {
public:
	virtual bool mount(const char* path);
	virtual void dismount();
	virtual bool get_name(char* name);
	virtual dvda_fileobject_t* file_open(const char* name);
	virtual void file_close(dvda_fileobject_t* p_file);
};

class iso_dvda_fileobject_t : public dvda_fileobject_t {
	friend class iso_dvda_filesystem_t;
	iso_dvda_filesystem_t* fs;
	dvda_file_t fo;
	uint32_t lba;
public:
	virtual int  read(void* buffer, int count);
	virtual bool seek(int64_t offset);
	virtual void close();
};

class iso_dvda_filesystem_t : public dvda_filesystem_t {
	friend class iso_dvda_fileobject_t;
	dvd_reader_t* iso_reader;
public:
	virtual bool mount(const char* path);
	virtual void dismount();
	virtual bool get_name(char* name);
	virtual dvda_fileobject_t* file_open(const char* name);
	virtual void file_close(dvda_fileobject_t* p_file);
};

#endif
