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

#include "../client/ResourceManager.h"
#include "../client/SettingsManager.h"
#include "../client/ConnectionManager.h"
#include "../client/DownloadManager.h"
#include "../client/UploadManager.h"
#include "../client/QueueManager.h"
#include "../client/QueueItem.h"

#include "WinUtil.h"
#include "TransferView.h"
#include "MainFrm.h"

#include "BarShader.h"

int TransferView::columnIndexes[] = { COLUMN_USER, COLUMN_HUB, COLUMN_STATUS, COLUMN_TIMELEFT, COLUMN_SPEED, COLUMN_FILE, COLUMN_SIZE, COLUMN_PATH, COLUMN_IP, COLUMN_RATIO };
int TransferView::columnSizes[] = { 150, 100, 250, 75, 75, 175, 100, 200, 50, 75 };

static ResourceManager::Strings columnNames[] = { ResourceManager::USER, ResourceManager::HUB_SEGMENTS, ResourceManager::STATUS,
ResourceManager::TIME_LEFT, ResourceManager::SPEED, ResourceManager::FILENAME, ResourceManager::SIZE, ResourceManager::PATH,
ResourceManager::IP_BARE, ResourceManager::RATIO};

TransferView::~TransferView() {
	arrows.Destroy();
	OperaColors::ClearCache();
}

LRESULT TransferView::onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {

	arrows.CreateFromImage(IDB_ARROWS, 16, 3, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION | LR_SHARED);
	ctrlTransfers.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_HSCROLL | WS_VSCROLL | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS, WS_EX_CLIENTEDGE, IDC_TRANSFERS);
	ctrlTransfers.SetExtendedListViewStyle(LVS_EX_LABELTIP | LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP);

	WinUtil::splitTokens(columnIndexes, SETTING(MAINFRAME_ORDER), COLUMN_LAST);
	WinUtil::splitTokens(columnSizes, SETTING(MAINFRAME_WIDTHS), COLUMN_LAST);

	for(uint8_t j=0; j<COLUMN_LAST; j++) {
		int fmt = (j == COLUMN_SIZE || j == COLUMN_TIMELEFT || j == COLUMN_SPEED) ? LVCFMT_RIGHT : LVCFMT_LEFT;
		ctrlTransfers.InsertColumn(j, CTSTRING_I(columnNames[j]), fmt, columnSizes[j], j);
	}

	ctrlTransfers.setColumnOrderArray(COLUMN_LAST, columnIndexes);
	ctrlTransfers.setVisible(SETTING(MAINFRAME_VISIBLE));

	ctrlTransfers.SetBkColor(WinUtil::bgColor);
	ctrlTransfers.SetTextBkColor(WinUtil::bgColor);
	ctrlTransfers.SetTextColor(WinUtil::textColor);
	ctrlTransfers.setFlickerFree(WinUtil::bgBrush);

	ctrlTransfers.SetImageList(arrows, LVSIL_SMALL);
	ctrlTransfers.setSortColumn(COLUMN_USER);

	transferMenu.CreatePopupMenu();
	appendUserItems(transferMenu);
	transferMenu.AppendMenu(MF_SEPARATOR);
	transferMenu.AppendMenu(MF_STRING, IDC_FORCE, CTSTRING(FORCE_ATTEMPT));
	transferMenu.AppendMenu(MF_SEPARATOR);
	transferMenu.AppendMenu(MF_STRING, IDC_SEARCH_ALTERNATES, CTSTRING(SEARCH_FOR_ALTERNATES));

	previewMenu.CreatePopupMenu();
	previewMenu.AppendMenu(MF_SEPARATOR);
	usercmdsMenu.CreatePopupMenu();

	transferMenu.AppendMenu(MF_POPUP, (UINT)(HMENU)previewMenu, CTSTRING(PREVIEW_MENU));
	transferMenu.AppendMenu(MF_POPUP, (UINT)(HMENU)usercmdsMenu, CTSTRING(SETTINGS_USER_COMMANDS));
	transferMenu.AppendMenu(MF_SEPARATOR);
	transferMenu.AppendMenu(MF_STRING, IDC_MENU_SLOWDISCONNECT, CTSTRING(SETCZDC_DISCONNECTING_ENABLE));
	transferMenu.AppendMenu(MF_SEPARATOR);
	transferMenu.AppendMenu(MF_STRING, IDC_REMOVE, CTSTRING(CLOSE_CONNECTION));
	transferMenu.SetMenuDefaultItem(IDC_PRIVATEMESSAGE);

	segmentedMenu.CreatePopupMenu();
	segmentedMenu.AppendMenu(MF_STRING, IDC_SEARCH_ALTERNATES, CTSTRING(SEARCH_FOR_ALTERNATES));
	segmentedMenu.AppendMenu(MF_POPUP, (UINT)(HMENU)previewMenu, CTSTRING(PREVIEW_MENU));
	segmentedMenu.AppendMenu(MF_STRING, IDC_MENU_SLOWDISCONNECT, CTSTRING(SETCZDC_DISCONNECTING_ENABLE));
	segmentedMenu.AppendMenu(MF_SEPARATOR);
	segmentedMenu.AppendMenu(MF_STRING, IDC_CONNECT_ALL, CTSTRING(CONNECT_ALL));
	segmentedMenu.AppendMenu(MF_STRING, IDC_DISCONNECT_ALL, CTSTRING(DISCONNECT_ALL));
	segmentedMenu.AppendMenu(MF_SEPARATOR);
	segmentedMenu.AppendMenu(MF_STRING, IDC_EXPAND_ALL, CTSTRING(EXPAND_ALL));
	segmentedMenu.AppendMenu(MF_STRING, IDC_COLLAPSE_ALL, CTSTRING(COLLAPSE_ALL));
	segmentedMenu.AppendMenu(MF_SEPARATOR);
	segmentedMenu.AppendMenu(MF_STRING, IDC_REMOVEALL, CTSTRING(REMOVE_ALL));

	ConnectionManager::getInstance()->addListener(this);
	DownloadManager::getInstance()->addListener(this);
	UploadManager::getInstance()->addListener(this);
	QueueManager::getInstance()->addListener(this);
	SettingsManager::getInstance()->addListener(this);
	return 0;
}

void TransferView::prepareClose() {
	ctrlTransfers.saveHeaderOrder(SettingsManager::MAINFRAME_ORDER, SettingsManager::MAINFRAME_WIDTHS,
		SettingsManager::MAINFRAME_VISIBLE);

	ConnectionManager::getInstance()->removeListener(this);
	DownloadManager::getInstance()->removeListener(this);
	UploadManager::getInstance()->removeListener(this);
	QueueManager::getInstance()->removeListener(this);
	SettingsManager::getInstance()->removeListener(this);
	
}

LRESULT TransferView::onSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	RECT rc;
	GetClientRect(&rc);
	ctrlTransfers.MoveWindow(&rc);

	return 0;
}

