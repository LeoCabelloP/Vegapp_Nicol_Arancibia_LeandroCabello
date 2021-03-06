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

#include <dsd_processor_service.h>
#include "sacd_setup.h"
#include <atldlgs.h>

constexpr LPTSTR FIR_FILTER = _T("Installable FIR (*.txt)\0*.txt\0All Files (*.*)\0*.*\0\0");

static const GUID g_guid_cfg_output_mode = { 0xd3d21b14, 0x68d2, 0x437f, { 0xa9, 0xc6, 0x5b, 0xa7, 0xb1, 0xd6, 0xe, 0x54 } };
static cfg_int g_cfg_output_mode(g_guid_cfg_output_mode, 0);

static const GUID g_guid_dop_for_converter = { 0x724178ae, 0x62c2, 0x4595, { 0xb4, 0xf6, 0x55, 0xd7, 0x81, 0xa8, 0x82, 0xbe } };
static cfg_uint g_cfg_dop_for_converter(g_guid_dop_for_converter, BST_UNCHECKED);

static const GUID g_guid_cfg_volume_adjust = { 0x9c3dce45, 0x67c2, 0x4221, { 0xa9, 0x1, 0x53, 0x60, 0x44, 0x9e, 0xa, 0x24 } };
static cfg_int g_cfg_volume_adjust(g_guid_cfg_volume_adjust, 0);

static const GUID g_guid_cfg_lfe_adjust = { 0x9e613f1f, 0xa5db, 0x4765,{ 0xab, 0xa, 0xe2, 0xa7, 0x5, 0xec, 0x22, 0x8b } };
static cfg_int g_cfg_lfe_adjust(g_guid_cfg_lfe_adjust, 0);

static const GUID g_guid_cfg_log_overloads = { 0xfb56f73a, 0x3fbc, 0x420e, { 0xac, 0x28, 0x27, 0xfd, 0xc9, 0x14, 0xc9, 0x1 } };
static cfg_uint g_cfg_log_overloads(g_guid_cfg_log_overloads, BST_UNCHECKED);

static const GUID g_guid_cfg_samplerate = { 0xde915fec, 0x5557, 0x4746, { 0x87, 0xbc, 0x6a, 0x6f, 0x99, 0xfe, 0x67, 0x5 } };
static cfg_int g_cfg_samplerate(g_guid_cfg_samplerate, 0);

static const GUID g_guid_cfg_converter_mode = { 0x745d7ac3, 0x80c1, 0x41c1, { 0xb9, 0x8, 0xbd, 0x6, 0xd6, 0xa2, 0x87, 0x82 } };
static cfg_int g_cfg_converter_mode(g_guid_cfg_converter_mode, 0);

static const GUID g_guid_cfg_user_fir_name = { 0x35f5e4a6, 0x6cb0, 0x4e3e, { 0xb9, 0x6, 0xec, 0x46, 0xfd, 0x61, 0x24, 0x50 } };
static cfg_string g_cfg_user_fir_name(g_guid_cfg_user_fir_name, "");

static const GUID g_guid_cfg_user_fir_coef = { 0x813f1a8c, 0xdd29, 0x4087, { 0x8c, 0x49, 0xcc, 0x5b, 0x3e, 0xc5, 0xd1, 0xc6 } };
static cfg_objList<double> g_cfg_user_fir_coef(g_guid_cfg_user_fir_coef);

static const GUID g_guid_cfg_area = { 0xa93bec29, 0xc34b, 0x4973, { 0xb7, 0x85, 0xea, 0xc8, 0xf8, 0x4e, 0x1d, 0x83 } };
static cfg_int g_cfg_area(g_guid_cfg_area, 0);

static const GUID g_guid_cfg_editable_tags = { 0x480cf6d8, 0x3512, 0x49ae, { 0xb8, 0xef, 0x3f, 0xe1, 0x84, 0x9c, 0xd7, 0x0 } };
static cfg_uint g_cfg_editable_tags(g_guid_cfg_editable_tags, BST_UNCHECKED);

static const GUID g_guid_cfg_store_tags_with_iso = { 0x4e3f3dd0, 0x7407, 0x4115, { 0xbd, 0xe8, 0x2a, 0x48, 0x2e, 0x71, 0x26, 0x7f } };
static cfg_uint g_cfg_store_tags_with_iso(g_guid_cfg_store_tags_with_iso, BST_UNCHECKED);

