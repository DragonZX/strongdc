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

#if !defined(AFX_DIRECTORYFRM_H__A7078724_FD85_4F39_8463_5A08A5F45E33__INCLUDED_)
#define AFX_DIRECTORYFRM_H__A7078724_FD85_4F39_8463_5A08A5F45E33__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "../client/User.h"
#include "../client/FastAlloc.h"

#include "FlatTabCtrl.h"
#include "TypedListViewCtrl.h"
#include "WinUtil.h"
#include "QueueFrame.h"
#include "SearchFrm.h"

#include "../client/DirectoryListing.h"
#include "../client/StringSearch.h"
#include "../client/ShareManager.h"

#define STATUS_MESSAGE_MAP 9

class DirectoryListingFrame : public MDITabChildWindowImpl<DirectoryListingFrame, RGB(255, 0, 255), IDR_DIRECTORY>, public CSplitterImpl<DirectoryListingFrame>
{
public:
	static void openWindow(const string& aFile, const User::Ptr& aUser, const string& start = Util::emptyString);

	typedef MDITabChildWindowImpl<DirectoryListingFrame, RGB(255, 0, 255), IDR_DIRECTORY> baseClass;

	enum {
		COLUMN_FILENAME,
		COLUMN_TYPE,
		COLUMN_SIZE,
		COLUMN_EXACT_SIZE,
		COLUMN_TTH,
		COLUMN_LAST
	};
	
	DirectoryListingFrame(const string& aFile, const User::Ptr& aUser, const string& s);
	~DirectoryListingFrame() { 
		delete dl; 
	}

	DECLARE_FRAME_WND_CLASS("DirectoryListingFrame", IDR_DIRECTORY)

	virtual void OnFinalMessage(HWND /*hWnd*/) {
		delete this;
	}