LRESULT TransferView::onContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	if (reinterpret_cast<HWND>(wParam) == ctrlTransfers && ctrlTransfers.GetSelectedCount() > 0) { 
		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

		if(pt.x == -1 && pt.y == -1) {
			WinUtil::getContextMenuPos(ctrlTransfers, pt);
		}
		
		if(ctrlTransfers.GetSelectedCount() > 0) {
			int i = -1;
			bool bCustomMenu = false;
			const ItemInfo* ii = ctrlTransfers.getItemData(ctrlTransfers.GetNextItem(-1, LVNI_SELECTED));
			bool parent = !ii->parent && ctrlTransfers.findChildren(ii->getGroupCond()).size() > 1;

			if(!parent && (i = ctrlTransfers.GetNextItem(i, LVNI_SELECTED)) != -1) {
				const ItemInfo* itemI = ctrlTransfers.getItemData(i);
				bCustomMenu = true;
	
				usercmdsMenu.InsertSeparatorFirst(TSTRING(SETTINGS_USER_COMMANDS));
	
				if(itemI->user != (UserPtr)NULL)
					prepareMenu(usercmdsMenu, UserCommand::CONTEXT_CHAT, ClientManager::getInstance()->getHubs(itemI->user->getCID()));
			}
			WinUtil::ClearPreviewMenu(previewMenu);

			segmentedMenu.CheckMenuItem(IDC_MENU_SLOWDISCONNECT, MF_BYCOMMAND | MF_UNCHECKED);
			transferMenu.CheckMenuItem(IDC_MENU_SLOWDISCONNECT, MF_BYCOMMAND | MF_UNCHECKED);

			if(ii->download) {
				transferMenu.EnableMenuItem(IDC_SEARCH_ALTERNATES, MFS_ENABLED);
				transferMenu.EnableMenuItem(IDC_MENU_SLOWDISCONNECT, MFS_ENABLED);
				if(!ii->target.empty()) {
					string target = Text::fromT(ii->target);
					string ext = Util::getFileExt(target);
					if(ext.size()>1) ext = ext.substr(1);
					PreviewAppsSize = WinUtil::SetupPreviewMenu(previewMenu, ext);

					const QueueItem::StringMap& queue = QueueManager::getInstance()->lockQueue();

					QueueItem::StringIter qi = queue.find(&target);

					bool slowDisconnect = false;
					if(qi != queue.end())
						slowDisconnect = qi->second->isSet(QueueItem::FLAG_AUTODROP);

					QueueManager::getInstance()->unlockQueue();

					if(slowDisconnect) {
						segmentedMenu.CheckMenuItem(IDC_MENU_SLOWDISCONNECT, MF_BYCOMMAND | MF_CHECKED);
						transferMenu.CheckMenuItem(IDC_MENU_SLOWDISCONNECT, MF_BYCOMMAND | MF_CHECKED);
					}
				}
			} else {
				transferMenu.EnableMenuItem(IDC_SEARCH_ALTERNATES, MFS_DISABLED);
				transferMenu.EnableMenuItem(IDC_MENU_SLOWDISCONNECT, MFS_DISABLED);
			}

			if(previewMenu.GetMenuItemCount() > 0) {
				segmentedMenu.EnableMenuItem((UINT)(HMENU)previewMenu, MFS_ENABLED);
				transferMenu.EnableMenuItem((UINT)(HMENU)previewMenu, MFS_ENABLED);
			} else {
				segmentedMenu.EnableMenuItem((UINT)(HMENU)previewMenu, MFS_DISABLED);
				transferMenu.EnableMenuItem((UINT)(HMENU)previewMenu, MFS_DISABLED);
			}

			previewMenu.InsertSeparatorFirst(TSTRING(PREVIEW_MENU));
				
			if(!parent) {
				checkAdcItems(transferMenu);
				transferMenu.InsertSeparatorFirst(TSTRING(MENU_TRANSFERS));
				transferMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);
				transferMenu.RemoveFirstItem();
			} else {
				segmentedMenu.InsertSeparatorFirst(TSTRING(SETTINGS_SEGMENT));
				segmentedMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);
				segmentedMenu.RemoveFirstItem();
			}

			if ( bCustomMenu ) {
				WinUtil::ClearPreviewMenu(usercmdsMenu);
			}
			return TRUE; 
		}
	}
	bHandled = FALSE;
	return FALSE; 
}

void TransferView::runUserCommand(UserCommand& uc) {
	if(!WinUtil::getUCParams(m_hWnd, uc, ucLineParams))
		return;

	StringMap ucParams = ucLineParams;

	int i = -1;
	while((i = ctrlTransfers.GetNextItem(i, LVNI_SELECTED)) != -1) {
		const ItemInfo* itemI = ctrlTransfers.getItemData(i);
		if(!itemI->user->isOnline())
			continue;

		StringMap tmp = ucParams;
		ucParams["fileFN"] = Text::fromT(itemI->target);

		// compatibility with 0.674 and earlier
		ucParams["file"] = ucParams["fileFN"];
		
		ClientManager::getInstance()->userCommand(itemI->user, uc, tmp, true);
	}
}

LRESULT TransferView::onForce(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int i = -1;
	while( (i = ctrlTransfers.GetNextItem(i, LVNI_SELECTED)) != -1) {
		ctrlTransfers.SetItemText(i, COLUMN_STATUS, CTSTRING(CONNECTING_FORCED));
		ConnectionManager::getInstance()->force(ctrlTransfers.getItemData(i)->user);
	}
	return 0;
}