static const GUID g_guid_cfg_linked_tags = { 0x94053a83, 0x4d15, 0x4d57,{ 0xa1, 0xa8, 0x83, 0x77, 0xe6, 0x61, 0x4f, 0xf3 } };
static cfg_uint g_cfg_linked_tags(g_guid_cfg_linked_tags, BST_UNCHECKED);

static const GUID g_guid_cfg_emaster = { 0xb9fc1273, 0x2c44, 0x4b00, { 0x93, 0xc3, 0x56, 0x79, 0x79, 0x1a, 0x3a, 0x5f } };
static cfg_uint g_cfg_emaster(g_guid_cfg_emaster, BST_UNCHECKED);

static const GUID g_guid_cfg_std_tags = { 0x174dbf2c, 0x33ed, 0x48b7, { 0xa0, 0xc8, 0x8f, 0x13, 0x88, 0xc0, 0xa5, 0x1a } };
static cfg_uint g_cfg_std_tags(g_guid_cfg_std_tags, BST_CHECKED);

static const GUID g_guid_cfg_dsddsp = { 0x2ec4ef87, 0x6890, 0x4e73, { 0x8e, 0x39, 0xd3, 0xb7, 0x6e, 0xb, 0xec, 0x68 } };
static cfg_guid g_cfg_dsddsp(g_guid_cfg_dsddsp, GUID_NULL);

static const GUID g_guid_cfg_trace = { 0x21260974, 0x5312, 0x4c3f,{ 0x85, 0x18, 0x32, 0x64, 0xca, 0x6f, 0x73, 0x37 } };
static cfg_uint g_cfg_trace(g_guid_cfg_trace, BST_UNCHECKED);

bool CSACDPreferences::use_dsd_path() {
	return (g_cfg_output_mode.get_value() == 1 || g_cfg_output_mode.get_value() == 2) ? true : false;
}

bool CSACDPreferences::use_pcm_path() {
	return (g_cfg_output_mode.get_value() == 0 || g_cfg_output_mode.get_value() == 2) ? true : false;
}

int CSACDPreferences::get_output_mode() {
	return g_cfg_output_mode.get_value();
}

bool CSACDPreferences::get_dop_for_converter() {
	return g_cfg_dop_for_converter == BST_CHECKED;
}

float CSACDPreferences::get_volume_adjust() {
	return (float)g_cfg_volume_adjust.get_value();
}

float CSACDPreferences::get_lfe_adjust() {
	switch (g_cfg_lfe_adjust.get_value()) {
	case 0:
		return -10.0f;
	case 1:
		return 0.0f;
	case 2:
		return +10.0f;
	default:
		return 0.0f;
	}
}

bool CSACDPreferences::get_log_overloads() {
	return g_cfg_log_overloads == BST_CHECKED;
}

int CSACDPreferences::get_samplerate() {
	switch (g_cfg_samplerate.get_value()) {
	case 0:
		return 44100;
	case 1:
		return 88200;
	case 2:
		return 176400;
	case 3:
		return 352800;
	}
	return 44100;
}

int CSACDPreferences::get_converter_mode() {
	return g_cfg_converter_mode.get_value();
}

cfg_objList<double>& CSACDPreferences::get_user_fir() {
	return g_cfg_user_fir_coef;
}

int CSACDPreferences::get_area() {
	return g_cfg_area.get_value();
}

bool CSACDPreferences::get_editable_tags() {
	return g_cfg_editable_tags == BST_CHECKED;
}

bool CSACDPreferences::get_store_tags_with_iso() {
	return g_cfg_store_tags_with_iso == BST_CHECKED;
}

bool CSACDPreferences::get_linked_tags() {
	return g_cfg_linked_tags == BST_CHECKED;
}

bool CSACDPreferences::get_emaster() {
	return g_cfg_emaster == BST_CHECKED;
}

bool CSACDPreferences::get_std_tags() {
	return g_cfg_std_tags == BST_CHECKED;
}

GUID CSACDPreferences::get_dsd_processor() {
	return g_cfg_dsddsp;
}

bool CSACDPreferences::g_get_trace() {
	return g_cfg_trace == BST_CHECKED;
}

CSACDPreferences::CSACDPreferences(preferences_page_callback::ptr callback) : m_callback(callback) {
}

t_uint32 CSACDPreferences::get_state() {
	t_uint32 state = preferences_state::resettable;
	if (HasChanged()) {
		state |= preferences_state::changed;
	}
	return state;
}

