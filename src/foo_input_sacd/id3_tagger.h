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

#ifndef _ID3_TAGGER_H_INCLUDED
#define _ID3_TAGGER_H_INCLUDED

#include <map>
#include <vector>
#include <utility>
#include <foobar2000.h>

using std::tuple;
using std::make_tuple;
using std::vector;

class id3_tags_t {
public:
	vector<uint8_t> value;
	size_t          id = -1;
};

typedef tuple<const char*, size_t> id3_value_t;

class id3_tagger_t {
	bool               single_track;
	vector<id3_tags_t> tagstore;
public:
	class iterator {
		vector<id3_tags_t>& tagstore;
		size_t              tagindex;
	public:
		iterator(vector<id3_tags_t>& _tagstore, size_t _tagindex = 0) : tagstore(_tagstore), tagindex(_tagindex) {}
		iterator& operator++() {
			++tagindex;
			return *this;
		}
		iterator operator++(int) {
			iterator retval = *this;
			++(*this);
			return retval;
		}
		bool operator==(iterator other) const {
			return tagindex == other.tagindex;
		}
		bool operator!=(iterator other) const {
			return !(*this == other);
		}
		id3_value_t operator*() {
			return make_tuple(reinterpret_cast<const char*>(tagstore[tagindex].value.data()), tagstore[tagindex].value.size());
		}
	};
	static bool load_info(const vector<uint8_t>& buffer, file_info& info);
	static bool save_info(vector<uint8_t>& buffer, const file_info& info);
	id3_tagger_t() : single_track(false) {}
	iterator begin();
	iterator end();
	size_t get_count();
	vector<id3_tags_t>& get_info();
	bool is_single_track();
	void set_single_track(bool is_single);
	void append(const id3_tags_t& tags);
	void remove_all();
	bool get_info(size_t track_number, file_info& info);
	bool set_info(size_t track_number, const file_info& info);
	void update_tags();
	bool load_info(size_t track_index, file_info& info);
	bool save_info(size_t track_index, const file_info& info);
	void update_tags(size_t track_index);
	void get_albumart(size_t albumart_id, vector<t_uint8>& albumart_data);
	void set_albumart(size_t albumart_id, const vector<t_uint8>& albumart_data);
};

#endif
