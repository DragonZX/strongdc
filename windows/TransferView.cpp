/*
 * Copyright (C) 2001-2004 Jacek Sieka, j_s at telia com
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

#include "../client/QueueManager.h"
#include "../client/QueueItem.h"

#include "WinUtil.h"
#include "TransferView.h"
#include "CZDCLib.h"
#include "MainFrm.h"

#include "BarShader.h"

int TransferView::columnIndexes[] = { COLUMN_USER, COLUMN_HUB, COLUMN_STATUS, COLUMN_TIMELEFT, COLUMN_SPEED, COLUMN_FILE, COLUMN_SIZE, COLUMN_PATH, COLUMN_IP, COLUMN_RATIO };
int TransferView::columnSizes[] = { 150, 100, 250, 75, 75, 175, 100, 200, 50, 75 };

static ResourceManager::Strings columnNames[] = { ResourceManager::USER, ResourceManager::HUB_SEGMENTS, ResourceManager::STATUS,
ResourceManager::TIME_LEFT, ResourceManager::SPEED, ResourceManager::FILENAME, ResourceManager::SIZE, ResourceManager::PATH,
ResourceManager::IP_BARE, ResourceManager::RATIO};

TransferView::~TransferView() {
	delete[] headerBuf;
	arrows.Destroy();
	DestroyIcon(hIconCompressed);
}

LRESULT TransferView::onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {

	arrows.CreateFromImage(IDB_ARROWS, 16, 3, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION | LR_SHARED);
	ctrlTransfers.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_HSCROLL | WS_VSCROLL | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS, WS_EX_CLIENTEDGE, IDC_TRANSFERS);
	ctrlTransfers.SetExtendedListViewStyle(LVS_EX_LABELTIP | LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT | 0x00010000 | (BOOLSETTING(SHOW_INFOTIPS) ? LVS_EX_INFOTIP : 0));

	WinUtil::splitTokens(columnIndexes, SETTING(MAINFRAME_ORDER), COLUMN_LAST);
	WinUtil::splitTokens(columnSizes, SETTING(MAINFRAME_WIDTHS), COLUMN_LAST);

	for(int j=0; j<COLUMN_LAST; ++j) {
		int fmt = (j == COLUMN_SIZE || j == COLUMN_TIMELEFT || j == COLUMN_SPEED) ? LVCFMT_RIGHT : LVCFMT_LEFT;
		ctrlTransfers.insertColumn(j, CTSTRING_I(columnNames[j]), fmt, columnSizes[j], j);
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

	hIconCompressed = (HICON)LoadImage((HINSTANCE)::GetWindowLong(::GetParent(m_hWnd), GWL_HINSTANCE), MAKEINTRESOURCE(IDR_TRANSFER_COMPRESSED), IMAGE_ICON, 16, 16, LR_DEFAULTSIZE);

	ConnectionManager::getInstance()->addListener(this);
	DownloadManager::getInstance()->addListener(this);
	UploadManager::getInstance()->addListener(this);
	SettingsManager::getInstance()->addListener(this);

	return 0;
}

void TransferView::prepareClose() {
	ctrlTransfers.saveHeaderOrder(SettingsManager::MAINFRAME_ORDER, SettingsManager::MAINFRAME_WIDTHS,
		SettingsManager::MAINFRAME_VISIBLE);

	ConnectionManager::getInstance()->removeListener(this);
	DownloadManager::getInstance()->removeListener(this);
	UploadManager::getInstance()->removeListener(this);
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
			ItemInfo* itemI;
			bool bCustomMenu = false;
			if( (i = ctrlTransfers.GetNextItem(i, LVNI_SELECTED)) != -1) {
				itemI = ctrlTransfers.getItemData(i);
				bCustomMenu = true;
	
				usercmdsMenu.InsertSeparatorFirst(STRING(SETTINGS_USER_COMMANDS));
	
				if(itemI->user != (User::Ptr)NULL)
					prepareMenu(usercmdsMenu, UserCommand::CONTEXT_CHAT, Text::toT(itemI->user->getClientAddressPort()), itemI->user->isClientOp());
			}
			ItemInfo* ii = ctrlTransfers.getItemData(ctrlTransfers.GetNextItem(-1, LVNI_SELECTED));
			WinUtil::ClearPreviewMenu(previewMenu);

			segmentedMenu.CheckMenuItem(IDC_MENU_SLOWDISCONNECT, MF_BYCOMMAND | MF_UNCHECKED);
			transferMenu.CheckMenuItem(IDC_MENU_SLOWDISCONNECT, MF_BYCOMMAND | MF_UNCHECKED);

			if((ii->type == ItemInfo::TYPE_DOWNLOAD) && (ii->Target != Util::emptyString)) {
				string ext = Util::getFileExt(ii->Target);
				if(ext.size()>1) ext = ext.substr(1);
				PreviewAppsSize = WinUtil::SetupPreviewMenu(previewMenu, ext);

				QueueItem::StringMap queue = QueueManager::getInstance()->lockQueue();

				string tmp = ii->Target;
				QueueItem::StringIter qi = queue.find(&tmp);

				bool slowDisconnect = false;
				if(qi != queue.end())
					slowDisconnect = qi->second->isSet(QueueItem::FLAG_AUTODROP);

				QueueManager::getInstance()->unlockQueue();

				if(slowDisconnect) {
					segmentedMenu.CheckMenuItem(IDC_MENU_SLOWDISCONNECT, MF_BYCOMMAND | MF_CHECKED);
					transferMenu.CheckMenuItem(IDC_MENU_SLOWDISCONNECT, MF_BYCOMMAND | MF_CHECKED);
				}
			}

			if(previewMenu.GetMenuItemCount() > 0) {
				segmentedMenu.EnableMenuItem((UINT)(HMENU)previewMenu, MFS_ENABLED);
				transferMenu.EnableMenuItem((UINT)(HMENU)previewMenu, MFS_ENABLED);
			} else {
				segmentedMenu.EnableMenuItem((UINT)(HMENU)previewMenu, MFS_DISABLED);
				transferMenu.EnableMenuItem((UINT)(HMENU)previewMenu, MFS_DISABLED);
			}

			previewMenu.InsertSeparatorFirst(STRING(PREVIEW_MENU));
				
			if(ii->subItems.size() <= 1) {
				checkAdcItems(transferMenu);
				transferMenu.InsertSeparatorFirst(STRING(MENU_TRANSFERS));
				transferMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);
				transferMenu.RemoveFirstItem();
			} else {
				segmentedMenu.InsertSeparatorFirst(STRING(SETTINGS_SEGMENT));
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
	if(!WinUtil::getUCParams(m_hWnd, uc, ucParams))
		return;

	int i = -1;
	while((i = ctrlTransfers.GetNextItem(i, LVNI_SELECTED)) != -1) {
		ItemInfo* itemI = ctrlTransfers.getItemData(i);
		if(!itemI->user->isOnline())
			return;

		ucParams["mynick"] = itemI->user->getClientNick();
		ucParams["mycid"] = itemI->user->getClientCID().toBase32();
		ucParams["file"] = itemI->Target;

		StringMap tmp = ucParams;
		itemI->user->getParams(tmp);
		itemI->user->clientEscapeParams(tmp);
		itemI->user->sendUserCmd(Util::formatParams(uc.getCommand(), tmp));
	}
	return;
};

LRESULT TransferView::onForce(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int i = -1;
	while( (i = ctrlTransfers.GetNextItem(i, LVNI_SELECTED)) != -1) {
		ItemInfo *ii = ctrlTransfers.getItemData(i);
		ii->statusString = CTSTRING(CONNECTING_FORCED);
		ii->updateMask |= ItemInfo::MASK_STATUS;
		ii->update();
		ctrlTransfers.updateItem( ii );
		ii->user->connect();
	}
	return 0;
}

void TransferView::ItemInfo::removeAll() {
	if(subItems.size() <= 1) {
		QueueManager::getInstance()->removeSource(user, QueueItem::Source::FLAG_REMOVED);
	} else {
		if(!BOOLSETTING(CONFIRM_DELETE) || ::MessageBox(0, _T("Do you really want to remove this item?"), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES)
			QueueManager::getInstance()->remove(Target);
	}
}

LRESULT TransferView::onCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled) {
	CRect rc;
	NMLVCUSTOMDRAW* cd = (NMLVCUSTOMDRAW*)pnmh;

	switch(cd->nmcd.dwDrawStage) {
	case CDDS_PREPAINT:
		return CDRF_NOTIFYITEMDRAW;

	case CDDS_ITEMPREPAINT:
		return CDRF_NOTIFYSUBITEMDRAW;

	case CDDS_SUBITEM | CDDS_ITEMPREPAINT: {
		ItemInfo* ii = reinterpret_cast<ItemInfo*>(cd->nmcd.lItemlParam);

		// Let's draw a box if needed...
		LVCOLUMN lvc;
		lvc.mask = LVCF_TEXT;
		lvc.pszText = headerBuf;
		lvc.cchTextMax = 128;
		ctrlTransfers.GetColumn(cd->iSubItem, &lvc);
		cd->clrTextBk = WinUtil::bgColor;

		if(Util::stricmp(headerBuf, CTSTRING_I(columnNames[COLUMN_STATUS])) == 0 && ii->status == ItemInfo::STATUS_RUNNING) {
			if(BOOLSETTING(SHOW_PROGRESS_BARS) == false) {
				bHandled = FALSE;
				return 0;
			}

			// Get the text to draw
			// Get the color of this bar
			COLORREF clr = SETTING(PROGRESS_OVERRIDE_COLORS) ? 
				((ii->type == ItemInfo::TYPE_DOWNLOAD) ? 
					SETTING(DOWNLOAD_BAR_COLOR) : 
					SETTING(UPLOAD_BAR_COLOR)) : 
				GetSysColor(COLOR_HIGHLIGHT);

			//this is just severely broken, msdn says GetSubItemRect requires a one based index
			//but it wont work and index 0 gives the rect of the whole item
			if(cd->iSubItem == 0) {
				//use LVIR_LABEL to exclude the icon area since we will be painting over that
				//later
				ctrlTransfers.GetItemRect((int)cd->nmcd.dwItemSpec, &rc, LVIR_LABEL);
			} else {
				ctrlTransfers.GetSubItemRect((int)cd->nmcd.dwItemSpec, cd->iSubItem, LVIR_BOUNDS, &rc);
			}
				

			// Actually we steal one upper pixel to not have 2 borders together

			// Real rc, the original one.
			CRect real_rc = rc;
			// We need to offset the current rc to (0, 0) to paint on the New dc
			rc.MoveToXY(0, 0);

			// Text rect
			CRect rc2 = rc;
            rc2.left += 6; // indented with 6 pixels
			rc2.right -= 2; // and without messing with the border of the cell				

			if (ii->isSet(ItemInfo::FLAG_COMPRESSED))
				rc2.left += 9;
			
			CDC cdc;
			cdc.CreateCompatibleDC(cd->nmcd.hdc);
			HBITMAP hBmp = CreateCompatibleBitmap(cd->nmcd.hdc,  real_rc.Width(),  real_rc.Height());

			HBITMAP pOldBmp = cdc.SelectBitmap(hBmp);
			HDC& dc = cdc.m_hDC;

			HFONT oldFont = (HFONT)SelectObject(dc, WinUtil::font);
			SetBkMode(dc, TRANSPARENT);
		
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
			}

			// Draw the background and border of the bar
			if(ii->size == 0)
				ii->size = 1;

			bool mainItem = ii->mainItem || (ii->type == ItemInfo::TYPE_UPLOAD);
			if(!BOOLSETTING(PROGRESSBAR_ODC_STYLE)) {
				CBarShader statusBar(rc.bottom - rc.top, rc.right - rc.left, SETTING(PROGRESS_BACK_COLOR), ii->size);

				rc.right = rc.left + (int) (rc.Width() * ii->pos / ii->size); 
				if(ii->type == ItemInfo::TYPE_UPLOAD) {
					statusBar.FillRange(0, ii->start, HLS_TRANSFORM(clr, -20, 30));
					statusBar.FillRange(ii->start, ii->actual,  clr);
				} else {
					statusBar.FillRange(0, ii->actual, clr);
					if(!mainItem)
						statusBar.FillRange(ii->start, ii->actual, SETTING(PROGRESS_SEGMENT_COLOR));
				}
				if(ii->pos > ii->actual)
					statusBar.FillRange(ii->actual, ii->pos, SETTING(PROGRESS_COMPRESS_COLOR));

				statusBar.Draw(cdc, rc.top, rc.left, SETTING(PROGRESS_3DDEPTH));
			} else {
				int w = (LONG)(rc.Width() * ii->getRatio() + 0.5);
				rc.right = rc.left + (int) (((int64_t)w) * ii->actual / ii->size);
                
				COLORREF a, b;
				OperaColors::EnlightenFlood(mainItem ? clr : SETTING(PROGRESS_SEGMENT_COLOR), a, b);
				OperaColors::FloodFill(cdc, rc.left+1, rc.top+1, rc.right, rc.bottom-1, a, b);
			}

			// Draw the text only over the bar and with correct color

			if (ii->isSet(ItemInfo::FLAG_COMPRESSED))
				DrawIconEx(dc, rc2.left - 12, rc2.top + 2, hIconCompressed, 16, 16, NULL, NULL, DI_NORMAL | DI_COMPAT);

			// Get the color of this text bar
			COLORREF oldcol = ::SetTextColor(dc, SETTING(PROGRESS_OVERRIDE_COLORS2) ? 
				((ii->type == ItemInfo::TYPE_DOWNLOAD) ? SETTING(PROGRESS_TEXT_COLOR_DOWN) : SETTING(PROGRESS_TEXT_COLOR_UP)) : 
				OperaColors::TextFromBackground(clr));

			//rc2.right = right;
			LONG top = rc2.top + (rc2.Height() - WinUtil::getTextHeight(cd->nmcd.hdc) - 1)/2 + 1;
			::ExtTextOut(dc, rc2.left, top, ETO_CLIPPED, rc2, ii->getText(COLUMN_STATUS).c_str(), ii->getText(COLUMN_STATUS).length(), NULL);

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
		} else if(Util::stricmp(headerBuf, CTSTRING_I(columnNames[COLUMN_IP])) == 0 && BOOLSETTING(GET_USER_COUNTRY)) {
			ItemInfo* ii = (ItemInfo*)cd->nmcd.lItemlParam;
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
			CRect rc2 = rc;
			rc2.left += 2;
			HGDIOBJ oldpen = ::SelectObject(cd->nmcd.hdc, CreatePen(PS_SOLID,0, color));
			HGDIOBJ oldbr = ::SelectObject(cd->nmcd.hdc, CreateSolidBrush(color));
			Rectangle(cd->nmcd.hdc,rc.left, rc.top, rc.right, rc.bottom);

			DeleteObject(::SelectObject(cd->nmcd.hdc, oldpen));
			DeleteObject(::SelectObject(cd->nmcd.hdc, oldbr));

			TCHAR buf[256];
			ctrlTransfers.GetItemText((int)cd->nmcd.dwItemSpec, cd->iSubItem, buf, 255);
			buf[255] = 0;
			if(_tcslen(buf) > 0) {
				LONG top = rc2.top + (rc2.Height() - 15)/2;
				if((top - rc2.top) < 2)
					top = rc2.top + 1;

				POINT p = { rc2.left, top };
				WinUtil::flagImages.Draw(cd->nmcd.hdc, ii->flagImage, p, LVSIL_SMALL);
				top = rc2.top + (rc2.Height() - WinUtil::getTextHeight(cd->nmcd.hdc) - 1)/2;
				::ExtTextOut(cd->nmcd.hdc, rc2.left + 30, top + 1, ETO_CLIPPED, rc2, buf, _tcslen(buf), NULL);
				return CDRF_SKIPDEFAULT;
			}
		} else if((Util::stricmp(headerBuf, CTSTRING_I(columnNames[COLUMN_FILE])) == 0 || Util::stricmp(headerBuf, CTSTRING_I(columnNames[COLUMN_SIZE])) == 0 || 
			Util::stricmp(headerBuf, CTSTRING_I(columnNames[COLUMN_PATH])) == 0) && ii->type == ItemInfo::TYPE_DOWNLOAD && 
			ii->status != ItemInfo::STATUS_RUNNING) {
			cd->clrText = OperaColors::blendColors(WinUtil::bgColor, WinUtil::textColor, 0.4);
			return CDRF_NEWFONT;
		} else if (ii->type == ItemInfo::TYPE_DOWNLOAD && ii->status != ItemInfo::STATUS_RUNNING) {
			cd->clrText = WinUtil::textColor;
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
	if (item->iItem != -1) {		
		CRect rect;
		ctrlTransfers.GetItemRect(item->iItem, rect, LVIR_ICON);

		// if double click on state icon, ignore...
		if (item->ptAction.x < rect.left)
			return 0;

		ItemInfo* i = ctrlTransfers.getItemData(item->iItem);
		if(i->subItems.size() <= 1) {
			switch(SETTING(TRANSFERLIST_DBLCLICK)) {
				case 0:
					ctrlTransfers.getItemData(item->iItem)->pm();
					break;
				case 1:
					ctrlTransfers.getItemData(item->iItem)->getList();
					break;
				case 2:
					ctrlTransfers.getItemData(item->iItem)->matchQueue();
					break;
				case 4:
					ctrlTransfers.getItemData(item->iItem)->addFav();
					break;
			}
		}
	}
	return 0;
}

LRESULT TransferView::onSpeaker(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	if(wParam == ADD_ITEM) {
		ItemInfo* i = (ItemInfo*)lParam;
		if(i->type == ItemInfo::TYPE_DOWNLOAD) {
			ctrlTransfers.insertGroupedItem(i, false);
		} else {
			i->mainItem = true;
			ctrlTransfers.insertItem(ctrlTransfers.GetItemCount(), i, IMAGE_UPLOAD);
		}
	} else if(wParam == REMOVE_ITEM) {
		ItemInfo* i = (ItemInfo*)lParam;

		if(i->type == ItemInfo::TYPE_DOWNLOAD) {
			ctrlTransfers.removeGroupedItem(i);
		} else {
			ctrlTransfers.deleteItem(i);
			delete i;
		}
	} else if(wParam == UPDATE_ITEM) {
		ItemInfo* i = (ItemInfo*)lParam;
		if(!i->mainItem) {
			setMainItem(i);
			ItemInfo* main = i->main;

			if((i->status == ItemInfo::DOWNLOAD_STARTING)) {
				i->status = ItemInfo::STATUS_RUNNING;
				main->size = i->fullSize;
				if(main->status != ItemInfo::STATUS_RUNNING) {
					if ((!SETTING(BEGINFILE).empty()) && (!BOOLSETTING(SOUNDS_DISABLED)))
						PlaySound(Text::toT(SETTING(BEGINFILE)).c_str(), NULL, SND_FILENAME | SND_ASYNC);

					if(BOOLSETTING(POPUP_DOWNLOAD_START)) {
						MainFrame::getMainFrame()->ShowBalloonTip((
							TSTRING(FILE) + _T(": ")+ Text::toT(Util::getFileName(i->Target)) + _T("\n")+
							TSTRING(USER) + _T(": ") + Text::toT(i->user->getNick())).c_str(), CTSTRING(DOWNLOAD_STARTING));
					}
					main->status = ItemInfo::STATUS_RUNNING;
					main->statusString = TSTRING(DOWNLOAD_STARTING);
				}
			} else if(main->subItems.size() == 1) {
				if(i->status != ItemInfo::STATUS_RUNNING)
					i->status = ItemInfo::STATUS_WAITING;
				main->status = i->status;
				main->statusString = i->statusString;
				main->size = i->fullSize;
			} else {
				if(i->status == ItemInfo::STATUS_RUNNING) {
					main->status = ItemInfo::STATUS_RUNNING;
					main->size = i->fullSize;
				}
				if(i->status == ItemInfo::DOWNLOAD_FAILED) {
					i->status = ItemInfo::STATUS_WAITING;
					main->numberOfSegments = QueueManager::getInstance()->getRunningCount(i->user, i->Target);
					if(main->numberOfSegments == 0) {
						main->status = ItemInfo::STATUS_WAITING;
						main->statusString = i->statusString;
					}
				}
			}
			i->update();
			if(!main->collapsed)
				ctrlTransfers.updateItem(i);
			ctrlTransfers.updateItem(main);
		} else {
			i->update();
			ctrlTransfers.updateItem(i);
		}

		if(ctrlTransfers.getSortColumn() != COLUMN_USER)
			ctrlTransfers.resort();
	} else if(wParam == UPDATE_ITEMS) {
		vector<ItemInfo*>* v = (vector<ItemInfo*>*)lParam;
		ctrlTransfers.SetRedraw(FALSE);
		for(vector<ItemInfo*>::iterator j = v->begin(); j != v->end(); ++j) {
			ItemInfo* i = *j;
			if(i->main)	i->main->upperUpdated = false;
			i->update();
			if(i->mainItem || !i->main->collapsed)
				ctrlTransfers.updateItem(i);
			if(i->upperUpdated)
				ctrlTransfers.updateItem(i->main);
		}

		if(ctrlTransfers.getSortColumn() != COLUMN_STATUS)
			ctrlTransfers.resort();
		ctrlTransfers.SetRedraw(TRUE);

		delete v;
	}

	return 0;
}

void TransferView::ItemInfo::update() {
	u_int32_t colMask = updateMask;
	
	if(main != NULL) {
		main->updateMask |= colMask | ItemInfo::MASK_USER | ItemInfo::MASK_HUB;
		main->update();

		if(main->collapsed && !(colMask & MASK_IP)) return;
	}

	updateMask = 0;

	if(colMask & MASK_USER) {
		if(!mainItem || (subItems.size() == 1) || (type == TYPE_UPLOAD)) {
			columns[COLUMN_USER] = Text::toT(user->getNick());
		} else {
			TCHAR buf[256];
			_sntprintf(buf, 255, _T("%d %s"), subItems.size(), TSTRING(USERS));
			buf[255] = NULL;
			columns[COLUMN_USER] = buf;
		}
	}
	if(colMask & MASK_HUB) {
		if(!mainItem || (subItems.size() == 1) || (type == TYPE_UPLOAD)) {
			columns[COLUMN_HUB] = Text::toT(user->getClientName());
		} else {
			TCHAR buf[256];
			_sntprintf(buf, 255, _T("%d %s"), numberOfSegments, TSTRING(NUMBER_OF_SEGMENTS));
			buf[255] = NULL;
			columns[COLUMN_HUB] = buf;
		}
	}
	if(colMask & MASK_STATUS) {
		columns[COLUMN_STATUS] = statusString;
	}
	if (status == STATUS_RUNNING) {
		if(colMask & MASK_TIMELEFT) {
			columns[COLUMN_TIMELEFT] = Text::toT(Util::formatSeconds(timeLeft));
		}
		if(colMask & MASK_SPEED) {
			columns[COLUMN_SPEED] = Text::toT(Util::formatBytes(speed) + "/s");
		}
		if(colMask & MASK_RATIO) {
			columns[COLUMN_RATIO] = Text::toT(Util::toString(getRatio()));
		}
	} else {
		columns[COLUMN_TIMELEFT] = Util::emptyStringT;
		columns[COLUMN_SPEED] = Util::emptyStringT;
		columns[COLUMN_RATIO] = Util::emptyStringT;
	}
	if(colMask & MASK_FILE) {
		columns[COLUMN_FILE] = !file.empty() ? file : Text::toT(Util::getFileName(Target));
	}
	if(colMask & MASK_SIZE) {
		columns[COLUMN_SIZE] = Text::toT(Util::formatBytes(size));
	}
	if(colMask & MASK_PATH) {
		columns[COLUMN_PATH] = Text::toT(Util::getFilePath(Target));
	}
	if(colMask & MASK_IP) {
		if(!mainItem || (type == TYPE_UPLOAD)) {
			if(!country.empty())
				columns[COLUMN_IP] = country + _T(" (") + IP + _T(")");
			else
				columns[COLUMN_IP] = IP;
			if((type == TYPE_DOWNLOAD) && (main != NULL) && (main->subItems.size() <= 1)) {
				main->flagImage = flagImage;
				main->columns[COLUMN_IP] = columns[COLUMN_IP];
			}
		}
	}
}

void TransferView::on(ConnectionManagerListener::Added, ConnectionQueueItem* aCqi) {
	ItemInfo::Types t = aCqi->getDownload() ? ItemInfo::TYPE_DOWNLOAD : ItemInfo::TYPE_UPLOAD;
	ItemInfo* i = new ItemInfo(aCqi->getUser(), t, ItemInfo::STATUS_WAITING);

	string aTarget = Util::emptyString;
	int64_t aSize = 0;
	int aFlags;

	bool hasInfo = false;
	if(t == ItemInfo::TYPE_DOWNLOAD) {
		hasInfo = QueueManager::getInstance()->getQueueInfo(aCqi->getUser(), aTarget, aSize, aFlags);
	}

	{
		Lock l(cs);
		dcassert(transferItems.find(aCqi) == transferItems.end());
		transferItems.insert(make_pair(aCqi, i));
		i->columns[COLUMN_STATUS] = i->statusString = TSTRING(CONNECTING);

		if(hasInfo) {
			i->Target = aTarget;
			i->size = aSize;
			i->columns[COLUMN_FILE] = Text::toT(Util::getFileName(aTarget));
			i->columns[COLUMN_PATH] = Text::toT(Util::getFilePath(aTarget));
			i->columns[COLUMN_SIZE] = Text::toT(Util::formatBytes(aSize));
		}
	}

	PostMessage(WM_SPEAKER, ADD_ITEM, (LPARAM)i);
}

void TransferView::on(ConnectionManagerListener::StatusChanged, ConnectionQueueItem* aCqi) {
	ItemInfo* i;

	string aTarget;
	int64_t aSize;
	int aFlags;

	bool hasInfo = QueueManager::getInstance()->getQueueInfo(aCqi->getUser(), aTarget, aSize, aFlags);
	{
		Lock l(cs);
		dcassert(transferItems.find(aCqi) != transferItems.end());
		i = transferItems[aCqi];		
		
		if(hasInfo) {
			if (aFlags & QueueItem::FLAG_TESTSUR) {
				i->statusString = TSTRING(CHECKING_CLIENT);
			} else {
				i->statusString = TSTRING(CONNECTING);
			}
			i->Target = aTarget;
			i->size = aSize;
			i->updateMask |= (ItemInfo::MASK_FILE | ItemInfo::MASK_PATH | ItemInfo::MASK_SIZE);
		} else
			i->statusString = TSTRING(CONNECTING);

		i->updateMask |= ItemInfo::MASK_STATUS;
	}

	PostMessage(WM_SPEAKER, UPDATE_ITEM, (LPARAM)i);
}

void TransferView::on(ConnectionManagerListener::Removed, ConnectionQueueItem* aCqi) {
	ItemInfo* i;
	{
		Lock l(cs);
		ItemInfo::MapIter ii = transferItems.find(aCqi);
		dcassert(ii != transferItems.end());
		i = ii->second;
		transferItems.erase(ii);
	}
	PostMessage(WM_SPEAKER, REMOVE_ITEM, (LPARAM)i);
}

void TransferView::on(ConnectionManagerListener::Failed, ConnectionQueueItem* aCqi, const string& aReason) {
	ItemInfo* i;
	{
		Lock l(cs);
		dcassert(transferItems.find(aCqi) != transferItems.end());
		i = transferItems[aCqi];		
		i->statusString = Text::toT(aReason);
		i->status = ItemInfo::STATUS_WAITING;

		i->updateMask |= ItemInfo::MASK_STATUS;
	}
	PostMessage(WM_SPEAKER, UPDATE_ITEM, (LPARAM)i);
}

void TransferView::on(DownloadManagerListener::Starting, Download* aDownload) {
	ConnectionQueueItem* aCqi = aDownload->getUserConnection()->getCQI();
	ItemInfo* i;
	{
		Lock l(cs);
		dcassert(transferItems.find(aCqi) != transferItems.end());
		i = transferItems[aCqi];		
		i->status = ItemInfo::DOWNLOAD_STARTING;
		i->pos = 0;

		bool chunkInfo = aDownload->isSet(Download::FLAG_MULTI_CHUNK) && !aDownload->isSet(Download::FLAG_TREE_DOWNLOAD);
		i->start =  chunkInfo ? 0 : aDownload->getPos();
		
		i->size = chunkInfo ? aDownload->getSegmentSize() : aDownload->getSize();
		i->fullSize = aDownload->getSize();

		i->actual = i->start;
		i->Target = aDownload->getTarget();
		i->statusString = TSTRING(DOWNLOAD_STARTING);

		string ip = aDownload->getUserConnection()->getRemoteIp();
		string country = Util::getIpCountry(ip);
		i->IP = Text::toT(ip);
		i->country = Text::toT(country);
		i->flagImage = WinUtil::getFlagImage(country.c_str());
		i->updateMask |= ItemInfo::MASK_HUB | ItemInfo::MASK_STATUS | ItemInfo::MASK_FILE | ItemInfo::MASK_PATH |
			ItemInfo::MASK_SIZE | ItemInfo::MASK_IP;

		if(aDownload->isSet(Download::FLAG_TREE_DOWNLOAD)) {
			i->file = _T("TTH: ") + Text::toT(Util::getFileName(i->Target));
			i->status = ItemInfo::TREE_DOWNLOAD;
		} else if(aDownload->isSet(Download::FLAG_PARTIAL)){
			i->file = _T("Part: ") + Text::toT(Util::getFileName(i->Target));
		} else {
			i->file = Util::emptyStringT;
		}
	}

	PostMessage(WM_SPEAKER, UPDATE_ITEM, (LPARAM)i);
}

void TransferView::on(DownloadManagerListener::Tick, const Download::List& dl) {
	vector<ItemInfo*>* v = new vector<ItemInfo*>();
	v->reserve(dl.size());

	u_int32_t stringSize = TSTRING(DOWNLOADED_BYTES).size() + 64;
	AutoArray<TCHAR> buf(stringSize);

	{
		Lock l(cs);
		for(Download::List::const_iterator j = dl.begin(); j != dl.end(); ++j) {
			Download* d = *j;

			if(!d->getQI()) continue;

			ConnectionQueueItem* aCqi = d->getUserConnection()->getCQI();
			ItemInfo* i = transferItems[aCqi];	
			i->actual = i->start + d->getActual();
			i->pos = i->start + d->getTotal();
			i->timeLeft = d->getSecondsLeft();
			i->speed = d->getRunningAverage();
			i->upperUpdated = false;

			if(!d->isSet(Download::FLAG_TREE_DOWNLOAD)) {
				i->timeLeft = d->isSet(Download::FLAG_MULTI_CHUNK) ? ((i->speed > 0) ? ((i->size - d->getTotal()) / i->speed) : 0) : i->timeLeft;
				ItemInfo* u = i->main;

				if(u && !u->upperUpdated) {
					u->upperUpdated = true;
					i->upperUpdated = true;

					int64_t totalSpeed = 0;
					double ratio = 0;
					int segs = 0;
					bool compression = false;
					for(Download::List::const_iterator k = dl.begin(); k != dl.end(); ++k) {
						Download* l = *k;
						if(d->getTarget() == l->getTarget()) {
							totalSpeed += l->getRunningAverage();
							ratio += (l->getTotal() > 0) ? (double)l->getActual() / (double)l->getTotal() : 1.0;
							if(l->isSet(Download::FLAG_ZDOWNLOAD)) compression = true;
							segs++;
						}
					}
					ratio = ratio / segs;

					int64_t total = d->isSet(Download::FLAG_MULTI_CHUNK) ? d->getQueueTotal() : d->getPos();
					_stprintf(buf, CTSTRING(DOWNLOADED_BYTES), Text::toT(Util::formatBytes(total)).c_str(), 
						(double)total*100.0/(double)d->getSize(), Text::toT(Util::formatSeconds((GET_TICK() - d->getQI()->getStart())/1000)).c_str());

					u->numberOfSegments = segs;
					u->statusString = buf;
					u->actual = total * ratio;
					u->pos = total;
					u->status = ItemInfo::STATUS_RUNNING;
					u->speed = totalSpeed;
					u->timeLeft = (u->speed > 0) ? ((d->getSize() - u->pos) / u->speed) : 0;
					d->getQI()->setAverageSpeed(u->speed);

					if(compression) {
						u->setFlag(ItemInfo::FLAG_COMPRESSED);
					} else {
						u->unsetFlag(ItemInfo::FLAG_COMPRESSED);
					}
				}
				if(d->isSet(Download::FLAG_MULTI_CHUNK)) {
					double progress = (double)(d->getTotal())*100.0/(double)i->size;
					_stprintf(buf, CTSTRING(DOWNLOADED_BYTES), Text::toT(Util::formatBytes(d->getTotal())).c_str(), 
						progress, Text::toT(Util::formatSeconds((GET_TICK() - d->getStart())/1000)).c_str());
					if(progress > 100) {
						// workaround to fix > 100% percentage
						d->getUserConnection()->disconnect();
						continue;
					}
				}
			} else {
				_stprintf(buf, CTSTRING(DOWNLOADED_BYTES), Text::toT(Util::formatBytes(d->getPos())).c_str(), 
					(double)d->getPos()*100.0/(double)d->getSize(), Text::toT(Util::formatSeconds((GET_TICK() - d->getStart())/1000)).c_str());
			}

			buf[stringSize-1] = NULL;
			if (BOOLSETTING(SHOW_PROGRESS_BARS)) {
				if(d->isSet(Download::FLAG_ZDOWNLOAD)) {
					i->setFlag(ItemInfo::FLAG_COMPRESSED);
				} else {
					i->unsetFlag(ItemInfo::FLAG_COMPRESSED);
				}
				i->statusString = buf;
			} else {
				if(d->isSet(Download::FLAG_ZDOWNLOAD)) {
					i->statusString = _T("* ") + tstring(buf);
				} else {
					i->statusString = buf;
				}
			}
			i->updateMask |= ItemInfo::MASK_STATUS | ItemInfo::MASK_TIMELEFT | ItemInfo::MASK_SPEED | ItemInfo::MASK_RATIO;

			if((d->getRunningAverage() == 0) && ((GET_TICK() - d->getStart()) > 1000)) {
				d->getUserConnection()->disconnect();
			} else {
				v->push_back(i);
			}
		}
	}

	PostMessage(WM_SPEAKER, UPDATE_ITEMS, (LPARAM)v);
}

void TransferView::on(DownloadManagerListener::Failed, Download* aDownload, const string& aReason) {
	ConnectionQueueItem* aCqi = aDownload->getUserConnection()->getCQI();
	ItemInfo* i;
	{
		Lock l(cs);
		dcassert(transferItems.find(aCqi) != transferItems.end());
		i = transferItems[aCqi];		
		i->status = ItemInfo::DOWNLOAD_FAILED;
		i->pos = 0;

		i->statusString = Text::toT(aReason);
		i->size = aDownload->getSize();
		i->fullSize = aDownload->getSize();
		i->Target = aDownload->getTarget();

		if(BOOLSETTING(POPUP_DOWNLOAD_FAILED)) {
			MainFrame::getMainFrame()->ShowBalloonTip((
				TSTRING(FILE)+_T(": ") + Text::toT(Util::getFileName(i->Target)) + _T("\n")+
				TSTRING(USER)+_T(": ") + Text::toT(i->user->getNick()) + _T("\n")+
				TSTRING(REASON)+_T(": ") + Text::toT(aReason)).c_str(), CTSTRING(DOWNLOAD_FAILED), NIIF_WARNING);
		}

		i->updateMask |= ItemInfo::MASK_HUB | ItemInfo::MASK_STATUS | ItemInfo::MASK_SIZE | ItemInfo::MASK_FILE | ItemInfo::MASK_PATH;
		
		if(aDownload->isSet(Download::FLAG_TREE_DOWNLOAD)) {
			i->file = _T("TTH: ") + Text::toT(Util::getFileName(i->Target));
		} else if(aDownload->isSet(Download::FLAG_PARTIAL)){
			i->file = _T("Part: ") + Text::toT(Util::getFileName(i->Target));
		} else {
			i->file = Util::emptyStringT;
		}			
	}
	PostMessage(WM_SPEAKER, UPDATE_ITEM, (LPARAM)i);
}

void TransferView::on(DownloadManagerListener::Status, ConnectionQueueItem* aCqi, const string& aMessage) {
	ItemInfo* i;
	{
		Lock l(cs);
		dcassert(transferItems.find(aCqi) != transferItems.end());
		i = transferItems[aCqi];		
		i->status = ItemInfo::STATUS_WAITING;
		i->statusString = Text::toT(aMessage);
		if((i->main->status != ItemInfo::STATUS_RUNNING) && (aMessage == STRING(ALL_FILE_SLOTS_TAKEN))) {	
			i->main->statusString = Text::toT(aMessage);
		}
		i->updateMask |= ItemInfo::MASK_HUB | ItemInfo::MASK_STATUS;	
	}
	PostMessage(WM_SPEAKER, UPDATE_ITEM, (LPARAM)i);
}

void TransferView::on(DownloadManagerListener::Verifying, const string& fileName, int64_t verified) {
	ItemInfo* i = NULL;

	{
		Lock l(cs);

		i = ctrlTransfers.findMainItem(fileName);
		
		if(i == NULL)
			return;

		i->pos = verified;
		i->actual = verified;
		i->start = -1;
		i->statusString = TSTRING(CHECKING_TTH) + Text::toT(Util::toString(verified * 100 / i->size) + "%");
		i->updateMask = ItemInfo::MASK_STATUS;
	}
	PostMessage(WM_SPEAKER, UPDATE_ITEM, (LPARAM)i);
}

void TransferView::on(UploadManagerListener::Starting, Upload* aUpload) {
	ConnectionQueueItem* aCqi = aUpload->getUserConnection()->getCQI();
	ItemInfo* i;
	{
		Lock l(cs);
		dcassert(transferItems.find(aCqi) != transferItems.end());
		i = transferItems[aCqi];		
		i->pos = 0;
		i->start = aUpload->getPos();
		i->actual = i->start;
		i->size = aUpload->isSet(Upload::FLAG_TTH_LEAVES) ? aUpload->getSize() : aUpload->getFullSize();
		i->status = ItemInfo::STATUS_RUNNING;
		i->speed = 0;
		i->timeLeft = 0;
		i->Target = aUpload->getFileName();
		i->statusString = TSTRING(UPLOAD_STARTING);
		string ip = aUpload->getUserConnection()->getRemoteIp();
		string country = Util::getIpCountry(ip);
		i->IP = Text::toT(ip);
		i->country = Text::toT(country);
		i->flagImage = WinUtil::getFlagImage(country.c_str());
		i->updateMask |= ItemInfo::MASK_STATUS | ItemInfo::MASK_FILE | ItemInfo::MASK_PATH |
			ItemInfo::MASK_SIZE | ItemInfo::MASK_IP;

		if(aUpload->isSet(Upload::FLAG_TTH_LEAVES)) {
			i->file = _T("TTH: ") + Text::toT(Util::getFileName(i->Target));
		} else {
			i->file = Util::emptyStringT;
		}

	}

	PostMessage(WM_SPEAKER, UPDATE_ITEM, (LPARAM)i);
}

void TransferView::on(UploadManagerListener::Tick, const Upload::List& ul) {
	vector<ItemInfo*>* v = new vector<ItemInfo*>();
	v->reserve(ul.size());

    AutoArray<TCHAR> buf(STRING(UPLOADED_BYTES).size() + 64);

	{
		Lock l(cs);
		for(Upload::List::const_iterator j = ul.begin(); j != ul.end(); ++j) {
			Upload* u = *j;

			if (u->getTotal() == 0) continue;

			ConnectionQueueItem* aCqi = u->getUserConnection()->getCQI();
			ItemInfo* i = transferItems[aCqi];	
			i->actual = i->start + u->getActual();
			i->pos = i->start + u->getTotal();
			i->timeLeft = u->getSecondsLeft();
			i->speed = u->getRunningAverage();
			i->status = ItemInfo::STATUS_RUNNING;

			if (u->getPos() > 0) {
				_stprintf(buf, CTSTRING(UPLOADED_BYTES), Text::toT(Util::formatBytes(u->getPos())).c_str(), 
				(double)u->getPos()*100.0/(double)(u->isSet(Upload::FLAG_TTH_LEAVES) ? u->getSize() : u->getFullSize()), Text::toT(Util::formatSeconds((GET_TICK() - u->getStart())/1000)).c_str());
            } else _stprintf(buf, CTSTRING(UPLOAD_STARTING));
			if (BOOLSETTING(SHOW_PROGRESS_BARS)) {
				u->isSet(Upload::FLAG_ZUPLOAD) ? i->setFlag(ItemInfo::FLAG_COMPRESSED) : i->unsetFlag(ItemInfo::FLAG_COMPRESSED);
				i->statusString = buf;
			} else {
				if(u->isSet(Upload::FLAG_ZUPLOAD)) {
				i->statusString = _T("* ") + tstring(buf);
				} else {
					i->statusString = buf;
				}
			}

			i->updateMask |= ItemInfo::MASK_STATUS | ItemInfo::MASK_TIMELEFT | ItemInfo::MASK_SPEED | ItemInfo::MASK_RATIO;
			v->push_back(i);

			if((u->getRunningAverage() == 0) && ((GET_TICK() - u->getStart()) > 1000)) {
				u->getUserConnection()->disconnect();
			}
		}
	}

	PostMessage(WM_SPEAKER, UPDATE_ITEMS, (LPARAM)v);
}

void TransferView::onTransferComplete(Transfer* aTransfer, bool isUpload, bool isTree) {
	ConnectionQueueItem* aCqi = aTransfer->getUserConnection()->getCQI();
	ItemInfo* i;
	{
		Lock l(cs);
		dcassert(transferItems.find(aCqi) != transferItems.end());
		i = transferItems[aCqi];		

		if(!isUpload) {
			i->main->status = ItemInfo::STATUS_WAITING;
			i->main->pos = 0;
			i->main->actual = 0;
			if(!isTree) {
				i->numberOfSegments = 0;
				i->main->numberOfSegments = 0;
				i->main->statusString = TSTRING(DOWNLOAD_FINISHED_IDLE);
				i->updateMask |= ItemInfo::MASK_HUB;
			}
			if(BOOLSETTING(POPUP_DOWNLOAD_FINISHED) && !isTree) {
				MainFrame::getMainFrame()->ShowBalloonTip((
					TSTRING(FILE) + _T(": ") + Text::toT(Util::getFileName(i->Target)) + _T("\n")+
					TSTRING(USER) + _T(": ") + Text::toT(i->user->getNick())).c_str(), CTSTRING(DOWNLOAD_FINISHED_IDLE));
			}
		} else {
			if(BOOLSETTING(POPUP_UPLOAD_FINISHED)) {
				MainFrame::getMainFrame()->ShowBalloonTip((
					TSTRING(FILE) + _T(": ") + Text::toT(Util::getFileName(i->Target)) + _T("\n")+
					TSTRING(USER) + _T(": ") + Text::toT(i->user->getNick())).c_str(), CTSTRING(UPLOAD_FINISHED_IDLE));
			}
		}

		i->status = ItemInfo::STATUS_WAITING;
		i->pos = 0;

		i->statusString = isUpload ? TSTRING(UPLOAD_FINISHED_IDLE) : TSTRING(DOWNLOAD_FINISHED_IDLE);
		i->updateMask |= ItemInfo::MASK_STATUS;
	}
	PostMessage(WM_SPEAKER, UPDATE_ITEM, (LPARAM)i);
}

void TransferView::ItemInfo::disconnect() {
	ConnectionManager::getInstance()->removeConnection(user, (type == TYPE_DOWNLOAD));
}

LRESULT TransferView::onSearchAlternates(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int i = ctrlTransfers.GetNextItem(-1, LVNI_SELECTED);

	if(i != -1) {
		ItemInfo *ii = ctrlTransfers.getItemData(i);

		QueueItem::StringMap queue = QueueManager::getInstance()->lockQueue();

		string tmp = ii->Target;
		QueueItem::StringIter qi = queue.find(&tmp);

		//create a copy of the tth to avoid holding the filequeue lock while calling
		//into searchframe, searchmanager and all of that
		TTHValue *val = NULL;
		if(qi != queue.end() && qi->second->getTTH()) {
			val = new TTHValue(*qi->second->getTTH());
		}

		QueueManager::getInstance()->unlockQueue();

		if(val) {
			WinUtil::searchHash(val);
			delete val;
		}
	}

	return 0;
}
			
LRESULT TransferView::onPreviewCommand(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/){
	int i = ctrlTransfers.GetNextItem(-1, LVNI_SELECTED);

	if(i != -1) {
		ItemInfo *ii = ctrlTransfers.getItemData(i);

		QueueItem::StringMap queue = QueueManager::getInstance()->lockQueue();

		string tmp = ii->Target;
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
		if((m->type == ItemInfo::TYPE_DOWNLOAD) && (!m->mainItem)) {
			ctrlTransfers.deleteItem(m); 
		}
		if((m->type == ItemInfo::TYPE_DOWNLOAD) && m->mainItem) {
			m->collapsed = true;
			ctrlTransfers.SetItemState(ctrlTransfers.findItem(m), INDEXTOSTATEIMAGEMASK(1), LVIS_STATEIMAGEMASK);
		 }
	}
}

void TransferView::ExpandAll() {
	for(vector<ItemInfo*>::iterator i = ctrlTransfers.mainItems.begin(); i != ctrlTransfers.mainItems.end(); ++i) {
		if((*i)->collapsed) {
			ctrlTransfers.Expand(*i, ctrlTransfers.findItem(*i));
		}
	}
}

LRESULT TransferView::onConnectAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(ctrlTransfers.GetSelectedCount() == 1) {
		int i = ctrlTransfers.GetNextItem(-1, LVNI_SELECTED);
		ItemInfo* ii = ctrlTransfers.getItemData(i);
		ctrlTransfers.SetItemText(i, COLUMN_STATUS, CTSTRING(CONNECTING_FORCED));
		for(ItemInfo::Map::iterator j = transferItems.begin(); j != transferItems.end(); ++j) {
			ItemInfo* m = j->second;
			if((m->Target == ii->Target) && (m->type == ItemInfo::TYPE_DOWNLOAD)) {	
				int h = ctrlTransfers.findItem(m);
				if(h != -1)
					ctrlTransfers.SetItemText(h, COLUMN_STATUS, CTSTRING(CONNECTING_FORCED));
				m->user->connect();
			}
		}
	}
	return 0;
}

LRESULT TransferView::onDisconnectAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(ctrlTransfers.GetSelectedCount() == 1) {
		int i = ctrlTransfers.GetNextItem(-1, LVNI_SELECTED);
		ItemInfo* ii = ctrlTransfers.getItemData(i);
		ctrlTransfers.SetItemText(i, COLUMN_STATUS, CTSTRING(DISCONNECTED));
		for(ItemInfo::Map::iterator j = transferItems.begin(); j != transferItems.end(); ++j) {
			ItemInfo* m = j->second;
			if((m->Target == ii->Target) && (m->type == ItemInfo::TYPE_DOWNLOAD)) {	
				int h = ctrlTransfers.findItem(m);
				if(h != -1)
					ctrlTransfers.SetItemText(h, COLUMN_STATUS, CTSTRING(DISCONNECTED));
				m->disconnect();
			}
		}
	}
	return 0;
}

LRESULT TransferView::onSlowDisconnect(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int i = ctrlTransfers.GetNextItem(-1, LVNI_SELECTED);

	if(i != -1) {
		ItemInfo *ii = ctrlTransfers.getItemData(i);

		QueueItem::StringMap queue = QueueManager::getInstance()->lockQueue();

		string tmp = ii->Target;
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

void TransferView::on(SettingsManagerListener::Save, SimpleXML* /*xml*/) throw() {
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

/**
 * @file
 * $Id$
 */