void TransferView::ItemInfo::removeAll() {
	if(childCount <= 1) {
		QueueManager::getInstance()->removeSource(user, QueueItem::Source::FLAG_REMOVED);
	} else {
		if(!BOOLSETTING(CONFIRM_DELETE) || ::MessageBox(0, _T("Do you really want to remove this item?"), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES)
			QueueManager::getInstance()->remove(Text::fromT(target));
	}
}

LRESULT TransferView::onCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled) {
	NMLVCUSTOMDRAW* cd = (NMLVCUSTOMDRAW*)pnmh;

	switch(cd->nmcd.dwDrawStage) {
	case CDDS_PREPAINT:
		return CDRF_NOTIFYITEMDRAW;

	case CDDS_ITEMPREPAINT:
		return CDRF_NOTIFYSUBITEMDRAW;

	case CDDS_SUBITEM | CDDS_ITEMPREPAINT: {
		ItemInfo* ii = reinterpret_cast<ItemInfo*>(cd->nmcd.lItemlParam);

		int colIndex = ctrlTransfers.findColumn(cd->iSubItem);
		cd->clrTextBk = WinUtil::bgColor;

		if((colIndex == COLUMN_STATUS) && (ii->status == ItemInfo::STATUS_RUNNING || ii->status == ItemInfo::TREE_DOWNLOAD)) {
			if(!BOOLSETTING(SHOW_PROGRESS_BARS)) {
				bHandled = FALSE;
				return 0;
			}

			// Get the text to draw
			// Get the color of this bar
			COLORREF clr = SETTING(PROGRESS_OVERRIDE_COLORS) ? 
				(ii->download ? SETTING(DOWNLOAD_BAR_COLOR) : SETTING(UPLOAD_BAR_COLOR)) : 
				GetSysColor(COLOR_HIGHLIGHT);

			//this is just severely broken, msdn says GetSubItemRect requires a one based index
			//but it wont work and index 0 gives the rect of the whole item
			CRect rc;
			if(cd->iSubItem == 0) {
				//use LVIR_LABEL to exclude the icon area since we will be painting over that
				//later
				ctrlTransfers.GetItemRect((int)cd->nmcd.dwItemSpec, &rc, LVIR_LABEL);
			} else {
				ctrlTransfers.GetSubItemRect((int)cd->nmcd.dwItemSpec, cd->iSubItem, LVIR_BOUNDS, &rc);
			}
				
			// Real rc, the original one.
			CRect real_rc = rc;
			// We need to offset the current rc to (0, 0) to paint on the New dc
			rc.MoveToXY(0, 0);

			CDC cdc;
			cdc.CreateCompatibleDC(cd->nmcd.hdc);

			HBITMAP pOldBmp = cdc.SelectBitmap(CreateCompatibleBitmap(cd->nmcd.hdc,  real_rc.Width(),  real_rc.Height()));
			HDC& dc = cdc.m_hDC;

			HFONT oldFont = (HFONT)SelectObject(dc, WinUtil::font);
			SetBkMode(dc, TRANSPARENT);
			
			// Draw the background and border of the bar	
			if(ii->size == 0) ii->size = 1;		
			
			if(BOOLSETTING(PROGRESSBAR_ODC_STYLE)) {
				// New style progressbar tweaks the current colors
				HLSTRIPLE hls_bk = OperaColors::RGB2HLS(cd->clrTextBk);

				// Create pen (ie outline border of the cell)
				HPEN penBorder = ::CreatePen(PS_SOLID, 1, OperaColors::blendColors(cd->clrTextBk, clr, (hls_bk.hlstLightness > 0.75) ? 0.6 : 0.4));
				HGDIOBJ pOldPen = ::SelectObject(dc, penBorder);

				// Draw the outline (but NOT the background) using pen
				HBRUSH hBrOldBg = CreateSolidBrush(cd->clrTextBk);
				hBrOldBg = (HBRUSH)::SelectObject(dc, hBrOldBg);
				::Rectangle(dc, rc.left, rc.top, rc.right, rc.bottom);
				DeleteObject(::SelectObject(dc, hBrOldBg));

				// Set the background color, by slightly changing it
				HBRUSH hBrDefBg = CreateSolidBrush(OperaColors::blendColors(cd->clrTextBk, clr, (hls_bk.hlstLightness > 0.75) ? 0.85 : 0.70));
				HGDIOBJ oldBg = ::SelectObject(dc, hBrDefBg);

				// Draw the outline AND the background using pen+brush
				::Rectangle(dc, rc.left, rc.top, rc.left + (LONG)(rc.Width() * ii->getRatio() + 0.5), rc.bottom);

				// Reset pen
				DeleteObject(::SelectObject(dc, pOldPen));
				// Reset bg (brush)
				DeleteObject(::SelectObject(dc, oldBg));

				COLORREF a, b;
				OperaColors::EnlightenFlood(!ii->parent ? clr : SETTING(PROGRESS_SEGMENT_COLOR), a, b);
				OperaColors::FloodFill(cdc, rc.left+1, rc.top+1,  rc.left + (int) ((int64_t)rc.Width() * ii->actual / ii->size), rc.bottom-1, a, b);
			} else {
				CBarShader statusBar(rc.bottom - rc.top, rc.right - rc.left, SETTING(PROGRESS_BACK_COLOR), ii->size);

				//rc.right = rc.left + (int) (rc.Width() * ii->pos / ii->size); 
				if(!ii->download) {
					statusBar.FillRange(0, ii->actual,  clr);
				} else {
					statusBar.FillRange(0, ii->actual, ii->parent ? SETTING(PROGRESS_SEGMENT_COLOR) : clr);
				}
				if(ii->pos > ii->actual)
					statusBar.FillRange(ii->actual, ii->pos, SETTING(PROGRESS_COMPRESS_COLOR));

				statusBar.Draw(cdc, rc.top, rc.left, SETTING(PROGRESS_3DDEPTH));
			}

			// Get the color of this text bar
			COLORREF oldcol = ::SetTextColor(dc, SETTING(PROGRESS_OVERRIDE_COLORS2) ? 
				(ii->download ? SETTING(PROGRESS_TEXT_COLOR_DOWN) : SETTING(PROGRESS_TEXT_COLOR_UP)) : 
				OperaColors::TextFromBackground(clr));

			rc.left += 6;
			rc.right -= 2;
			LONG top = rc.top + (rc.Height() - WinUtil::getTextHeight(cd->nmcd.hdc) - 1)/2 + 1;
			::ExtTextOut(dc, rc.left, top, ETO_CLIPPED, rc, ii->statusString.c_str(), ii->statusString.length(), NULL);

			SelectObject(dc, oldFont);
			::SetTextColor(dc, oldcol);

			// New way:
			BitBlt(cd->nmcd.hdc, real_rc.left, real_rc.top, real_rc.Width(), real_rc.Height(), dc, 0, 0, SRCCOPY);
			DeleteObject(cdc.SelectBitmap(pOldBmp));

			//bah crap, if we return CDRF_SKIPDEFAULT windows won't paint the icons
			//so we have to do it
			if(cd->iSubItem == 0){
				LVITEM lvItem;
				lvItem.iItem = cd->nmcd.dwItemSpec;
				lvItem.iSubItem = 0;
				lvItem.mask = LVIF_IMAGE | LVIF_STATE;
				lvItem.stateMask = LVIS_SELECTED;
				ctrlTransfers.GetItem(&lvItem);

				HIMAGELIST imageList = (HIMAGELIST)::SendMessage(ctrlTransfers.m_hWnd, LVM_GETIMAGELIST, LVSIL_SMALL, 0);
				if(imageList) {
					//let's find out where to paint it
					//and draw the background to avoid having 
					//the selection color as background
					CRect iconRect;
					ctrlTransfers.GetSubItemRect((int)cd->nmcd.dwItemSpec, 0, LVIR_ICON, iconRect);
					ImageList_Draw(imageList, lvItem.iImage, cd->nmcd.hdc, iconRect.left, iconRect.top, ILD_TRANSPARENT);
				}
			}
			return CDRF_SKIPDEFAULT;
		} else if(BOOLSETTING(GET_USER_COUNTRY) && (colIndex == COLUMN_IP)) {
			ItemInfo* ii = (ItemInfo*)cd->nmcd.lItemlParam;
			CRect rc;
			ctrlTransfers.GetSubItemRect((int)cd->nmcd.dwItemSpec, cd->iSubItem, LVIR_BOUNDS, rc);
			COLORREF color;
			if(ctrlTransfers.GetItemState((int)cd->nmcd.dwItemSpec, LVIS_SELECTED) & LVIS_SELECTED) {
				if(ctrlTransfers.m_hWnd == ::GetFocus()) {
					color = GetSysColor(COLOR_HIGHLIGHT);
					SetBkColor(cd->nmcd.hdc, GetSysColor(COLOR_HIGHLIGHT));
					SetTextColor(cd->nmcd.hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));
				} else {
					color = GetBkColor(cd->nmcd.hdc);
					SetBkColor(cd->nmcd.hdc, color);
				}				
			} else {
				color = WinUtil::bgColor;
				SetBkColor(cd->nmcd.hdc, WinUtil::bgColor);
				SetTextColor(cd->nmcd.hdc, WinUtil::textColor);
			}
			HGDIOBJ oldpen = ::SelectObject(cd->nmcd.hdc, CreatePen(PS_SOLID,0, color));
			HGDIOBJ oldbr = ::SelectObject(cd->nmcd.hdc, CreateSolidBrush(color));
			Rectangle(cd->nmcd.hdc,rc.left, rc.top, rc.right, rc.bottom);

			DeleteObject(::SelectObject(cd->nmcd.hdc, oldpen));
			DeleteObject(::SelectObject(cd->nmcd.hdc, oldbr));

			TCHAR buf[256];
			ctrlTransfers.GetItemText((int)cd->nmcd.dwItemSpec, cd->iSubItem, buf, 255);
			buf[255] = 0;
			if(_tcslen(buf) > 0) {
				rc.left += 2;
				LONG top = rc.top + (rc.Height() - 15)/2;
				if((top - rc.top) < 2)
					top = rc.top + 1;

				POINT p = { rc.left, top };
				WinUtil::flagImages.Draw(cd->nmcd.hdc, ii->flagImage, p, LVSIL_SMALL);
				top = rc.top + (rc.Height() - WinUtil::getTextHeight(cd->nmcd.hdc) - 1)/2;
				::ExtTextOut(cd->nmcd.hdc, rc.left + 30, top + 1, ETO_CLIPPED, rc, buf, _tcslen(buf), NULL);
				return CDRF_SKIPDEFAULT;
			}
		} else if((colIndex != COLUMN_USER) && (colIndex != COLUMN_HUB) && (colIndex != COLUMN_STATUS) && (colIndex != COLUMN_IP) &&
			(ii->status != ItemInfo::STATUS_RUNNING)) {
			cd->clrText = OperaColors::blendColors(WinUtil::bgColor, WinUtil::textColor, 0.4);
			return CDRF_NEWFONT;
		}
		// Fall through
	}
	default:
		return CDRF_DODEFAULT;
	}
}

