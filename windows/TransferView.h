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

#if !defined(TRANSFER_VIEW_H)
#define TRANSFER_VIEW_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "..\client\DownloadManagerListener.h"
#include "..\client\UploadManagerListener.h"
#include "..\client\ConnectionManagerListener.h"
#include "..\client\QueueManagerListener.h"
#include "..\client\TaskQueue.h"
#include "..\client\forward.h"
#include "..\client\Util.h"
#include "..\client\Download.h"
#include "..\client\Upload.h"

#include "OMenu.h"
#include "UCHandler.h"
#include "TypedListViewCtrl.h"
#include "WinUtil.h"
#include "resource.h"
#include "SearchFrm.h"

class TransferView : public CWindowImpl<TransferView>, private DownloadManagerListener, 
	private UploadManagerListener, private ConnectionManagerListener, private QueueManagerListener,
	public UserInfoBaseHandler<TransferView>, public UCHandler<TransferView>,
	private SettingsManagerListener
{
public:
	DECLARE_WND_CLASS(_T("TransferView"))

	TransferView() : PreviewAppsSize(0) { }
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
		COMMAND_ID_HANDLER(IDC_SEARCH_ALTERNATES, onSearchAlternates)
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
		ctrlTransfers.deleteAllItems();
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
	class ItemInfo;	
public:
	typedef TypedTreeListViewCtrl<ItemInfo, IDC_TRANSFERS, tstring, noCaseStringHash, noCaseStringEq> ItemInfoList;
	ItemInfoList& getUserList() { return ctrlTransfers; }
private:
	enum {
		ADD_ITEM,
		REMOVE_ITEM,
		UPDATE_ITEM,
		UPDATE_PARENT
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

	struct UpdateInfo;
	class ItemInfo : public UserInfoBase {
	public:
		enum Status {
			STATUS_RUNNING,
			STATUS_WAITING,
			// special statuses
			TREE_DOWNLOAD
		};

		ItemInfo(const UserPtr& u, bool aDownload);

		bool download;
		bool transferFailed;
		bool collapsed;
		
		uint8_t flagImage;
		int16_t running;

		ItemInfo* parent;
		UserPtr user;
		Status status;
		size_t childCount;
		
		int64_t pos;
		int64_t size;
		int64_t actual;
		int64_t speed;
		int64_t timeLeft;
		uint64_t fileBegin;
		
		tstring ip;
		tstring statusString;
		tstring target;

		void update(const UpdateInfo& ui);

		const UserPtr& getUser() const { return user; }

		void disconnect();
		void removeAll();

		double getRatio() const { return (pos > 0) ? (double)actual / (double)pos : 1.0; }

		const tstring getText(uint8_t col) const {
			switch(col) {
				case COLUMN_USER: return (childCount <= 1) ? Text::toT(user->getFirstNick()) : (Util::toStringW(childCount) + _T(' ') + TSTRING(USERS));
				case COLUMN_HUB: return (running == -1) ? WinUtil::getHubNames(user).first : (Util::toStringW(running) + _T(' ') + TSTRING(NUMBER_OF_SEGMENTS));
				case COLUMN_STATUS: return statusString;
				case COLUMN_TIMELEFT: return (status == STATUS_RUNNING) ? Util::formatSeconds(timeLeft) : Util::emptyStringT;
				case COLUMN_SPEED: return (status == STATUS_RUNNING) ? (Util::formatBytesW(speed) + _T("/s")) : Util::emptyStringT;
				case COLUMN_FILE: {
					tstring file = (status == TREE_DOWNLOAD ? _T("TTH: ") : Util::emptyStringT);
					file += Util::getFileName(target);
					return file;
				}
				case COLUMN_SIZE: return Util::formatBytesW(size); 
				case COLUMN_PATH: return Util::getFilePath(target);
				case COLUMN_IP: return ip;
				case COLUMN_RATIO: return (status == STATUS_RUNNING) ? Util::toStringW(getRatio()) : Util::emptyStringT;
				default: return Util::emptyStringT;
			}
		}

		static int compareItems(const ItemInfo* a, const ItemInfo* b, uint8_t col);

		uint8_t imageIndex() const { return static_cast<uint8_t>(!download ? IMAGE_UPLOAD : (!parent ? IMAGE_DOWNLOAD : IMAGE_SEGMENT)); }

		ItemInfo* createParent() {
	  		ItemInfo* h = new ItemInfo(user, true);
			h->target = target;
			h->statusString = TSTRING(CONNECTING);
			return h;
		}

		void updateParent(vector<ItemInfo*>& children) {
			parent->childCount = children.size();
			if(childCount == 1) {
				ItemInfo* i = children.front();
				parent->user = i->user;
				parent->running = -1;
				parent->flagImage = i->flagImage;
				parent->ip = i->ip;
			} else {
				ip = Util::emptyStringT;
			}
		}

		const tstring& getGroupCond() const { return target; }

		inline void deleteSelf() { delete this; }
	};

	struct UpdateInfo : public Task {
		enum {
			MASK_POS			= 0x01,
			MASK_SIZE			= 0x02,
			MASK_ACTUAL			= 0x04,
			MASK_SPEED			= 0x08,
			MASK_FILE			= 0x10,
			MASK_STATUS			= 0x20,
			MASK_TIMELEFT		= 0x40,
			MASK_IP				= 0x80,
			MASK_STATUS_STRING	= 0x100,
			MASK_SEGMENT		= 0x200
		};

		bool operator==(const ItemInfo& ii) const { return download == ii.download && user == ii.user; }

		UpdateInfo(const UserPtr& aUser, bool isDownload, bool isTransferFailed = false) : updateMask(0), user(aUser), queueItem(NULL), download(isDownload), transferFailed(isTransferFailed), fileList(false), flagImage(0) { }
		UpdateInfo(QueueItem* qi, bool isDownload, bool isTransferFailed = false) : updateMask(0), queueItem(qi), download(isDownload), transferFailed(isTransferFailed), fileList(false), flagImage(0) { qi->inc(); }

		~UpdateInfo() { if(queueItem) queueItem->dec(); }

		uint32_t updateMask;

		UserPtr user;
		QueueItem* queueItem;

		bool download;
		bool transferFailed;
		bool fileList;
		uint8_t flagImage;		
		void setRunning(int16_t aRunning) { running = aRunning; updateMask |= MASK_SEGMENT; }
		int16_t running;
		void setStatus(ItemInfo::Status aStatus) { status = aStatus; updateMask |= MASK_STATUS; }
		ItemInfo::Status status;
		void setPos(int64_t aPos) { pos = aPos; updateMask |= MASK_POS; }
		int64_t pos;
		void setSize(int64_t aSize) { size = aSize; updateMask |= MASK_SIZE; }
		int64_t size;
		void setActual(int64_t aActual) { actual = aActual; updateMask |= MASK_ACTUAL; }
		int64_t actual;
		void setSpeed(int64_t aSpeed) { speed = aSpeed; updateMask |= MASK_SPEED; }
		int64_t speed;
		void setTimeLeft(int64_t aTimeLeft) { timeLeft = aTimeLeft; updateMask |= MASK_TIMELEFT; }
		int64_t timeLeft;
		void setStatusString(const tstring& aStatusString) { statusString = aStatusString; updateMask |= MASK_STATUS_STRING; }
		tstring statusString;
		void setTarget(const tstring& aTarget) { target = aTarget; updateMask |= MASK_FILE; }
		tstring target;
		void setIP(const tstring& aIP, uint8_t aFlagImage) { IP = aIP; flagImage = aFlagImage, updateMask |= MASK_IP; }
		tstring IP;
	};

	void speak(uint8_t type, UpdateInfo* ui) { tasks.add(type, ui); PostMessage(WM_SPEAKER); }

	ItemInfoList ctrlTransfers;
	static int columnIndexes[];
	static int columnSizes[];

	OMenu transferMenu;
	OMenu segmentedMenu;
	OMenu usercmdsMenu;
	OMenu previewMenu;
	CImageList arrows;

	TaskQueue tasks;

	StringMap ucLineParams;
	int PreviewAppsSize;

	void on(ConnectionManagerListener::Added, const ConnectionQueueItem* aCqi) throw();
	void on(ConnectionManagerListener::Failed, const ConnectionQueueItem* aCqi, const string& aReason) throw();
	void on(ConnectionManagerListener::Removed, const ConnectionQueueItem* aCqi) throw();
	void on(ConnectionManagerListener::StatusChanged, const ConnectionQueueItem* aCqi) throw();

	void on(DownloadManagerListener::Complete, const Download* aDownload, bool isTree) throw() { onTransferComplete(aDownload, false, Util::getFileName(aDownload->getTarget()), isTree);}
	void on(DownloadManagerListener::Failed, const Download* aDownload, const string& aReason) throw();
	void on(DownloadManagerListener::Starting, const Download* aDownload) throw();
	void on(DownloadManagerListener::Tick, const DownloadList& aDownload) throw();
	void on(DownloadManagerListener::Status, const UserConnection*, const string&) throw();

	void on(UploadManagerListener::Starting, const Upload* aUpload) throw();
	void on(UploadManagerListener::Tick, const UploadList& aUpload) throw();
	void on(UploadManagerListener::Complete, const Upload* aUpload) throw() { onTransferComplete(aUpload, true, aUpload->getSourceFile(), false); }

	void on(QueueManagerListener::StatusUpdated, const QueueItem*) throw();
	void on(QueueManagerListener::Removed, const QueueItem*) throw();
	void on(QueueManagerListener::Finished, const QueueItem*, const string&, int64_t) throw();

	void on(SettingsManagerListener::Save, SimpleXML& /*xml*/) throw();

	void onTransferComplete(const Transfer* aTransfer, bool isUpload, const string& aFileName, bool isTree);

	void CollapseAll();
	void ExpandAll();

	ItemInfo* findItem(const UpdateInfo& ui, int& pos) const;
	void updateItem(int ii, uint32_t updateMask);
};

#endif // !defined(TRANSFER_VIEW_H)

/**
 * @file
 * $Id$
 */