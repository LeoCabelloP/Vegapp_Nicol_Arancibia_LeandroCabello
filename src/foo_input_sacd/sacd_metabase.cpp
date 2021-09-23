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

#include "sacd_metabase.h"

auto utf2xml = [](auto src) {
	auto dst{ string8() };
	for (auto i = 0; src[i] != 0; i++) {
		if (src[i] == '\r') {
			dst += "&#13;";
		}
		else if (src[i] == '\n') {
			dst += "&#10;";
		}
		else {
			dst += string8(&src[i], 1);
		}
	}
	return dst;
};

auto xml2utf = [](auto src) {
	auto dst{ string8() };
	for (auto i = 0; src[i] != 0; i++) {
		if (strncmp(&src[i], "&#13;", 5) == 0) {
			dst += "\r";
			i += 4;
		}
		else if (strncmp(&src[i], "&#10;", 5) == 0) {
			dst += "\n";
			i += 4;
		}
		else {
			dst += string8(&src[i], 1);
		}
	}
	return dst;
};

auto get_node_attribute = [](auto nodemap, auto attribute_name) {
	auto attribute_value{ string8() };
	if (nodemap) {
		auto node{ nodemap->getNamedItem(attribute_name) };
 		if (node) {
			auto bstr_value{ CComVariant() };
			node->get_nodeValue(&bstr_value);
			if (V_VT(&bstr_value) == VT_BSTR) {
				attribute_value = xml2utf(string_utf8_from_wide(V_BSTR(&bstr_value)));
			}
		}
	}
	return attribute_value;
};

auto set_node_attribute = [](auto document, auto element, auto attribute_name, auto attribute_value) {
	auto ok{ false };
	auto attribute{ document->createAttribute(attribute_name) };
	if (attribute) {
		attribute->value = CComVariant(string_wide_from_utf8(utf2xml(attribute_value)));
		element->setAttributeNode(attribute);
		ok = true;
	}
	return ok;
};

auto is_linkable_tag = [](auto tag_name) {
	return !tag_name.equals("dynamic range") && !tag_name.equals("album dynamic range");
};

string8 get_md5(sacd_disc_t* p_disc) {
	uint8_t md5_source[MASTER_TOC_LEN * SACD_LSN_SIZE];
	auto md5_string{ string8()};
	if (p_disc->read_blocks_raw(START_OF_MASTER_TOC, MASTER_TOC_LEN, md5_source)) {
		md5_string = hasher_md5::get()->process_single(md5_source, sizeof(md5_source)).asString();
	}
	else {
		console::error("Cannot read MD5 hash source");
	}
	return md5_string;
}

sacd_metabase_t::sacd_metabase_t(sacd_disc_t* p_disc, const char* p_metafile) {
	initialized = false;
	store_id = get_md5(p_disc);
	if (store_id.is_empty()) {
		return;
	}
	store_path = core_api::get_profile_path();
	store_path.end_with_slash();
	store_path += METABASE_CATALOG;
	store_path.end_with_slash();
	auto store_file = store_path;
	store_file.end_with_slash();
	store_file += store_id;
	store_file += ".xml";
	if (!p_metafile) {
		if (!filesystem::g_exists(store_path, media_abort)) {
			filesystem::g_create_directory(store_path, media_abort);
			if (filesystem::g_exists(store_path, media_abort)) {
				console::info(string_printf("SACD metabase folder '%s' has been created", store_path.c_str()));
			}
			else {
				popup_message::g_show(string_printf("Cannot create SACD metabase folder '%s'", store_path.c_str()), METABASE_CATALOG, popup_message::icon_error);
			}
		}
	}
	else {
		if (!filesystem::g_exists(p_metafile, media_abort)) {
			if (filesystem::g_exists(store_file, media_abort)) {
				filesystem::g_copy(store_file, p_metafile, media_abort);
				if (filesystem::g_exists(p_metafile, media_abort)) {
					console::info(string_printf("SACD metabase file is copied from '%s' to '%s'", store_file.c_str(), p_metafile));
				}
				else {
					popup_message::g_show(string_printf("Cannot copy SACD metabase file to '%s'", p_metafile), METABASE_CATALOG, popup_message::icon_error);
				}
			}
		}
	}
	auto hr{ HRESULT(S_OK) };
	hr = CoInitialize(NULL);
	if (FAILED(hr)) {
		console::printf("Error: Cannot initialize COM (%d)", hr);
	}
	hr = xmldoc.CreateInstance(__uuidof(CXML_Document));
	if (SUCCEEDED(hr)) {
		auto xml_file{ string8() };
		filesystem::g_get_native_path(p_metafile ? p_metafile : store_file, xml_file);
		xmlfile = string_wide_from_utf8(xml_file);
		initialized = init_xmldoc("SACD");
		if (!initialized) {
			console::error("Cannot initialize XML document");
		}
	}
	else {
		console::error("Cannot create XML instance");
	}
}

