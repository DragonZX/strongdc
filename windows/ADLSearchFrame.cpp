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

/*
 * Automatic Directory Listing Search
 * Henrik Engstr�m, henrikengstrom@home.se
 */

#include "stdafx.h"
#include "Resource.h"
#include "../client/DCPlusPlus.h"
#include "../client/Client.h"
#include "ADLSearchFrame.h"
#include "AdlsProperties.h"
#include "CZDCLib.h"

int ADLSearchFrame::columnIndexes[] = { 
	COLUMN_ACTIVE_SEARCH_STRING,
	COLUMN_SOURCE_TYPE,
	COLUMN_DEST_DIR,
	COLUMN_MIN_FILE_SIZE,
	COLUMN_MAX_FILE_SIZE
};
int ADLSearchFrame::columnSizes[] = { 
	120, 
	90, 
	90, 
	90, 
	90 
};
static ResourceManager::Strings columnNames[] = { 
	ResourceManager::ACTIVE_SEARCH_STRING, 
	ResourceManager::SOURCE_TYPE, 
	ResourceManager::DESTINATION, 
	ResourceManager::SIZE_MIN, 
	ResourceManager::SIZE_MAX, 
};

// Frame creation
LRESULT ADLSearchFrame::onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	// Create status bar
	CreateSimpleStatusBar(ATL_IDS_IDLEMESSAGE, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | SBARS_SIZEGRIP);
	ctrlStatus.Attach(m_hWndStatusBar);
	int w[1] = { 0 };
	ctrlStatus.SetParts(1, w);

	// Create list control
	ctrlList.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_HSCROLL | WS_VSCROLL | LVS_REPORT | LVS_SHOWSELALWAYS, WS_EX_CLIENTEDGE, IDC_ADLLIST);
	listContainer.SubclassWindow(ctrlList.m_hWnd);

	// Ev. set full row select
	DWORD styles = LVS_EX_CHECKBOXES | LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT;
	if (BOOLSETTING(SHOW_INFOTIPS))
		styles |= LVS_EX_INFOTIP;
	ctrlList.SetExtendedListViewStyle(styles);

	// Set background color
	ctrlList.SetBkColor(WinUtil::bgColor);
	ctrlList.SetTextBkColor(WinUtil::bgColor);
	ctrlList.SetTextColor(WinUtil::textColor);

	// Create listview columns
	WinUtil::splitTokens(columnIndexes, SETTING(ADLSEARCHFRAME_ORDER), COLUMN_LAST);
	WinUtil::splitTokens(columnSizes, SETTING(ADLSEARCHFRAME_WIDTHS), COLUMN_LAST);
	for(int j = 0; j < COLUMN_LAST; j++) 
	{
		int fmt = LVCFMT_LEFT;
		ctrlList.InsertColumn(j, CSTRING_I(columnNames[j]), fmt, columnSizes[j], j);
	}
	ctrlList.SetColumnOrderArray(COLUMN_LAST, columnIndexes);

	// Create buttons
	ctrlAdd.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		BS_PUSHBUTTON , 0, IDC_ADD);
	ctrlAdd.SetWindowText(CSTRING(NEW));
	ctrlAdd.SetFont(WinUtil::font);

	ctrlEdit.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		BS_PUSHBUTTON , 0, IDC_EDIT);
	ctrlEdit.SetWindowText(CSTRING(PROPERTIES));
	ctrlEdit.SetFont(WinUtil::font);

	ctrlRemove.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		BS_PUSHBUTTON , 0, IDC_REMOVE);
	ctrlRemove.SetWindowText(CSTRING(REMOVE));
	ctrlRemove.SetFont(WinUtil::font);

	ctrlMoveUp.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		BS_PUSHBUTTON , 0, IDC_MOVE_UP);
	ctrlMoveUp.SetWindowText(CSTRING(MOVE_UP));
	ctrlMoveUp.SetFont(WinUtil::font);

	ctrlMoveDown.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		BS_PUSHBUTTON , 0, IDC_MOVE_DOWN);
	ctrlMoveDown.SetWindowText(CSTRING(MOVE_DOWN));
	ctrlMoveDown.SetFont(WinUtil::font);

	ctrlHelp.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		BS_PUSHBUTTON , 0, IDC_HELP_FAQ);
	ctrlHelp.SetWindowText(CSTRING(WHATS_THIS));
	ctrlHelp.SetFont(WinUtil::font);

	// Create context menu
	contextMenu.CreatePopupMenu();
	contextMenu.AppendMenu(MF_STRING, IDC_ADD,    CSTRING(NEW));
	contextMenu.AppendMenu(MF_STRING, IDC_REMOVE, CSTRING(REMOVE));
	contextMenu.AppendMenu(MF_STRING, IDC_EDIT,   CSTRING(PROPERTIES));

	// Load all searches
	LoadAll();

	bHandled = FALSE;
	return TRUE;
}