void CSACDPreferences::apply() {
	g_cfg_output_mode = SendDlgItemMessage(IDC_OUTPUT_MODE_COMBO, CB_GETCURSEL, 0, 0);
	g_cfg_dop_for_converter = SendDlgItemMessage(IDC_DOP_FOR_CONVERTER, BM_GETCHECK, 0, 0);
	g_cfg_volume_adjust = SendDlgItemMessage(IDC_VOLUME_ADJ_COMBO, CB_GETCURSEL, 0, 0);
	g_cfg_lfe_adjust = SendDlgItemMessage(IDC_LFE_ADJ_COMBO, CB_GETCURSEL, 0, 0);
	g_cfg_log_overloads = SendDlgItemMessage(IDC_LOG_OVERLOADS, BM_GETCHECK, 0, 0);
	g_cfg_samplerate = SendDlgItemMessage(IDC_SAMPLERATE_COMBO, CB_GETCURSEL, 0, 0);
	g_cfg_converter_mode = SendDlgItemMessage(IDC_CONVERTER_MODE_COMBO, CB_GETCURSEL, 0, 0);
	g_cfg_area = SendDlgItemMessage(IDC_AREA_COMBO, CB_GETCURSEL, 0, 0);
	g_cfg_editable_tags = SendDlgItemMessage(IDC_EDITABLE_TAGS, BM_GETCHECK, 0, 0);
	g_cfg_store_tags_with_iso = SendDlgItemMessage(IDC_STORE_TAGS_WITH_ISO, BM_GETCHECK, 0, 0);
	g_cfg_linked_tags = SendDlgItemMessage(IDC_LINKED_TAGS, BM_GETCHECK, 0, 0);
	g_cfg_emaster = SendDlgItemMessage(IDC_EMASTER, BM_GETCHECK, 0, 0);
	g_cfg_std_tags = SendDlgItemMessage(IDC_STD_TAGS, BM_GETCHECK, 0, 0);
	g_cfg_dsddsp = get_dsddsp_guid_by_item(SendDlgItemMessage(IDC_DSD_PROCESSOR_COMBO, CB_GETCURSEL, 0, 0));
	g_cfg_trace = SendDlgItemMessage(IDC_TRACE, BM_GETCHECK, 0, 0);
	OnChanged();
}

void CSACDPreferences::reset() {
	g_cfg_output_mode = 0;
	SendDlgItemMessage(IDC_OUTPUT_MODE_COMBO, CB_SETCURSEL, g_cfg_output_mode, 0);
	g_cfg_dop_for_converter = BST_UNCHECKED;
	SendDlgItemMessage(IDC_DOP_FOR_CONVERTER, BM_SETCHECK, g_cfg_dop_for_converter, 0);
	g_cfg_volume_adjust = 6;
	SendDlgItemMessage(IDC_VOLUME_ADJ_COMBO, CB_SETCURSEL, g_cfg_volume_adjust, 0);
	g_cfg_lfe_adjust = 1;
	SendDlgItemMessage(IDC_LFE_ADJ_COMBO, CB_SETCURSEL, g_cfg_lfe_adjust, 0);
	g_cfg_log_overloads = BST_UNCHECKED;
	SendDlgItemMessage(IDC_LOG_OVERLOADS, BM_SETCHECK, g_cfg_log_overloads, 0);
	g_cfg_samplerate = 0;
	SendDlgItemMessage(IDC_SAMPLERATE_COMBO, CB_SETCURSEL, g_cfg_samplerate, 0);
	g_cfg_converter_mode = 0;
	SendDlgItemMessage(IDC_CONVERTER_MODE_COMBO, CB_SETCURSEL, g_cfg_converter_mode, 0);
	g_cfg_user_fir_name.reset();
	g_cfg_user_fir_coef.remove_all();
	GetDlgItem(IDC_LOAD_FIR_BUTTON).EnableWindow(FALSE);
	GetDlgItem(IDC_SAVE_FIR_BUTTON).EnableWindow(FALSE);
	SendDlgItemMessage(IDC_INSTALLABLE_FILTER_TEXT, WM_SETTEXT, 0, (LPARAM)0);
	SetPcmControls();
	g_cfg_area = 0;
	SendDlgItemMessage(IDC_AREA_COMBO, CB_SETCURSEL, g_cfg_area, 0);
	g_cfg_editable_tags = BST_UNCHECKED;
	SendDlgItemMessage(IDC_EDITABLE_TAGS, BM_SETCHECK, g_cfg_editable_tags, 0);
	GetDlgItem(IDC_STORE_TAGS_WITH_ISO).EnableWindow(g_cfg_editable_tags != 0);
	g_cfg_store_tags_with_iso = BST_UNCHECKED;
	SendDlgItemMessage(IDC_STORE_TAGS_WITH_ISO, BM_SETCHECK, g_cfg_store_tags_with_iso, 0);
	GetDlgItem(IDC_LINKED_TAGS).EnableWindow(g_cfg_editable_tags != 0);
	g_cfg_linked_tags = BST_UNCHECKED;
	SendDlgItemMessage(IDC_LINKED_TAGS, BM_SETCHECK, g_cfg_linked_tags, 0);
	g_cfg_emaster = BST_UNCHECKED;
	SendDlgItemMessage(IDC_EMASTER, BM_SETCHECK, g_cfg_emaster, 0);
	g_cfg_std_tags = BST_CHECKED;
	SendDlgItemMessage(IDC_STD_TAGS, BM_SETCHECK, g_cfg_std_tags, 0);
	g_cfg_dsddsp = GUID_NULL;
	SendDlgItemMessage(IDC_DSD_PROCESSOR_COMBO, CB_SETCURSEL, get_dsddsp_item_by_guid(g_cfg_dsddsp.get_value()), 0);
	g_cfg_trace = BST_UNCHECKED;
	SendDlgItemMessage(IDC_TRACE, BM_SETCHECK, g_cfg_trace, 0);
	OnChanged();
}

