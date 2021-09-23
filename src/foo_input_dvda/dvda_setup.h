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

#ifndef _DVDA_SETUP_H_INCLUDED
#define _DVDA_SETUP_H_INCLUDED

#include <helpers/foobar2000+atl.h>
#include <helpers/atl-misc.h>
#include "resource.h"

static const GUID g_guid_cfg_chmode = { 0x3183b385, 0xf994, 0x4f08, { 0x85, 0xe3, 0x5a, 0xe2, 0xbd, 0xa1, 0xd5, 0x2c } };
static cfg_int g_cfg_chmode(g_guid_cfg_chmode, 0);

static const GUID g_guid_cfg_no_downmixes = {0x4268c62d, 0x48a2, 0x4438, {0xb7, 0x9, 0xea, 0x62, 0x8, 0x8e, 0xaf, 0x3d}};
static cfg_uint g_cfg_no_downmixes(g_guid_cfg_no_downmixes, BST_UNCHECKED);

static const GUID g_guid_cfg_no_short_tracks = {0x4f444d0d, 0xb8fd, 0x44ba, {0xa9, 0x17, 0xa7, 0x5a, 0x25, 0x7, 0x1e, 0x33}};
static cfg_uint g_cfg_no_short_tracks(g_guid_cfg_no_short_tracks, BST_UNCHECKED);

static const GUID g_guid_cfg_no_untagged_tracks = { 0x633345b, 0x5cdb, 0x4c5a, { 0x99, 0xc0, 0x69, 0xab, 0x95, 0xc, 0x83, 0x9c } };
static cfg_uint g_cfg_no_untagged_tracks(g_guid_cfg_no_untagged_tracks, BST_UNCHECKED);

static const GUID g_guid_cfg_store_tags_with_iso = { 0x4d450dc1, 0x4438, 0x4bea, { 0x9e, 0xff, 0xfb, 0x3d, 0xf0, 0x4e, 0xf6, 0x3b } };
static cfg_uint g_cfg_store_tags_with_iso(g_guid_cfg_store_tags_with_iso, BST_UNCHECKED);

class CDVDAPreferences : public CDialogImpl<CDVDAPreferences>, public preferences_page_instance {
public:
	static int get_chmode() {
		return g_cfg_chmode.get_value();
	}

	static bool get_no_downmixes() {
		return g_cfg_no_downmixes == BST_CHECKED;
	}

	static bool get_no_short_tracks() {
		return g_cfg_no_short_tracks == BST_CHECKED;
	}

	static bool get_no_untagged_tracks() {
		return g_cfg_no_untagged_tracks == BST_CHECKED;
	}

	static bool get_store_tags_with_iso() {
		return g_cfg_store_tags_with_iso == BST_CHECKED;
	}

	CDVDAPreferences(preferences_page_callback::ptr callback) : m_callback(callback) {
	}

	enum {IDD = IDD_DVDAPREFERENCES};

	t_uint32 get_state() {
		t_uint32 state = preferences_state::resettable;
		if (HasChanged())
			state |= preferences_state::changed;
		return state;
	}

	void apply() {
		g_cfg_chmode = SendDlgItemMessage(IDC_CHMODE, CB_GETCURSEL, 0, 0);
		g_cfg_no_downmixes = SendDlgItemMessage(IDC_NO_DOWNMIXES, BM_GETCHECK, 0, 0);
		g_cfg_no_short_tracks = SendDlgItemMessage(IDC_NO_SHORT_TRACKS, BM_GETCHECK, 0, 0);
		g_cfg_no_untagged_tracks = SendDlgItemMessage(IDC_NO_UNTAGGED_TRACKS, BM_GETCHECK, 0, 0);
		g_cfg_store_tags_with_iso = SendDlgItemMessage(IDC_STORE_TAGS_WITH_ISO, BM_GETCHECK, 0, 0);
		OnChanged();
	}

