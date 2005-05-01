/*
 * Copyright (C) 2001-2005 Jacek Sieka, arnetheduck on gmail point com
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

#if !defined(TRANSFER_VIEW_H)
#define TRANSFER_VIEW_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "../client/DownloadManager.h"
#include "../client/UploadManager.h"
#include "../client/CriticalSection.h"
#include "../client/ConnectionManagerListener.h"
#include "../client/ConnectionManager.h"
#include "../client/HashManager.h"

#include "OMenu.h"
#include "UCHandler.h"
#include "TypedListViewCtrl.h"
#include "WinUtil.h"
#include "resource.h"
#include "SearchFrm.h"

class TransferView : public CWindowImpl<TransferView>, private DownloadManagerListener, 
	private UploadManagerListener, private ConnectionManagerListener,
	public UserInfoBaseHandler<TransferView>, public UCHandler<TransferView>,
	private SettingsManagerListener
{
public:
	DECLARE_WND_CLASS(_T("TransferView"))

	TransferView() : PreviewAppsSize(0) {
		headerBuf = new TCHAR[128];
	};
	~TransferView(void);

	typedef UserInfoBaseHandler<TransferView> uibBase;
	typedef UCHandler<TransferView> ucBase;

	BEGIN_MSG_MAP(TransferView)
		NOTIFY_HANDLER(IDC_TRANSFERS, LVN_GETDISPINFO, ctrlTransfers.onGetDispInfo)
		NOTIFY_HANDLER(IDC_TRANSFERS, LVN_COLUMNCLICK, ctrlTransfers.onColumnClick)
		NOTIFY_HANDLER(IDC_TRANSFERS, LVN_GETINFOTIP, ctrlTransfers.onInfoTip)
		NOTIFY_HANDLER(IDC_TRANSFERS, LVN_KEYDOWN, onKeyDownTransfers)
		NOTIFY_HANDLER(IDC_TRANSFERS, NM_CUSTOMDRAW, onCustomDraw)
		NOTIFY_HANDLER(IDC_TRANSFERS, NM_DBLCLK, onDoubleClickTransfers)
		MESSAGE_HANDLER(WM_CREATE, onCreate)
		MESSAGE_HANDLER(WM_DESTROY, onDestroy)
		MESSAGE_HANDLER(WM_SPEAKER, onSpeaker)
		MESSAGE_HANDLER(WM_CONTEXTMENU, onContextMenu)
		MESSAGE_HANDLER(WM_SIZE, onSize)
		MESSAGE_HANDLER(WM_NOTIFYFORMAT, onNotifyFormat)
		COMMAND_ID_HANDLER(IDC_FORCE, onForce)
		COMMAND_ID_HANDLER(IDC_SEARCH_ALTERNATES, onSearchAlternates)
		COMMAND_ID_HANDLER(IDC_REMOVE, onRemove)
		COMMAND_ID_HANDLER(IDC_REMOVEALL, onRemoveAll)
		COMMAND_ID_HANDLER(IDC_CONNECT_ALL, onConnectAll)
		COMMAND_ID_HANDLER(IDC_DISCONNECT_ALL, onDisconnectAll)
		COMMAND_ID_HANDLER(IDC_COLLAPSE_ALL, onCollapseAll)
		COMMAND_ID_HANDLER(IDC_EXPAND_ALL, onExpandAll)
		COMMAND_ID_HANDLER(IDC_MENU_SLOWDISCONNECT, onSlowDisconnect)
		MESSAGE_HANDLER_HWND(WM_INITMENUPOPUP, OMenu::onInitMenuPopup)
		MESSAGE_HANDLER_HWND(WM_MEASUREITEM, OMenu::onMeasureItem)
		MESSAGE_HANDLER_HWND(WM_DRAWITEM, OMenu::onDrawItem)
		COMMAND_RANGE_HANDLER(IDC_PREVIEW_APP, IDC_PREVIEW_APP + PreviewAppsSize, onPreviewCommand)
		CHAIN_COMMANDS(ucBase)
		CHAIN_COMMANDS(uibBase)
	END_MSG_MAP()

	LRESULT onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onSpeaker(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled);
	LRESULT onSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled);
	LRESULT onForce(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);			
	LRESULT onSearchAlternates(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled);
	LRESULT onDoubleClickTransfers(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
	LRESULT onConnectAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onDisconnectAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onSlowDisconnect(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onPreviewCommand(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onWhoisIP(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	void runUserCommand(UserCommand& uc);
	void prepareClose();

	LRESULT onCollapseAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		CollapseAll();
		return 0;
	}

	LRESULT onExpandAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		ExpandAll();
		return 0;
	}

	LRESULT onKeyDownTransfers(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
		NMLVKEYDOWN* kd = (NMLVKEYDOWN*) pnmh;
		if(kd->wVKey == VK_DELETE) {
			ctrlTransfers.forEachSelected(&ItemInfo::disconnect);
		}
		return 0;
	}

	LRESULT onRemove(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		ctrlTransfers.forEachSelected(&ItemInfo::disconnect);
		return 0;
	}

	LRESULT onRemoveAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		ctrlTransfers.forEachSelected(&ItemInfo::removeAll);
		return 0;
	}

	LRESULT onDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		ItemInfo::Map::iterator i;
		for(i = transferItems.begin(); i != transferItems.end(); ++i) {
			delete i->second;
		}

		for(map<string, ItemInfo*>::iterator i = ctrlTransfers.mainItems.begin(); i != ctrlTransfers.mainItems.end(); i++) {
			delete i->second;
		}

		return 0;
	}

	LRESULT onNotifyFormat(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
#ifdef _UNICODE
		return NFR_UNICODE;
#else
		return NFR_ANSI;
#endif		
	}

private:
	/** Parameter map for user commands */
	StringMap ucParams;
	TCHAR * headerBuf;

	class ItemInfo;	
	int PreviewAppsSize;