void CSACDPreferences::translate_string(const char* p_inp, array_t<TCHAR>& p_out) {
	if (p_inp) {
		string8 str = string8(p_inp);
		p_out.set_count(str.get_length() + 1);
#ifdef UNICODE
		convert_utf8_to_wide(p_out.get_ptr(), p_out.get_count(), str.get_ptr(), str.get_length());
#else
		convert_utf8_to_ascii(p_out.get_ptr(), p_out.get_count(), str.get_ptr(), str.get_length());
#endif
	}
	else {
		p_out.set_count(0);
	}
}

t_size CSACDPreferences::get_dsddsp_item_by_guid(GUID p_guid) {
	for (t_size item = 0; item < m_dsddsps.get_count(); item++) {
		if (m_dsddsps[item].m_guid == p_guid) {
			return item;
		}
	}
	return 0;
}

GUID CSACDPreferences::get_dsddsp_guid_by_item(t_size p_item) {
	if (p_item < m_dsddsps.get_count()) {
		return m_dsddsps[p_item].m_guid;
	}
	return GUID_NULL;
}

BOOL CSACDPreferences::OnInitDialog(CWindow, LPARAM) {
	GetOutputModeList();
	SendDlgItemMessage(IDC_DOP_FOR_CONVERTER, BM_SETCHECK, g_cfg_dop_for_converter, 0);
	GetVolumeList();
	GetLFEAdjustList();
	SendDlgItemMessage(IDC_LOG_OVERLOADS, BM_SETCHECK, g_cfg_log_overloads, 0);
	GetSamplerateList();
	GetConverterModeList();
	SetPcmControls();
	GetAreaList();
	GetDSDDSPList();
	SendDlgItemMessage(IDC_EDITABLE_TAGS, BM_SETCHECK, g_cfg_editable_tags, 0);
	GetDlgItem(IDC_STORE_TAGS_WITH_ISO).EnableWindow(SendDlgItemMessage(IDC_EDITABLE_TAGS, BM_GETCHECK, 0, 0));
	GetDlgItem(IDC_LINKED_TAGS).EnableWindow(SendDlgItemMessage(IDC_EDITABLE_TAGS, BM_GETCHECK, 0, 0));
	SendDlgItemMessage(IDC_STORE_TAGS_WITH_ISO, BM_SETCHECK, g_cfg_store_tags_with_iso, 0);
	SendDlgItemMessage(IDC_LINKED_TAGS, BM_SETCHECK, g_cfg_linked_tags, 0);
	SendDlgItemMessage(IDC_EMASTER, BM_SETCHECK, g_cfg_emaster, 0);
	SendDlgItemMessage(IDC_STD_TAGS, BM_SETCHECK, g_cfg_std_tags, 0);
	SendDlgItemMessage(IDC_TRACE, BM_SETCHECK, g_cfg_trace, 0);
	return FALSE;
}