// Close window
LRESULT ADLSearchFrame::onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) 
{	if(!closed) {
		closed = true;		
		ADLSearchManager::getInstance()->Save();

		CZDCLib::setButtonPressed(IDC_FILE_ADL_SEARCH, false);
		PostMessage(WM_CLOSE);
		return 0;
	} else {
		WinUtil::saveHeaderOrder(ctrlList, SettingsManager::ADLSEARCHFRAME_ORDER, 
		SettingsManager::ADLSEARCHFRAME_WIDTHS, COLUMN_LAST, columnIndexes, columnSizes);

		MDIDestroy(m_hWnd);
		return 0;
	}	
}

// Recalculate frame control layout
void ADLSearchFrame::UpdateLayout(BOOL bResizeBars /* = TRUE */) 
{
	RECT rect;
	GetClientRect(&rect);

	// Position bars and offset their dimensions
	UpdateBarsPosition(rect, bResizeBars);
	if(ctrlStatus.IsWindow()) 
	{
		CRect sr;
		int w[1];
		ctrlStatus.GetClientRect(sr);
		w[0] = sr.Width() - 16;
		ctrlStatus.SetParts(1, w);
	}

	// Position list control
	CRect rc = rect;
	rc.top += 2;
	rc.bottom -= 28;
	ctrlList.MoveWindow(rc);

	// Position buttons
	const long bwidth = 90;
	const long bspace = 10;
	rc = rect;
	rc.bottom -= 2;
	rc.top = rc.bottom - 22;

	rc.left = 2;
	rc.right = rc.left + bwidth;
	ctrlAdd.MoveWindow(rc);

	rc.left += bwidth + 2;
	rc.right = rc.left + bwidth;
	ctrlEdit.MoveWindow(rc);

	rc.left += bwidth + 2;
	rc.right = rc.left + bwidth;
	ctrlRemove.MoveWindow(rc);

	rc.left += bspace;

	rc.left += bwidth + 2;
	rc.right = rc.left + bwidth;
	ctrlMoveUp.MoveWindow(rc);

	rc.left += bwidth + 2;
	rc.right = rc.left + bwidth;
	ctrlMoveDown.MoveWindow(rc);

	rc.left += bspace;

	rc.left += bwidth + 2;
	rc.right = rc.left + bwidth;
	ctrlHelp.MoveWindow(rc);

}

// Keyboard shortcuts
LRESULT ADLSearchFrame::onChar(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
{
	switch(wParam) 
	{
	case VK_INSERT:
		onAdd(0, 0, 0, bHandled);
		break;
	case VK_DELETE:
		onRemove(0, 0, 0, bHandled);
		break;
	case VK_RETURN:
		onEdit(0, 0, 0, bHandled);
		break;
	default:
		bHandled = FALSE;
	}
	return 0;
}
	
LRESULT ADLSearchFrame::onContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) 
{
	// Get the bounding rectangle of the client area. 
	RECT rc;
	POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
	ctrlList.GetClientRect(&rc);
	ctrlList.ScreenToClient(&pt); 
	
	// Hit-test
	if(PtInRect(&rc, pt)) 
	{ 
		ctrlList.ClientToScreen(&pt);
		contextMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);
		return TRUE; 
	}
	
	return FALSE; 
}

// Add new search
LRESULT ADLSearchFrame::onAdd(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) 
{
	// Invoke edit dialog with fresh search
	ADLSearch search;
	ADLSProperties dlg(&search);
	if(dlg.DoModal((HWND)*this) == IDOK)
	{
		// Add new search to the end or if selected, just before
		ADLSearchManager::SearchCollection& collection = ADLSearchManager::getInstance()->collection;
		

		int i = ctrlList.GetNextItem(-1, LVNI_SELECTED);
		if(i < 0)
		{
			// Add to end
			collection.push_back(search);
			i = collection.size() - 1;
		}
		else
		{
			// Add before selection
			collection.insert(collection.begin() + i, search);
		}

		// Update list control
		int j = i;
		while(j < (int)collection.size())
		{
			UpdateSearch(j++);
		}
		ctrlList.EnsureVisible(i, FALSE);
	}

	return 0;
}

