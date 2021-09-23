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

#ifndef _DVDA_METABASE_H_INCLUDED
#define _DVDA_METABASE_H_INCLUDED

#include "dvda_config.h"
#include "dvda_filesystem.h"

#include <atlcomcli.h>
#import <msxml6.dll>

using CXML_Document = MSXML2::DOMDocument60;
using IXML_DocumentPtr = MSXML2::IXMLDOMDocumentPtr;
using IXML_NodePtr = MSXML2::IXMLDOMNodePtr;

constexpr char* TAG_ROOT = "root";
constexpr char* TAG_STORE = "store";
constexpr char* TAG_TRACK = "track";
constexpr char* TAG_INFO = "info";
constexpr char* TAG_META = "meta";
constexpr char* TAG_REPLAYGAIN = "replaygain";
constexpr char* TAG_ALBUMART = "albumart";

constexpr char* ATT_ID = "id";
constexpr char* ATT_NAME = "name";
constexpr char* ATT_TYPE = "type";
constexpr char* ATT_VALUE = "value";
constexpr char* ATT_VALSEP = ";";
constexpr char* ATT_VERSION = "version";

constexpr char* METABASE_CATALOG = "dvda_metabase";
constexpr char* METABASE_VERSION = "1.2";

class dvda_metabase_t {
	abort_callback_impl media_abort;
	string8             store_id;
	string8             store_path;
	CComVariant         xmlfile;
	IXML_DocumentPtr    xmldoc;
	bool                initialized;
public:
	dvda_metabase_t(dvda_filesystem_t* p_filesystem, const char* p_metafile);
	~dvda_metabase_t();
	void get_track_info(t_uint32 track_number, file_info& track_info);
	void set_track_info(t_uint32 track_number, const file_info& track_info);
	void get_albumart(t_uint32 albumart_id, vector<t_uint8>& albumart_data);
	void set_albumart(t_uint32 albumart_id, const vector<t_uint8>& albumart_data);
	void commit();
private:
	bool init_xmldoc(const char* store_type);
	void delete_tags(IXML_NodePtr node, const char* tag_type);
	void insert_tag(IXML_NodePtr node, const char* tag_type, const char* tag_name, const char* tag_value);
	IXML_NodePtr get_node(const char* tag_type, const char* att_id, bool create = false);
	IXML_NodePtr new_node(const char* tag_type, const char* att_id);
};

#endif
