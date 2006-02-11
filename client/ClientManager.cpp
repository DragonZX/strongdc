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

#include "stdinc.h"
#include "DCPlusPlus.h"

#include "ClientManager.h"

#include "ShareManager.h"
#include "SearchManager.h"
#include "CryptoManager.h"
#include "ConnectionManager.h"
#include "FavoriteManager.h"
#include "QueueManager.h"
#include "FinishedManager.h"
#include "SimpleXML.h"
#include "UserCommand.h"
#include "ResourceManager.h"

#include "AdcHub.h"
#include "NmdcHub.h"


Client* ClientManager::getClient(const string& aHubURL) {
	Client* c;
	if(Util::strnicmp("adc://", aHubURL.c_str(), 6) == 0) {
		c = new AdcHub(aHubURL, false);
	} else if(Util::strnicmp("adcs://", aHubURL.c_str(), 7) == 0) {
		c = new AdcHub(aHubURL, true);
	} else {
		c = new NmdcHub(aHubURL);
	}

	{
		Lock l(cs);
		clients.push_back(c);
	}

	c->addListener(this);
	return c;
}

void ClientManager::putClient(Client* aClient) {
	aClient->disconnect(true);
	fire(ClientManagerListener::ClientDisconnected(), aClient);
	aClient->removeListeners();

	{
		Lock l(cs);

		// Either I'm stupid or the msvc7 optimizer is doing something _very_ strange here...
		// STL-port -D_STL_DEBUG complains that .begin() and .end() don't have the same owner (!)
		//		dcassert(find(clients.begin(), clients.end(), aClient) != clients.end());
		//		clients.erase(find(clients.begin(), clients.end(), aClient));
		
		for(Client::List::iterator i = clients.begin(); i != clients.end(); ++i) {
			if(*i == aClient) {
				clients.erase(i);
				break;
			}
		}
	}
	delete aClient;
}

size_t ClientManager::getUserCount() {
	Lock l(cs);
	return onlineUsers.size();
}

StringList ClientManager::getHubs(const CID& cid) {
	Lock l(cs);
	StringList lst;
	OnlinePair op = onlineUsers.equal_range(cid);
	for(OnlineIter i = op.first; i != op.second; ++i) {
		lst.push_back(i->second->getClient().getHubUrl());
	}
	return lst;
}

StringList ClientManager::getHubNames(const CID& cid) {
	Lock l(cs);
	StringList lst;
	OnlinePair op = onlineUsers.equal_range(cid);
	for(OnlineIter i = op.first; i != op.second; ++i) {
		lst.push_back(i->second->getClient().getHubName());
	}
	return lst;
}

StringList ClientManager::getNicks(const CID& cid) {
	Lock l(cs);
	StringList lst;
	OnlinePair op = onlineUsers.equal_range(cid);
	for(OnlineIter i = op.first; i != op.second; ++i) {
		lst.push_back(i->second->getIdentity().getNick());
	}
	if(lst.empty()) {
		// Offline perhaps?
		UserIter i = users.find(cid);
		if(i != users.end())
			lst.push_back(i->second->getFirstNick());
	}
	return lst;
}

string ClientManager::getConnection(const CID& cid) {
	Lock l(cs);
	OnlineIter i = onlineUsers.find(cid);
	if(i != onlineUsers.end()) {
		return i->second->getIdentity().getConnection();
	}
	return STRING(OFFLINE);
}

int64_t ClientManager::getAvailable() {
	Lock l(cs);
	int64_t bytes = 0;
	for(OnlineIter i = onlineUsers.begin(); i != onlineUsers.end(); ++i) {
		bytes += i->second->getIdentity().getBytesShared();
	}

	return bytes;
}

bool ClientManager::isConnected(const string& aUrl) {
	Lock l(cs);

	for(Client::Iter i = clients.begin(); i != clients.end(); ++i) {
		if((*i)->getHubUrl() == aUrl) {
			return true;
		}
	}
	return false;
}

string ClientManager::findHub(const string& ipPort) {
	Lock l(cs);

	string ip;
	short port = 411;
	string::size_type i = ipPort.find(':');
	if(i == string::npos) {
		ip = ipPort;
	} else {
		ip = ipPort.substr(0, i);
		port = (short)Util::toInt(ipPort.substr(i+1));
	}

	for(Client::Iter i = clients.begin(); i != clients.end(); ++i) {
		Client* c = *i;
		if(c->getPort() == port && c->getIp() == ip)
			return c->getHubUrl();
	}
	return Util::emptyString;
}

