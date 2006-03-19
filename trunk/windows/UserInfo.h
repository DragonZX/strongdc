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

#ifndef USERINFO_H
#define USERINFO_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "TypedListViewCtrl.h"
#include "WinUtil.h"

#include "../client/FastAlloc.h"

	enum Speakers { UPDATE_USER_JOIN, UPDATE_USER, REMOVE_USER, ADD_CHAT_LINE,
		ADD_STATUS_LINE, ADD_SILENT_STATUS_LINE, SET_WINDOW_TITLE, GET_PASSWORD, 
		PRIVATE_MESSAGE, STATS, CONNECTED, DISCONNECTED, CHEATING_USER,
		GET_SHUTDOWN, SET_SHUTDOWN, KICK_MSG
	};

	struct Task {
		Task(Speakers speaker_) : speaker(speaker_) { }
		virtual ~Task() { }
		Speakers speaker;
	};

	struct UserTask : public Task {
		UserTask(Speakers speaker_, const OnlineUser& ou) : Task(speaker_), user(ou.getUser()), identity(ou.getIdentity()) { }

		User::Ptr user;
		Identity identity;
	};

	struct StringTask : public Task {
		StringTask(Speakers speaker_, const tstring& msg_) : Task(speaker_), msg(msg_) { }
		tstring msg;
	};

	struct MessageTask : public StringTask {
		MessageTask(Speakers speaker_, const Identity& from_, const User::Ptr& to_, const User::Ptr& replyTo_, const tstring& m) : StringTask(speaker_, m), from(from_), to(to_), replyTo(replyTo_) { }
		Identity from;
		User::Ptr to;
		User::Ptr replyTo;
	};

class UserInfo : public UserInfoBase, public FastAlloc<UserInfo> {
friend struct CompareItems;
public:
	enum {
		COLUMN_FIRST,
		COLUMN_NICK = COLUMN_FIRST, 
		COLUMN_SHARED, 
		COLUMN_EXACT_SHARED, 
		COLUMN_DESCRIPTION, 
		COLUMN_TAG,
		COLUMN_CONNECTION, 
		COLUMN_EMAIL, 
		COLUMN_CLIENTID, 
		COLUMN_VERSION, 
		COLUMN_MODE, 
		COLUMN_HUBS, 
		COLUMN_SLOTS,
		COLUMN_UPLOAD_SPEED, 
		COLUMN_IP,
		COLUMN_PK, 
		COLUMN_LOCK, 
		COLUMN_SUPPORTS,
		COLUMN_LAST
	};
	UserInfo(const UserTask& u) : UserInfoBase(u.user) {
		update(u.identity, -1); 
	};
	const tstring& getText(int col) const;

	static int compareItems(const UserInfo* a, const UserInfo* b, int col);
	int imageIndex() {
		return WinUtil::getImage(identity);
	}

	bool update(const Identity& identity, int sortCol);

	tstring columns[COLUMN_LAST];
	GETSET(Identity, identity, Identity);
	GETSET(int64_t, bytes, Bytes);
};

#endif //USERINFO_H