void CSACDPreferences::OnOutputModeChange(UINT, int, CWindow) {
	SetPcmControls();
	OnChanged();
}

void CSACDPreferences::OnDoPForConverterClicked(UINT, int, CWindow) {
	OnChanged();
}

void CSACDPreferences::OnVolumeAdjustChange(UINT, int, CWindow) {
	OnChanged();
}

void CSACDPreferences::OnLFEAdjustChange(UINT, int, CWindow) {
	OnChanged();
}

void CSACDPreferences::OnDeClickerClicked(UINT, int, CWindow) {
	OnChanged();
}

void CSACDPreferences::OnLogOverloadsClicked(UINT, int, CWindow) {
	OnChanged();
}

void CSACDPreferences::OnSamplerateChange(UINT, int, CWindow) {
	OnChanged();
}

void CSACDPreferences::OnConverterModeChange(UINT, int, CWindow) {
	SetUserFirState();
	OnChanged();
}

void CSACDPreferences::OnLoadFirClicked(UINT, int, CWindow) {
	CFileDialog dlg(TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_FILEMUSTEXIST, FIR_FILTER);
	if (dlg.DoModal(dlg) == IDOK) {
		bool fir_name_is_read = false;
		FILE* fir;
		if (_tfopen_s(&fir, dlg.m_szFileName, _T("r")) == 0) {
			g_cfg_user_fir_name.reset();
			g_cfg_user_fir_coef.remove_all();
			while (!feof(fir)) {
				double coef;
				TCHAR str[4096];
				if (_fgetts(str, sizeof(str) / sizeof(*str), fir)) {
					if (_tcslen(str) > 0) {
						if (str[0] == _T('#')) {
							if (_tcslen(str) > 1 && !fir_name_is_read) {
								string_utf8_from_wide(su);
								su = &str[1];
								g_cfg_user_fir_name.set_string(su);
								while (g_cfg_user_fir_name.get_length() > 0 && g_cfg_user_fir_name.get_ptr()[0] == ' ') {
									g_cfg_user_fir_name.remove_chars(0, 1);
								}
								fir_name_is_read = true;
							}
						}
						else {
							if (_stscanf_s(str, _T("%lg"), &coef) == 1) {
								g_cfg_user_fir_coef.add_item(coef);
							}
						}
					}
				}
			}
			fclose(fir);
		}
		string_wide_from_utf8(filter_description);
		if (fir_name_is_read) {
			filter_description = g_cfg_user_fir_name.get_ptr();
		}
		else {
			string_utf8_from_wide(file_path);
			file_path = dlg.m_szFileName;
			string_filename_ext filename_ext(file_path);
			g_cfg_user_fir_name.set_string(filename_ext);
		}
		filter_description = g_cfg_user_fir_name.get_ptr();
		SendDlgItemMessage(IDC_INSTALLABLE_FILTER_TEXT, WM_SETTEXT, 0, (LPARAM)filter_description.get_ptr());
	}
	OnChanged();
}

void CSACDPreferences::OnSaveFirClicked(UINT, int, CWindow) {
	CFileDialog dlg(FALSE, _T("txt"), _T("untitledfir"), OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, FIR_FILTER);
	if (dlg.DoModal(dlg) == IDOK) {
		FILE* fir;
		if (_tfopen_s(&fir, dlg.m_szFileName, _T("w")) == 0) {
			double* fir_coefs  = g_cfg_user_fir_coef.get_ptr();
			int     fir_length = g_cfg_user_fir_coef.get_size();
			if (g_cfg_user_fir_name.get_length() > 0) {
				_ftprintf(fir, _T("# %S\n"), g_cfg_user_fir_name.get_ptr());
			}
			for (int i = 0; i < fir_length; i++) {
				_ftprintf(fir, _T("%.16lg\n"), fir_coefs[i]);
			}
			fclose(fir);
		}
	}
	OnChanged();
}

void CSACDPreferences::OnAreaChange(UINT, int, CWindow) {
	OnChanged();
}