LRESULT TransferView::onDoubleClickTransfers(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
    NMITEMACTIVATE* item = (NMITEMACTIVATE*)pnmh;
	if (item->iItem != -1 ) {
		CRect rect;
		ctrlTransfers.GetItemRect(item->iItem, rect, LVIR_ICON);

		// if double click on state icon, ignore...
		if (item->ptAction.x < rect.left)
			return 0;

		ItemInfo* i = ctrlTransfers.getItemData(item->iItem);
		const vector<ItemInfo*>& children = ctrlTransfers.findChildren(i->getGroupCond());
		if(children.size() <= 1) {
			switch(SETTING(TRANSFERLIST_DBLCLICK)) {
				case 0:
					i->pm();
					break;
				case 1:
					i->getList();
					break;
				case 2:
					i->matchQueue();
					break;
				case 4:
					i->addFav();
					break;
			}
		}
	}
	return 0;
}

int TransferView::ItemInfo::compareItems(const ItemInfo* a, const ItemInfo* b, uint8_t col) {
	if(a->status == b->status) {
		if(a->download != b->download) {
			return a->download ? -1 : 1;
		}
	} else {
		return (a->status == ItemInfo::STATUS_RUNNING) ? -1 : 1;
	}

	switch(col) {
		case COLUMN_USER: {
			if(a->childCount == b->childCount)
				return lstrcmpi(a->getText(COLUMN_USER).c_str(), b->getText(COLUMN_USER).c_str());
			return compare(a->childCount, b->childCount);						
		}
		case COLUMN_HUB: {
			if(a->running == b->running)
				return lstrcmpi(a->getText(COLUMN_HUB).c_str(), b->getText(COLUMN_HUB).c_str());
			return compare(a->running, b->running);						
		}
		case COLUMN_STATUS: return 0;
		case COLUMN_TIMELEFT: return compare(a->timeLeft, b->timeLeft);
		case COLUMN_SPEED: return compare(a->speed, b->speed);
		case COLUMN_SIZE: return compare(a->size, b->size);
		case COLUMN_RATIO: return compare(a->getRatio(), b->getRatio());
		default: return lstrcmpi(a->getText(col).c_str(), b->getText(col).c_str());
	}
}

TransferView::ItemInfo* TransferView::findItem(const UpdateInfo& ui, int& pos) const {
	if(ui.download) {
		for(ItemInfoList::ParentMap::const_iterator k = ctrlTransfers.parents.begin(); k != ctrlTransfers.parents.end(); ++k) {
			for(vector<ItemInfo*>::const_iterator j = (*k).second.children.begin(); j != (*k).second.children.end(); j++) {
				ItemInfo* ii = *j;
				if(ui == *ii) {
					return ii;
				}
			}
		}
	} else {
		for(int j = 0; j < ctrlTransfers.GetItemCount(); ++j) {
			ItemInfo* ii = ctrlTransfers.getItemData(j);
			if(ui == *ii) {
				pos = j;
				return ii;
			}
		}
	}
	return NULL;
}

