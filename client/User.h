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

#if !defined(USER_H)
#define USER_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Util.h"
#include "Pointer.h"
#include "CriticalSection.h"
#include "CID.h"
#include "FastAlloc.h"
#include "SettingsManager.h"
#include "ResourceManager.h"

class Client;
class FavoriteUser;

/** A user connected to one or more hubs. */
class User : public FastAlloc<User>, public PointerBase, public Flags
{
public:
	enum Bits {
		OP_BIT,
		ONLINE_BIT,
		DCPLUSPLUS_BIT,
		PASSIVE_BIT,
		BOT_BIT,
		HUB_BIT,
		TTH_GET_BIT,
		QUIT_HUB_BIT,
		HIDDEN_BIT,
		AWAY_BIT,
		SERVER_BIT,
		FIREBALL_BIT

	};

	/** Each flag is set if it's true in at least one hub */
	enum UserFlags {
		OP = 1<<OP_BIT,
		ONLINE = 1<<ONLINE_BIT,
		DCPLUSPLUS = 1<<DCPLUSPLUS_BIT,
		PASSIVE = 1<<PASSIVE_BIT,
		BOT = 1<<BOT_BIT,
		HUB = 1<<HUB_BIT,
		TTH_GET = 1<<TTH_GET_BIT,		//< User supports getting files by tth -> don't have path in queue...
		QUIT_HUB = 1<<QUIT_HUB_BIT,
		HIDDEN = 1<<HIDDEN_BIT,
		AWAY = 1<<AWAY_BIT,
		SERVER = 1<<SERVER_BIT,
		FIREBALL = 1<<FIREBALL_BIT,
	};

	typedef Pointer<User> Ptr;
	typedef vector<Ptr> List;
	typedef List::iterator Iter;
	typedef HASH_MAP_X(string,Ptr,noCaseStringHash,noCaseStringEq,noCaseStringLess) NickMap;
	typedef NickMap::iterator NickIter;
	typedef HASH_MAP_X(CID, Ptr, CID::Hash, equal_to<CID>, less<CID>) CIDMap;
	typedef CIDMap::iterator CIDIter;

	struct HashFunction {
		static const size_t bucket_size = 4;
		static const size_t min_buckets = 8;
		size_t operator()(const Ptr& x) const { return ((size_t)(&(*x)))/sizeof(User); };
		bool operator()(const Ptr& a, const Ptr& b) const { return (&(*a)) < (&(*b)); };
	};

	User(const CID& aCID) : cid(aCID), bytesShared(0), slots(0), udpPort(0), client(NULL), favoriteUser(NULL) { unCacheClientInfo(); }
	User(const string& aNick) throw() : nick(aNick), bytesShared(0), slots(0), udpPort(0), client(NULL), favoriteUser(NULL), autoextraslot(false),
			ctype(10), status(1), ip(Util::emptyString), downloadSpeed(-1), hasTestSURinQueue(false), cid(NULL) {
		unCacheClientInfo();
		unsetFlag(User::OP);
		unsetFlag(User::PASSIVE);
		unsetFlag(User::DCPLUSPLUS);
		unsetFlag(User::AWAY);
		unsetFlag(User::SERVER);
		unsetFlag(User::FIREBALL);				
	}

	virtual ~User() throw();

	void setClient(Client* aClient);
	void connect();
	const string& getClientNick() const;
	CID getClientCID() const;
	const string& getClientName() const;
	string getClientAddressPort() const;
	void privateMessage(const string& aMsg);
	void clientMessage(const string& aMsg);
	bool isClientOp() const;
	void send(const string& msg);
	void sendUserCmd(const string& aUserCmd);
	
	string getFullNick() const { 
		string tmp(getNick());
		tmp += " (";
		tmp += getClientName();
		tmp += ")";
		return tmp;
	}
	
	void setBytesShared(const string& aSharing) { setBytesShared(Util::toInt64(aSharing)); };

	bool isOnline() const { return isSet(ONLINE); };
	bool isClient(Client* aClient) const { return client == aClient; };

	void getParams(StringMap& ucParams, bool myNick = false);

	// favorite user stuff
	void setFavoriteUser(FavoriteUser* aUser);
	bool isFavoriteUser() const;
	void setFavoriteLastSeen(u_int32_t anOfflineTime = 0);
	u_int32_t getFavoriteLastSeen() const;
	const string& getUserDescription() const;
	void setUserDescription(const string& aDescription);
	string insertUserData(const string& s, Client* aClient = NULL);

	static void updated(User::Ptr& aUser);
	
	Client* getClient() { return client; }
	
