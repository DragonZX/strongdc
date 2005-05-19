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

#if !defined(SEARCH_FRM_H)
#define SEARCH_FRM_H

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "FlatTabCtrl.h"
#include "TypedListViewCtrl.h"
#include "WinUtil.h"

#include "../client/Client.h"
#include "../client/SearchManager.h"
#include "../client/CriticalSection.h"
#include "../client/ClientManagerListener.h"
#include "../client/FavoriteManager.h"
#include "../client/QueueManager.h"

#include "UCHandler.h"

#define SEARCH_MESSAGE_MAP 6		// This could be any number, really...
#define SHOWUI_MESSAGE_MAP 7
#define FILTER_MESSAGE_MAP 8

class SearchFrame : public MDITabChildWindowImpl<SearchFrame, RGB(127, 127, 255), IDR_SEARCH>, 
	private SearchManagerListener, private ClientManagerListener,
	public UCHandler<SearchFrame>, public UserInfoBaseHandler<SearchFrame>,
	private SettingsManagerListener, private TimerManagerListener
{
public:
	static void openWindow(const tstring& str = Util::emptyStringW, LONGLONG size = 0, SearchManager::SizeModes mode = SearchManager::SIZE_ATLEAST, SearchManager::TypeModes type = SearchManager::TYPE_ANY);

	DECLARE_FRAME_WND_CLASS_EX(_T("SearchFrame"), IDR_SEARCH, 0, COLOR_3DFACE)

	typedef MDITabChildWindowImpl<SearchFrame, RGB(127, 127, 255), IDR_SEARCH> baseClass;
	typedef UCHandler<SearchFrame> ucBase;
	typedef UserInfoBaseHandler<SearchFrame> uicBase;

	BEGIN_MSG_MAP(SearchFrame)
		NOTIFY_HANDLER(IDC_RESULTS, LVN_GETDISPINFO, ctrlResults.onGetDispInfo)
		NOTIFY_HANDLER(IDC_RESULTS, LVN_COLUMNCLICK, ctrlResults.onColumnClick)
		NOTIFY_HANDLER(IDC_RESULTS, LVN_GETINFOTIP, ctrlResults.onInfoTip)
		NOTIFY_HANDLER(IDC_HUB, LVN_GETDISPINFO, ctrlHubs.onGetDispInfo)
		NOTIFY_HANDLER(IDC_RESULTS, NM_DBLCLK, onDoubleClickResults)
		NOTIFY_HANDLER(IDC_RESULTS, LVN_KEYDOWN, onKeyDown)
		NOTIFY_HANDLER(IDC_HUB, LVN_ITEMCHANGED, onItemChangedHub)
		NOTIFY_HANDLER(IDC_RESULTS, NM_CUSTOMDRAW, onCustomDraw)
		MESSAGE_HANDLER(WM_CREATE, onCreate)
		MESSAGE_HANDLER(WM_SETFOCUS, onFocus)
		MESSAGE_HANDLER(WM_CONTEXTMENU, onContextMenu)
		MESSAGE_HANDLER(WM_SPEAKER, onSpeaker)
		MESSAGE_HANDLER(WM_CTLCOLOREDIT, onCtlColor)
		MESSAGE_HANDLER(WM_CTLCOLORSTATIC, onCtlColor)
		MESSAGE_HANDLER(WM_CTLCOLORLISTBOX, onCtlColor)
		MESSAGE_HANDLER(WM_CLOSE, onClose)
		MESSAGE_HANDLER(WM_DRAWITEM, onDrawItem)
		MESSAGE_HANDLER(WM_MEASUREITEM, onMeasure)
		COMMAND_ID_HANDLER(IDC_DOWNLOAD, onDownload)
		COMMAND_ID_HANDLER(IDC_DOWNLOADTO, onDownloadTo)
		COMMAND_ID_HANDLER(IDC_DOWNLOADDIR, onDownloadWhole)
		COMMAND_ID_HANDLER(IDC_DOWNLOADDIRTO, onDownloadWholeTo)
		COMMAND_ID_HANDLER(IDC_VIEW_AS_TEXT, onViewAsText)
		COMMAND_ID_HANDLER(IDC_MP3, onMP3Info)
		COMMAND_ID_HANDLER(IDC_REMOVE, onRemove)
		COMMAND_ID_HANDLER(IDC_SEARCH, onSearch)
		COMMAND_ID_HANDLER(IDC_SEARCH_PAUSE, onPause)
		COMMAND_ID_HANDLER(IDC_COPY_NICK, onCopy)
		COMMAND_ID_HANDLER(IDC_COPY_FILENAME, onCopy)
		COMMAND_ID_HANDLER(IDC_COPY_PATH, onCopy)
		COMMAND_ID_HANDLER(IDC_COPY_SIZE, onCopy)
		COMMAND_ID_HANDLER(IDC_FREESLOTS, onFreeSlots)
		COMMAND_ID_HANDLER(IDC_TTHONLY, onTTHOnly)
		COMMAND_ID_HANDLER(IDC_COLLAPSED, onCollapsed)		
		COMMAND_ID_HANDLER(IDC_GETLIST, onGetList)
		COMMAND_ID_HANDLER(IDC_BROWSELIST, onBrowseList)
		COMMAND_ID_HANDLER(IDC_SEARCH_ALTERNATES, onSearchByTTH)
		COMMAND_ID_HANDLER(IDC_BITZI_LOOKUP, onBitziLookup)
		COMMAND_ID_HANDLER(IDC_COPY_LINK, onCopy)
		COMMAND_ID_HANDLER(IDC_COPY_TTH, onCopy)
		COMMAND_ID_HANDLER(IDC_PURGE, onPurge)		
		COMMAND_RANGE_HANDLER(IDC_DOWNLOAD_FAVORITE_DIRS, IDC_DOWNLOAD_FAVORITE_DIRS + FavoriteManager::getInstance()->getFavoriteDirs().size(), onDownloadFavoriteDirs)
		COMMAND_RANGE_HANDLER(IDC_DOWNLOAD_WHOLE_FAVORITE_DIRS, IDC_DOWNLOAD_WHOLE_FAVORITE_DIRS + FavoriteManager::getInstance()->getFavoriteDirs().size(), onDownloadWholeFavoriteDirs)
		COMMAND_RANGE_HANDLER(IDC_DOWNLOAD_TARGET, IDC_DOWNLOAD_TARGET + targets.size() + WinUtil::lastDirs.size(), onDownloadTarget)
		COMMAND_RANGE_HANDLER(IDC_DOWNLOAD_WHOLE_TARGET, IDC_DOWNLOAD_WHOLE_TARGET + WinUtil::lastDirs.size(), onDownloadWholeTarget)
		CHAIN_COMMANDS(ucBase)
		CHAIN_COMMANDS(uicBase)
		CHAIN_MSG_MAP(baseClass)
	ALT_MSG_MAP(SEARCH_MESSAGE_MAP)
		MESSAGE_HANDLER(WM_CHAR, onChar)
		MESSAGE_HANDLER(WM_KEYDOWN, onChar)
		MESSAGE_HANDLER(WM_KEYUP, onChar)
	ALT_MSG_MAP(SHOWUI_MESSAGE_MAP)
		MESSAGE_HANDLER(BM_SETCHECK, onShowUI)
	ALT_MSG_MAP(FILTER_MESSAGE_MAP)
		MESSAGE_HANDLER(WM_CTLCOLORLISTBOX, onCtlColor)
		MESSAGE_HANDLER(WM_KEYUP, onFilterChar)
		COMMAND_CODE_HANDLER(CBN_SELCHANGE, onSelChange)
	END_MSG_MAP()

	SearchFrame() : 
	searchBoxContainer(WC_COMBOBOX, this, SEARCH_MESSAGE_MAP),
		searchContainer(WC_EDIT, this, SEARCH_MESSAGE_MAP), 
		purgeContainer(WC_EDIT, this, SEARCH_MESSAGE_MAP), 
		sizeContainer(WC_EDIT, this, SEARCH_MESSAGE_MAP), 
		modeContainer(WC_COMBOBOX, this, SEARCH_MESSAGE_MAP),
		sizeModeContainer(WC_COMBOBOX, this, SEARCH_MESSAGE_MAP),
		fileTypeContainer(WC_COMBOBOX, this, SEARCH_MESSAGE_MAP),
		showUIContainer(WC_COMBOBOX, this, SHOWUI_MESSAGE_MAP),
		slotsContainer(WC_COMBOBOX, this, SEARCH_MESSAGE_MAP),
		tthContainer(WC_COMBOBOX, this, SEARCH_MESSAGE_MAP),
		collapsedContainer(WC_COMBOBOX, this, SEARCH_MESSAGE_MAP),
		doSearchContainer(WC_COMBOBOX, this, SEARCH_MESSAGE_MAP),
		resultsContainer(WC_LISTVIEW, this, SEARCH_MESSAGE_MAP),
		hubsContainer(WC_LISTVIEW, this, SEARCH_MESSAGE_MAP),
		ctrlFilterContainer(WC_EDIT, this, FILTER_MESSAGE_MAP),
		ctrlFilterSelContainer(WC_COMBOBOX, this, FILTER_MESSAGE_MAP),
		initialSize(0), initialMode(SearchManager::SIZE_ATLEAST), initialType(SearchManager::TYPE_ANY),
		showUI(true), onlyFree(false), closed(false), isHash(false), droppedResults(0),
		expandSR(false), exactSize1(false), exactSize2(0), onlyTTH(BOOLSETTING(SEARCH_ONLY_TTH)), searches(0)
	{	
		SearchManager::getInstance()->addListener(this);
		headerBuf = new TCHAR[128];
		useGrouping = BOOLSETTING(GROUP_SEARCH_RESULTS);
	}

	virtual ~SearchFrame() {
		images.Destroy();
		searchTypes.Destroy();
		delete[] headerBuf;
	}
	virtual void OnFinalMessage(HWND /*hWnd*/) { delete this; }

	LRESULT onChar(UINT uMsg, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onClose(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT onDrawItem(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT onMeasure(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT onContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled);
	LRESULT onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onCtlColor(UINT uMsg, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onDoubleClickResults(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
	LRESULT onDownloadTarget(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onDownloadTo(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onDownloadWholeTarget(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onDownloadWholeTo(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onDownloadFavoriteDirs(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onDownloadWholeFavoriteDirs(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onSpeaker(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT onSearchByTTH(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onCopy(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onBitziLookup(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled);
	LRESULT onFilterChar(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onSelChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onPurge(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onGetList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onBrowseList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	void UpdateLayout(BOOL bResizeBars = TRUE);
	void runUserCommand(UserCommand& uc);

	void removeSelected() {
		int i = -1;
		while( (i = ctrlResults.GetNextItem(-1, LVNI_SELECTED)) != -1) {
			ctrlResults.removeGroupedItem(ctrlResults.getItemData(i));
		}
	}
	
	LRESULT onMP3Info(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		ctrlResults.forEachSelected(&SearchInfo::GetMP3Info);
		return 0;
	}
	
	LRESULT onDownload(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		ctrlResults.forEachSelectedT(SearchInfo::Download(Text::toT(SETTING(DOWNLOAD_DIRECTORY))));
		return 0;
	}

	LRESULT onViewAsText(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		ctrlResults.forEachSelected(&SearchInfo::view);
		return 0;
	}

	LRESULT onDownloadWhole(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		ctrlResults.forEachSelectedT(SearchInfo::DownloadWhole(Text::toT(SETTING(DOWNLOAD_DIRECTORY))));
		return 0;
	}
	
	LRESULT onRemove(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		removeSelected();
		return 0;
	}

	LRESULT onFreeSlots(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		onlyFree = (ctrlSlots.GetCheck() == 1);
		return 0;
	}

	LRESULT onTTHOnly(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		onlyTTH = (ctrlTTH.GetCheck() == 1);
		return 0;
	}

	LRESULT onCollapsed(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		expandSR = (ctrlCollapsed.GetCheck() == 1);
		return 0;
	}

	LRESULT onSearch(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		onEnter();
 		return 0;
	}

	LRESULT onKeyDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
		NMLVKEYDOWN* kd = (NMLVKEYDOWN*) pnmh;
		
		if(kd->wVKey == VK_DELETE) {
			removeSelected();
		} 
		return 0;
	}

	LRESULT onFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		if(::IsWindow(ctrlSearch))
			ctrlSearch.SetFocus();
		return 0;
	}

	LRESULT onShowUI(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
		bHandled = FALSE;
		showUI = (wParam == BST_CHECKED);
		UpdateLayout(FALSE);
		return 0;
	}

	void SearchFrame::setInitial(const tstring& str, LONGLONG size, SearchManager::SizeModes mode, SearchManager::TypeModes type) {
		initialString = str; initialSize = size; initialMode = mode; initialType = type; bPaused = false;
	}

	LRESULT onPause(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		if(bPaused == true) {
			bPaused = false;
			for(SearchInfo::Iter i = PausedResults.begin(); i != PausedResults.end(); ++i) {
				PostMessage(WM_SPEAKER, ADD_RESULT, (LPARAM)(*i));
			}
			PausedResults.clear();
			ctrlStatus.SetText(2, Text::toT(Util::toString(ctrlResults.GetItemCount()) + " " + STRING(FILES)).c_str());			
			ctrlPauseSearch.SetWindowText(CTSTRING(PAUSE_SEARCH));
		} else {
			bPaused = true;
			ctrlPauseSearch.SetWindowText(CTSTRING(CONTINUE_SEARCH));
		}
		return 0;
	}
	
private:
	class SearchInfo;
	TCHAR * headerBuf;
public:
	TypedTreeListViewCtrl<SearchInfo, IDC_RESULTS>& getUserList() { return ctrlResults; };

private:
	enum {
		COLUMN_FIRST,
		COLUMN_FILENAME = COLUMN_FIRST,
		COLUMN_HITS,
		COLUMN_NICK,
		COLUMN_TYPE,
		COLUMN_SIZE,
		COLUMN_PATH,
		COLUMN_SLOTS,
		COLUMN_CONNECTION,
		COLUMN_HUB,
		COLUMN_EXACT_SIZE,
		COLUMN_UPLOAD,
		COLUMN_IP,		
		COLUMN_TTH,
		COLUMN_LAST
	};

	enum Images {
		IMAGE_UNKOWN,
		IMAGE_SLOW,
		IMAGE_NORMAL,
		IMAGE_FAST
	};

	class SearchInfo : public UserInfoBase {
	public:
		SearchResult* sr;

		typedef SearchInfo* Ptr;
		typedef vector<Ptr> List;
		typedef List::iterator Iter;

		SearchInfo::List subItems;

		SearchInfo(SearchResult* aSR) : UserInfoBase(aSR->getUser()), sr(aSR), collapsed(true), mainItem(false), main(NULL) { 
			sr->incRef(); update();
		};
		~SearchInfo() { 
			sr->decRef(); 
		};

		bool collapsed;
		bool mainItem;
		SearchInfo* main;

		void getList();
		void browseList();

		void view();
		void GetMP3Info();
		struct Download {
			Download(const tstring& aTarget) : tgt(aTarget) { };
			void operator()(SearchInfo* si);
			const tstring& tgt;
		};
		struct DownloadWhole {
			DownloadWhole(const tstring& aTarget) : tgt(aTarget) { };
			void operator()(SearchInfo* si);
			const tstring& tgt;
		};
		struct DownloadTarget {
			DownloadTarget(const tstring& aTarget) : tgt(aTarget) { };
			void operator()(SearchInfo* si);
			const tstring& tgt;
		};
		struct CheckSize {
			CheckSize() : size(-1), op(true), oneHub(true), hasTTH(false), firstTTH(true) { };
			void operator()(SearchInfo* si);
			tstring ext;
			int64_t size;
			bool oneHub;
			tstring hub;
			bool op;
			bool hasTTH;
			bool firstTTH;
			tstring tth;
		};
        
		const tstring& getText(int col) const {
			switch(col) {
				case COLUMN_NICK: return nick;
				case COLUMN_FILENAME: return fileName;
				case COLUMN_TYPE: return type;
				case COLUMN_SIZE: return size;
				case COLUMN_PATH: return path;
				case COLUMN_SLOTS: return slots;
				case COLUMN_CONNECTION: return connection;
				case COLUMN_HUB: return hubName;
				case COLUMN_EXACT_SIZE: return exactSize;
				case COLUMN_UPLOAD: return uploadSpeed;
				case COLUMN_IP: return ip;
				case COLUMN_TTH: return tth;
				case COLUMN_HITS: return totalUsers;
				default: return Util::emptyStringT;
			}
		}

		static int compareItems(SearchInfo* a, SearchInfo* b, int col) {
			if(!a->sr || !b->sr)
				return 0;

			switch(col) {
				case COLUMN_TYPE: 
					if(a->sr->getType() == b->sr->getType())
						return lstrcmpi(a->type.c_str(), b->type.c_str());
					else
						return(a->sr->getType() == SearchResult::TYPE_DIRECTORY) ? -1 : 1;
				case COLUMN_HITS: return compare(a->subItems.size(), b->subItems.size());
				case COLUMN_SLOTS: 
					if(a->sr->getFreeSlots() == b->sr->getFreeSlots())
						return compare(a->sr->getSlots(), b->sr->getSlots());
					else
						return compare(a->sr->getFreeSlots(), b->sr->getFreeSlots());
				case COLUMN_SIZE:
				case COLUMN_EXACT_SIZE: return compare(a->sr->getSize(), b->sr->getSize());
				case COLUMN_UPLOAD: return compare(a->uploadSpeed,b->uploadSpeed);
				default: return lstrcmpi(a->getText(col).c_str(), b->getText(col).c_str());
			}
		}

		int imageIndex() {
			int image = 0;
			if (BOOLSETTING(USE_SYSTEM_ICONS)) {
				image = sr->getType() == SearchResult::TYPE_FILE ? WinUtil::getIconIndex(Text::toT(sr->getFile())) : WinUtil::getDirIconIndex();
			} else {
				const string& tmp = sr->getUser()->getConnection();
				if( (tmp == "28.8Kbps") ||
					(tmp == "33.6Kbps") ||
					(tmp == "56Kbps") ||
					(tmp == SettingsManager::connectionSpeeds[SettingsManager::SPEED_MODEM]) ||
					(tmp == SettingsManager::connectionSpeeds[SettingsManager::SPEED_SATELLITE]) ||
					(tmp == SettingsManager::connectionSpeeds[SettingsManager::SPEED_WIRELESS]) ||
					(tmp == SettingsManager::connectionSpeeds[SettingsManager::SPEED_ISDN]) ) {
					image = 1;
				} else if( (tmp == SettingsManager::connectionSpeeds[SettingsManager::SPEED_CABLE]) ||
					(tmp == SettingsManager::connectionSpeeds[SettingsManager::SPEED_DSL]) ) {
					image = 2;
				} else if( (tmp == SettingsManager::connectionSpeeds[SettingsManager::SPEED_T1]) ||
					(tmp == SettingsManager::connectionSpeeds[SettingsManager::SPEED_T3]) ) {
					image = 3;
				}
				if(sr->getType() == SearchResult::TYPE_FILE)
					image+=4;
			}
			return image;
		}

		void update() { 
			if(sr->getType() == SearchResult::TYPE_FILE) {
				if(sr->getFile().rfind(_T('\\')) == tstring::npos) {
					fileName = Text::toT(sr->getUtf8() ? sr->getFile() : Text::acpToUtf8(sr->getFile()));
				} else {
					fileName = Text::toT(Util::getFileName(sr->getUtf8() ? sr->getFile() : Text::acpToUtf8(sr->getFile())));
					path = Text::toT(Util::getFilePath(sr->getUtf8() ? sr->getFile() : Text::acpToUtf8(sr->getFile())));
				}

				type = Text::toT(Util::getFileExt(Text::fromT(fileName)));
				if(!type.empty() && type[0] == _T('.'))
					type.erase(0, 1);
				size = Text::toT(Util::formatBytes(sr->getSize()));
				exactSize = Text::toT(Util::formatExactSize(sr->getSize()));
			} else {
				fileName = Text::toT(sr->getUtf8() ? sr->getFileName() : Text::acpToUtf8(sr->getFileName()));
				path = Text::toT(sr->getUtf8() ? sr->getFile() : Text::acpToUtf8(sr->getFile()));
				type = TSTRING(DIRECTORY);
			}
			nick = Text::toT(sr->getUser()->getNick());
			connection = Text::toT(sr->getUser()->getConnection());
			hubName = Text::toT(sr->getHubName());
			slots = Text::toT(sr->getSlotString());
			ip = Text::toT(sr->getIP());
			tstring tmpCountry = Text::toT(Util::getIpCountry(sr->getIP()));
			if(!tmpCountry.empty())
				ip = tmpCountry + _T(" (") + ip + _T(")");
			if(sr->getTTH() != NULL)
				setTTH(Text::toT(sr->getTTH()->toBase32()));
			
			if(!sr->getIP().empty())
				flagimage = WinUtil::getFlagImage(Text::fromT(tmpCountry).c_str());
			else 
				flagimage = 0;

			if(user->getDownloadSpeed()<1) {
				int status = user->getStatus();
				string Omezeni = user->getUpload();
				if (!Omezeni.empty()) {
					uploadSpeed = Text::toT(Util::formatBytes(Util::toInt64(Omezeni)*1024)+"/s");
				} else if( (status == 8) || (status == 9)  || (status == 10) || (status == 11)) {
					uploadSpeed = Text::toT(">=100 kB/s");
				} else {
					uploadSpeed = Text::toT("N/A");
				}
			} else
				uploadSpeed = Text::toT(Util::formatBytes(user->getDownloadSpeed())+"/s");

		}

		SearchInfo* createMainItem() { return this; }
		const string getGroupingString() { return Text::fromT(tth); }
		void updateMainItem() {
			u_int32_t total = main->subItems.size();
			if(total != 0) {
				TCHAR buf[256];
				_sntprintf(buf, 255, _T("%d %s"), total + 1, TSTRING(USERS));
				buf[255] = NULL;
				main->totalUsers = buf;
			} else {
				main->totalUsers = Util::emptyStringT;
			}
		}

		GETSET(tstring, nick, Nick);
		GETSET(tstring, connection, Connection)
		GETSET(tstring, fileName, FileName);
		GETSET(tstring, path, Path);
		GETSET(tstring, type, Type);
		GETSET(tstring, hubName, HubName);
		GETSET(tstring, size, Size);
		GETSET(tstring, slots, Slots);
		GETSET(tstring, exactSize, ExactSize);
		GETSET(tstring, ip, IP);
		GETSET(tstring, tth, TTH);
		GETSET(int, flagimage, flagImage);
		GETSET(tstring, uploadSpeed, UploadSpeed);
		tstring totalUsers;
	};

	struct HubInfo : public FastAlloc<HubInfo> {
		HubInfo(const tstring& aIpPort, const tstring& aName, bool aOp) : ipPort(aIpPort),
			name(aName), op(aOp) { };

		const tstring& getText(int col) const {
			return (col == 0) ? name : Util::emptyStringT;
		}
		static int compareItems(HubInfo* a, HubInfo* b, int col) {
			return (col == 0) ? lstrcmpi(a->name.c_str(), b->name.c_str()) : 0;
		}
		int imageIndex() {
			return 0;
		}

		tstring ipPort;
		tstring name;
		bool op;
	};

	// WM_SPEAKER
	enum Speakers {
		ADD_RESULT,
		FILTER_RESULT,
		HUB_ADDED,
		HUB_CHANGED,
		HUB_REMOVED,
		QUEUE_STATS,
		SEARCH_START,
		RESORT
	};

	tstring initialString;
	int64_t initialSize;
	SearchManager::SizeModes initialMode;
	SearchManager::TypeModes initialType;

	CStatusBarCtrl ctrlStatus;
	CEdit ctrlSearch;
	CComboBox ctrlSearchBox;
	CEdit ctrlSize;
	CComboBox ctrlMode;
	CComboBox ctrlSizeMode;
	CComboBox ctrlFiletype;
	CImageList searchTypes;
	CButton ctrlDoSearch;
	CButton ctrlPauseSearch;
	CButton ctrlPurge;	

	BOOL ListMeasure(HWND hwnd, UINT uCtrlId, MEASUREITEMSTRUCT *mis);
	BOOL ListDraw(HWND hwnd, UINT uCtrlId, DRAWITEMSTRUCT *dis);
	
	CContainedWindow searchContainer;
	CContainedWindow searchBoxContainer;
	CContainedWindow sizeContainer;
	CContainedWindow modeContainer;
	CContainedWindow sizeModeContainer;
	CContainedWindow fileTypeContainer;
	CContainedWindow slotsContainer;
	CContainedWindow tthContainer;
	CContainedWindow collapsedContainer;
	CContainedWindow showUIContainer;
	CContainedWindow doSearchContainer;
	CContainedWindow resultsContainer;
	CContainedWindow hubsContainer;
	CContainedWindow purgeContainer;
	CContainedWindow ctrlFilterContainer;
	CContainedWindow ctrlFilterSelContainer;
	string filter;
	
	CStatic searchLabel, sizeLabel, optionLabel, typeLabel, hubsLabel, srLabel;
	CButton ctrlSlots, ctrlShowUI, ctrlTTH, ctrlCollapsed;
	bool showUI;

	CImageList images;
	TypedTreeListViewCtrl<SearchInfo, IDC_RESULTS> ctrlResults;
	TypedListViewCtrl<HubInfo, IDC_HUB> ctrlHubs;

	OMenu grantMenu;
	OMenu resultsMenu;
	OMenu targetMenu;
	OMenu targetDirMenu;
	OMenu copyMenu;
	
	TStringList search;
	StringList targets;
	StringList wholeTargets;
	SearchInfo::List PausedResults;

	CEdit ctrlFilter;
	CComboBox ctrlFilterSel;

	/** Parameter map for user commands */
	StringMap ucParams;

	bool onlyFree;
	bool onlyTTH;
	bool expandSR;
	bool isHash;
	bool bPaused;
	bool exactSize1;
	bool useGrouping;
	int64_t exactSize2;

	CriticalSection cs;

	static TStringList lastSearches;
	size_t droppedResults;

	bool closed;
	int searches;

	COLORREF barva;
	
	static int columnIndexes[];
	static int columnSizes[];


	void downloadSelected(const tstring& aDir, bool view = false); 
	void downloadWholeSelected(const tstring& aDir);
	void onEnter();
	void onTab(bool shift);

	void download(SearchResult* aSR, const tstring& aDir, bool view);
	
	virtual void on(SearchManagerListener::SR, SearchResult* aResult) throw();
	virtual void on(SearchManagerListener::Searching, SearchQueueItem* aSearch) throw();
	virtual void on(SearchManagerListener::Resort) throw();

	virtual void on(TimerManagerListener::Second, u_int32_t aTick) throw();

	// ClientManagerListener
	virtual void on(ClientConnected, Client* c) throw() { speak(HUB_ADDED, c); }
	virtual void on(ClientUpdated, Client* c) throw() { speak(HUB_CHANGED, c); }
	virtual void on(ClientDisconnected, Client* c) throw() { speak(HUB_REMOVED, c); }
	virtual void on(SettingsManagerListener::Save, SimpleXML* /*xml*/) throw();

	void initHubs();
	void onHubAdded(HubInfo* info);
	void onHubChanged(HubInfo* info);
	void onHubRemoved(HubInfo* info);
	void addEntry(SearchInfo* item, int pos);
	void updateSearchList();

	LRESULT onItemChangedHub(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

	void speak(Speakers s, Client* aClient) {
		HubInfo* hubInfo = new HubInfo(Text::toT(aClient->getIpPort()), Text::toT(aClient->getName()), aClient->getOp());
		PostMessage(WM_SPEAKER, WPARAM(s), LPARAM(hubInfo)); 
	};
};

#endif // !defined(SEARCH_FRM_H)

/**
 * @file
 * $Id$
 */