LRESULT TransferView::onSpeaker(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	TaskQueue::List t;
	tasks.get(t);
	if(t.size() > 2) {
		ctrlTransfers.SetRedraw(FALSE);
	}

	for(TaskQueue::Iter i = t.begin(); i != t.end(); ++i) {	
		if(i->first == ADD_ITEM) {
			auto_ptr<UpdateInfo> ui(reinterpret_cast<UpdateInfo*>(i->second));
			ItemInfo* ii = new ItemInfo(ui->user, ui->download);
			ii->update(*ui);
			if(ii->download) {
				ctrlTransfers.insertGroupedItem(ii, false, ui->fileList);
			} else {
				ctrlTransfers.insertItem(ii, IMAGE_UPLOAD);
			}
		} else if(i->first == REMOVE_ITEM) {
			auto_ptr<UpdateInfo> ui(reinterpret_cast<UpdateInfo*>(i->second));

			int pos = -1;
			ItemInfo* ii = findItem(*ui, pos);
			if(ii) {
				if(ui->download) {
					ctrlTransfers.removeGroupedItem(ii);
				} else {
					dcassert(pos != -1);
					ctrlTransfers.DeleteItem(pos);
					delete ii;
				}
			}
		} else if(i->first == UPDATE_ITEM) {
			auto_ptr<UpdateInfo> ui(reinterpret_cast<UpdateInfo*>(i->second));

			int pos = -1;
			ItemInfo* ii = findItem(*ui, pos);
			if(ii) {
				ii->update(*ui);
				if(ui->download) {
					if(ii->parent) {
						ItemInfo* parent = ii->parent;
						if(ui->updateMask && UpdateInfo::MASK_FILE) {
							if(parent->target != ii->target) {
								ctrlTransfers.removeGroupedItem(ii, false);
								ctrlTransfers.insertGroupedItem(ii, false);
								parent = ii->parent;
							}
						}
						if(parent->status == ItemInfo::STATUS_WAITING) {
							parent->statusString = ii->statusString;
							updateItem(ctrlTransfers.findItem(parent), COLUMN_STATUS);
						}
						if(!parent->collapsed) {
							updateItem(ctrlTransfers.findItem(ii), ui->updateMask);
						}
						continue;
					}
				}
				dcassert(pos != -1);
				updateItem(pos, ui->updateMask);
			}
		} else if(i->first == UPDATE_PARENT) {
			auto_ptr<UpdateInfo> ui(reinterpret_cast<UpdateInfo*>(i->second));
			ItemInfo* parent = ctrlTransfers.findParent(ui->target);
			if(parent) {
				size_t totalSpeed = 0; double ratio = 0; uint8_t segs = 0; int64_t size = -1;
				tstring ip; uint8_t flagImage = 0; ItemInfo* l = NULL;

				const vector<ItemInfo*>& children = ctrlTransfers.findChildren(parent->getGroupCond());
				for(vector<ItemInfo*>::const_iterator k = children.begin(); k != children.end(); ++k) {
					l = *k;
					ip = l->ip;
					flagImage = l->flagImage;

					if(l->status == ItemInfo::STATUS_RUNNING) {
						segs++;

						totalSpeed += (size_t)l->speed;
						ratio += l->getRatio();

						size = l->size;
						if(!ui->queueItem->isSet(QueueItem::FLAG_MULTI_SOURCE)) break;
					}
				}				
				
				ratio = segs > 0 ? ratio / segs : 1.00;

				if(ui->size == -1) ui->setSize(size);

				if(parent->status == ItemInfo::STATUS_WAITING && (segs > 0)) {
					parent->fileBegin = GET_TICK();

					if ((!SETTING(BEGINFILE).empty()) && (!BOOLSETTING(SOUNDS_DISABLED)))
						PlaySound(Text::toT(SETTING(BEGINFILE)).c_str(), NULL, SND_FILENAME | SND_ASYNC);

					if(BOOLSETTING(POPUP_DOWNLOAD_START)) {
						MainFrame::getMainFrame()->ShowBalloonTip((
							TSTRING(FILE) + _T(": ")+ Util::getFileName(parent->target) + _T("\n")+
							TSTRING(USER) + _T(": ") + Text::toT(parent->user->getFirstNick())).c_str(), CTSTRING(DOWNLOAD_STARTING));
					}
				}

				ui->setSpeed(totalSpeed);
				ui->setTimeLeft((totalSpeed > 0) ? ((ui->size - ui->pos) / totalSpeed) : 0);
				ui->setActual((int64_t)((double)ui->pos * ratio));
				ui->setStatus((segs > 0 && ui->status == ItemInfo::STATUS_RUNNING) ? ItemInfo::STATUS_RUNNING : ItemInfo::STATUS_WAITING);
				ui->setIP(children.size() == 1 ? ip : Util::emptyStringT, flagImage);
				
				if(ui->status == ItemInfo::STATUS_WAITING) {
					segs = 0;
					ui->queueItem->setAverageSpeed(0);
				} else {
					ui->queueItem->setAverageSpeed(totalSpeed);
				}

				int16_t running = children.size() == 1 ? -1 : segs;
				if(running != parent->running) ui->setRunning(running);

				if(ui->status == ItemInfo::STATUS_RUNNING) {
					uint64_t time = GET_TICK() - parent->fileBegin;
					if(time > 800) {
						if(ui->queueItem->isSet(QueueItem::FLAG_MULTI_SOURCE)) {
							AutoArray<TCHAR> buf(TSTRING(DOWNLOADED_BYTES).size() + 64);
							_stprintf(buf, CTSTRING(DOWNLOADED_BYTES), Util::formatBytesW(ui->pos).c_str(), 
								(double)ui->pos*100.0/(double)ui->size, Util::formatSeconds(time/1000).c_str());
						
							tstring statusString;
							if(ratio < 1.00000000) {
								statusString = _T("[Z]");
							}
							statusString += buf;
							ui->setStatusString(statusString);
						} else {
							ui->setStatusString(l->statusString);
						}
					} else {
						ui->setStatusString(TSTRING(DOWNLOAD_STARTING));
					}
				}			

				parent->update(*ui);
				updateItem(ctrlTransfers.findItem(parent), ui->updateMask);
			}
		}
	}

	if(!t.empty()) {
		ctrlTransfers.resort();
		if(t.size() > 2) {
			ctrlTransfers.SetRedraw(TRUE);
		}
	}
	
	return 0;
}

LRESULT TransferView::onSearchAlternates(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int i = -1;
	while((i = ctrlTransfers.GetNextItem(i, LVNI_SELECTED)) != -1) {
		const ItemInfo *ii = ctrlTransfers.getItemData(i);

		TTHValue tth;
		if(QueueManager::getInstance()->getTTH(Text::fromT(ii->target), tth)) {
			WinUtil::searchHash(tth);
		}
	}

	return 0;
}
	
TransferView::ItemInfo::ItemInfo(const UserPtr& u, bool aDownload) : user(u), download(aDownload), transferFailed(false),
	status(STATUS_WAITING), pos(0), size(0), actual(0), speed(0), timeLeft(0), ip(Util::emptyStringT), target(Util::emptyStringT),
	flagImage(0), collapsed(true), parent(NULL), childCount(0), statusString(Util::emptyStringT), running(-1) { }

void TransferView::ItemInfo::update(const UpdateInfo& ui) {
	if(ui.updateMask & UpdateInfo::MASK_STATUS) {
		status = ui.status;
	}
	if(ui.updateMask & UpdateInfo::MASK_STATUS_STRING) {
		// No slots etc from transfermanager better than disconnected from connectionmanager
		if(!transferFailed)
			statusString = ui.statusString;
		transferFailed = ui.transferFailed;
	}
	if(ui.updateMask & UpdateInfo::MASK_SIZE) {
		size = ui.size;
	}
	if(ui.updateMask & UpdateInfo::MASK_POS) {
		pos = ui.pos;
	}
	if(ui.updateMask & UpdateInfo::MASK_ACTUAL) {
		actual = ui.actual;
	}
	if(ui.updateMask & UpdateInfo::MASK_SPEED) {
		speed = ui.speed;
	}
	if(ui.updateMask & UpdateInfo::MASK_FILE) {
		target = ui.target;
	}
	if(ui.updateMask & UpdateInfo::MASK_TIMELEFT) {
		timeLeft = ui.timeLeft;
	}
	if(ui.updateMask & UpdateInfo::MASK_IP) {
		flagImage = ui.flagImage;
		ip = ui.IP;
	}
	if(ui.updateMask & UpdateInfo::MASK_SEGMENT) {
		running = ui.running;
	}
}