User::Ptr ClientManager::getLegacyUser(const string& aNick) const throw() {
	Lock l(cs);
	dcassert(aNick.size() > 0);

	for(OnlineMap::const_iterator i = onlineUsers.begin(); i != onlineUsers.end(); ++i) {
		const OnlineUser* ou = i->second;
		if(ou->getUser()->isSet(User::NMDC) && Util::stricmp(ou->getIdentity().getNick(), aNick) == 0)
			return ou->getUser();
	}
	return User::Ptr();
}

User::Ptr ClientManager::getUser(const string& aNick, const string& aHubUrl) throw() {
	CID cid = makeCid(aNick, aHubUrl);
	Lock l(cs);

	UserIter ui = users.find(cid);
	if(ui != users.end()) {
		if(ui->second->getFirstNick().empty())		// Could happen on bad queue loads etc...
			ui->second->setFirstNick(aNick);	
		ui->second->setFlag(User::NMDC);
		return ui->second;
	}

	User::Ptr p(new User(aNick));

	p->setCID(cid);
	users.insert(make_pair(cid, p));

	return p;
}

User::Ptr ClientManager::getUser(const CID& cid) throw() {
	Lock l(cs);
	UserIter ui = users.find(cid);
	if(ui != users.end()) {
		return ui->second;
	}

	User::Ptr p(new User(cid));
	users.insert(make_pair(cid, p));
	return p;
}

User::Ptr ClientManager::findUser(const CID& cid) throw() {
	Lock l(cs);
	UserIter ui = users.find(cid);
	if(ui != users.end()) {
		return ui->second;
	}
	return NULL;
}

bool ClientManager::isOp(const User::Ptr& user, const string& aHubUrl) {
	Lock l(cs);
	pair<OnlineIter, OnlineIter> p = onlineUsers.equal_range(user->getCID());
	for(OnlineIter i = p.first; i != p.second; ++i) {
		if(i->second->getClient().getHubUrl() == aHubUrl) {
			return i->second->getClient().getMyIdentity().isOp();
		}
	}
	return false;
}

CID ClientManager::makeCid(const string& aNick, const string& aHubUrl) throw() {
	string n = Text::toLower(aNick);
	TigerHash th;
	th.update(n.c_str(), n.length());
	th.update(Text::toLower(aHubUrl).c_str(), aHubUrl.length());
	// Construct hybrid CID from the first 64 bits of the tiger hash - should be
	// fairly random, and hopefully low-collision
	return CID(th.finalize());
}

void ClientManager::putOnline(OnlineUser& ou) throw() {
	{
		Lock l(cs);
		dcassert(!ou.getUser()->getCID().isZero());
		onlineUsers.insert(make_pair(ou.getUser()->getCID(), &ou));
	}

	if(!ou.getUser()->isOnline()) {
		ou.getUser()->setFlag(User::ONLINE);
		fire(ClientManagerListener::UserConnected(), ou.getUser());
	}
}

void ClientManager::putOffline(OnlineUser& ou) throw() {
	bool lastUser = false;
	{
		Lock l(cs);
		pair<OnlineMap::iterator, OnlineMap::iterator> op = onlineUsers.equal_range(ou.getUser()->getCID());
		dcassert(op.first != op.second);
		for(OnlineMap::iterator i = op.first; i != op.second; ++i) {
			OnlineUser* ou2 = i->second;
			/// @todo something nicer to compare with...
			if(&ou.getClient() == &ou2->getClient()) {
				lastUser = (distance(op.first, op.second) == 1);
				onlineUsers.erase(i);
				break;
			}
		}
	}

	if(lastUser) {
		ou.getUser()->unsetFlag(User::ONLINE);
		fire(ClientManagerListener::UserDisconnected(), ou.getUser());
	}
}

void ClientManager::connect(const User::Ptr& p) {
	Lock l(cs);
	OnlineIter i = onlineUsers.find(p->getCID());
	if(i != onlineUsers.end()) {
		OnlineUser* u = i->second;
		u->getClient().connect(*u);
	}
}

void ClientManager::privateMessage(const User::Ptr& p, const string& msg) {
	Lock l(cs);
	OnlineIter i = onlineUsers.find(p->getCID());
	if(i != onlineUsers.end()) {
		OnlineUser* u = i->second;
		u->getClient().privateMessage(*u, msg);
	}
}