	GETSET(string, connection, Connection);
	GETSET(int, ctype, cType);
	GETSET(int, status, Status);
	GETSET(int, fileListDisconnects, FileListDisconnects);
	GETSET(int, connectionTimeouts, ConnectionTimeouts);
	GETSET(string, nick, Nick);
	GETSET(string, email, Email);
	GETSET(string, description, Description);
	GETSET(string, tag, Tag);
	GETSET(string, version, Version);
	GETSET(string, mode, Mode);
	GETSET(string, hubs, Hubs);
	GETSET(string, upload, Upload);
	GETSET(string, lastHubAddress, LastHubAddress);
	GETSET(string, lastHubName, LastHubName);
	GETSET(string, ip, Ip);
	GETSET(CID, cid, CID);
	GETSET(string, supports, Supports);
	GETSET(string, lock, Lock);
	GETSET(string, pk, Pk);
	GETSET(string, clientType, ClientType);
	GETSET(string, generator, Generator);
	GETSET(string, testSUR, TestSUR);
	GETSET(string, unknownCommand, UnknownCommand);
	GETSET(string, comment, Comment);	
	GETSET(string, cheatingString, CheatingString);
	GETSET(int64_t, downloadSpeed, DownloadSpeed);
	GETSET(int64_t, fileListSize, FileListSize);
	GETSET(int64_t, bytesShared, BytesShared);
	GETSET(int, slots, Slots);
	GETSET(short, udpPort, UDPPort);
	GETSET(int64_t, realBytesShared, RealBytesShared);
	GETSET(int64_t, fakeShareBytesShared, FakeShareBytesShared);
	GETSET(int64_t, listLength, ListLength);
	GETSET(bool, autoextraslot, AutoExtraSlot);
	GETSET(bool, testSURComplete, TestSURComplete);
	GETSET(bool, filelistComplete, FilelistComplete);
	GETSET(bool, badClient, BadClient);	
	GETSET(bool, badFilelist, BadFilelist);
	GETSET(bool, isInList, IsInList);
	GETSET(bool, hasTestSURinQueue, HasTestSURinQueue);
	StringMap& clientEscapeParams(StringMap& sm) const;

	void setCheat(const string& aCheatDescription, bool aBadClient, bool postToChat = true) {
		if(isSet(User::OP) || !isClientOp()) return;

		if ((!SETTING(FAKERFILE).empty()) && (!BOOLSETTING(SOUNDS_DISABLED)))
			PlaySound(Text::toT(SETTING(FAKERFILE)).c_str(), NULL, SND_FILENAME | SND_ASYNC);
		
		StringMap ucParams;
		getParams(ucParams);
		string cheat = Util::formatParams(aCheatDescription, ucParams);
		if(postToChat)
			addLine("*** "+STRING(USER)+" "+nick+" - "+cheat);
		cheatingString = cheat;
		badClient = aBadClient;
	}
		
	void TagParts();
	void updateClientType();
	bool matchProfile(const string& aString, const string& aProfile);
	string getReport();
	void sendRawCommand(const int aRawCommand);
	void unCacheClientInfo() {
		if(!isFavoriteUser()) {
			lastHubAddress = Util::emptyString;
			lastHubName = Util::emptyString;
			description = Util::emptyString;
		}
		connection = Util::emptyString;
		email = Util::emptyString;
		tag = Util::emptyString;
		version = Util::emptyString;
		mode = Util::emptyString;
		hubs = Util::emptyString;
		upload = Util::emptyString;
		ip = Util::emptyString;
		supports = Util::emptyString;
		bytesShared = 0;
		realBytesShared = -1;
		fakeShareBytesShared = -1;
		isInList = false;
		testSURComplete = false;
		filelistComplete = false;
		pk = Util::emptyString;
		lock = Util::emptyString;
		supports = Util::emptyString;
		clientType = Util::emptyString;
		generator = Util::emptyString;
		testSUR = Util::emptyString;
		unknownCommand = Util::emptyString;
		cheatingString = Util::emptyString;
		comment = Util::emptyString;
		fileListSize = -1;
		listLength = -1;
		badClient = false;
		badFilelist = false;
		fileListDisconnects = 0;
		connectionTimeouts = 0;
	};
	void addLine(const string& aLine);
	void updated();
	bool fileListDisconnected();
	bool connectionTimeout();
	void setPassive();
private:
	mutable RWLock<> cs;

	User(const User&);
	User& operator=(const User&);

	Client* client;
	FavoriteUser* favoriteUser;
	StringMap getPreparedFormatedStringMap(Client* aClient = NULL); 
	string getVersion(const string& aExp, const string& aTag);
	string splitVersion(const string& aExp, const string& aTag, const int part);
};

/** One of possibly many identities of a user, mainly for UI purposes */
class Identity : public Flags {
public:
	Identity(const User::Ptr& ptr) : user(ptr) { }
	Identity(const Identity& rhs) : user(rhs.user), hubURL(rhs.hubURL), info(rhs.info) { }
	Identity& operator=(const Identity& rhs) { user = rhs.user; hubURL = rhs.hubURL; info = rhs.info; }

	const string& getNick() { return get("NI"); }
	const string& getDescription() { return get("DE"); }

	const string& get(const char* name) {
		InfIter i = info.find(*(short*)name);
		return i == info.end() ? Util::emptyString : i->second;
	}

	GETSET(User::Ptr, user, User);
	GETSET(string, hubURL, HubURL);
private:
	typedef map<short, string> InfMap;
	typedef InfMap::iterator InfIter;

	InfMap info;
};

#endif // !defined(USER_H)

/**
 * @file
 * $Id$
 */