	void reset() {
		SendDlgItemMessage(IDC_CHMODE, CB_SETCURSEL, 0, 0);
		SendDlgItemMessage(IDC_NO_DOWNMIXES, BM_SETCHECK, BST_UNCHECKED, 0);
		SendDlgItemMessage(IDC_NO_SHORT_TRACKS, BM_SETCHECK, BST_UNCHECKED, 0);
		SendDlgItemMessage(IDC_NO_UNTAGGED_TRACKS, BM_SETCHECK, BST_UNCHECKED, 0);
		SendDlgItemMessage(IDC_STORE_TAGS_WITH_ISO, BM_SETCHECK, BST_UNCHECKED, 0);
		OnChanged();
	}

	BEGIN_MSG_MAP(CDVDAPreferences)
		MSG_WM_INITDIALOG(OnInitDialog)
		COMMAND_HANDLER_EX(IDC_CHMODE, CBN_SELCHANGE, OnPropertyChange)
		COMMAND_HANDLER_EX(IDC_NO_DOWNMIXES, BN_CLICKED, OnPropertyChange)
		COMMAND_HANDLER_EX(IDC_NO_SHORT_TRACKS, BN_CLICKED, OnPropertyChange)
		COMMAND_HANDLER_EX(IDC_NO_UNTAGGED_TRACKS, BN_CLICKED, OnPropertyChange)
		COMMAND_HANDLER_EX(IDC_STORE_TAGS_WITH_ISO, BN_CLICKED, OnPropertyChange)
	END_MSG_MAP()

private:
	BOOL OnInitDialog(CWindow, LPARAM) {
		GetChModeList();
		SendDlgItemMessage(IDC_NO_DOWNMIXES, BM_SETCHECK, g_cfg_no_downmixes, 0);
		SendDlgItemMessage(IDC_NO_SHORT_TRACKS, BM_SETCHECK, g_cfg_no_short_tracks, 0);
		SendDlgItemMessage(IDC_NO_UNTAGGED_TRACKS, BM_SETCHECK, g_cfg_no_untagged_tracks, 0);
		SendDlgItemMessage(IDC_STORE_TAGS_WITH_ISO, BM_SETCHECK, g_cfg_store_tags_with_iso, 0);
		return TRUE;
	}

	void OnPropertyChange(UINT, int, CWindow) {
		OnChanged();
	}

	bool HasChanged() {
		return
			SendDlgItemMessage(IDC_CHMODE, CB_GETCURSEL, 0, 0) != g_cfg_chmode ||
			SendDlgItemMessage(IDC_NO_DOWNMIXES, BM_GETCHECK, 0, 0) != g_cfg_no_downmixes ||
			SendDlgItemMessage(IDC_NO_SHORT_TRACKS, BM_GETCHECK, 0, 0) != g_cfg_no_short_tracks ||
			SendDlgItemMessage(IDC_NO_UNTAGGED_TRACKS, BM_GETCHECK, 0, 0) != g_cfg_no_untagged_tracks ||
			SendDlgItemMessage(IDC_STORE_TAGS_WITH_ISO, BM_GETCHECK, 0, 0) != g_cfg_store_tags_with_iso;
	}

	void OnChanged() {
		m_callback->on_state_changed();
	}

	void GetChModeList() {
		SendDlgItemMessage(IDC_CHMODE, CB_ADDSTRING, 0, (LPARAM)_T("None"));
		SendDlgItemMessage(IDC_CHMODE, CB_ADDSTRING, 0, (LPARAM)_T("Stereo"));
		SendDlgItemMessage(IDC_CHMODE, CB_ADDSTRING, 0, (LPARAM)_T("Multichannel"));
		SendDlgItemMessage(IDC_CHMODE, CB_SETCURSEL, g_cfg_chmode.get_value(), 0);
	}

	const preferences_page_callback::ptr m_callback;
};

class preferences_page_dvda_t : public preferences_page_impl<CDVDAPreferences> {
public:
	const char* get_name() {
		return "DVD-Audio";
	}
	GUID get_guid() {
		return GUID({ 0xe29b38e5, 0x5ecd, 0x4c85, {0x95, 0x30, 0x7f, 0x61, 0x39, 0x10, 0x20, 0xd} });;
	}
	GUID get_parent_guid() {
		return guid_tools;
	}
};

static preferences_page_factory_t<preferences_page_dvda_t> g_preferences_page_dvda_factory;

#endif