void ClientManager::send(AdcCommand& cmd, const CID& cid) {
	Lock l(cs);
	OnlineIter i = onlineUsers.find(cid);
	if(i != onlineUsers.end()) {
		OnlineUser* u = i->second;
		if(cmd.getType() == AdcCommand::TYPE_UDP && !u->getIdentity().isUdpActive()) {
			cmd.setType(AdcCommand::TYPE_DIRECT);
		}
		cmd.setTo(u->getSID());
		u->getClient().send(cmd);
	}
}

void ClientManager::infoUpdated(bool antispam) {
	if(GET_TICK() > (quickTick + 10000) || antispam == false) {
		Lock l(cs);
		for(Client::Iter i = clients.begin(); i != clients.end(); ++i) {
			if((*i)->isConnected()) {
				(*i)->info();
			}
		}
		quickTick = GET_TICK();
	}
}

void ClientManager::on(NmdcSearch, Client* aClient, const string& aSeeker, int aSearchType, int64_t aSize, 
									int aFileType, const string& aString, bool isPassive) throw() 
{
	Speaker<ClientManagerListener>::fire(ClientManagerListener::IncomingSearch(), aSeeker, aString);

	// We don't wan't to answer passive searches if we're in passive mode...
	if(isPassive && !ClientManager::getInstance()->isActive(aClient->getHubUrl())) {
		return;
	}

	SearchResult::List l;
	ShareManager::getInstance()->search(l, aString, aSearchType, aSize, aFileType, aClient, isPassive ? 5 : 10);
	if(l.size() > 0) {
		if(isPassive) {
			string name = aSeeker.substr(4);
			// Good, we have a passive seeker, those are easier...
			string str;
			for(SearchResult::Iter i = l.begin(); i != l.end(); ++i) {
				SearchResult* sr = *i;
				str += sr->toSR(*aClient);
				str[str.length()-1] = 5;
				str += name;
				str += '|';

				sr->decRef();
			}
			
			if(str.size() > 0)
				aClient->send(str);
			
		} else {
			try {
				string ip, file;
				u_int16_t port = 0;
				Util::decodeUrl(aSeeker, ip, port, file);
				ip = Socket::resolve(ip);
				if(port == 0) port = 412;
				for(SearchResult::Iter i = l.begin(); i != l.end(); ++i) {
					SearchResult* sr = *i;
					s.writeTo(ip, port, sr->toSR(*aClient));
					sr->decRef();
				}
			} catch(const SocketException& /* e */) {
				dcdebug("Search caught error\n");
			}
		}
	} else if(!isPassive && (aFileType == SearchManager::TYPE_TTH) && (aString.compare(0, 4, "TTH:") == 0)/* && aClient->getMe()->isClientOp()*/) {
		PartsInfo partialInfo;
		TTHValue aTTH(aString.substr(4));
		if(!QueueManager::getInstance()->handlePartialSearch(aTTH, partialInfo)) {
			// if not found, try to find in finished list
			if(!FinishedManager::getInstance()->handlePartialRequest(aTTH, partialInfo)){
				return;
			}
		}

		try {
			char buf[1024];
			// $PSR user myUdpPort hubIpPort TTH partialCount partialInfo
			string hubIpPort = aClient->getIpPort();
			string tth = aTTH.toBase32();
			string user = Text::utf8ToAcp(aClient->getMyNick());
			_snprintf(buf, 1023, "$PSR %s$%d$%s$%s$%d$%s$|", user.c_str(), SETTING(UDP_PORT), hubIpPort.c_str(), tth.c_str(), partialInfo.size() / 2, GetPartsString(partialInfo).c_str());
			buf[1023] = NULL;
			string ip, file;
			u_int16_t port = 0;
			Util::decodeUrl(aSeeker, ip, port, file);
			ip = Socket::resolve(ip);
			if(port == 0) port = 412;
			s.writeTo(ip, port, buf);
		} catch(const SocketException&) {
			dcdebug("Search caught error\n");
		}
	}
}

void ClientManager::userCommand(const User::Ptr& p, const ::UserCommand& uc, StringMap& params, bool compatibility) {
	OnlineIter i = onlineUsers.find(p->getCID());
	if(i == onlineUsers.end())
		return;

	OnlineUser& ou = *i->second;
	ou.getIdentity().getParams(params, "user", compatibility);
	ou.getClient().getHubIdentity().getParams(params, "hub", false);
	ou.getClient().getMyIdentity().getParams(params, "my", compatibility);
	ou.getClient().escapeParams(params);
	ou.getClient().sendUserCmd(Util::formatParams(uc.getCommand(), params));
}