void CSACDPreferences::OnEditableTagsClicked(UINT, int, CWindow) {
	GetDlgItem(IDC_STORE_TAGS_WITH_ISO).EnableWindow(SendDlgItemMessage(IDC_EDITABLE_TAGS, BM_GETCHECK, 0, 0));
	GetDlgItem(IDC_LINKED_TAGS).EnableWindow(SendDlgItemMessage(IDC_EDITABLE_TAGS, BM_GETCHECK, 0, 0));
	OnChanged();
}

void CSACDPreferences::OnStoreWithIsoClicked(UINT, int, CWindow) {
	OnChanged();
}

void CSACDPreferences::OnLinkedTagsClicked(UINT, int, CWindow) {
	OnChanged();
}

void CSACDPreferences::OnEMasterClicked(UINT, int, CWindow) {
	OnChanged();
}

void CSACDPreferences::OnStdTagsClicked(UINT, int, CWindow) {
	OnChanged();
}

void CSACDPreferences::OnDSDDSPChange(UINT, int, CWindow) {
	OnChanged();
}

void CSACDPreferences::OnTraceClicked(UINT, int, CWindow) {
	OnChanged();
}

bool CSACDPreferences::HasChanged() {
	if (g_cfg_output_mode.get_value() != SendDlgItemMessage(IDC_OUTPUT_MODE_COMBO, CB_GETCURSEL, 0, 0)) {
		return true;
	}
	if (g_cfg_dop_for_converter.get_value() != SendDlgItemMessage(IDC_DOP_FOR_CONVERTER, BM_GETCHECK, 0, 0)) {
		return true;
	}
	if (g_cfg_volume_adjust.get_value() != SendDlgItemMessage(IDC_VOLUME_ADJ_COMBO, CB_GETCURSEL, 0, 0)) {
		return true;
	}
	if (g_cfg_lfe_adjust.get_value() != SendDlgItemMessage(IDC_LFE_ADJ_COMBO, CB_GETCURSEL, 0, 0)) {
		return true;
	}
	if (g_cfg_log_overloads.get_value() != SendDlgItemMessage(IDC_LOG_OVERLOADS, BM_GETCHECK, 0, 0)) {
		return true;
	}
	if (g_cfg_samplerate.get_value() != SendDlgItemMessage(IDC_SAMPLERATE_COMBO, CB_GETCURSEL, 0, 0)) {
		return true;
	}
	if (g_cfg_converter_mode.get_value() != SendDlgItemMessage(IDC_CONVERTER_MODE_COMBO, CB_GETCURSEL, 0, 0)) {
		return true;
	}
	if (g_cfg_area.get_value() != SendDlgItemMessage(IDC_AREA_COMBO, CB_GETCURSEL, 0, 0)) {
		return true;
	}
	if (g_cfg_editable_tags.get_value() != SendDlgItemMessage(IDC_EDITABLE_TAGS, BM_GETCHECK, 0, 0)) {
		return true;
	}
	if (g_cfg_store_tags_with_iso.get_value() != SendDlgItemMessage(IDC_STORE_TAGS_WITH_ISO, BM_GETCHECK, 0, 0)) {
		return true;
	}
	if (g_cfg_linked_tags.get_value() != SendDlgItemMessage(IDC_LINKED_TAGS, BM_GETCHECK, 0, 0)) {
		return true;
	}
	if (g_cfg_emaster.get_value() != SendDlgItemMessage(IDC_EMASTER, BM_GETCHECK, 0, 0)) {
		return true;
	}
	if (g_cfg_std_tags.get_value() != SendDlgItemMessage(IDC_STD_TAGS, BM_GETCHECK, 0, 0)) {
		return true;
	}
	if (g_cfg_dsddsp.get_value() != get_dsddsp_guid_by_item(SendDlgItemMessage(IDC_DSD_PROCESSOR_COMBO, CB_GETCURSEL, 0, 0))) {
		return true;
	}
	if (g_cfg_trace.get_value() != SendDlgItemMessage(IDC_TRACE, BM_GETCHECK, 0, 0)) {
		return true;
	}
	return false;
}

void CSACDPreferences::OnChanged() {
	m_callback->on_state_changed();
}

void CSACDPreferences::GetOutputModeList() {
	SendDlgItemMessage(IDC_OUTPUT_MODE_COMBO, CB_ADDSTRING, 0, (LPARAM)_T("PCM"));
	SendDlgItemMessage(IDC_OUTPUT_MODE_COMBO, CB_ADDSTRING, 0, (LPARAM)_T("DSD"));
	SendDlgItemMessage(IDC_OUTPUT_MODE_COMBO, CB_ADDSTRING, 0, (LPARAM)_T("DSD+PCM"));
	SendDlgItemMessage(IDC_OUTPUT_MODE_COMBO, CB_SETCURSEL, g_cfg_output_mode.get_value(), 0);
}

