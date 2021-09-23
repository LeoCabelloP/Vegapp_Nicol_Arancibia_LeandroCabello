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

#ifndef _SACD_SETUP_H_INCLUDED
#define _SACD_SETUP_H_INCLUDED

#include <helpers/foobar2000+atl.h>
#include <helpers/atl-misc.h>
#include "resource.h"

#include "sacd_config.h"

class CSACDPreferences : public CDialogImpl<CSACDPreferences>, public preferences_page_instance {
public:
	static bool use_dsd_path();
	static bool use_pcm_path();
	static int get_output_mode();
	static bool get_dop_for_converter();
	static float get_volume_adjust();
	static float get_lfe_adjust();
	static bool get_log_overloads();
	static int get_samplerate();
	static int get_converter_mode();
	static cfg_objList<double>& get_user_fir();
	static int get_area();
	static bool get_editable_tags();
	static bool get_store_tags_with_iso();
	static bool get_linked_tags();
	static bool get_emaster();
	static bool get_std_tags();
	static GUID get_dsd_processor();
	static bool g_get_trace();
	CSACDPreferences(preferences_page_callback::ptr callback);

	enum {IDD = IDD_SACD_PREFERENCES};

	t_uint32 get_state();
	void apply();
	void reset();

	BEGIN_MSG_MAP(CSACDPreferences)
		MSG_WM_INITDIALOG(OnInitDialog)
		COMMAND_HANDLER_EX(IDC_OUTPUT_MODE_COMBO, CBN_SELCHANGE, OnOutputModeChange)
		COMMAND_HANDLER_EX(IDC_DOP_FOR_CONVERTER, BN_CLICKED, OnDoPForConverterClicked)
		COMMAND_HANDLER_EX(IDC_VOLUME_ADJ_COMBO, CBN_SELCHANGE, OnVolumeAdjustChange)
		COMMAND_HANDLER_EX(IDC_LFE_ADJ_COMBO, CBN_SELCHANGE, OnLFEAdjustChange)
		COMMAND_HANDLER_EX(IDC_LOG_OVERLOADS, BN_CLICKED, OnLogOverloadsClicked)
		COMMAND_HANDLER_EX(IDC_SAMPLERATE_COMBO, CBN_SELCHANGE, OnSamplerateChange)
		COMMAND_HANDLER_EX(IDC_CONVERTER_MODE_COMBO, CBN_SELCHANGE, OnConverterModeChange)
		COMMAND_HANDLER_EX(IDC_LOAD_FIR_BUTTON, BN_CLICKED, OnLoadFirClicked)
		COMMAND_HANDLER_EX(IDC_SAVE_FIR_BUTTON, BN_CLICKED, OnSaveFirClicked)
		COMMAND_HANDLER_EX(IDC_AREA_COMBO, CBN_SELCHANGE, OnAreaChange)
		COMMAND_HANDLER_EX(IDC_EDITABLE_TAGS, BN_CLICKED, OnEditableTagsClicked)
		COMMAND_HANDLER_EX(IDC_STORE_TAGS_WITH_ISO, BN_CLICKED, OnStoreWithIsoClicked)
		COMMAND_HANDLER_EX(IDC_LINKED_TAGS, BN_CLICKED, OnLinkedTagsClicked)
		COMMAND_HANDLER_EX(IDC_EMASTER, BN_CLICKED, OnEMasterClicked)
		COMMAND_HANDLER_EX(IDC_STD_TAGS, BN_CLICKED, OnStdTagsClicked)
		COMMAND_HANDLER_EX(IDC_DSD_PROCESSOR_COMBO, CBN_SELCHANGE, OnDSDDSPChange)
		COMMAND_HANDLER_EX(IDC_TRACE, BN_CLICKED, OnTraceClicked)
	END_MSG_MAP()

private:

	static void translate_string(const char* p_inp, array_t<TCHAR>& p_out);
	t_size get_dsddsp_item_by_guid(GUID p_guid);
	GUID get_dsddsp_guid_by_item(t_size p_item);
	BOOL OnInitDialog(CWindow, LPARAM);
	void OnOutputModeChange(UINT, int, CWindow);
	void OnDoPForConverterClicked(UINT, int, CWindow);
	void OnVolumeAdjustChange(UINT, int, CWindow);
	void OnLFEAdjustChange(UINT, int, CWindow);
	void OnDeClickerClicked(UINT, int, CWindow);
	void OnLogOverloadsClicked(UINT, int, CWindow);
	void OnSamplerateChange(UINT, int, CWindow);
	void OnConverterModeChange(UINT, int, CWindow);
	void OnLoadFirClicked(UINT, int, CWindow);
	void OnSaveFirClicked(UINT, int, CWindow);
	void OnAreaChange(UINT, int, CWindow);
	void OnEditableTagsClicked(UINT, int, CWindow);
	void OnStoreWithIsoClicked(UINT, int, CWindow);
	void OnLinkedTagsClicked(UINT, int, CWindow);
	void OnEMasterClicked(UINT, int, CWindow);
	void OnStdTagsClicked(UINT, int, CWindow);
	void OnDSDDSPChange(UINT, int, CWindow);
	void OnTraceClicked(UINT, int, CWindow);
	bool HasChanged();
	void OnChanged();
	void GetOutputModeList();
	void GetVolumeList();
	void GetLFEAdjustList();
	void GetSamplerateList();
	void GetConverterModeList();
	void GetAreaList();
	void GetDSDDSPList();
	void SetPcmControls();
	void SetUserFirState();

	class dsddsp_t {
	public:
		GUID m_guid;
		string8 m_name;
		dsddsp_t() {
			m_guid = GUID_NULL;
		}
		dsddsp_t(GUID p_guid, const char* p_name) {
			m_guid = p_guid;
			m_name = p_name;
		}
	};
	array_t<dsddsp_t> m_dsddsps;

	const preferences_page_callback::ptr m_callback;
};

class preferences_page_sacd_t : public preferences_page_impl<CSACDPreferences> {
public:
	const char* get_name();
	GUID get_guid();
	GUID get_parent_guid();
};

#endif