void ClientManager::on(AdcSearch, Client*, const AdcCommand& adc, const CID& from) throw() {
	SearchManager::getInstance()->respond(adc, from);
}

Identity ClientManager::getIdentity(const User::Ptr& aUser) {
	Lock l(cs);
	OnlineIter i = onlineUsers.find(aUser->getCID());
	if(i != onlineUsers.end()) {
		return i->second->getIdentity();
	}
	return Identity(aUser, Util::emptyString);
}

void ClientManager::search(int aSizeMode, int64_t aSize, int aFileType, const string& aString, const string& aToken) {
	Lock l(cs);

	updateCachedIp(); // no point in doing a resolve for every single hub we're searching on

	for(Client::Iter i = clients.begin(); i != clients.end(); ++i) {
		if((*i)->isConnected()) {
			(*i)->search(aSizeMode, aSize, aFileType, aString, aToken);
		}
	}
}

void ClientManager::search(StringList& who, int aSizeMode, int64_t aSize, int aFileType, const string& aString, const string& aToken) {
	Lock l(cs);

	updateCachedIp(); // no point in doing a resolve for every single hub we're searching on

	for(StringIter it = who.begin(); it != who.end(); ++it) {
		string& client = *it;
		for(Client::Iter j = clients.begin(); j != clients.end(); ++j) {
			Client* c = *j;
			if(c->isConnected() && c->getIpPort() == client) {
				c->search(aSizeMode, aSize, aFileType, aString, aToken);
			}
		}
	}
}

void ClientManager::on(TimerManagerListener::Minute, u_int32_t /* aTick */) throw() {
	{
		Lock l(cs);

		// Collect some garbage...
		UserIter i = users.begin();
		while(i != users.end()) {
			if(i->second->unique()) {
				users.erase(i++);
			} else {
				++i;
			}
		}

		for(Client::Iter j = clients.begin(); j != clients.end(); ++j) {
			(*j)->info();
		}
	}
	// TODO SetProcessWorkingSetSize
	SetProcessWorkingSetSize(GetCurrentProcess(), 0xffffffff, 0xffffffff);
}

void ClientManager::on(Save, SimpleXML*) throw() {
	Lock l(cs);

	try {
		string tmp;

		File ff(getUsersFile() + ".tmp", File::WRITE, File::CREATE | File::TRUNCATE);
		BufferedOutputStream<false> f(&ff);

		f.write(SimpleXML::utf8Header);
		f.write(LIT("<Users Version=\"1\">\r\n"));
		for(UserIter i = users.begin(); i != users.end(); ++i) {
			User::Ptr& p = i->second;
			if(p->isSet(User::SAVE_NICK) && !p->getCID().isZero() && !p->getFirstNick().empty()) {
				f.write(LIT("\t<User CID=\""));
				f.write(p->getCID().toBase32());
				f.write(LIT("\" Nick=\""));
				f.write(SimpleXML::escape(p->getFirstNick(), tmp, true));
				f.write(LIT("\"/>\r\n"));
			}
		}

		f.write("</Users>\r\n");
		f.flush();
		ff.close();
		File::deleteFile(getUsersFile());
		File::renameFile(getUsersFile() + ".tmp", getUsersFile());

	} catch(const FileException&) {
		// ...
	}
}

User::Ptr& ClientManager::getMe() {
	if(!me) {
		Lock l(cs);
		if(!me) {
			me = new User(getMyCID());
			me->setFirstNick(SETTING(NICK));
		}
	}
	return me;
}

const CID& ClientManager::getMyPID() {
	if(pid.isZero())
		pid = CID(SETTING(PRIVATE_ID));
	return pid;
}

CID ClientManager::getMyCID() {
	TigerHash tiger;
	tiger.update(pid.getData(), CID::SIZE);
	return CID(tiger.finalize());
}

