#ifndef CLIENTSPAGE_H
#define CLIENTSPAGE_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "PropPage.h"
#include "ExListViewCtrl.h"
#include "../client/HttpConnection.h"
#include "../client/File.h"

class ClientProfile;

class ClientsPage : public CPropertyPage<IDD_CLIENTS_PAGE>, public PropPage, private HttpConnectionListener
{
public:
	enum { WM_PROFILE = WM_APP + 53 };

	ClientsPage(SettingsManager *s) : PropPage(s) {
		title = strdup((STRING(SETTINGS_CZDC) + '\\' + STRING(SETTINGS_FAKEDETECT) + '\\' + STRING(SETTINGS_CLIENTS)).c_str());
		SetTitle(title);
	};

	virtual ~ClientsPage() { 
		ctrlProfiles.Detach();
		delete[] title;
	};

	BEGIN_MSG_MAP(ClientsPage)
		MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
		COMMAND_ID_HANDLER(IDC_ADD_CLIENT, onAddClient)
		COMMAND_ID_HANDLER(IDC_REMOVE_CLIENT, onRemoveClient)
		COMMAND_ID_HANDLER(IDC_CHANGE_CLIENT, onChangeClient)
		COMMAND_ID_HANDLER(IDC_MOVE_CLIENT_UP, onMoveClientUp)
		COMMAND_ID_HANDLER(IDC_MOVE_CLIENT_DOWN, onMoveClientDown)
		COMMAND_ID_HANDLER(IDC_RELOAD_CLIENTS, onReload)
		COMMAND_ID_HANDLER(IDC_UPDATE, onUpdate)
		NOTIFY_HANDLER(IDC_CLIENT_ITEMS, NM_CUSTOMDRAW, onCustomDraw)
		NOTIFY_HANDLER(IDC_CLIENT_ITEMS, NM_DBLCLK, onDblClick)
		NOTIFY_HANDLER(IDC_CLIENT_ITEMS, LVN_GETINFOTIP, onInfoTip)
	END_MSG_MAP()

	LRESULT onInitDialog(UINT, WPARAM, LPARAM, BOOL&);

	LRESULT onAddClient(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onChangeClient(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onRemoveClient(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onMoveClientUp(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onMoveClientDown(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onReload(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onUpdate(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onInfoTip(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);

	LRESULT onDblClick(int idCtrl, LPNMHDR /* pnmh */, BOOL& bHandled) {
		return onChangeClient(0, 0, 0, bHandled);
	}
	LRESULT onCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled);
	// Common PropPage interface
	PROPSHEETPAGE *getPSP() { return (PROPSHEETPAGE *)*this; }
	virtual void write();
	
protected:
	ExListViewCtrl ctrlProfiles;

	static Item items[];
	static TextItem texts[];
	char* title;
	void addEntry(const ClientProfile& cp, int pos);
private:
	virtual void on(HttpConnectionListener::Data, HttpConnection* /*conn*/, const u_int8_t* buf, size_t len) throw() {
		downBuf.append((char*)buf, len);
	}

	virtual void on(HttpConnectionListener::Complete, HttpConnection* conn, string const& /*aLine*/) throw() {
		conn->removeListener(this);
		if(!downBuf.empty()) {
			string fname = Util::getAppPath() + SETTINGS_DIR + "Profiles.xml";
			File f(fname + ".tmp", File::WRITE, File::CREATE | File::TRUNCATE);
			f.write(downBuf);
			f.close();
			File::deleteFile(fname);
			File::renameFile(fname + ".tmp", fname);
				reloadFromHttp();
			MessageBox("Client profiles now updated.", "Updated", MB_OK);
		}
	}

	virtual void on(HttpConnectionListener::Failed, HttpConnection* conn, const string& aLine) throw() {
		conn->removeListener(this);
		{
			string msg = "Client profiles download failed.\r\n" + aLine;
			MessageBox(msg.c_str(), "Failed", MB_OK);
		}
	}
	void reload();
	void reloadFromHttp();

	HttpConnection c;
	string downBuf;
};

#endif //CLIENTSPAGE_H

/**
 * @file
 * $Id$
 */