// Edit existing search
LRESULT ADLSearchFrame::onEdit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) 
{
	// Get selection info
	int i = ctrlList.GetNextItem(-1, LVNI_SELECTED);
	if(i < 0)
	{
		// Nothing selected
		return 0;
	}

	// Edit existing
	ADLSearchManager::SearchCollection& collection = ADLSearchManager::getInstance()->collection;
	ADLSearch search = collection[i];

	// Invoke dialog with selected search
	ADLSProperties dlg(&search);
	if(dlg.DoModal((HWND)*this) == IDOK)
	{
		// Update search collection
		collection[i] = search;

		// Update list control
		UpdateSearch(i);	  
	}

	return 0;
}

// Remove searches
LRESULT ADLSearchFrame::onRemove(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) 
{
	ADLSearchManager::SearchCollection& collection = ADLSearchManager::getInstance()->collection;

	// Loop over all selected items
	int i;
	while((i = ctrlList.GetNextItem(-1, LVNI_SELECTED)) >= 0)
	{
		collection.erase(collection.begin() + i);
		ctrlList.DeleteItem(i);
	}
	return 0;
}

// Help
LRESULT ADLSearchFrame::onHelp(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) 
{
	char title[] =
		"ADLSearch brief description";

	char message[] = 
		"ADLSearch is a tool for fast searching of directory listings downloaded from users. \n"
		"Create a new ADLSearch entering 'avi' as search string for example. When you \n"
		"download a directory listing from a user, all avi-files will be placed in a special folder \n"
		"called <<<ADLSearch>>> for easy finding. It is almost the same as using the standard \n"
		"'Find' multiple times in a directory listing. \n"
		"\n"
		"Special options: \n"
		"- 'Active' check box selects if the search is used or not. \n"
		"- 'Source Type' can be the following options; 'Filename' matches search against filename, \n"
		"   'Directory' matches against current subdirectory and places the whole structure in the \n"
		"   special folder, 'Full Path' matches against whole directory + filename. \n"
		"- 'Destination Directory' selects the special output folder for a search. Multiple folders \n"
		"   with different names can exist simultaneously. \n"
		"- 'Min/Max Size' sets file size limits. This is not used for 'Directory' type searches. \n"
		"- 'Move Up'/'Move Down' can be used to organize the list of searches. \n"
		"\n"
		"There is a new option in the context menu (right-click) for directory listings. It is called \n"
		"'Go to directory' and can be used to jump to the original location of the file or directory. \n"
		"\n"
		"Extra features:\n"
		"\n"
		"1) If you use %y.%m.%d in a search string it will be replaced by todays date. Switch \n"
		"place on y/m/d, or leave any of them out to alter the substitution. If you use %[nick] \n"
		"it will be replaced by the nick of the user you download the directory listing from. \n"
		"\n"
		"2) If you name a destination directory 'discard', it will not be shown in the total result. \n"
		"Useful with the extra feature 3) below to remove uninteresting results. \n"
		" \n"
		"3) There is a switch called 'Break on first ADLSearch match' in Settings->Advanced'.  \n"
		"If enabled, ADLSearch will stop after the first match for a specific file/directory. \n"
		"The order in the ADLSearch windows is therefore important. Example: Add a search \n"
		"item at the top of the list with string='xxx' and destination='discard'. It will catch \n"
		"many pornographic files and they will not be included in any following search results. \n"
		;

	MessageBox(message, title, MB_OK);
	return 0;
}

// Move selected entries up one step
LRESULT ADLSearchFrame::onMoveUp(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) 
{
	ADLSearchManager::SearchCollection& collection = ADLSearchManager::getInstance()->collection;

	// Get selection
	vector<int> sel;
	int i = -1;
	while((i = ctrlList.GetNextItem(i, LVNI_SELECTED)) >= 0)
	{
		sel.push_back(i);
	}
	if(sel.size() < 1)
	{
		return 0;
	}

	// Find out where to insert
	int i0 = sel[0];
	if(i0 > 0)
	{
		i0 = i0 - 1;
	}

	// Backup selected searches
	ADLSearchManager::SearchCollection backup;
	for(i = 0; i < (int)sel.size(); ++i)
	{
		backup.push_back(collection[sel[i]]);
	}

	// Erase selected searches
	for(i = sel.size() - 1; i >= 0; --i)
	{
		collection.erase(collection.begin() + sel[i]);
	}

	// Insert (grouped together)
	for(i = 0; i < (int)sel.size(); ++i)
	{
		collection.insert(collection.begin() + i0 + i, backup[i]);
	}

	// Update UI
	LoadAll();

	// Restore selection
	for(i = 0; i < (int)sel.size(); ++i)
	{
		ctrlList.SetItemState(i0 + i, LVNI_SELECTED, LVNI_SELECTED);
	}

	return 0;
}

