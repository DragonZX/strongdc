/* 
 * 
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

#if !defined(_UPLOAD_QUEUE_FRAME_H_)
#define _UPLOAD_QUEUE_FRAME_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "FlatTabCtrl.h"
#include "TypedListViewCtrl.h"
#include "WinUtil.h"
#include "../client/TimerManager.h"
#include "../client/UploadManager.h"

#define SHOWTREE_MESSAGE_MAP 12

class UploadQueueFrame : public MDITabChildWindowImpl<UploadQueueFrame, RGB(0, 0, 0), IDR_UPLOAD_QUEUE>, public StaticFrame<UploadQueueFrame, ResourceManager::UPLOAD_QUEUE, IDC_UPLOAD_QUEUE>,
	private UploadManagerListener, public CSplitterImpl<UploadQueueFrame>, private SettingsManagerListener
{
public:
	DECLARE_FRAME_WND_CLASS_EX(_T("UploadQueueFrame"), IDR_UPLOAD_QUEUE, 0, COLOR_3DFACE);

	UploadQueueFrame() : showTree(true), closed(false), usingUserMenu(false), 
		showTreeContainer(_T("BUTTON"), this, SHOWTREE_MESSAGE_MAP)
	{
		headerBuf = new TCHAR[128];
	}
	virtual ~UploadQueueFrame() {
		delete[] headerBuf;
	}

	virtual void OnFinalMessage(HWND /*hWnd*/) {
		delete this;
	}

	enum {
		ADD_ITEM,
		REMOVE,
		REMOVE_ITEM,
		UPDATE_ITEMS
	};

	typedef MDITabChildWindowImpl<UploadQueueFrame, RGB(0, 0, 0), IDR_UPLOAD_QUEUE> baseClass;
	typedef CSplitterImpl<UploadQueueFrame> splitBase;

	// Inline message map
	BEGIN_MSG_MAP(UploadQueueFrame)
		MESSAGE_HANDLER(WM_CREATE, onCreate)
		MESSAGE_HANDLER(WM_CLOSE, onClose)
		MESSAGE_HANDLER(WM_CONTEXTMENU, onContextMenu)
		MESSAGE_HANDLER(WM_SPEAKER, onSpeaker)
		COMMAND_HANDLER(IDC_GETLIST, BN_CLICKED, onGetList)
		COMMAND_HANDLER(IDC_REMOVE, BN_CLICKED, onRemove)
		COMMAND_HANDLER(IDC_GRANTSLOT, BN_CLICKED, onGrantSlot)
		COMMAND_HANDLER(IDC_GRANTSLOT_HOUR, BN_CLICKED, onGrantSlotHour)
		COMMAND_HANDLER(IDC_GRANTSLOT_DAY, BN_CLICKED, onGrantSlotDay)
		COMMAND_HANDLER(IDC_GRANTSLOT_WEEK, BN_CLICKED, onGrantSlotWeek)
		COMMAND_HANDLER(IDC_UNGRANTSLOT, BN_CLICKED, onUnGrantSlot)
		COMMAND_HANDLER(IDC_ADD_TO_FAVORITES, BN_CLICKED, onAddToFavorites)
		COMMAND_HANDLER(IDC_PRIVATEMESSAGE, BN_CLICKED, onPrivateMessage)
		NOTIFY_HANDLER(IDC_UPLOAD_QUEUE, LVN_GETDISPINFO, ctrlList.onGetDispInfo)
		NOTIFY_HANDLER(IDC_UPLOAD_QUEUE, LVN_COLUMNCLICK, ctrlList.onColumnClick)