void CSACDPreferences::GetVolumeList() {
	SendDlgItemMessage(IDC_VOLUME_ADJ_COMBO, CB_ADDSTRING, 0, (LPARAM)_T("0dB"));
	SendDlgItemMessage(IDC_VOLUME_ADJ_COMBO, CB_ADDSTRING, 0, (LPARAM)_T("+1dB"));
	SendDlgItemMessage(IDC_VOLUME_ADJ_COMBO, CB_ADDSTRING, 0, (LPARAM)_T("+2dB"));
	SendDlgItemMessage(IDC_VOLUME_ADJ_COMBO, CB_ADDSTRING, 0, (LPARAM)_T("+3dB"));
	SendDlgItemMessage(IDC_VOLUME_ADJ_COMBO, CB_ADDSTRING, 0, (LPARAM)_T("+4dB"));
	SendDlgItemMessage(IDC_VOLUME_ADJ_COMBO, CB_ADDSTRING, 0, (LPARAM)_T("+5dB"));
	SendDlgItemMessage(IDC_VOLUME_ADJ_COMBO, CB_ADDSTRING, 0, (LPARAM)_T("+6dB"));
	SendDlgItemMessage(IDC_VOLUME_ADJ_COMBO, CB_SETCURSEL, g_cfg_volume_adjust.get_value(), 0);
}

void CSACDPreferences::GetLFEAdjustList() {
	SendDlgItemMessage(IDC_LFE_ADJ_COMBO, CB_ADDSTRING, 0, (LPARAM)_T("LFE-10dB"));
	SendDlgItemMessage(IDC_LFE_ADJ_COMBO, CB_ADDSTRING, 0, (LPARAM)_T("LFE as-is"));
	SendDlgItemMessage(IDC_LFE_ADJ_COMBO, CB_ADDSTRING, 0, (LPARAM)_T("LFE+10dB"));
	SendDlgItemMessage(IDC_LFE_ADJ_COMBO, CB_SETCURSEL, g_cfg_lfe_adjust.get_value(), 0);
}

void CSACDPreferences::GetSamplerateList() {
	SendDlgItemMessage(IDC_SAMPLERATE_COMBO, CB_ADDSTRING, 0, (LPARAM)_T("44100"));
	SendDlgItemMessage(IDC_SAMPLERATE_COMBO, CB_ADDSTRING, 0, (LPARAM)_T("88200"));
	SendDlgItemMessage(IDC_SAMPLERATE_COMBO, CB_ADDSTRING, 0, (LPARAM)_T("176400"));
	SendDlgItemMessage(IDC_SAMPLERATE_COMBO, CB_ADDSTRING, 0, (LPARAM)_T("352800"));
	SendDlgItemMessage(IDC_SAMPLERATE_COMBO, CB_SETCURSEL, g_cfg_samplerate.get_value(), 0);
}

void CSACDPreferences::GetConverterModeList() {
	SendDlgItemMessage(IDC_CONVERTER_MODE_COMBO, CB_ADDSTRING, 0, (LPARAM)_T("Multistage (32fp)"));
	SendDlgItemMessage(IDC_CONVERTER_MODE_COMBO, CB_ADDSTRING, 0, (LPARAM)_T("Multistage (64fp)"));
	SendDlgItemMessage(IDC_CONVERTER_MODE_COMBO, CB_ADDSTRING, 0, (LPARAM)_T("Direct (32fp, 30kHz lowpass)"));
	SendDlgItemMessage(IDC_CONVERTER_MODE_COMBO, CB_ADDSTRING, 0, (LPARAM)_T("Direct (64fp, 30kHz lowpass)"));
	SendDlgItemMessage(IDC_CONVERTER_MODE_COMBO, CB_ADDSTRING, 0, (LPARAM)_T("Installable FIR (32fp)"));
	SendDlgItemMessage(IDC_CONVERTER_MODE_COMBO, CB_ADDSTRING, 0, (LPARAM)_T("Installable FIR (64fp)"));
	SendDlgItemMessage(IDC_CONVERTER_MODE_COMBO, CB_SETCURSEL, g_cfg_converter_mode.get_value(), 0);
	SetUserFirState();
}

