/* 
 * Copyright (C) 2001-2006 Jacek Sieka, arnetheduck on gmail point com
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

#include "FavoritesFrm.h"
#include "HubFrame.h"
#include "FavHubProperties.h"
#include "FavHubGroupsDlg.h"

#include "../client/ClientManager.h"
#include "../client/StringTokenizer.h"
#include "../client/version.h"

int FavoriteHubsFrame::columnIndexes[] = { COLUMN_NAME, COLUMN_DESCRIPTION, COLUMN_NICK, COLUMN_PASSWORD, COLUMN_SERVER, COLUMN_USERDESCRIPTION };
int FavoriteHubsFrame::columnSizes[] = { 200, 290, 125, 100, 100, 125 };
static ResourceManager::Strings columnNames[] = { ResourceManager::AUTO_CONNECT, ResourceManager::DESCRIPTION, 
ResourceManager::NICK, ResourceManager::PASSWORD, ResourceManager::SERVER, ResourceManager::USER_DESCRIPTION
};

LRESULT FavoriteHubsFrame::onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	ctrlHubs.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_HSCROLL | WS_VSCROLL | LVS_REPORT | LVS_SHOWSELALWAYS, WS_EX_CLIENTEDGE, IDC_HUBLIST);
	ctrlHubs.SetExtendedListViewStyle(LVS_EX_LABELTIP | LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES | LVS_EX_HEADERDRAGDROP | LVS_EX_INFOTIP);	
	ctrlHubs.SetBkColor(WinUtil::bgColor);
	ctrlHubs.SetTextBkColor(WinUtil::bgColor);
	ctrlHubs.SetTextColor(WinUtil::textColor);
	ctrlHubs.EnableGroupView(TRUE);

	// Create listview columns
	WinUtil::splitTokens(columnIndexes, SETTING(FAVORITESFRAME_ORDER), COLUMN_LAST);
	WinUtil::splitTokens(columnSizes, SETTING(FAVORITESFRAME_WIDTHS), COLUMN_LAST);
	
	for(int j=0; j<COLUMN_LAST; j++) {
		int fmt = LVCFMT_LEFT;
		ctrlHubs.InsertColumn(j, CTSTRING_I(columnNames[j]), fmt, columnSizes[j], j);
	}
	
	ctrlHubs.SetColumnOrderArray(COLUMN_LAST, columnIndexes);
	
	ctrlConnect.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_DISABLED | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		BS_PUSHBUTTON , 0, IDC_CONNECT);
	ctrlConnect.SetWindowText(CTSTRING(CONNECT));
	ctrlConnect.SetFont(WinUtil::font);

	ctrlNew.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		BS_PUSHBUTTON , 0, IDC_NEWFAV);
	ctrlNew.SetWindowText(CTSTRING(NEW));
	ctrlNew.SetFont(WinUtil::font);

	ctrlProps.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_DISABLED | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		BS_PUSHBUTTON , 0, IDC_EDIT);
	ctrlProps.SetWindowText(CTSTRING(PROPERTIES));
	ctrlProps.SetFont(WinUtil::font);

	ctrlRemove.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_DISABLED | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		BS_PUSHBUTTON , 0, IDC_REMOVE);
	ctrlRemove.SetWindowText(CTSTRING(REMOVE));
	ctrlRemove.SetFont(WinUtil::font);

	ctrlUp.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		BS_PUSHBUTTON , 0, IDC_MOVE_UP);
	ctrlUp.SetWindowText(CTSTRING(MOVE_UP));
	ctrlUp.SetFont(WinUtil::font);

	ctrlDown.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		BS_PUSHBUTTON , 0, IDC_MOVE_DOWN);
	ctrlDown.SetWindowText(CTSTRING(MOVE_DOWN));
	ctrlDown.SetFont(WinUtil::font);

	ctrlManageGroups.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		BS_PUSHBUTTON , 0, IDC_MANAGE_GROUPS);
	ctrlManageGroups.SetWindowText(_T("Manage Groups"));
	ctrlManageGroups.SetFont(WinUtil::font);

	FavoriteManager::getInstance()->addListener(this);
	SettingsManager::getInstance()->addListener(this);

	fillList();
	
	hubsMenu.CreatePopupMenu();
	hubsMenu.AppendMenu(MF_STRING, IDC_OPEN_HUB_LOG, CTSTRING(OPEN_HUB_LOG));
	hubsMenu.AppendMenu(MF_SEPARATOR);
	hubsMenu.AppendMenu(MF_STRING, IDC_CONNECT, CTSTRING(CONNECT));
	hubsMenu.AppendMenu(MF_STRING, IDC_NEWFAV, CTSTRING(NEW));
	hubsMenu.AppendMenu(MF_STRING, IDC_MOVE_UP, CTSTRING(MOVE_UP));
	hubsMenu.AppendMenu(MF_STRING, IDC_MOVE_DOWN, CTSTRING(MOVE_DOWN));
	hubsMenu.AppendMenu(MF_SEPARATOR);
	hubsMenu.AppendMenu(MF_STRING, IDC_REMOVE, CTSTRING(REMOVE));
	hubsMenu.AppendMenu(MF_SEPARATOR);
	hubsMenu.AppendMenu(MF_STRING, IDC_EDIT, CTSTRING(PROPERTIES));
	hubsMenu.SetMenuDefaultItem(IDC_CONNECT);

	nosave = false;

	bHandled = FALSE;
	return TRUE;
}

void FavoriteHubsFrame::openSelected() {
	if(!checkNick())
		return;
	
	int i = -1;
	while( (i = ctrlHubs.GetNextItem(i, LVNI_SELECTED)) != -1) {
		FavoriteHubEntry* entry = (FavoriteHubEntry*)ctrlHubs.GetItemData(i);
		RecentHubEntry r;
		r.setName(entry->getName());
		r.setDescription(entry->getDescription());
		r.setUsers("*");
		r.setShared("*");
		r.setServer(entry->getServer());
		FavoriteManager::getInstance()->addRecent(r);
		HubFrame::openWindow(Text::toT(entry->getServer())
			, entry->getRawOne()
			, entry->getRawTwo()
			, entry->getRawThree()
			, entry->getRawFour()
			, entry->getRawFive()
			, entry->getChatUserSplit(), entry->getUserListState());
	}
	return;
}

void FavoriteHubsFrame::addEntry(const FavoriteHubEntry* entry, int pos, int groupIndex) {
	TStringList l;
	l.push_back(Text::toT(entry->getName()));
	l.push_back(Text::toT(entry->getDescription()));
	l.push_back(Text::toT(entry->getNick(false)));
	l.push_back(tstring(entry->getPassword().size(), '*'));
	l.push_back(Text::toT(entry->getServer()));
	l.push_back(Text::toT(entry->getUserDescription()));	
	l.push_back(Text::toT(entry->getRawOne()));
	l.push_back(Text::toT(entry->getRawTwo()));
	l.push_back(Text::toT(entry->getRawThree()));
	l.push_back(Text::toT(entry->getRawFour()));
	l.push_back(Text::toT(entry->getRawFive()));
	bool b = entry->getConnect();
	int i = ctrlHubs.insert(pos, l, 0, (LPARAM)entry);
	ctrlHubs.SetCheckState(i, b);

    LVITEM lvItem = { 0 };
    lvItem.mask = LVIF_GROUPID;
    lvItem.iItem = i;
    lvItem.iSubItem = 0;
    lvItem.iGroupId = groupIndex;
    ctrlHubs.SetItem( &lvItem );
}

LRESULT FavoriteHubsFrame::onContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	if(reinterpret_cast<HWND>(wParam) == ctrlHubs) {
		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

		CRect rc;
		ctrlHubs.GetHeader().GetWindowRect(&rc);
		if (PtInRect(&rc, pt)) {
			return 0;
		}

		if(pt.x == -1 && pt.y == -1) {
			WinUtil::getContextMenuPos(ctrlHubs, pt);
		}

		int status = ctrlHubs.GetSelectedCount() > 0 ? MFS_ENABLED : MFS_DISABLED;
		hubsMenu.EnableMenuItem(IDC_OPEN_HUB_LOG, status);
		hubsMenu.EnableMenuItem(IDC_CONNECT, status);
		hubsMenu.EnableMenuItem(IDC_EDIT, status);
		hubsMenu.EnableMenuItem(IDC_MOVE_UP, status);
		hubsMenu.EnableMenuItem(IDC_MOVE_DOWN, status);
		hubsMenu.EnableMenuItem(IDC_REMOVE, status);

		tstring x;
		if (ctrlHubs.GetSelectedCount() == 1) {
			FavoriteHubEntry* f = (FavoriteHubEntry*)ctrlHubs.GetItemData(WinUtil::getFirstSelectedIndex(ctrlHubs));
			x = Text::toT(f->getName());
		} else {
			x = _T("");
		}
		if (!x.empty())
			hubsMenu.InsertSeparatorFirst(x);

		hubsMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);

		if (!x.empty())
			hubsMenu.RemoveFirstItem();

		return TRUE; 
	}
	
	bHandled = FALSE;
	return FALSE; 
}

LRESULT FavoriteHubsFrame::onDoubleClickHublist(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	NMITEMACTIVATE* item = (NMITEMACTIVATE*) pnmh;

	if(item->iItem == -1) {
		PostMessage(WM_COMMAND, IDC_NEWFAV, 0);
	} else {
		PostMessage(WM_COMMAND, IDC_CONNECT, 0);
	}

	return 0;
}

LRESULT FavoriteHubsFrame::onKeyDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled) {
	NMLVKEYDOWN* kd = (NMLVKEYDOWN*) pnmh;
	switch(kd->wVKey) {
	case VK_INSERT:
		PostMessage(WM_COMMAND, IDC_NEWFAV, 0);
		break;
	case VK_DELETE:
		PostMessage(WM_COMMAND, IDC_REMOVE, 0);
		break;
	case VK_RETURN:
		PostMessage(WM_COMMAND, IDC_CONNECT, 0);
		break;
	default:
		bHandled = FALSE;
	}
	return 0;
}

LRESULT FavoriteHubsFrame::onRemove(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int i = -1;
	if(!BOOLSETTING(CONFIRM_HUB_REMOVAL) || MessageBox(CTSTRING(REALLY_REMOVE), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES) {
		while( (i = ctrlHubs.GetNextItem(-1, LVNI_SELECTED)) != -1) {
			FavoriteManager::getInstance()->removeFavorite((FavoriteHubEntry*)ctrlHubs.GetItemData(i));
		}
	}
	return 0;
}

LRESULT FavoriteHubsFrame::onEdit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int i = -1;
	if((i = ctrlHubs.GetNextItem(i, LVNI_SELECTED)) != -1)
	{
		FavoriteHubEntry* e = (FavoriteHubEntry*)ctrlHubs.GetItemData(i);
		dcassert(e != NULL);
		FavHubProperties dlg(e);
		if(dlg.DoModal(m_hWnd) == IDOK)
		{
			ctrlHubs.DeleteAllItems(); 
			fillList();
		}
	}
	return 0;
}

LRESULT FavoriteHubsFrame::onNew(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	FavoriteHubEntry e;
	FavHubProperties dlg(&e);

	while (true) {
		if(dlg.DoModal((HWND)*this) == IDOK) {
			if (FavoriteManager::getInstance()->isFavoriteHub(e.getServer())){
				MessageBox(
					CTSTRING(FAVORITE_HUB_ALREADY_EXISTS), _T(" "), MB_ICONWARNING | MB_OK);
			} else {
				FavoriteManager::getInstance()->addFavorite(e);
				break;
			}
		} else {
			break;
		}
	}
	return 0;
}

bool FavoriteHubsFrame::checkNick() {
	if(SETTING(NICK).empty()) {
		MessageBox(CTSTRING(ENTER_NICK), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_ICONSTOP | MB_OK);
		return false;
	}
	return true;
}

LRESULT FavoriteHubsFrame::onMoveUp(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	handleMove(true);
	return 0;
}

LRESULT FavoriteHubsFrame::onMoveDown(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	handleMove(false);
	return 0;
}

void FavoriteHubsFrame::handleMove(bool up)
{
	FavoriteHubEntryList& fh = FavoriteManager::getInstance()->getFavoriteHubs();

	ctrlHubs.SetRedraw(FALSE);

	// in grouped mode, the indexes of each item are completely random, so use entry pointers instead
	FavoriteHubEntryList selected;
	for(int i = ctrlHubs.GetItemCount() - 2; i >= 0; --i) {
		if(ctrlHubs.GetItemState(i, LVIS_SELECTED))
			selected.push_back((FavoriteHubEntry*)ctrlHubs.GetItemData(i));
	}
	
	FavoriteHubEntryList fh_copy = fh;
	if(!up)
		reverse(fh_copy.begin(), fh_copy.end());
	for(FavoriteHubEntryList::iterator i = fh_copy.begin() + 1; i != fh_copy.end(); ++i) {
		if(find(selected.begin(), selected.end(), *i) == selected.end())
			continue;
		const string& group = (*i)->getGroup();
		for(FavoriteHubEntryList::iterator j = i - 1; ; --j) {
			if((*j)->getGroup() == group) {
				swap(*i, *j);
				break;
			}
			if(j == fh_copy.begin())
				break;
		}
	}
	if(!up)
		reverse(fh_copy.begin(), fh_copy.end());
	fh = fh_copy;
	FavoriteManager::getInstance()->save();

	ctrlHubs.DeleteAllItems();
	fillList();

	ctrlHubs.SetRedraw(TRUE);

	int count = ctrlHubs.GetItemCount() - 2;
	for(FavoriteHubEntryList::const_iterator i = selected.begin(), iend = selected.end(); i != iend; ++i) {
		for(int j = count; j >= 0; --j) {
			if((FavoriteHubEntry*)ctrlHubs.GetItemData(j) == *i)
			{
				ctrlHubs.SetItemState(j, LVIS_SELECTED, LVIS_SELECTED);
				break;
			}
		}
	}
}

LRESULT FavoriteHubsFrame::onItemChanged(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	NMITEMACTIVATE* l = (NMITEMACTIVATE*)pnmh;
	::EnableWindow(GetDlgItem(IDC_CONNECT), ctrlHubs.GetItemState(l->iItem, LVIS_SELECTED));
	::EnableWindow(GetDlgItem(IDC_REMOVE), ctrlHubs.GetItemState(l->iItem, LVIS_SELECTED));
	::EnableWindow(GetDlgItem(IDC_EDIT), ctrlHubs.GetItemState(l->iItem, LVIS_SELECTED));
	if(!nosave && l->iItem != -1 && ((l->uNewState & LVIS_STATEIMAGEMASK) != (l->uOldState & LVIS_STATEIMAGEMASK))) {
		FavoriteHubEntry* f = (FavoriteHubEntry*)ctrlHubs.GetItemData(l->iItem);
		f->setConnect(ctrlHubs.GetCheckState(l->iItem) != FALSE);
		FavoriteManager::getInstance()->save();
	}
	return 0;
}

LRESULT FavoriteHubsFrame::onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	if(!closed) {
		FavoriteManager::getInstance()->removeListener(this);
		SettingsManager::getInstance()->removeListener(this);
		closed = true;		
		WinUtil::setButtonPressed(IDC_FAVORITES, false);
		PostMessage(WM_CLOSE);
		return 0;
	} else {
		WinUtil::saveHeaderOrder(ctrlHubs, SettingsManager::FAVORITESFRAME_ORDER, 
		SettingsManager::FAVORITESFRAME_WIDTHS, COLUMN_LAST, columnIndexes, columnSizes);

		bHandled = FALSE;
		return 0;
	}	
}

void FavoriteHubsFrame::UpdateLayout(BOOL bResizeBars /* = TRUE */)
{
	RECT rect;
	GetClientRect(&rect);
	// position bars and offset their dimensions
	UpdateBarsPosition(rect, bResizeBars);

	CRect rc = rect;
	rc.bottom -=28;
	ctrlHubs.MoveWindow(rc);

	const long bwidth = 90;
	const long bspace = 10;

	rc = rect;
	rc.bottom -= 2;
	rc.top = rc.bottom - 22;

	rc.left = 2;
	rc.right = rc.left + bwidth;
	ctrlNew.MoveWindow(rc);

	rc.OffsetRect(bwidth+2, 0);
	ctrlProps.MoveWindow(rc);

	rc.OffsetRect(bwidth+2, 0);
	ctrlRemove.MoveWindow(rc);

	rc.OffsetRect(bspace + bwidth +2, 0);
	ctrlUp.MoveWindow(rc);

	rc.OffsetRect(bwidth+2, 0);
	ctrlDown.MoveWindow(rc);

	rc.OffsetRect(bspace + bwidth +2, 0);
	ctrlConnect.MoveWindow(rc);

	rc.OffsetRect(bspace + bwidth +2, 0);
	ctrlManageGroups.MoveWindow(rc);
}