sacd_metabase_t::~sacd_metabase_t() {
	xmlfile.Clear();
	CoUninitialize();
}

void sacd_metabase_t::get_track_info(t_uint32 track_number, file_info& track_info) {
	string_formatter(node_id);
	node_id << track_number;
	auto node_track{ get_node(TAG_TRACK, node_id) };
	if (!node_track) {
		return;
	}
	auto list_tags{ node_track->GetchildNodes() };
	if (!list_tags) {
		return;
	}
	auto replaygain_info{ track_info.get_replaygain() };
	for (auto i = 0l; i < list_tags->Getlength(); i++) {
		auto node_tag{ list_tags->Getitem(i) };
		if (node_tag) {
			auto node_name{ string8(node_tag->GetnodeName()) };
			auto nodemap_attr{ node_tag->Getattributes() };
			if (nodemap_attr) {
				auto tag_name{ string8(get_node_attribute(nodemap_attr, ATT_NAME)) };
				auto tag_value{ string8(get_node_attribute(nodemap_attr, ATT_VALUE)) };
				if (node_name.equals(TAG_META)) {
					auto head_chunk = true;
					auto sep_pos{ t_size(0) };
					do {
						sep_pos = string_find_first(tag_value, ATT_VALSEP);
						auto tag_value_head{ string8() };
						tag_value_head.add_string(tag_value, sep_pos);
						while (tag_value_head.length() > 0 && tag_value_head[0] == ' ') {
							tag_value_head.remove_chars(0, 1);
						}
						if (head_chunk) {
							track_info.meta_set(tag_name, tag_value_head);
							head_chunk = false;
						}
						else {
							track_info.meta_add(tag_name, tag_value_head);
						}
						tag_value.remove_chars(0, sep_pos + 1);
					} while (sep_pos != ~0);
				}
				if (node_name.equals(TAG_REPLAYGAIN)) {
					if (tag_name.equals("replaygain_track_gain")) {
						replaygain_info.set_track_gain_text(tag_value);
					}
					if (tag_name.equals("replaygain_track_peak")) {
						replaygain_info.set_track_peak_text(tag_value);
					}
					if (tag_name.equals("replaygain_album_gain")) {
						replaygain_info.set_album_gain_text(tag_value);
					}
					if (tag_name.equals("replaygain_album_peak")) {
						replaygain_info.set_album_peak_text(tag_value);
					}
				}
			}
		}
	}
	track_info.set_replaygain(replaygain_info);
}