//		NOTIFY_HANDLER(IDC_UPLOAD_QUEUE, LVN_ITEMCHANGED, onItemChangedQueue)
		NOTIFY_HANDLER(IDC_UPLOAD_QUEUE, LVN_KEYDOWN, onKeyDown)
		NOTIFY_HANDLER(IDC_DIRECTORIES, TVN_SELCHANGED, onItemChanged)
		NOTIFY_HANDLER(IDC_DIRECTORIES, TVN_KEYDOWN, onKeyDownDirs)
		NOTIFY_HANDLER(IDC_UPLOAD_QUEUE, NM_CUSTOMDRAW, onCustomDraw)

		CHAIN_MSG_MAP(splitBase)
		CHAIN_MSG_MAP(baseClass)
	ALT_MSG_MAP(SHOWTREE_MESSAGE_MAP)
		MESSAGE_HANDLER(BM_SETCHECK, onShowTree)
	END_MSG_MAP()

	// Message handlers
	LRESULT onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onGetList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onRemove(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onItemChanged(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
	LRESULT onContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled);
	LRESULT onPrivateMessage(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onGrantSlot(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onGrantSlotHour(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onGrantSlotDay(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onGrantSlotWeek(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onUnGrantSlot(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onAddToFavorites(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled);
	LRESULT onSpeaker(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled);

	LRESULT onShowTree(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
		bHandled = FALSE;
		showTree = (wParam == BST_CHECKED);
		UpdateLayout(FALSE);
		return 0;
	}
	
	// Update control layouts
	void UpdateLayout(BOOL bResizeBars = TRUE);

private:
	enum {
		COLUMN_FIRST,
		COLUMN_FILE = COLUMN_FIRST,
		COLUMN_PATH,
		COLUMN_NICK,
		COLUMN_HUB,
		COLUMN_TRANSFERRED,
		COLUMN_SIZE,
		COLUMN_ADDED,
		COLUMN_WAITING,
		COLUMN_LAST
	};
	static int columnSizes[COLUMN_LAST];
	static int columnIndexes[COLUMN_LAST];
	
	User::Ptr getSelectedUser() {
		HTREEITEM selectedItem = GetParentItem();
		if (!selectedItem) return NULL;
		TCHAR nickBuf[512];
		ctrlQueued.GetItemText(selectedItem, nickBuf, 511);
		for(User::Iter i = UQFUsers.begin(); i != UQFUsers.end(); ++i) {
			if(((*i)->getNick()+" ("+(*i)->getLastHubName()+")") == Text::fromT(nickBuf)) {
				return *i;
				break;
			}
		}
		return NULL;
	}

	LRESULT onKeyDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
		NMLVKEYDOWN* kd = (NMLVKEYDOWN*) pnmh;
		if(kd->wVKey == VK_DELETE) {
			removeSelected();
		}
		return 0;
	}

	LRESULT onKeyDownDirs(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
		NMTVKEYDOWN* kd = (NMTVKEYDOWN*) pnmh;
		if(kd->wVKey == VK_DELETE) {
			removeSelectedUser();
			}
			return 0;
		}

	void removeSelected() {
		int i = -1;
		User::List RemoveUsers;
		while((i = ctrlList.GetNextItem(i, LVNI_SELECTED)) != -1) {
			// Ok let's cheat here, if you try to remove more users here is not working :(
			RemoveUsers.push_back(((UploadQueueItem*)ctrlList.getItemData(i))->User);
		}
		for(User::Iter i = RemoveUsers.begin(); i != RemoveUsers.end(); ++i) {
			UploadManager::getInstance()->clearUserFiles(*i);
		}
		updateStatus();
	}
	
	void removeSelectedUser() {
		User::Ptr User = getSelectedUser();
		if(User) {
			UploadManager::getInstance()->clearUserFiles(User);
		}
		updateStatus();
	}

	// Communication with manager
	void LoadAll();
	void UpdateSearch(int index, BOOL doDelete = TRUE);

	HTREEITEM GetParentItem();

	// Contained controls
	CButton ctrlShowTree;
	CContainedWindow showTreeContainer;
	
	bool showTree;
	bool closed;
	bool usingUserMenu;
	TCHAR * headerBuf;
	
	User::List UQFUsers;

	TypedListViewCtrl<UploadQueueItem, IDC_UPLOAD_QUEUE> ctrlList;
	CTreeViewCtrl ctrlQueued;
	
	CStatusBarCtrl ctrlStatus;
	int statusSizes[4];
	
	OMenu grantMenu;
	OMenu contextMenu;
	
	void AddFile(UploadQueueItem* aUQI);
	void RemoveUser(const User::Ptr& aUser);
	
	void addAllFiles(Upload * /*aUser*/);
	void updateStatus();

	// UploadManagerListener
	virtual void on(UploadManagerListener::QueueAdd, UploadQueueItem* aUQI) throw() {
		PostMessage(WM_SPEAKER, ADD_ITEM, (LPARAM)aUQI);
	}
	virtual void on(UploadManagerListener::QueueRemove, const User::Ptr& aUser) throw() {
		PostMessage(WM_SPEAKER, REMOVE, (LPARAM) new UserInfoBase(aUser));
	}
	virtual void on(UploadManagerListener::QueueItemRemove, UploadQueueItem* aUQI) throw() {
		PostMessage(WM_SPEAKER, REMOVE_ITEM, (LPARAM)aUQI);
	}
	virtual void on(UploadManagerListener::QueueUpdate) throw() {
		PostMessage(WM_SPEAKER, UPDATE_ITEMS, NULL);
	}
	virtual void on(SettingsManagerListener::Save, SimpleXML* /*xml*/) throw();
};

#endif