public:
	TypedTreeListViewCtrl<ItemInfo, IDC_TRANSFERS>& getUserList() { return ctrlTransfers; };
private:
	enum {
		ADD_ITEM,
		REMOVE_ITEM,
		UPDATE_ITEM,
		UPDATE_ITEMS
	};

	enum {
		COLUMN_FIRST,
		COLUMN_USER = COLUMN_FIRST,
		COLUMN_HUB,
		COLUMN_STATUS,
		COLUMN_TIMELEFT,
		COLUMN_SPEED,
		COLUMN_FILE,
		COLUMN_SIZE,
		COLUMN_PATH,
		COLUMN_IP,
		COLUMN_RATIO,
		COLUMN_LAST
	};

	enum {
		IMAGE_DOWNLOAD = 0,
		IMAGE_UPLOAD,
		IMAGE_SEGMENT
	};

	class ItemInfo : public UserInfoBase, public Flags {
	public:
		typedef HASH_MAP<ConnectionQueueItem*, ItemInfo*, PointerHash<ConnectionQueueItem> > Map;
		typedef Map::iterator MapIter;

		typedef ItemInfo* Ptr;
		typedef vector<Ptr> List;
		typedef List::iterator Iter;

		ItemInfo::List subItems;

		enum Flags {
			FLAG_COMPRESSED = 0x01
		};
		enum Status {
			STATUS_RUNNING,
			STATUS_WAITING
		};
		enum Types {
			TYPE_DOWNLOAD,
			TYPE_UPLOAD
		};

		ItemInfo(const User::Ptr& u, Types t = TYPE_DOWNLOAD, Status s = STATUS_WAITING, 
			int64_t p = 0, int64_t sz = 0, int st = 0, int a = 0) : UserInfoBase(u), type(t), 
			status(s), pos(p), size(sz), start(st), actual(a), speed(0), timeLeft(0),
			updateMask((u_int32_t)-1), collapsed(true), mainItem(false), main(NULL),
			Target(Util::emptyString), file(Util::emptyStringT), numberOfSegments(0),
			compressRatio(1.0), flagImage(0), upperUpdated(false) { update(); };

		Types type;
		Status status;
		int64_t pos;
		int64_t size;
		int64_t start;
		int64_t actual;
		int64_t speed;
		int64_t timeLeft;
		tstring statusString;
		tstring file;
		tstring IP;
		tstring country;		
		ItemInfo* main;
		string Target;
		bool collapsed;
		bool mainItem;
		u_int16_t numberOfSegments;
		double compressRatio;
		bool upperUpdated;
		int flagImage;

		enum {
			MASK_USER = 1 << COLUMN_USER,
			MASK_HUB = 1 << COLUMN_HUB,
			MASK_STATUS = 1 << COLUMN_STATUS,
			MASK_TIMELEFT = 1 << COLUMN_TIMELEFT,
			MASK_SPEED = 1 << COLUMN_SPEED,
			MASK_FILE = 1 << COLUMN_FILE,
			MASK_SIZE = 1 << COLUMN_SIZE,
			MASK_PATH = 1 << COLUMN_PATH,
			MASK_IP = 1 << COLUMN_IP,
			MASK_RATIO = 1 << COLUMN_RATIO
	};
		tstring columns[COLUMN_LAST];
		u_int32_t updateMask;
		void update();

		void disconnect();
		void removeAll();
		void deleteSelf() { delete this; }	

		double getRatio() {
			if(mainItem) {
				if((compressRatio > 1) || (compressRatio < 0)) compressRatio = 1.0;
				return compressRatio;
			}
			return (pos > 0) ? (double)actual / (double)pos : 1.0;
		}

		const tstring& getText(int col) const {
			return columns[col];
		}

		static int compareItems(ItemInfo* a, ItemInfo* b, int col) {
			if(a->status == b->status) {
				if(a->type != b->type) {
					return (a->type == ItemInfo::TYPE_DOWNLOAD) ? -1 : 1;					
				}
			} else {
				return (a->status == ItemInfo::STATUS_RUNNING) ? -1 : 1;
			}

			switch(col) {
				case COLUMN_USER:
					{
						dcassert(a->mainItem == b->mainItem);

						if(a->subItems.size() == b->subItems.size())
							return lstrcmpi(a->columns[COLUMN_USER].c_str(), b->columns[COLUMN_USER].c_str());

						return compare(a->subItems.size(), b->subItems.size());						
					}
				case COLUMN_STATUS: return 0;
				case COLUMN_TIMELEFT: return compare(a->timeLeft, b->timeLeft);
				case COLUMN_SPEED: return compare(a->speed, b->speed);
				case COLUMN_SIZE: return compare(a->size, b->size);
				case COLUMN_RATIO: return compare(a->getRatio(), b->getRatio());
				default: return lstrcmpi(a->columns[col].c_str(), b->columns[col].c_str());
			}
		}

		int imageIndex() {
			return (type == TYPE_UPLOAD) ? IMAGE_UPLOAD : (mainItem ? IMAGE_DOWNLOAD : IMAGE_SEGMENT);
		}

		ItemInfo* createMainItem() {
	  		ItemInfo* h = new ItemInfo(user, ItemInfo::TYPE_DOWNLOAD, ItemInfo::STATUS_WAITING);
			h->Target = Target;
			h->size = size;
			h->columns[COLUMN_STATUS] = h->statusString = TSTRING(CONNECTING);
			h->numberOfSegments = 0;
			h->updateMask |= ItemInfo::MASK_FILE | ItemInfo::MASK_PATH | ItemInfo::MASK_SIZE;
			h->update();

			return h;
		}
		void updateMainItem() {
			if(main->subItems.size() == 1) {
				main->user = main->subItems.front()->user;
			}
			main->updateMask |= ItemInfo::MASK_USER;
			main->update();
		}
	};

	CriticalSection cs;
	ItemInfo::Map transferItems;

	TypedTreeListViewCtrl<ItemInfo, IDC_TRANSFERS> ctrlTransfers;
	static int columnIndexes[];
	static int columnSizes[];

	OMenu transferMenu;
	OMenu segmentedMenu;
	OMenu usercmdsMenu;
	OMenu previewMenu;
	CImageList arrows;
	HICON hIconCompressed;
	COLORREF barva;

	HDC hDCDB; // Double buffer DC
	HBITMAP hDCBitmap; // Double buffer Bitmap for above DC
	StringList searchFilter;

	virtual void on(ConnectionManagerListener::Added, ConnectionQueueItem* aCqi) throw();
	virtual void on(ConnectionManagerListener::Failed, ConnectionQueueItem* aCqi, const string& aReason) throw();
	virtual void on(ConnectionManagerListener::Removed, ConnectionQueueItem* aCqi) throw();
	virtual void on(ConnectionManagerListener::StatusChanged, ConnectionQueueItem* aCqi) throw();

	virtual void on(DownloadManagerListener::Complete, Download* aDownload, bool isTree) throw() { onTransferComplete(aDownload, false, isTree);}
	virtual void on(DownloadManagerListener::Failed, Download* aDownload, const string& aReason) throw();
	virtual void on(DownloadManagerListener::Starting, Download* aDownload, bool isActiveSegment, u_int16_t) throw();
	virtual void on(DownloadManagerListener::Tick, const Download::List& aDownload) throw();
	virtual void on(DownloadManagerListener::Status, ConnectionQueueItem* aCqi, const string& aMessage) throw();
	virtual void on(DownloadManagerListener::Verifying, const string& fileName, int64_t) throw();

	virtual void on(UploadManagerListener::Starting, Upload* aUpload) throw();
	virtual void on(UploadManagerListener::Tick, const Upload::List& aUpload) throw();
	virtual void on(UploadManagerListener::Complete, Upload* aUpload) throw() { onTransferComplete(aUpload, true, false); }

	virtual void on(SettingsManagerListener::Save, SimpleXML* /*xml*/) throw();

	void onTransferComplete(Transfer* aTransfer, bool isUpload, bool isTree);

	void CollapseAll();
	void ExpandAll();

	inline void setMainItem(ItemInfo* i) {
		if(i->main != NULL) {
			ItemInfo* h = i->main;		
			if(h->Target != i->Target) {
				Lock l(cs);
				ctrlTransfers.removeGroupedItem(i, false);
				ctrlTransfers.insertGroupedItem(i, i->Target, false);
			}
		} else {
			i->main = ctrlTransfers.findMainItem(i->Target);
		}
	}
};

#endif // !defined(TRANSFER_VIEW_H)

/**
 * @file
 * $Id$
 */