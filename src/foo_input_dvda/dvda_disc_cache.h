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

#ifndef _DVDA_DISC_CACHE_H_INCLUDED
#define _DVDA_DISC_CACHE_H_INCLUDED

#include <vector>
#include "audio_track.h"
#include "dvda_config.h"
#include "dvda_metabase.h"
#include "dvda_zone.h"

using std::vector;

class dvda_zone_key_t {
public:
	size_t   size;
	uint64_t timestamp;
	bool operator<(dvda_zone_key_t& dvda_zone_key) const {
		return true;
	}
};

class dvda_disc_t {
public:
	t_filestats        dvda_stats;
	dvda_filesystem_t* dvda_filesystem;
	dvda_zone_t*       dvda_zone;
	track_list_t*      track_list;
	dvda_metabase_t*   dvda_metabase;
};

class dvda_disc_cache_t {
	CRITICAL_SECTION lock;
	vector<dvda_disc_t> dvda_discs;
public:
	volatile LONG references;
	DWORD thread_id;

	dvda_disc_cache_t() {
		InitializeCriticalSection(&lock);
		references = 0;
		thread_id = 0;
	}

	~dvda_disc_cache_t() {
		DeleteCriticalSection(&lock);
	}

	void add_ref() {
		InterlockedIncrement(&references);
	}

	void release() {
		InterlockedDecrement(&references);
		if (!(references > 0)) {
			delayed_flush();
		}
	}

	void add(dvda_disc_t dvda_disc) {
		EnterCriticalSection(&lock);
		dvda_discs.push_back(dvda_disc);
		LeaveCriticalSection(&lock);
	}

	bool cached(t_filestats dvda_stats) {
		bool in_cache;
		EnterCriticalSection(&lock);
		in_cache = false;
		for (auto& disc : dvda_discs) {
			if (disc.dvda_stats == dvda_stats) {
				in_cache = true;
				break;
			}
		}
		LeaveCriticalSection(&lock);
		return in_cache;
	}

	bool cached(t_filestats dvda_stats, dvda_filesystem_t** p_dvda_filesystem, dvda_zone_t** p_dvda_zone, track_list_t** p_track_list, dvda_metabase_t** p_dvda_metabase) {
		bool in_cache;
		EnterCriticalSection(&lock);
		in_cache = false;
		for (auto& disc : dvda_discs) {
			if (disc.dvda_stats == dvda_stats) {
				*p_dvda_filesystem = disc.dvda_filesystem;
				*p_dvda_zone = disc.dvda_zone;
				*p_track_list = disc.track_list;
				*p_dvda_metabase = disc.dvda_metabase;
				in_cache = true;
				break;
			}
		}
		LeaveCriticalSection(&lock);
		return in_cache;
	}

	bool uncache(t_filestats dvda_stats) {
		bool in_cache;
		EnterCriticalSection(&lock);
		in_cache = false;
		for (auto iter = dvda_discs.begin(); iter != dvda_discs.end(); iter++) {
			if ((*iter).dvda_stats == dvda_stats) {
				dvda_discs.erase(iter);
				in_cache = true;
				break;
			}
		}
		LeaveCriticalSection(&lock);
		return in_cache;
	}

	dvda_disc_t& operator[](std::vector<dvda_disc_t>::size_type i) {
		EnterCriticalSection(&lock);
		dvda_disc_t& dvda_disc = dvda_discs[i];
		LeaveCriticalSection(&lock);
		return dvda_disc;
	}

	void flush() {
		EnterCriticalSection(&lock);
		if (dvda_discs.size() > 0) {
			console_printf("Release cached DVD-Audio discs");
			for (auto& disc : dvda_discs) {
				if (disc.dvda_zone) {
					disc.dvda_zone->close();
					delete disc.dvda_zone;
				}
				if (disc.dvda_filesystem) {
					disc.dvda_filesystem->dismount();
					delete disc.dvda_filesystem;
				}
				if (disc.track_list) {
					disc.track_list->clear();
					delete disc.track_list;
				}
				if (disc.dvda_metabase)
					delete disc.dvda_metabase;
			}
			dvda_discs.clear();
		}
		LeaveCriticalSection(&lock);
	}

	void delayed_flush();
};

#endif