LRESULT FavoriteHubsFrame::onOpenHubLog(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(ctrlHubs.GetSelectedCount() == 1) {
		int i = ctrlHubs.GetNextItem(-1, LVNI_SELECTED);
		FavoriteHubEntry* entry = (FavoriteHubEntry*)ctrlHubs.GetItemData(i);
		StringMap params;
		params["hubNI"] = entry->getName();
		params["hubURL"] = entry->getServer();
		params["myNI"] = entry->getNick(); 
		tstring filename = Text::toT(Util::validateFileName(SETTING(LOG_DIRECTORY) + Util::formatParams(SETTING(LOG_FILE_MAIN_CHAT), params, false)));
		if(Util::fileExists(Text::fromT(filename))){
			ShellExecute(NULL, NULL, filename.c_str(), NULL, NULL, SW_SHOWNORMAL);
		} else {
			MessageBox(CTSTRING(NO_LOG_FOR_HUB), CTSTRING(NO_LOG_FOR_HUB), MB_OK );	  
		}
	}
	return 0;
}

LRESULT FavoriteHubsFrame::onManageGroups(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	FavHubGroupsDlg dlg;
	dlg.DoModal();
	return 0;
}

void FavoriteHubsFrame::on(SettingsManagerListener::Save, SimpleXML& /*xml*/) throw() {
	bool refresh = false;
	if(ctrlHubs.GetBkColor() != WinUtil::bgColor) {
		ctrlHubs.SetBkColor(WinUtil::bgColor);
		ctrlHubs.SetTextBkColor(WinUtil::bgColor);
		refresh = true;
	}
	if(ctrlHubs.GetTextColor() != WinUtil::textColor) {
		ctrlHubs.SetTextColor(WinUtil::textColor);
		refresh = true;
	}
	if(refresh == true) {
		RedrawWindow(NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
	}
}

LRESULT FavoriteHubsFrame::onColumnClickHublist(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	NMLISTVIEW* l = (NMLISTVIEW*)pnmh;
	if(l->iSubItem == ctrlHubs.getSortColumn()) {
		if (!ctrlHubs.isAscending())
			ctrlHubs.setSort(-1, ctrlHubs.getSortType());
		else
			ctrlHubs.setSortDirection(false);
	} else {
		ctrlHubs.setSort(l->iSubItem, ExListViewCtrl::SORT_STRING_NOCASE);
	}
	return 0;
}

/**
 * @file
 * $Id$
 */