	BEGIN_MSG_MAP(DirectoryListingFrame)
		NOTIFY_HANDLER(IDC_FILES, LVN_GETDISPINFO, ctrlList.onGetDispInfo)
		NOTIFY_HANDLER(IDC_FILES, LVN_COLUMNCLICK, ctrlList.onColumnClick)
		NOTIFY_HANDLER(IDC_FILES, LVN_KEYDOWN, onKeyDown)
		NOTIFY_HANDLER(IDC_FILES, NM_DBLCLK, onDoubleClickFiles)
		NOTIFY_HANDLER(IDC_FILES, LVN_ITEMCHANGED, onItemChanged)
		NOTIFY_HANDLER(IDC_DIRECTORIES, TVN_KEYDOWN, onKeyDownDirs)
		NOTIFY_HANDLER(IDC_DIRECTORIES, TVN_SELCHANGED, onSelChangedDirectories)
		MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBackground)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_CONTEXTMENU, onContextMenu)
		MESSAGE_HANDLER(WM_CLOSE, onClose)
		MESSAGE_HANDLER(WM_SETFOCUS, onSetFocus)
		MESSAGE_HANDLER(FTM_CONTEXTMENU, onTabContextMenu)
		COMMAND_ID_HANDLER(IDC_DOWNLOAD, onDownload)
		COMMAND_ID_HANDLER(IDC_DOWNLOADDIR, onDownloadDir)
		COMMAND_ID_HANDLER(IDC_DOWNLOADDIRTO, onDownloadDirTo)
		COMMAND_ID_HANDLER(IDC_DOWNLOADTO, onDownloadTo)
		COMMAND_ID_HANDLER(IDC_GO_TO_DIRECTORY, onGoToDirectory)
		COMMAND_ID_HANDLER(IDC_VIEW_AS_TEXT, onViewAsText)
		COMMAND_ID_HANDLER(IDC_SEARCH_BY_TTH, onSearchAlternates)
		COMMAND_ID_HANDLER(IDC_COPY_LINK, onCopy)
		COMMAND_ID_HANDLER(IDC_COPY_TTH, onCopy)
		COMMAND_ID_HANDLER(IDC_MP3, onMP3Info)
		COMMAND_ID_HANDLER(IDC_ADD_TO_FAVORITES, onAddToFavorites)
		COMMAND_ID_HANDLER(IDC_PRIVATEMESSAGE, onPM)
		COMMAND_ID_HANDLER(IDC_COPY_NICK, onCopy);
		COMMAND_ID_HANDLER(IDC_COPY_FILENAME, onCopy);
		COMMAND_ID_HANDLER(IDC_COPY_SIZE, onCopy);
		COMMAND_ID_HANDLER(IDC_SEARCH_ALTERNATES, onSearchAlternates)
		COMMAND_RANGE_HANDLER(IDC_DOWNLOAD_TARGET, IDC_DOWNLOAD_TARGET + targets.size() + WinUtil::lastDirs.size(), onDownloadTarget)
		COMMAND_RANGE_HANDLER(IDC_DOWNLOAD_TARGET_DIR, IDC_DOWNLOAD_TARGET_DIR + WinUtil::lastDirs.size(), onDownloadTargetDir)
		COMMAND_RANGE_HANDLER(IDC_PRIORITY_PAUSED, IDC_PRIORITY_HIGHEST, onDownloadWithPrio)
		COMMAND_RANGE_HANDLER(IDC_PRIORITY_PAUSED+90, IDC_PRIORITY_HIGHEST+90, onDownloadDirWithPrio)
		CHAIN_MSG_MAP(baseClass)
		CHAIN_MSG_MAP(CSplitterImpl<DirectoryListingFrame>)
	ALT_MSG_MAP(STATUS_MESSAGE_MAP)
		COMMAND_ID_HANDLER(IDC_FIND, onFind)
		COMMAND_ID_HANDLER(IDC_NEXT, onNext)
		COMMAND_ID_HANDLER(IDC_MATCH_QUEUE, onMatchQueue)
	END_MSG_MAP()

	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onDownload(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onDownloadWithPrio(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onDownloadDir(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onDownloadDirWithPrio(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onDownloadDirTo(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onDownloadTo(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onViewAsText(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onCopy(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onMP3Info(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onAddToFavorites(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onPM(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onGoToDirectory(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onDownloadTarget(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onDownloadTargetDir(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onDoubleClickFiles(int idCtrl, LPNMHDR pnmh, BOOL& bHandled); 
	LRESULT onSelChangedDirectories(int idCtrl, LPNMHDR pnmh, BOOL& bHandled); 
	LRESULT onContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled);
	LRESULT onSearchAlternates(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onTabContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/); 

	void downloadList(const string& aTarget, bool view = false,  QueueItem::Priority prio = QueueItem::Priority::DEFAULT);
	void downloadMP3List(const string& aTarget);
	void updateTree(DirectoryListing::Directory* tree, HTREEITEM treeItem);
	void UpdateLayout(BOOL bResizeBars = TRUE);
	void findFile(bool findNext);
	
	LRESULT onItemChanged(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/) {
		updateStatus();
		return 0;
	}

	LRESULT onSetFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		ctrlList.SetFocus();
		return 0;
	}

	void setWindowTitle() {
		if(error.empty())
			SetWindowText(dl->getUser()->getFullNick().c_str());
		else
			SetWindowText(error.c_str());		
	}

	LRESULT OnEraseBackground(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		return 1;
	}

	void clearList() {
		int j = ctrlList.GetItemCount();
		for(int i = 0; i < j; i++) {
			delete (ItemInfo*)ctrlList.GetItemData(i);
		}
		ctrlList.DeleteAllItems();
	}

	LRESULT onFind(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		searching = true;
		findFile(false);
		searching = false;
		updateStatus();
		return 0;
	}
	LRESULT onNext(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		searching = true;
		findFile(true);
		searching = false;
		updateStatus();
		return 0;
	}

	LRESULT onMatchQueue(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	LRESULT onKeyDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);

	LRESULT onKeyDownDirs(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
		NMTVKEYDOWN* kd = (NMTVKEYDOWN*) pnmh;
		if(kd->wVKey == VK_TAB) {
			onTab();
		}
		return 0;
	}

	void onTab() {
		HWND focus = ::GetFocus();
		if(focus == ctrlTree.m_hWnd) {
			ctrlList.SetFocus();
		} else if(focus == ctrlList.m_hWnd) {
			ctrlTree.SetFocus();
		}
	}
private:
	void changeDir(DirectoryListing::Directory* d, BOOL enableRedraw);
	HTREEITEM findFile(const StringSearch& str, HTREEITEM root, int &foundFile, int &skipHits);
	void updateStatus();
	void GoToDirectory(HTREEITEM hItem, StringList::iterator& iPath, const StringList::iterator& iPathEnd);

	class ItemInfo : public FastAlloc<ItemInfo> {
	public:
		enum ItemType {
			FILE,
			DIRECTORY
		} type;
		
		union {
			DirectoryListing::File* file;
			DirectoryListing::Directory* dir;
		};

		ItemInfo(DirectoryListing::File* f, bool utf8) : type(FILE), file(f) { 
			columns[COLUMN_FILENAME] = f->getName();
			if(utf8)
				Util::toAcp(columns[COLUMN_FILENAME]);
			columns[COLUMN_TYPE] = Util::getFileExt(columns[COLUMN_FILENAME]);
			if(columns[COLUMN_TYPE].size() > 0 && columns[COLUMN_TYPE][0] == '.')
				columns[COLUMN_TYPE].erase(0, 1);

			columns[COLUMN_SIZE] = Util::formatBytes(f->getSize());
			columns[COLUMN_EXACT_SIZE] = Util::formatExactSize(f->getSize());
			if(f->getTTH() != NULL)
                columns[COLUMN_TTH] = f->getTTH()->toBase32();
		};
		ItemInfo(DirectoryListing::Directory* d, bool utf8) : type(DIRECTORY), dir(d) { 
			columns[COLUMN_FILENAME] = d->getName();
			if(utf8 && Util::needsAcp(columns[COLUMN_FILENAME]))
				Util::toAcp(columns[COLUMN_FILENAME]);
			columns[COLUMN_SIZE] = Util::formatBytes(d->getTotalSize());
			columns[COLUMN_EXACT_SIZE] = Util::formatExactSize(d->getTotalSize());
		};

		const string& getText(int col) {
			return columns[col];
		}
		
		struct TotalSize {
			TotalSize() : total(0) { }
			void operator()(ItemInfo* a) { total += a->type == DIRECTORY ? a->dir->getTotalSize() : a->file->getSize(); }
			int64_t total;
		};

		static int compareItems(ItemInfo* a, ItemInfo* b, int col) {
			if(a->type == DIRECTORY) {
				if(b->type == DIRECTORY) {
					switch(col) {
					case COLUMN_SIZE:
					case COLUMN_EXACT_SIZE:
						return compare(a->dir->getTotalSize(), b->dir->getTotalSize());
					default: return Util::stricmp(a->columns[col], b->columns[col]);
					}
				} else {
					return -1;
				}
			} else if(b->type == DIRECTORY) {
				return 1;
			} else {
				switch(col) {
				case COLUMN_SIZE: 
				case COLUMN_EXACT_SIZE: 
					return compare(a->file->getSize(), b->file->getSize());
				default: return Util::stricmp(a->columns[col], b->columns[col]);
				}
			}
		}

	private:
		string columns[COLUMN_LAST];
	};
	
	CMenu targetMenu;
	CMenu targetDirMenu;
	CMenu fileMenu;
	CMenu directoryMenu;
	CMenu priorityMenu;
	CMenu priorityDirMenu;
	CMenu copyMenu;
	CMenu searchMenu;
	OMenu tabMenu;

	CContainedWindow statusContainer;

	StringList targets;
	
	CTreeViewCtrl ctrlTree;
	TypedListViewCtrl<ItemInfo, IDC_FILES> ctrlList;
	CStatusBarCtrl ctrlStatus;
	HTREEITEM treeRoot;
	
	CButton ctrlFind, ctrlFindNext;
	CButton ctrlMatchQueue;

	string findStr;
	string error;
	string size;

	string start;

	int skipHits;

	int files;

	bool updating;
	bool searching;
	bool closed;

	int statusSizes[8];
	
	DirectoryListing* dl;
	StringList searchFilter;
};

#endif // !defined(AFX_CHILDFRM_H__A7078724_FD85_4F39_8463_5A08A5F45E33__INCLUDED_)

/**
 * @file
 * $Id$
 */
