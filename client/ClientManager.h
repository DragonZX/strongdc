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

#if !defined(CLIENT_MANAGER_H)
#define CLIENT_MANAGER_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "TimerManager.h"

#include "Client.h"
#include "Singleton.h"
#include "SettingsManager.h"
#include "DirectoryListing.h"

#include "ClientManagerListener.h"

class UserCommand;

class ClientManager : public Speaker<ClientManagerListener>, 
	private ClientListener, public Singleton<ClientManager>, 
	private TimerManagerListener, private SettingsManagerListener
{
public:
	Client* getClient(const string& aHubURL);
	void putClient(Client* aClient);

	size_t getUserCount();
	int64_t getAvailable();
	StringList getHubs(const CID& cid);
	StringList getHubNames(const CID& cid);
	StringList getNicks(const CID& cid);
	string getConnection(const CID& cid);

	bool isConnected(const string& aUrl);
	
	void search(int aSizeMode, int64_t aSize, int aFileType, const string& aString, const string& aToken);
	void search(StringList& who, int aSizeMode, int64_t aSize, int aFileType, const string& aString, const string& aToken);
	void infoUpdated(bool antispam);

	User::Ptr getUser(const string& aNick, const string& aHubUrl) throw();
	User::Ptr getUser(const CID& cid) throw();

	string findHub(const string& ipPort);

	User::Ptr findUser(const string& aNick, const string& aHubUrl) throw() { return findUser(makeCid(aNick, aHubUrl)); }
	User::Ptr findUser(const CID& cid) throw();
	User::Ptr findLegacyUser(const string& aNick) const throw();

	bool isOnline(const User::Ptr& aUser) {
		Lock l(cs);
		return onlineUsers.find(aUser->getCID()) != onlineUsers.end();
	}
	
	void setIPUser(const string& IP, User::Ptr user) {
		Lock l(cs);
		ipList[IP] = user->getFirstNick();
		OnlinePair p = onlineUsers.equal_range(user->getCID());
		for (OnlineIter i = p.first; i != p.second; i++) i->second->getIdentity().setIp(IP);
	}	
	
	const string& getIPNick(const string& IP) const {
		NickMap::const_iterator it;
		if ((it = ipList.find(IP)) != ipList.end() && !it->second.empty())
			return it->second;
		else
			return IP;
	}

	OnlineUser& getOnlineUser(const User::Ptr& p) {
		Lock l(cs);
		OnlineIter i = onlineUsers.find(p->getCID());
		if(i != onlineUsers.end()) {
			return *i->second;
		}
		return *(OnlineUser*)NULL;
	}

	bool isOp(const User::Ptr& aUser, const string& aHubUrl);

	/** Constructs a synthetic, hopefully unique CID */
	CID makeCid(const string& nick, const string& hubUrl) throw();

	void putOnline(OnlineUser& ou) throw();
	void putOffline(OnlineUser& ou) throw();

	User::Ptr& getMe();
	
	void connect(const User::Ptr& p);
	void send(AdcCommand& c, const CID& to);
	void privateMessage(const User::Ptr& p, const string& msg);

	void userCommand(const User::Ptr& p, const ::UserCommand& uc, StringMap& params, bool compatibility);

	int getMode(const string& aHubUrl);
	bool isActive(const string& aHubUrl) { return getMode(aHubUrl) != SettingsManager::INCOMING_FIREWALL_PASSIVE; }

	void lock() throw() { cs.enter(); }
	void unlock() throw() { cs.leave(); }

	Identity getIdentity(const User::Ptr& aUser);

	Client::List& getClients() { return clients; }

 	void removeClientListener(ClientListener* listener) {
 		Lock l(cs);
 		Client::Iter endIt = clients.end();
 		for(Client::Iter it = clients.begin(); it != endIt; ++it) {
 			Client* client = *it;
 			client->removeListener(listener);
 		}
 	}

	string getCachedIp() const { Lock l(cs); return cachedIp; }

	CID getMyCID();
	const CID& getMyPID();
	
	void save();
		
	// fake detection methods
	void setListLength(const User::Ptr& p, const string& listLen);
	void setPkLock(const User::Ptr& p, const string& aPk, const string& aLock);
	bool fileListDisconnected(const User::Ptr& p);
	bool connectionTimeout(const User::Ptr& p);
	void checkCheating(const User::Ptr& p, DirectoryListing* dl);
	void setCheating(const User::Ptr& p, const string& aTestSURString, const string& aCheatString, const int aRawCommand, bool aBadClient);


private:
	typedef HASH_MAP<string, User::Ptr> LegacyMap;
	typedef LegacyMap::iterator LegacyIter;

	typedef HASH_MAP_X(CID, User::Ptr, CID::Hash, equal_to<CID>, less<CID>) UserMap;
	typedef UserMap::iterator UserIter;
	typedef map<string, string, noCaseStringLess> NickMap;

	typedef HASH_MULTIMAP_X(CID, OnlineUser*, CID::Hash, equal_to<CID>, less<CID>) OnlineMap;
	typedef OnlineMap::const_iterator OnlineIter;
	typedef pair<OnlineIter, OnlineIter> OnlinePair;

	Client::List clients;
	mutable CriticalSection cs;
	NickMap ipList;
	
	UserMap users;
	OnlineMap onlineUsers;

	User::Ptr me;
	
	Socket s;

	string cachedIp;
	CID pid;	

	int64_t quickTick;
	int infoTick;

	friend class Singleton<ClientManager>;

	ClientManager() { 
		TimerManager::getInstance()->addListener(this); 
		SettingsManager::getInstance()->addListener(this);
		quickTick = GET_TICK();
		infoTick = 0;
	}

	virtual ~ClientManager() throw() { 
		SettingsManager::getInstance()->removeListener(this);
		TimerManager::getInstance()->removeListener(this); 
	}

	string getUsersFile() { return Util::getConfigPath() + "Users.xml"; }

	void updateCachedIp();

	// SettingsManagerListener
	virtual void on(Load, SimpleXML*) throw();
	virtual void on(Save, SimpleXML*) throw();

	// ClientListener
	virtual void on(Connected, Client* c) throw() { fire(ClientManagerListener::ClientConnected(), c); }
	virtual void on(UserUpdated, Client*, const OnlineUser& user) throw() { fire(ClientManagerListener::UserUpdated(), user); }
	virtual void on(UsersUpdated, Client* c, const User::List&) throw() { fire(ClientManagerListener::ClientUpdated(), c); }
	virtual void on(Failed, Client*, const string&) throw();
	virtual void on(HubUpdated, Client* c) throw() { fire(ClientManagerListener::ClientUpdated(), c); }
	virtual void on(UserCommand, Client*, int, int, const string&, const string&) throw();
	virtual void on(NmdcSearch, Client* aClient, const string& aSeeker, int aSearchType, int64_t aSize, 
		int aFileType, const string& aString, bool) throw();
	virtual void on(AdcSearch, Client* c, const AdcCommand& adc, const CID& from) throw();
	// TimerManagerListener
	virtual void on(TimerManagerListener::Minute, u_int32_t aTick) throw();
};

#endif // !defined(CLIENT_MANAGER_H)

/**
 * @file
 * $Id$
 */