void CSACDPreferences::GetAreaList() {
	SendDlgItemMessage(IDC_AREA_COMBO, CB_ADDSTRING, 0, (LPARAM)_T("None"));
	SendDlgItemMessage(IDC_AREA_COMBO, CB_ADDSTRING, 0, (LPARAM)_T("Stereo"));
	SendDlgItemMessage(IDC_AREA_COMBO, CB_ADDSTRING, 0, (LPARAM)_T("Multichannel"));
	SendDlgItemMessage(IDC_AREA_COMBO, CB_SETCURSEL, g_cfg_area.get_value(), 0);
}

void CSACDPreferences::GetDSDDSPList() {
	service_enum_t<dsd_processor_service> e_dsddsp;
	service_ptr_t<dsd_processor_service> dsddsp;
	m_dsddsps.append_single(dsddsp_t(GUID_NULL, "None"));
	while (e_dsddsp.next(dsddsp)) {
		m_dsddsps.append_single(dsddsp_t(dsddsp->get_guid(), dsddsp->get_name()));
	}
	for (t_size i = 0; i < m_dsddsps.get_count(); i++) {
		array_t<TCHAR> dsddsp_name;
		translate_string(m_dsddsps[i].m_name.get_ptr(), dsddsp_name);
		SendDlgItemMessage(IDC_DSD_PROCESSOR_COMBO, CB_ADDSTRING, 0, (LPARAM)dsddsp_name.get_ptr());
	}
	SendDlgItemMessage(IDC_DSD_PROCESSOR_COMBO, CB_SETCURSEL, get_dsddsp_item_by_guid(g_cfg_dsddsp.get_value()), 0);
}

void CSACDPreferences::SetPcmControls() {
	int output_mode = SendDlgItemMessage(IDC_OUTPUT_MODE_COMBO, CB_GETCURSEL, 0, 0);
	BOOL enabled = (output_mode == 0 || output_mode == 2) ? TRUE : FALSE;
	GetDlgItem(IDC_VOLUME_ADJ_COMBO).EnableWindow(enabled);
	GetDlgItem(IDC_LFE_ADJ_COMBO).EnableWindow(enabled);
	GetDlgItem(IDC_LOG_OVERLOADS).EnableWindow(enabled);
	GetDlgItem(IDC_SAMPLERATE_COMBO).EnableWindow(enabled);
	GetDlgItem(IDC_CONVERTER_MODE_COMBO).EnableWindow(enabled);
	if (enabled) {
		SetUserFirState();
	}
	else {
		GetDlgItem(IDC_LOAD_FIR_BUTTON).EnableWindow(FALSE);
		GetDlgItem(IDC_SAVE_FIR_BUTTON).EnableWindow(FALSE);
	}
}

void CSACDPreferences::SetUserFirState() {
	string_wide_from_utf8(sw);
	switch (SendDlgItemMessage(IDC_CONVERTER_MODE_COMBO, CB_GETCURSEL, 0, 0)) {
	case 4:
	case 5:
		GetDlgItem(IDC_LOAD_FIR_BUTTON).EnableWindow(TRUE);
		GetDlgItem(IDC_SAVE_FIR_BUTTON).EnableWindow(TRUE);
		sw = g_cfg_user_fir_name.get_ptr();
		SendDlgItemMessage(IDC_INSTALLABLE_FILTER_TEXT, WM_SETTEXT, 0, (LPARAM)sw.get_ptr());
		break;
	default:
		GetDlgItem(IDC_LOAD_FIR_BUTTON).EnableWindow(FALSE);
		GetDlgItem(IDC_SAVE_FIR_BUTTON).EnableWindow(FALSE);
		SendDlgItemMessage(IDC_INSTALLABLE_FILTER_TEXT, WM_SETTEXT, 0, (LPARAM)0);
		break;
	}
}

const char* preferences_page_sacd_t::get_name() {
	return "SACD";
}

GUID preferences_page_sacd_t::get_guid() {
	return GUID({ 0x532b5e38, 0x267, 0x4a2d,{ 0xa7, 0x89, 0xb0, 0x88, 0x99, 0xd2, 0xb1, 0x48 } });
}

GUID preferences_page_sacd_t::get_parent_guid() {
	return guid_tools;
}

static preferences_page_factory_t<preferences_page_sacd_t> g_preferences_page_sacd_factory;