void TransferView::updateItem(int ii, uint32_t updateMask) {
	if(	updateMask & UpdateInfo::MASK_STATUS || updateMask & UpdateInfo::MASK_STATUS_STRING ||
		updateMask & UpdateInfo::MASK_POS || updateMask & UpdateInfo::MASK_ACTUAL) {
		ctrlTransfers.updateItem(ii, COLUMN_STATUS);
	}
	if(updateMask & UpdateInfo::MASK_POS || updateMask & UpdateInfo::MASK_ACTUAL) {
		ctrlTransfers.updateItem(ii, COLUMN_RATIO);
	}
	if(updateMask & UpdateInfo::MASK_SIZE) {
		ctrlTransfers.updateItem(ii, COLUMN_SIZE);
	}
	if(updateMask & UpdateInfo::MASK_SPEED) {
		ctrlTransfers.updateItem(ii, COLUMN_SPEED);
	}
	if(updateMask & UpdateInfo::MASK_FILE) {
		ctrlTransfers.updateItem(ii, COLUMN_PATH);
		ctrlTransfers.updateItem(ii, COLUMN_FILE);
	}
	if(updateMask & UpdateInfo::MASK_TIMELEFT) {
		ctrlTransfers.updateItem(ii, COLUMN_TIMELEFT);
	}
	if(updateMask & UpdateInfo::MASK_IP) {
		ctrlTransfers.updateItem(ii, COLUMN_IP);
	}
	if(updateMask & UpdateInfo::MASK_SEGMENT) {
		ctrlTransfers.updateItem(ii, COLUMN_HUB);
	}
}

void TransferView::on(ConnectionManagerListener::Added, const ConnectionQueueItem* aCqi) {
	UpdateInfo* ui = new UpdateInfo(aCqi->getUser(), aCqi->getDownload());

	if(ui->download) {
		string aTarget; int64_t aSize; int aFlags;
		if(QueueManager::getInstance()->getQueueInfo(aCqi->getUser(), aTarget, aSize, aFlags, ui->fileList)) {
			ui->setTarget(Text::toT(aTarget));
			ui->setSize(aSize);
		}
	}

	ui->setStatus(ItemInfo::STATUS_WAITING);
	ui->setStatusString(TSTRING(CONNECTING));

	speak(ADD_ITEM, ui);
}

void TransferView::on(ConnectionManagerListener::StatusChanged, const ConnectionQueueItem* aCqi) {
	UpdateInfo* ui = new UpdateInfo(aCqi->getUser(), aCqi->getDownload());
	string aTarget;	int64_t aSize; int aFlags = 0;

	if(QueueManager::getInstance()->getQueueInfo(aCqi->getUser(), aTarget, aSize, aFlags, ui->fileList)) {
		ui->setTarget(Text::toT(aTarget));
		ui->setSize(aSize);
	}

	ui->setStatusString((aFlags & QueueItem::FLAG_TESTSUR) ? TSTRING(CHECKING_CLIENT) : TSTRING(CONNECTING));
	ui->setStatus(ItemInfo::STATUS_WAITING);

	speak(UPDATE_ITEM, ui);
}

void TransferView::on(ConnectionManagerListener::Removed, const ConnectionQueueItem* aCqi) {
	speak(REMOVE_ITEM, new UpdateInfo(aCqi->getUser(), aCqi->getDownload()));
}

void TransferView::on(ConnectionManagerListener::Failed, const ConnectionQueueItem* aCqi, const string& aReason) {
	UpdateInfo* ui = new UpdateInfo(aCqi->getUser(), aCqi->getDownload());
	if(aCqi->getUser()->isSet(User::OLD_CLIENT)) {
		ui->setStatusString(TSTRING(SOURCE_TOO_OLD));
	} else {
		ui->setStatusString(Text::toT(aReason));
	}
	ui->setStatus(ItemInfo::STATUS_WAITING);
	speak(UPDATE_ITEM, ui);
}

void TransferView::on(DownloadManagerListener::Starting, const Download* aDownload) {
	UpdateInfo* ui = new UpdateInfo(aDownload->getUser(), true);
	bool chunkInfo = aDownload->isSet(Download::FLAG_MULTI_CHUNK) && !aDownload->isSet(Download::FLAG_TREE_DOWNLOAD);

	ui->setStatus(ItemInfo::STATUS_RUNNING);
	ui->setPos(chunkInfo ? 0 : aDownload->getPos());
	ui->setActual(chunkInfo ? 0 : aDownload->getStartPos() + aDownload->getActual());
	ui->setSize(chunkInfo ? aDownload->getChunkSize() : aDownload->getSize());
	ui->setTarget(Text::toT(aDownload->getTarget()));
	ui->setStatusString(TSTRING(DOWNLOAD_STARTING));
	
	string ip = aDownload->getUserConnection().getRemoteIp();
	string country = Util::getIpCountry(ip);
	if(country.empty()) {
		ui->setIP(Text::toT(ip), 0);
	} else {
		ui->setIP(Text::toT(country + " (" + ip + ")"), WinUtil::getFlagImage(country.c_str()));
	}
	if(aDownload->isSet(Download::FLAG_TREE_DOWNLOAD)) {
		ui->setStatus(ItemInfo::TREE_DOWNLOAD);
	}

	speak(UPDATE_ITEM, ui);
}

void TransferView::on(DownloadManagerListener::Tick, const DownloadList& dl) {
	AutoArray<TCHAR> buf(TSTRING(DOWNLOADED_BYTES).size() + 64);

	for(DownloadList::const_iterator j = dl.begin(); j != dl.end(); ++j) {
		Download* d = *j;
		
		bool chunkInfo = d->isSet(Download::FLAG_MULTI_CHUNK) && !d->isSet(Download::FLAG_TREE_DOWNLOAD);
		
		UpdateInfo* ui = new UpdateInfo(d->getUser(), true);
		ui->setActual(chunkInfo ? d->getActual() : d->getStartPos() + d->getActual());
		ui->setPos(chunkInfo ? d->getTotal() : d->getPos());
		ui->setTimeLeft(d->getSecondsLeft());
		ui->setSpeed(d->getRunningAverage());

		if(chunkInfo) {
			ui->setSize(d->isSet(Download::FLAG_MULTI_CHUNK) ? d->getChunkSize() : d->getSize());
			ui->timeLeft = (ui->speed > 0) ? ((ui->size - d->getTotal()) / ui->speed) : 0;

			double progress = (double)(d->getTotal())*100.0/(double)ui->size;
			_stprintf(buf, CTSTRING(DOWNLOADED_BYTES), Util::formatBytesW(d->getTotal()).c_str(), 
				progress, Util::formatSeconds((GET_TICK() - d->getStart())/1000).c_str());
			if(progress > 100) {
				// workaround to fix > 100% percentage
				d->getUserConnection().disconnect();
				continue;
			}
		} else {
			_stprintf(buf, CTSTRING(DOWNLOADED_BYTES), Util::formatBytesW(d->getPos()).c_str(), 
				(double)d->getPos()*100.0/(double)d->getSize(), Util::formatSeconds((GET_TICK() - d->getStart())/1000).c_str());
		}

		tstring statusString;

		if(d->isSet(Download::FLAG_PARTIAL)) {
			statusString += _T("[P]");
		}
		if(d->getUserConnection().isSecure()) {
			if(d->getUserConnection().isTrusted()) {
				statusString += _T("[S]");
			} else {
				statusString += _T("[U]");
			}
		}
		if(d->isSet(Download::FLAG_TTH_CHECK)) {
			statusString += _T("[T]");
		}
		if(d->isSet(Download::FLAG_ZDOWNLOAD)) {
			statusString += _T("[Z]");
		}
		if(d->isSet(Download::FLAG_CHUNKED)) {
			statusString += _T("[C]");
		}
		if(!statusString.empty()) {
			statusString += _T(" ");
		}
		statusString += buf;
		ui->setStatusString(statusString);
		if((d->getRunningAverage() == 0) && ((GET_TICK() - d->getStart()) > 1000)) {
			d->getUserConnection().disconnect();
		}
			
		tasks.add(UPDATE_ITEM, ui);
	}

	PostMessage(WM_SPEAKER);
}

