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

#include "stdafx.h"
#include "../client/DCPlusPlus.h"
#include "Resource.h"

#include "FavHubProperties.h"

#include "../client/HubManager.h"

LRESULT FavHubProperties::OnInitDialog(UINT, WPARAM, LPARAM, BOOL&)
{
	SetDlgItemText(IDC_HUBNAME, entry->getName().c_str());
	SetDlgItemText(IDC_HUBDESCR, entry->getDescription().c_str());
	SetDlgItemText(IDC_HUBADDR, entry->getServer().c_str());
	SetDlgItemText(IDC_HUBNICK, entry->getNick(false).c_str());
	SetDlgItemText(IDC_HUBPASS, entry->getPassword().c_str());
	SetDlgItemText(IDC_HUBUSERDESCR, entry->getUserDescription().c_str());
	SetDlgItemText(IDC_HUBUSERDESCR, entry->getUserDescription().c_str());
	CheckDlgButton(IDC_STEALTH, entry->getStealth() ? BST_CHECKED : BST_UNCHECKED);
	// CDM EXTENSION BEGINS FAVS
	SetDlgItemText(IDC_RAW_ONE, entry->getRawOne().c_str());
	SetDlgItemText(IDC_RAW_TWO, entry->getRawTwo().c_str());
	SetDlgItemText(IDC_RAW_THREE, entry->getRawThree().c_str());
	SetDlgItemText(IDC_RAW_FOUR, entry->getRawFour().c_str());
	SetDlgItemText(IDC_RAW_FIVE, entry->getRawFive().c_str());
	// CDM EXTENSION ENDS

	CEdit tmp;
	tmp.Attach(GetDlgItem(IDC_HUBNAME));
	tmp.SetFocus();
	tmp.SetSel(0,-1);
	tmp.Detach();
	tmp.Attach(GetDlgItem(IDC_HUBNICK));
	tmp.LimitText(35);
	tmp.Detach();
	tmp.Attach(GetDlgItem(IDC_HUBUSERDESCR));
	tmp.LimitText(50);
	tmp.Detach();
	tmp.Attach(GetDlgItem(IDC_HUBPASS));
	tmp.SetPasswordChar('*');
	tmp.Detach();
	CenterWindow(GetParent());
	return FALSE;
}

LRESULT FavHubProperties::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	if(wID == IDOK)
	{
		char buf[512];
		GetDlgItemText(IDC_HUBNAME, buf, 256);
		entry->setName(buf);
		GetDlgItemText(IDC_HUBDESCR, buf, 256);
		entry->setDescription(buf);
		GetDlgItemText(IDC_HUBADDR, buf, 256);
		entry->setServer(buf);
		GetDlgItemText(IDC_HUBNICK, buf, 256);
		entry->setNick(buf);
		GetDlgItemText(IDC_HUBPASS, buf, 256);
		entry->setPassword(buf);
		GetDlgItemText(IDC_HUBUSERDESCR, buf, 256);
		entry->setUserDescription(buf);
		entry->setStealth(IsDlgButtonChecked(IDC_STEALTH));
		// CDM EXTENSION BEGINS
		GetDlgItemText(IDC_RAW_ONE, buf, 512);
		entry->setRawOne(buf);
		GetDlgItemText(IDC_RAW_TWO, buf, 512);
		entry->setRawTwo(buf);
		GetDlgItemText(IDC_RAW_THREE, buf, 512);
		entry->setRawThree(buf);
		GetDlgItemText(IDC_RAW_FOUR, buf, 512);
		entry->setRawFour(buf);
		GetDlgItemText(IDC_RAW_FIVE, buf, 512);
		entry->setRawFive(buf);
		// CDM EXTENSION ENDS
		HubManager::getInstance()->save();
	}
	EndDialog(wID);
	return 0;
}

LRESULT FavHubProperties::OnTextChanged(WORD /*wNotifyCode*/, WORD wID, HWND hWndCtl, BOOL& /*bHandled*/)
{
	char buf[256];

	GetDlgItemText(wID, buf, 256);
	string old = buf;

	// Strip '$', '|' and ' ' from text
	char *b = buf, *f = buf, c;
	while( (c = *b++) != 0 )
	{
		if(c != '$' && c != '|' && (wID == IDC_HUBUSERDESCR || c != ' ') && ( (wID != IDC_HUBNICK) || (c != '<' && c != '>')) )
			*f++ = c;
	}

	*f = '\0';

	if(old != buf)
	{
		// Something changed; update window text without changing cursor pos
		CEdit tmp;
		tmp.Attach(hWndCtl);
		int start, end;
		tmp.GetSel(start, end);
		tmp.SetWindowText(buf);
		if(start > 0) start--;
		if(end > 0) end--;
		tmp.SetSel(start, end);
		tmp.Detach();
	}

	return TRUE;
}

/**
 * @file
 * $Id$
 */
