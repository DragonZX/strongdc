/* 
 * Copyright (C) 2001-2003 Jacek Sieka, j_s@telia.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef GENERALPAGE_H
#define GENERALPAGE_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#include "PropPage.h"
#include "atldlgs.h"

class GeneralPage : public CPropertyPage<IDD_GENERALPAGE>, public PropPage
{
public:
	GeneralPage(SettingsManager *s) : PropPage(s) { 
		SetTitle(CSTRING(SETTINGS_GENERAL));
	};
	virtual ~GeneralPage() { ConnTypes.Destroy(); };

	BEGIN_MSG_MAP(GeneralPage)
		MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
		COMMAND_ID_HANDLER(IDC_ACTIVE, onClickedActive)
		COMMAND_ID_HANDLER(IDC_PASSIVE, onClickedActive)
		COMMAND_ID_HANDLER(IDC_SHOW_SPEED_CHECK, onClickedActive)
		COMMAND_ID_HANDLER(IDC_SOCKS5, onClickedActive)
		COMMAND_HANDLER(IDC_GETIP, BN_CLICKED, onGetIP)

		COMMAND_HANDLER(IDC_NICK, EN_CHANGE, onTextChanged)
		COMMAND_HANDLER(IDC_EMAIL, EN_CHANGE, onTextChanged)
		COMMAND_HANDLER(IDC_DESCRIPTION, EN_CHANGE, onTextChanged)
	END_MSG_MAP()

	LRESULT onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onGetIP(WORD /* wNotifyCode */, WORD /* wID */, HWND /* hWndCtl */, BOOL& /* bHandled */);
	LRESULT onClickedActive(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT onTextChanged(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	string getMyIP();
	// Common PropPage interface
	PROPSHEETPAGE *getPSP() { return (PROPSHEETPAGE *)*this; }
	virtual void write();
	
private:
	static Item items[];
	static TextItem texts[];
	CComboBoxEx ctrlConnection;
	CComboBox ctrlDownloadSpeed, ctrlUploadSpeed;
	CEdit nick;
	CImageList ConnTypes;

	void fixControls();
};

#endif // GENERALPAGE_H

/**
 * @file
 * $Id$
 */