void TransferView::on(DownloadManagerListener::Failed, const Download* aDownload, const string& aReason) {
	UpdateInfo* ui = new UpdateInfo(aDownload->getUser(), true, true);
	ui->setStatus(ItemInfo::STATUS_WAITING);
	ui->setPos(0);
	ui->setStatusString(Text::toT(aReason));
	ui->setSize(aDownload->getSize());
	ui->setTarget(Text::toT(aDownload->getTarget()));

	string ip = aDownload->getUserConnection().getRemoteIp();
	string country = Util::getIpCountry(ip);
	if(country.empty()) {
		ui->setIP(Text::toT(ip), 0);
	} else {
		ui->setIP(Text::toT(country + " (" + ip + ")"), WinUtil::getFlagImage(country.c_str()));
	}
	if(BOOLSETTING(POPUP_DOWNLOAD_FAILED)) {
		MainFrame::getMainFrame()->ShowBalloonTip((
			TSTRING(FILE)+_T(": ") + Util::getFileName(ui->target) + _T("\n")+
			TSTRING(USER)+_T(": ") + Text::toT(ui->user->getFirstNick()) + _T("\n")+
			TSTRING(REASON)+_T(": ") + Text::toT(aReason)).c_str(), CTSTRING(DOWNLOAD_FAILED), NIIF_WARNING);
	}

	speak(UPDATE_ITEM, ui);
}

void TransferView::on(DownloadManagerListener::Status, const UserConnection* uc, const string& aReason) {
	UpdateInfo* ui = new UpdateInfo(uc->getUser(), true);
	ui->setStatus(ItemInfo::STATUS_WAITING);
	ui->setPos(0);
	ui->setStatusString(Text::toT(aReason));

	speak(UPDATE_ITEM, ui);
}

void TransferView::on(UploadManagerListener::Starting, const Upload* aUpload) {
	UpdateInfo* ui = new UpdateInfo(aUpload->getUser(), false);

	ui->setStatus(ItemInfo::STATUS_RUNNING);
	ui->setPos(aUpload->getPos());
	ui->setActual(aUpload->getStartPos() + aUpload->getActual());
	ui->setSize(aUpload->isSet(Upload::FLAG_TTH_LEAVES) ? aUpload->getSize() : aUpload->getFileSize());
	ui->setTarget(Text::toT(aUpload->getSourceFile()));

	if(!aUpload->isSet(Upload::FLAG_RESUMED)) {
		ui->setStatusString(TSTRING(UPLOAD_STARTING));
	}

	string ip = aUpload->getUserConnection().getRemoteIp();
	string country = Util::getIpCountry(ip);
	if(country.empty()) {
		ui->setIP(Text::toT(ip), 0);
	} else {
		ui->setIP(Text::toT(country + " (" + ip + ")"), WinUtil::getFlagImage(country.c_str()));
	}

	speak(UPDATE_ITEM, ui);
}

void TransferView::on(UploadManagerListener::Tick, const UploadList& ul) {
	AutoArray<TCHAR> buf(TSTRING(UPLOADED_BYTES).size() + 64);

	for(UploadList::const_iterator j = ul.begin(); j != ul.end(); ++j) {
		Upload* u = *j;

		if (u->getTotal() == 0) continue;

		UpdateInfo* ui = new UpdateInfo(u->getUser(), false);
		ui->setActual(u->getStartPos() + u->getActual());
		ui->setPos(u->getPos());
		ui->setTimeLeft(u->getSecondsLeft(true)); // we are interested when whole file is finished and not only one chunk
		ui->setSpeed(u->getRunningAverage());

		_stprintf(buf, CTSTRING(UPLOADED_BYTES), Util::formatBytesW(u->getPos()).c_str(), 
			(double)u->getPos()*100.0/(double)(u->isSet(Upload::FLAG_TTH_LEAVES) ? u->getSize() : u->getFileSize()), Util::formatSeconds((GET_TICK() - u->getStart())/1000).c_str());

		tstring statusString;

		if(u->isSet(Upload::FLAG_PARTIAL_SHARE)) {
			statusString += _T("[P]");
		}
		if(u->isSet(Upload::FLAG_CHUNKED)) {
			statusString += _T("[C]");
		}
		if(u->getUserConnection().isSecure()) {
			if(u->getUserConnection().isTrusted()) {
				statusString += _T("[S]");
			} else {
				statusString += _T("[U]");
			}
		}
		if(u->isSet(Upload::FLAG_ZUPLOAD)) {
			statusString += _T("[Z]");
		}
		if(!statusString.empty()) {
			statusString += _T(" ");
		}			
		statusString += buf;

		ui->setStatusString(statusString);
					
		tasks.add(UPDATE_ITEM, ui);
		if((u->getRunningAverage() == 0) && ((GET_TICK() - u->getStart()) > 1000)) {
			u->getUserConnection().disconnect(true);
		}
	}

	PostMessage(WM_SPEAKER);
}

void TransferView::onTransferComplete(const Transfer* aTransfer, bool isUpload, const string& aFileName, bool isTree) {
	UpdateInfo* ui = new UpdateInfo(aTransfer->getUser(), !isUpload);

	ui->setStatus(ItemInfo::STATUS_WAITING);	
	ui->setPos(0);
	ui->setStatusString(isUpload ? TSTRING(UPLOAD_FINISHED_IDLE) : TSTRING(DOWNLOAD_FINISHED_IDLE));

	if(!isUpload) {
		if(BOOLSETTING(POPUP_DOWNLOAD_FINISHED) && !isTree) {
			MainFrame::getMainFrame()->ShowBalloonTip((
				TSTRING(FILE) + _T(": ") + Text::toT(aFileName) + _T("\n")+
				TSTRING(USER) + _T(": ") + Text::toT(aTransfer->getUser()->getFirstNick())).c_str(), CTSTRING(DOWNLOAD_FINISHED_IDLE));
		}
	} else {
		if(BOOLSETTING(POPUP_UPLOAD_FINISHED) && !isTree) {
			MainFrame::getMainFrame()->ShowBalloonTip((
				TSTRING(FILE) + _T(": ") + Text::toT(aFileName) + _T("\n")+
				TSTRING(USER) + _T(": ") + Text::toT(aTransfer->getUser()->getFirstNick())).c_str(), CTSTRING(UPLOAD_FINISHED_IDLE));
		}
	}
	
	speak(UPDATE_ITEM, ui);
}

