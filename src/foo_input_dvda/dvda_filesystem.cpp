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

#include <string>
#include "dvda_filesystem.h"

int dir_dvda_fileobject_t::read(void* buffer, int count) {
	EnterCriticalSection(&lock);
	count = fo.read(buffer, count);
	LeaveCriticalSection(&lock);
	return count;
}

bool dir_dvda_fileobject_t::seek(int64_t offset) {
	bool ok;
	EnterCriticalSection(&lock);
	ok = fo.seek(offset);
	LeaveCriticalSection(&lock);
	return ok;
}

void dir_dvda_fileobject_t::close() {
	fs->file_close(this);
};

bool dir_dvda_filesystem_t::mount(const char* path) {
	EnterCriticalSection(&lock);
	dvda_filesystem_t::mount(path);
	LeaveCriticalSection(&lock);
	return true;
}

void dir_dvda_filesystem_t::dismount() {
	EnterCriticalSection(&lock);
	dvda_filesystem_t::dismount();
	LeaveCriticalSection(&lock);
}

bool dir_dvda_filesystem_t::get_name(char* name) {
	char m_disc[4];
	bool ok;
	EnterCriticalSection(&lock);
	ok = false;
	m_disc[0] = '\0';
	const char* drive = strstr(path.c_str(), "://");
	if (drive && strlen(drive + 3) > 2) {
		memcpy(m_disc, drive + 3, 3);
		m_disc[3] = '\0';
	}
	if (m_disc[0] && GetDriveTypeA(m_disc) == DRIVE_CDROM) {
		DWORD max_component_length;
		DWORD file_system_flags;
		ok = GetVolumeInformationA(m_disc, name, 256, NULL, &max_component_length, &file_system_flags, NULL, 0) == TRUE;
	}
	LeaveCriticalSection(&lock);
	return ok;
}

dvda_fileobject_t* dir_dvda_filesystem_t::file_open(const char* name) {
	dir_dvda_fileobject_t* p_file;
	EnterCriticalSection(&lock);
	p_file = new dir_dvda_fileobject_t;
	if (p_file) {
		p_file->fs = this;
		std::string filepath = path + "\\" + name;
		p_file->fo.open(filepath.c_str());
		if (p_file->fo.is_open()) {
			p_file->size = (uint32_t)p_file->fo.get_size(filepath.c_str());
		}
		else {
			delete p_file;
			p_file = NULL;
		}
	}
	LeaveCriticalSection(&lock);
	return p_file;
}

void dir_dvda_filesystem_t::file_close(dvda_fileobject_t* p_file) {
	EnterCriticalSection(&lock);
	if (p_file) {
		reinterpret_cast<dir_dvda_fileobject_t*>(p_file)->fo.close();
		delete p_file;
	}
	LeaveCriticalSection(&lock);
}

int iso_dvda_fileobject_t::read(void* buffer, int count) {
	EnterCriticalSection(&lock);
	count = fo.read(buffer, count);
	LeaveCriticalSection(&lock);
	return count;
}

bool iso_dvda_fileobject_t::seek(int64_t offset) {
	bool ok;
	EnterCriticalSection(&lock);
	ok = false;
	if (offset < size)
		ok = fo.seek((int64_t)2048 * (int64_t)lba + offset);
	LeaveCriticalSection(&lock);
	return ok;
}

void iso_dvda_fileobject_t::close() {
	fs->file_close(this);
};

bool iso_dvda_filesystem_t::mount(const char* path) {
	EnterCriticalSection(&lock);
	dvda_filesystem_t::mount(path);
	iso_reader = DVDOpen(path);
	LeaveCriticalSection(&lock);
	return iso_reader != NULL;
}

void iso_dvda_filesystem_t::dismount() {
	EnterCriticalSection(&lock);
	if (iso_reader)
		DVDClose(iso_reader);
	dvda_filesystem_t::dismount();
	LeaveCriticalSection(&lock);
}

bool iso_dvda_filesystem_t::get_name(char* name) {
	bool ok;
	EnterCriticalSection(&lock);
	ok = UDFGetVolumeIdentifier(iso_reader, name, 32) > 0;
	LeaveCriticalSection(&lock);
	return ok;
}

dvda_fileobject_t* iso_dvda_filesystem_t::file_open(const char* name) {
	iso_dvda_fileobject_t* p_file;
	EnterCriticalSection(&lock);
	p_file = new iso_dvda_fileobject_t;
	if (!p_file) {
		LeaveCriticalSection(&lock);
		return NULL;
	}
	p_file->fs = this;
	std::string filepath = "/AUDIO_TS/";
	filepath += name;
	uint32_t filesize = 0;
	p_file->lba = UDFFindFile(iso_reader, (char*)filepath.c_str(), &filesize);
	p_file->size = filesize;
	if (p_file->lba == 0) {
		delete p_file;
		LeaveCriticalSection(&lock);
		return NULL;
	}
	p_file->fo.open(path.c_str());
	if (!p_file->fo.is_open()) {
		delete p_file;
		LeaveCriticalSection(&lock);
		return NULL;
	}
	if (!p_file->seek(0)) {
		delete p_file;
		LeaveCriticalSection(&lock);
		return NULL;
	}
	LeaveCriticalSection(&lock);
	return p_file;
}

void iso_dvda_filesystem_t::file_close(dvda_fileobject_t* p_file) {
	EnterCriticalSection(&lock);
	if (p_file) {
		reinterpret_cast<iso_dvda_fileobject_t*>(p_file)->fo.close();
		delete p_file;
	}
	LeaveCriticalSection(&lock);
}