void ClientManager::on(Load, SimpleXML*) throw() {
	users.insert(make_pair(getMe()->getCID(), getMe()));

	try {
		SimpleXML xml;
		xml.fromXML(File(getUsersFile(), File::READ, File::OPEN).read());
		if(xml.findChild("Users") && xml.getChildAttrib("Version") == "1") {
			xml.stepIn();
			while(xml.findChild("User")) {
				const string& cid = xml.getChildAttrib("CID");
				const string& nick = xml.getChildAttrib("Nick");
                if(cid.length() != 39 || nick.empty())
					continue;
				User::Ptr p(new User(CID(cid)));
				p->setFirstNick(xml.getChildData());
				users.insert(make_pair(p->getCID(), p));
			}
		}
	} catch(const Exception& e) {
		dcdebug("Error loading Users.xml: %s\n", e.getError().c_str());
	}
}

void ClientManager::on(Failed, Client* client, const string&) throw() { 
	FavoriteManager::getInstance()->removeUserCommand(client->getHubUrl());
	fire(ClientManagerListener::ClientDisconnected(), client);
}

void ClientManager::on(UserCommand, Client* client, int aType, int ctx, const string& name, const string& command) throw() { 
	if(BOOLSETTING(HUB_USER_COMMANDS)) {
		if(aType == ::UserCommand::TYPE_CLEAR) {
 			FavoriteManager::getInstance()->removeHubUserCommands(ctx, client->getHubUrl());
 		} else {
			FavoriteManager::getInstance()->addUserCommand(aType, ctx, ::UserCommand::FLAG_NOSAVE, name, command, client->getHubUrl());
		}
	}
}

void ClientManager::updateCachedIp() {
	// Best case - the server detected it
	if((!BOOLSETTING(NO_IP_OVERRIDE) || SETTING(EXTERNAL_IP).empty())) {
		for(Client::Iter i = clients.begin(); i != clients.end(); ++i) {
			if(!(*i)->getMyIdentity().getIp().empty()) {
				cachedIp = (*i)->getMyIdentity().getIp();
				return;
			}
		}
	}

	if(!SETTING(EXTERNAL_IP).empty()) {
		cachedIp = Socket::resolve(SETTING(EXTERNAL_IP));
		return;
	}

	//if we've come this far just use the first client to get the ip.
	if(clients.size() > 0)
		cachedIp = (*clients.begin())->getLocalIp();
}

void ClientManager::setListLength(const User::Ptr& p, const string& listLen) {
	Lock l(cs);
	OnlineIter i = onlineUsers.find(p->getCID());
	if(i != onlineUsers.end()) {
		i->second->getIdentity().setListLength(listLen);
	}
}

void ClientManager::setPkLock(const User::Ptr& p, const string& aPk, const string& aLock) {
	OnlineUser* ou;
	{
		Lock l(cs);
		OnlineIter i = onlineUsers.find(p->getCID());
		if(i == onlineUsers.end()) return;

		ou = i->second;
		ou->getIdentity().setPk(aPk);
		ou->getIdentity().setLock(aLock);
	}
	ou->getClient().updated(*ou);
}

bool ClientManager::fileListDisconnected(const User::Ptr& p) {
	string report = Util::emptyString;
	Client* c = NULL;
	{
		Lock l(cs);
		OnlineIter i = onlineUsers.find(p->getCID());
		if(i != onlineUsers.end()) {
			OnlineUser& ou = *i->second;
	
			int fileListDisconnects = Util::toInt(ou.getIdentity().getFileListDisconnects()) + 1;
			ou.getIdentity().setFileListDisconnects(Util::toString(fileListDisconnects));

			if(SETTING(ACCEPTED_DISCONNECTS) == 0)
				return false;

			if(fileListDisconnects == SETTING(ACCEPTED_DISCONNECTS)) {
				c = &ou.getClient();
				report = ou.getIdentity().setCheat(ou.getClient(), "Disconnected file list " + Util::toString(fileListDisconnects) + " times", false);
				ou.getIdentity().sendRawCommand(ou.getClient(), SETTING(DISCONNECT_RAW));
			}
		}
	}
	if(c && !report.empty()) {
		c->cheatMessage(report);
		return true;
	}
	return false;
}