void TransferView::ItemInfo::disconnect() {
	ConnectionManager::getInstance()->disconnect(user, download);
}
		
LRESULT TransferView::onPreviewCommand(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/){
	int i = -1;
	while((i = ctrlTransfers.GetNextItem(i, LVNI_SELECTED)) != -1) {
		const ItemInfo *ii = ctrlTransfers.getItemData(i);

		const QueueItem::StringMap& queue = QueueManager::getInstance()->lockQueue();

		string tmp = Text::fromT(ii->target);
		QueueItem::StringIter qi = queue.find(&tmp);

		string aTempTarget;
		if(qi != queue.end())
			aTempTarget = qi->second->getTempTarget();

		QueueManager::getInstance()->unlockQueue();

		WinUtil::RunPreviewCommand(wID - IDC_PREVIEW_APP, aTempTarget);
	}

	return 0;
}

void TransferView::CollapseAll() {
	for(int q = ctrlTransfers.GetItemCount()-1; q != -1; --q) {
		ItemInfo* m = (ItemInfo*)ctrlTransfers.getItemData(q);
		if(m->download && m->parent) {
			ctrlTransfers.deleteItem(m); 
		}
		if(m->download && !m->parent) {
			m->collapsed = true;
			ctrlTransfers.SetItemState(ctrlTransfers.findItem(m), INDEXTOSTATEIMAGEMASK(1), LVIS_STATEIMAGEMASK);
		 }
	}
}

void TransferView::ExpandAll() {
	for(ItemInfoList::ParentMap::const_iterator i = ctrlTransfers.parents.begin(); i != ctrlTransfers.parents.end(); ++i) {
		ItemInfo* l = (*i).second.parent;
		if(l->collapsed) {
			ctrlTransfers.Expand(l, ctrlTransfers.findItem(l));
		}
	}
}

LRESULT TransferView::onConnectAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int i = -1;
	while((i = ctrlTransfers.GetNextItem(i, LVNI_SELECTED)) != -1) {
		const ItemInfo* ii = ctrlTransfers.getItemData(i);
		ctrlTransfers.SetItemText(i, COLUMN_STATUS, CTSTRING(CONNECTING_FORCED));

		const vector<ItemInfo*>& children = ctrlTransfers.findChildren(ii->getGroupCond());
		for(vector<ItemInfo*>::const_iterator j = children.begin(); j != children.end(); ++j) {
			int h = ctrlTransfers.findItem(*j);
			if(h != -1)
				ctrlTransfers.SetItemText(h, COLUMN_STATUS, CTSTRING(CONNECTING_FORCED));
			ConnectionManager::getInstance()->force((*j)->user);
		}
	}
	return 0;
}

LRESULT TransferView::onDisconnectAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int i = -1;
	while((i = ctrlTransfers.GetNextItem(i, LVNI_SELECTED)) != -1) {
		const ItemInfo* ii = ctrlTransfers.getItemData(i);
		ctrlTransfers.SetItemText(i, COLUMN_STATUS, CTSTRING(DISCONNECTED));
		
		const vector<ItemInfo*>& children = ctrlTransfers.findChildren(ii->getGroupCond());
		for(vector<ItemInfo*>::const_iterator j = children.begin(); j != children.end(); ++j) {
			int h = ctrlTransfers.findItem(*j);
			if(h != -1)
				ctrlTransfers.SetItemText(h, COLUMN_STATUS, CTSTRING(DISCONNECTED));
			(*j)->disconnect();
		}
	}
	return 0;
}

LRESULT TransferView::onSlowDisconnect(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int i = -1;
	while((i = ctrlTransfers.GetNextItem(i, LVNI_SELECTED)) != -1) {
		const ItemInfo *ii = ctrlTransfers.getItemData(i);

		const QueueItem::StringMap& queue = QueueManager::getInstance()->lockQueue();

		string tmp = Text::fromT(ii->target);
		QueueItem::StringIter qi = queue.find(&tmp);

		if(qi != queue.end()) {
			if(qi->second->isSet(QueueItem::FLAG_AUTODROP)) {
				qi->second->unsetFlag(QueueItem::FLAG_AUTODROP);
			} else {
				qi->second->setFlag(QueueItem::FLAG_AUTODROP);
			}
		}

		QueueManager::getInstance()->unlockQueue();
	}

	return 0;
}

void TransferView::on(SettingsManagerListener::Save, SimpleXML& /*xml*/) throw() {
	bool refresh = false;
	if(ctrlTransfers.GetBkColor() != WinUtil::bgColor) {
		ctrlTransfers.SetBkColor(WinUtil::bgColor);
		ctrlTransfers.SetTextBkColor(WinUtil::bgColor);
		ctrlTransfers.setFlickerFree(WinUtil::bgBrush);
		refresh = true;
	}
	if(ctrlTransfers.GetTextColor() != WinUtil::textColor) {
		ctrlTransfers.SetTextColor(WinUtil::textColor);
		refresh = true;
	}
	if(refresh == true) {
		RedrawWindow(NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
	}
}

void TransferView::on(QueueManagerListener::StatusUpdated, const QueueItem* qi) throw() {
	UpdateInfo* ui = new UpdateInfo(const_cast<QueueItem*>(qi), true);
	ui->setTarget(Text::toT(qi->getTarget()));
	ui->setPos(qi->getDownloadedBytes());
	ui->setActual(ui->pos);
	ui->setSize(qi->getSize());
	ui->setStatus(qi->getStatus() == QueueItem::STATUS_WAITING ? ItemInfo::STATUS_WAITING : ItemInfo::STATUS_RUNNING);

	speak(UPDATE_PARENT, ui);
}

void TransferView::on(QueueManagerListener::Finished, const QueueItem* qi, const string&, int64_t) throw() {
	UpdateInfo* ui = new UpdateInfo(const_cast<QueueItem*>(qi), true);
	ui->setTarget(Text::toT(qi->getTarget()));
	ui->setPos(0);
	ui->setActual(0);
	ui->setStatusString(TSTRING(DOWNLOAD_FINISHED_IDLE));
	ui->setStatus(ItemInfo::STATUS_WAITING);

	speak(UPDATE_PARENT, ui);
}

void TransferView::on(QueueManagerListener::Removed, const QueueItem* qi) throw() {
	UpdateInfo* ui = new UpdateInfo(const_cast<QueueItem*>(qi), true);
	ui->setTarget(Text::toT(qi->getTarget()));
	ui->setPos(0);
	ui->setActual(0);
	ui->setStatus(ItemInfo::STATUS_WAITING);

	speak(UPDATE_PARENT, ui);
}

/**
 * @file
 * $Id$
 */