// Move selected entries down one step
LRESULT ADLSearchFrame::onMoveDown(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) 
{
	ADLSearchManager::SearchCollection& collection = ADLSearchManager::getInstance()->collection;

	// Get selection
	vector<int> sel;
	int i = -1;
	while((i = ctrlList.GetNextItem(i, LVNI_SELECTED)) >= 0)
	{
		sel.push_back(i);
	}
	if(sel.size() < 1)
	{
		return 0;
	}

	// Find out where to insert
	int i0 = sel[sel.size() - 1] + 2;
	if(i0 > (int)collection.size())
	{
		i0 = collection.size();
	}

	// Backup selected searches
	ADLSearchManager::SearchCollection backup;
	for(i = 0; i < (int)sel.size(); ++i)
	{
		backup.push_back(collection[sel[i]]);
	}

	// Erase selected searches
	for(i = sel.size() - 1; i >= 0; --i)
	{
		collection.erase(collection.begin() + sel[i]);
		if(i < i0)
		{
			i0--;
		}
	}

	// Insert (grouped together)
	for(i = 0; i < (int)sel.size(); ++i)
	{
		collection.insert(collection.begin() + i0 + i, backup[i]);
	}

	// Update UI
	LoadAll();

	// Restore selection
	for(i = 0; i < (int)sel.size(); ++i)
	{
		ctrlList.SetItemState(i0 + i, LVNI_SELECTED, LVNI_SELECTED);
	}
	ctrlList.EnsureVisible(i0, FALSE);

	return 0;
}

// Clicked 'Active' check box
LRESULT ADLSearchFrame::onItemChanged(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) 
{
	NMITEMACTIVATE* item = (NMITEMACTIVATE*)pnmh;

	if((item->uChanged & LVIF_STATE) == 0)
		return 0;
	if((item->uOldState & INDEXTOSTATEIMAGEMASK(0xf)) == 0)
		return 0;
	if((item->uNewState & INDEXTOSTATEIMAGEMASK(0xf)) == 0)
		return 0;

	if(item->iItem >= 0)
	{
		// Set new active status check box
		ADLSearchManager::SearchCollection& collection = ADLSearchManager::getInstance()->collection;
		ADLSearch& search = collection[item->iItem];
		search.isActive = (ctrlList.GetCheckState(item->iItem) != 0);
	}
	return 0;
}

// Double-click on list control
LRESULT ADLSearchFrame::onDoubleClickList(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled) 
{
	NMITEMACTIVATE* item = (NMITEMACTIVATE*)pnmh;

	// Hit-test
	LVHITTESTINFO info;
	info.pt = item->ptAction;
    int iItem = ctrlList.SubItemHitTest(&info);
	if(iItem >= 0)
	{
		// Treat as onEdit command
		onEdit(0, 0, 0, bHandled);
	}

	return 0;
}

// Load all searches from manager
void ADLSearchFrame::LoadAll()
{
	// Clear current contents
	ctrlList.DeleteAllItems();

	// Load all searches
	ADLSearchManager::SearchCollection& collection = ADLSearchManager::getInstance()->collection;
	for(unsigned long l = 0; l < collection.size(); l++)
	{
		UpdateSearch(l, FALSE);
	}
}

// Update a specific search item
void ADLSearchFrame::UpdateSearch(int index, BOOL doDelete)
{
	ADLSearchManager::SearchCollection& collection = ADLSearchManager::getInstance()->collection;

	// Check args
	if(index >= (int)collection.size())
	{
		return;
	}
	ADLSearch& search = collection[index];

	// Delete from list control
	if(doDelete)
	{
		ctrlList.DeleteItem(index);
	}

	// Generate values
	StringList line;
	char buf[32];
	string fs;
	line.push_back(search.searchString);
	line.push_back(search.SourceTypeToString(search.sourceType));
	line.push_back(search.destDir);

	fs = "";
	if(search.minFileSize >= 0)
	{
		fs = _i64toa(search.minFileSize, buf, 10);
		fs += " ";
		fs += search.SizeTypeToStringInternational(search.typeFileSize);
	}
	line.push_back(fs);

	fs = "";
	if(search.maxFileSize >= 0)
	{
		fs = _i64toa(search.maxFileSize, buf, 10);
		fs += " ";
		fs += search.SizeTypeToStringInternational(search.typeFileSize);
	}
	line.push_back(fs);

	// Insert in list control
	ctrlList.insert(index, line);

	// Update 'Active' check box
	ctrlList.SetCheckState(index, search.isActive);
}

/**
 * @file
 * $Id$
 */