bool ClientManager::connectionTimeout(const User::Ptr& p) {
	string report = Util::emptyString;
	Client* c = NULL;
	{
		Lock l(cs);
		OnlineIter i = onlineUsers.find(p->getCID());
		if(i != onlineUsers.end()) {
			OnlineUser& ou = *i->second;
	
			int connectionTimeouts = Util::toInt(ou.getIdentity().getConnectionTimeouts()) + 1;
			ou.getIdentity().setConnectionTimeouts(Util::toString(connectionTimeouts));
	
			if(SETTING(ACCEPTED_TIMEOUTS) == 0)
				return false;
	
			if(connectionTimeouts == SETTING(ACCEPTED_TIMEOUTS)) {
				c = &ou.getClient();
				report = ou.getIdentity().setCheat(ou.getClient(), "Connection timeout " + Util::toString(connectionTimeouts) + " times", false);
				try {
					QueueManager::getInstance()->removeTestSUR(p);
				} catch(...) {
				}
				ou.getIdentity().sendRawCommand(ou.getClient(), SETTING(TIMEOUT_RAW));
			}
		}
	}
	if(c && !report.empty()) {
		c->cheatMessage(report);
		return true;
	}
	return false;
}

void ClientManager::checkCheating(const User::Ptr& p, DirectoryListing* dl) {
	string report = Util::emptyString;
	OnlineUser* ou = NULL;
	{
		Lock l(cs);

		OnlineIter i = onlineUsers.find(p->getCID());
		if(i == onlineUsers.end())
			return;

		ou = i->second;

		int64_t statedSize = ou->getIdentity().getBytesShared();
		int64_t realSize = dl->getTotalSize();
	
		double multiplier = ((100+(double)SETTING(PERCENT_FAKE_SHARE_TOLERATED))/100); 
		int64_t sizeTolerated = (int64_t)(realSize*multiplier);
		string detectString = Util::emptyString;
		string inflationString = Util::emptyString;
		ou->getIdentity().setRealBytesShared(Util::toString(realSize));
		bool isFakeSharing = false;
	
		if(statedSize > sizeTolerated) {
			isFakeSharing = true;
		}

		if(isFakeSharing) {
			ou->getIdentity().setBadFilelist("1");
			detectString += STRING(CHECK_MISMATCHED_SHARE_SIZE);
			if(realSize == 0) {
				detectString += STRING(CHECK_0BYTE_SHARE);
			} else {
				double qwe = (double)((double)statedSize / (double)realSize);
				char buf[128];
				_snprintf(buf, 127, CSTRING(CHECK_INFLATED), Util::toString(qwe).c_str());
				buf[127] = 0;
				inflationString = buf;
				detectString += inflationString;
			}
			detectString += STRING(CHECK_SHOW_REAL_SHARE);

			report = ou->getIdentity().setCheat(ou->getClient(), Util::validateMessage(detectString, false), false);
			ou->getIdentity().sendRawCommand(ou->getClient(), SETTING(FAKESHARE_RAW));
		}
		ou->getIdentity().setFilelistComplete("1");
	}
	ou->getClient().updated(*ou);
	if(!report.empty())
		ou->getClient().cheatMessage(report);
}

void ClientManager::setCheating(const User::Ptr& p, const string& aTestSURString, const string& aCheatString, const int aRawCommand, bool aBadClient) {
	OnlineUser* ou = NULL;
	string report = Util::emptyString;
	{
		Lock l(cs);
		OnlineIter i = onlineUsers.find(p->getCID());
		if(i == onlineUsers.end()) return;
		
		ou = i->second;
		
		if(!aTestSURString.empty()) {
			ou->getIdentity().setTestSUR(aTestSURString);
			ou->getIdentity().setTestSURComplete("1");
			report = ou->getIdentity().updateClientType(*ou);
		}
		if(!aCheatString.empty()) {
			report = ou->getIdentity().setCheat(ou->getClient(), aCheatString, aBadClient);
		}
		if(aRawCommand != -1)
			ou->getIdentity().sendRawCommand(ou->getClient(), aRawCommand);
	}
	ou->getClient().updated(*ou);
	if(!report.empty())
		ou->getClient().cheatMessage(report);
}

int ClientManager::getMode(const string& aHubUrl) {
	if(aHubUrl.empty()) return SETTING(INCOMING_CONNECTIONS);

	int mode = 0;
	FavoriteHubEntry* hub = FavoriteManager::getInstance()->getFavoriteHubEntry(aHubUrl);
	if(hub) {
		switch(hub->getMode()) {
			case 1 :
				mode = SettingsManager::INCOMING_DIRECT;
				break;
			case 2 :
				mode = SettingsManager::INCOMING_FIREWALL_PASSIVE;
				break;
			default:
				mode = SETTING(INCOMING_CONNECTIONS);
		}
	} else {
		mode = SETTING(INCOMING_CONNECTIONS);
	}
	return mode;
}

/**
 * @file
 * $Id$
 */