void sacd_metabase_t::set_track_info(t_uint32 track_number, const file_info& track_info,bool is_linked) {
	string_formatter(node_id);
	node_id << track_number;
	auto node_track{ get_node(TAG_TRACK, node_id, true) };
	if (!node_track) {
		return;
	}
	delete_tags(node_track, TAG_META, is_linked);
	for (auto i = 0u; i < track_info.meta_get_count(); i++) {
		auto tag_name{ string8(track_info.meta_enum_name(i)) };
		auto tag_value{ string8(track_info.meta_enum_value(i, 0)) };
		for (auto j = 1u; j < track_info.meta_enum_value_count(i); j++) {
			tag_value += ATT_VALSEP;
			tag_value += track_info.meta_enum_value(i, j);
		}
		if (!is_linked || (is_linked && is_linkable_tag(tag_name))) {
			insert_tag(node_track, TAG_META, tag_name, tag_value);
		}
	}
	if (!is_linked) {
		delete_tags(node_track, TAG_REPLAYGAIN, false);
		auto replaygain_info{ track_info.get_replaygain() };
		replaygain_info::t_text_buffer tag_value;
		if (replaygain_info.is_track_gain_present()) {
			if (replaygain_info.format_track_gain(tag_value)) {
				insert_tag(node_track, TAG_REPLAYGAIN, "replaygain_track_gain", tag_value);
			}
		}
		if (replaygain_info.is_track_peak_present()) {
			if (replaygain_info.format_track_peak(tag_value)) {
				insert_tag(node_track, TAG_REPLAYGAIN, "replaygain_track_peak", tag_value);
			}
		}
		if (replaygain_info.is_album_gain_present()) {
			if (replaygain_info.format_album_gain(tag_value)) {
				insert_tag(node_track, TAG_REPLAYGAIN, "replaygain_album_gain", tag_value);
			}
		}
		if (replaygain_info.is_album_peak_present()) {
			if (replaygain_info.format_album_peak(tag_value)) {
				insert_tag(node_track, TAG_REPLAYGAIN, "replaygain_album_peak", tag_value);
			}
		}
	}
	auto list_tags{ node_track->GetchildNodes() };
	if (list_tags) {
		if (list_tags->Getlength() == 0) {
			auto node_store{ node_track->GetparentNode() };
			if (node_store) {
				node_store->removeChild(node_track);
			}
		}
	}
}

void sacd_metabase_t::get_albumart(t_uint32 albumart_id, vector<t_uint8>& albumart_data) {
	string_formatter(node_id);
	node_id << albumart_id;
	auto node_albumart{ get_node(TAG_ALBUMART, node_id) };
	if (!node_albumart) {
		return;
	}
	auto node_cdata = node_albumart->GetfirstChild();
	if (node_cdata) {
		auto bstr_value{ CComVariant() };
		if (node_cdata->get_nodeValue(&bstr_value) == S_OK) {
			if (V_VT(&bstr_value) == VT_BSTR) {
				ULARGE_INTEGER bstr_size;
				if (bstr_value.GetSizeMax(&bstr_size) == S_OK) {
					array_t<uint8_t> albumart_decode;
					base64_decode_array(albumart_decode, string_utf8_from_wide(bstr_value.bstrVal));
					albumart_data.assign(albumart_decode.get_ptr(), albumart_decode.get_ptr() + albumart_decode.get_size());
				}
			}
		}
	}
}

void sacd_metabase_t::set_albumart(t_uint32 albumart_id, const vector<t_uint8>& albumart_data) {
	string_formatter(node_id);
	node_id << albumart_id;
	auto create_node = albumart_data.size() > 0;
	auto node_albumart{ get_node(TAG_ALBUMART, node_id, create_node) };
	if (!node_albumart) {
		return;
	}
	if (create_node) {
		string8 albumart_base64;
		base64_encode(albumart_base64, albumart_data.data(), albumart_data.size());
		auto bstr_value{ CComVariant(string_wide_from_utf8(albumart_base64)) };
		auto new_node_cdata = xmldoc->createCDATASection(bstr_value.bstrVal);
		auto old_node_cdata = node_albumart->GetfirstChild();
		if (old_node_cdata) {
			node_albumart->replaceChild(new_node_cdata, old_node_cdata);
		}
		else {
			node_albumart->appendChild(new_node_cdata);
		}
	}
	else {
		auto node_parent = node_albumart->GetparentNode();
		node_parent->removeChild(node_albumart);
	}
}

void sacd_metabase_t::commit() {
	if (V_VT(&xmlfile) != VT_BSTR) {
		return;
	}
	if (FAILED(xmldoc->raw_save(xmlfile))) {
		console::error("Cannot save tag file");
	}
}

bool sacd_metabase_t::init_xmldoc(const char* store_type) {
	if (V_VT(&xmlfile) != VT_BSTR) {
		return false;
	}
	xmldoc->async = VARIANT_FALSE;
	xmldoc->preserveWhiteSpace = VARIANT_TRUE;
	auto is_ok{ VARIANT_BOOL(FALSE) };
	auto hr{ xmldoc->raw_load(xmlfile, &is_ok) };
	if (hr != S_OK || is_ok != VARIANT_TRUE) {
		auto inst_root{ xmldoc->createProcessingInstruction("xml", "version='1.0' encoding='utf-8'") };
		if (!inst_root) {
			return false;
		}
		xmldoc->appendChild(inst_root);
		auto comm_root{ xmldoc->createComment("SACD metabase file") };
		if (!comm_root) {
			return false;
		}
		xmldoc->appendChild(comm_root);
		auto elem_root{ xmldoc->createElement(TAG_ROOT) };
		if (!elem_root) {
			return false;
		}
		xmldoc->appendChild(elem_root);
		auto elem_store{ xmldoc->createElement(TAG_STORE) };
		if (!elem_store) {
			return false;
		}
		elem_root->appendChild(elem_store);
		if (!set_node_attribute(xmldoc, elem_store, ATT_ID, store_id)) {
			return false;
		}
		if (!set_node_attribute(xmldoc, elem_store, ATT_TYPE, store_type)) {
			return false;
		}
		if (!set_node_attribute(xmldoc, elem_store, ATT_VERSION, METABASE_VERSION)) {
			return false;
		}
	}
	return true;
}

void sacd_metabase_t::delete_tags(IXML_NodePtr node, const char* tag_type, bool is_linked) {
	auto list_tags{ node->GetchildNodes() };
	if (!list_tags) {
		return;
	}
	bool node_removed;
	do {
		node_removed = false;
		for (auto i = 0l; i < list_tags->Getlength(); i++) {
			auto node_tag{ list_tags->Getitem(i) };
			if (node_tag) {
				auto node_tag_type{ string8(node_tag->GetnodeName()) };
				if (node_tag_type.equals(tag_type)) {
					auto nodemap_attr{ node_tag->Getattributes() };
					auto tag_name{ string8(get_node_attribute(nodemap_attr, ATT_NAME)) };
					if (!is_linked || (is_linked && node_tag_type.equals(TAG_META) && is_linkable_tag(tag_name))) {
						node->removeChild(node_tag);
						node_removed = true;
						break;
					}
				}
			}
		}
	} while (node_removed);
}

void sacd_metabase_t::insert_tag(IXML_NodePtr node, const char* tag_type, const char* tag_name, const char* tag_value) {
	auto elem_tag{ xmldoc->createElement(tag_type) };
	if (elem_tag) {
		node->appendChild(elem_tag);
		set_node_attribute(xmldoc, elem_tag, ATT_NAME, tag_name);
		set_node_attribute(xmldoc, elem_tag, ATT_VALUE, tag_value);
	}
}

IXML_NodePtr sacd_metabase_t::get_node(const char* tag_type, const char* att_id, bool create) {
	string_formatter(xpath_track);
	xpath_track << TAG_ROOT << "/" << TAG_STORE << "[@" << ATT_ID << "='" << store_id << "']" << "/" << tag_type << "[@" << ATT_ID << "='" << att_id << "']";
	auto bstr_xpath{ CComVariant(string_wide_from_utf8(xpath_track)) };
	auto node_tag_type{ xmldoc->selectSingleNode(V_BSTR(&bstr_xpath)) };
	if (!node_tag_type && create) {
		node_tag_type = new_node(tag_type, att_id);
	}
	return node_tag_type;
}

IXML_NodePtr sacd_metabase_t::new_node(const char* tag_type, const char* att_id) {
	string_formatter(xpath_store);
	xpath_store << TAG_ROOT << "/" << TAG_STORE << "[@" << ATT_ID << "='" << store_id << "']";
	auto node_tag_type{ IXML_NodePtr() };
	auto bstr_xpath{ CComVariant(string_wide_from_utf8(xpath_store)) };
	auto node_store{ xmldoc->selectSingleNode(V_BSTR(&bstr_xpath)) };
	if (node_store) {
		auto elem_tag_type{ xmldoc->createElement(tag_type) };
		if (elem_tag_type) {
			set_node_attribute(xmldoc, elem_tag_type, ATT_ID, att_id);
			node_tag_type = node_store->appendChild(elem_tag_type);
		}
	}
	return node_tag_type;
}
