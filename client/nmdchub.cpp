/* 
 * Copyright (C) 2001-2003 Jacek Sieka, j_s@telia.com
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

#include "NmdcHub.h"

#include "ResourceManager.h"
#include "ClientManager.h"
#include "SearchManager.h"
#include "ShareManager.h"
#include "CryptoManager.h"
#include "ConnectionManager.h"

#include "Socket.h"
#include "UserCommand.h"
#include "StringTokenizer.h"
#include "DebugManager.h"

NmdcHub::NmdcHub(const string& aHubURL) : Client(aHubURL, '|'), supportFlags(0),  
	state(STATE_CONNECT), adapter(this),
	lastActivity(GET_TICK()), 
	reconnect(true), lastUpdate(0)
{
	TimerManager::getInstance()->addListener(this);
	dscrptn = (char *) calloc(96, sizeof(char));
	dscrptn[0] = NULL;
	dscrptnlen = 95;
};

NmdcHub::~NmdcHub() throw() {
	TimerManager::getInstance()->removeListener(this);
	Speaker<NmdcHubListener>::removeListeners();
	socket->removeListener(this);

	clearUsers();
	delete[] dscrptn;
};

void NmdcHub::connect() {
	setRegistered(false);
	reconnect = true;
	supportFlags = 0;
	lastmyinfo.clear();
	lastbytesshared = 0;
	validatenicksent = false;

	if(socket->isConnected()) {
		disconnect();
	}

	state = STATE_LOCK;

	if(getPort() == 0) {
		setPort(411);
	}
	socket->connect(getAddress(), getPort());
}

void NmdcHub::refreshUserList(bool unknownOnly /* = false */) {
	Lock l(cs);
	if(unknownOnly) {
		for(User::NickIter i = users.begin(); i != users.end(); ++i) {
			if(i->second->getConnection().empty()) {
				getInfo(i->second);
			}
		}
	} else {
		clearUsers();
		getNickList();
	}
}

void NmdcHub::clearUsers() {
	Lock l(cs);
	for(User::NickIter i = users.begin(); i != users.end(); ++i) {
		ClientManager::getInstance()->putUserOffline(i->second);		
	}
	users.clear();
}

void NmdcHub::onLine(const char *aLine) throw() {
	lastActivity = GET_TICK();
	
//	logovani vsech prichozich dat... posila se do chatu :-)
//	Speaker<NmdcHubListener>::fire(NmdcHubListener::Message(), this, aLine);

	if(aLine[0] != '$') {
		// Check if we're being banned...
		if(state != STATE_CONNECTED) {
			if(strstr(aLine, "banned") != 0) {
				reconnect = false;
			}
		}
		Speaker<NmdcHubListener>::fire(NmdcHubListener::Message(), this, Util::validateMessage(aLine, true));
		return;
	}

	if((temp = strtok((char*) aLine, " ")) == NULL) {
		cmd = (char*) aLine; param = NULL;
	} else {
		cmd = temp;
		param = strtok(NULL, "\0");
	}

	if(strcmp(cmd, "$Search") == 0) {
		if(state != STATE_CONNECTED || param == NULL)
			return;

		if((temp = strtok(param, " ")) == NULL)
			return;

		char *seeker = temp;

		// Filter own searches
		if(SETTING(CONNECTION_TYPE) == SettingsManager::CONNECTION_ACTIVE) {
			if((strstr(seeker, getLocalIp().c_str())) != 0) {
				return;
			}
		} else {
			if(strstr(seeker, getNick().c_str()) != 0) {
				return;
			}
		}

		{
			Lock l(cs);
			u_int32_t tick = GET_TICK();

			seekers.push_back(make_pair(seeker, tick));

			// First, check if it's a flooder
			FloodIter fi;
			for(fi = flooders.begin(); fi != flooders.end(); ++fi) {
				if(fi->first == seeker)
					return;
			}

			int count = 0;
			for(fi = seekers.begin(); fi != seekers.end(); ++fi) {
				if(fi->first == seeker)
					count++;

				if(count > 7) {
					if( strnicmp(seeker, "Hub:", 4) == 0 ) {
						if(strlen(seeker) > 4)
							Speaker<NmdcHubListener>::fire(NmdcHubListener::SearchFlood(), this, strtok(seeker+4, "\0"));
					} else {
						Speaker<NmdcHubListener>::fire(NmdcHubListener::SearchFlood(), this, seeker + STRING(NICK_UNKNOWN));
					}

					flooders.push_back(make_pair(seeker, tick));
					return;
				}
			}
		}
		int a;
		if((temp = strtok(NULL, "?")) == NULL)
			return;

		if(strnicmp(temp, "F", 1) == 0 ) {
			a = SearchManager::SIZE_DONTCARE;
			if((temp = strtok(NULL, "?")) == NULL)
				return;
		} else {
			if((temp = strtok(NULL, "?")) == NULL)
				return;

			if(strnicmp(temp, "F", 1) == 0 ) {
				a = SearchManager::SIZE_ATLEAST;
			} else {
				a = SearchManager::SIZE_ATMOST;
			}
		}

		if((temp = strtok(NULL, "?")) == NULL)
			return;

//		char *size = temp;
		int64_t size = (int64_t)temp;
		if((temp = strtok(NULL, "?")) == NULL)
			return;

		int type = Util::toInt(temp) - 1;
		if((temp = strtok(NULL, "\0")) != NULL) {
			Speaker<NmdcHubListener>::fire(NmdcHubListener::Search(), this, seeker, a, size, type, temp);
			if( strnicmp(seeker, "Hub:", 4) == 0 ) {
				User::Ptr u;
				if(strlen(seeker) > 4) {
					Lock l(cs);
						User::NickIter ni = users.find(strtok(seeker+4, "\0"));
						if(ni != users.end() && !ni->second->isSet(User::PASSIVE)) {
							u = ni->second;
							u->setFlag(User::PASSIVE);
					}
				}

				if(u) {
					updated(u);
				}
			}
		}
	} else if(strcmp(cmd, "$MyINFO") == 0) {
		if(param == NULL)
			return;

		if((temp = strtok(param+5, " ")) == NULL)
			return;

		User::Ptr u;
		dcassert(strlen(temp) > 0);

		{
			Lock l(cs);
			User::NickIter ni = users.find(temp);
			if(ni == users.end()) {
				u = users[temp] = ClientManager::getInstance()->getUser(temp, this);
			} else {
				u  = ni->second;
			}
		}
		// New MyINFO received, delete all user variables... if is bad MyINFO is user problem not my ;-)
		u->setBytesShared(0);
		u->setDescription(Util::emptyString);
		u->setStatus(1);
		u->setConnection(Util::emptyString);
		u->setEmail(Util::emptyString);
		u->setTag(Util::emptyString);
		u->setClientType(Util::emptyString);
		u->setVersion(Util::emptyString);
		u->setMode(Util::emptyString);
		u->setHubs(Util::emptyString);
		u->setSlots(Util::emptyString);
		u->setUpload(Util::emptyString);

		if(temp[strlen(temp)+1] != '$') {
			if((temp = strtok(NULL, "$")) != NULL) {
				paramlen = strlen(temp);
				if(paramlen > dscrptnlen) {
					dscrptn = (char *) realloc(dscrptn, paramlen+1);
					dscrptnlen = paramlen;
				}
				strcpy(dscrptn, temp);
			}
		}

		if((temp = strtok(NULL, "$")) != NULL) {
			if(temp[strlen(temp)+1] != '$') {
				if((temp = strtok(NULL, "$")) != NULL) {
					char status;
					status = temp[strlen(temp)-1];
					u->setStatus(status);
					temp[strlen(temp)-1] = '\0';
					u->setConnection(temp);
				}
			} else {
				u->setStatus(1);
				u->setConnection(Util::emptyString);
			}
		}

		if(temp != NULL) {
			if(temp[strlen(temp)+2] != '$') {
				if((temp = strtok(NULL, "$")) != NULL)
					u->setEmail(Util::validateMessage(temp, true));
			} else {
				u->setEmail(Util::emptyString);
			}
		}

		if(temp != NULL) {
			if(temp[strlen(temp)+1] != '$') {
				if((temp = strtok(NULL, "$")) != NULL)
					u->setBytesShared(temp);
			}
		}

		if(dscrptn != NULL) {
			if(strlen(dscrptn) > 0 && dscrptn[strlen(dscrptn)-1] == '>') {
				char *sTag;
				if((sTag = strrchr(dscrptn, '<')) != NULL) {
					u->TagParts(sTag);
					dscrptn[strlen(dscrptn)-strlen(sTag)] = '\0';
				}
			}
			u->setDescription(dscrptn);
			dscrptn[0] = NULL;
		}

		if (u->getNick() == getNick()) {
			if(state == STATE_MYINFO) {
				state = STATE_CONNECTED;
				updateCounts(false);
			}	

			u->setFlag(User::DCPLUSPLUS);
			if(SETTING(CONNECTION_TYPE) != SettingsManager::CONNECTION_ACTIVE)
				u->setFlag(User::PASSIVE);
		}
		Speaker<NmdcHubListener>::fire(NmdcHubListener::MyInfo(), this, u);
	} else if(strcmp(cmd, "$Quit") == 0) {
		if(param == NULL)
			return;

		User::Ptr u;
		{
			Lock l(cs);
			User::NickIter i = users.find(param);
			if(i == users.end()) {
				dcdebug("C::onLine Quitting user %s not found\n", param);
				return;
			}
			
			u = i->second;
			users.erase(i);
		}
		
		Speaker<NmdcHubListener>::fire(NmdcHubListener::Quit(), this, u);
		ClientManager::getInstance()->putUserOffline(u, true);
	} else if(strcmp(cmd, "$ConnectToMe") == 0) {
		if(state != STATE_CONNECTED || param == NULL)
			return;

		if((temp = strtok(param, " ")) == NULL)
			return;

		if(strcmp(temp, getNick().c_str()) != 0) // Check nick... is CTM really for me ? ;o)
			return;

		if((temp = strtok(NULL, ":")) == NULL)
			return;

		char *server = temp;
		if((temp = strtok(NULL, " ")) == NULL)
			return;

		ConnectionManager::getInstance()->connect(server, (short)Util::toInt(temp), getNick()); 
		Speaker<NmdcHubListener>::fire(NmdcHubListener::ConnectToMe(), this, server, (short)Util::toInt(temp));
	} else if(strcmp(cmd, "$RevConnectToMe") == 0) {
		if(state != STATE_CONNECTED || param == NULL) {
				return;
		}
		User::Ptr u;
		bool up = false;
		{
			Lock l(cs);
			if((temp = strtok(param, " ")) == NULL)
				return;

			User::NickIter i = users.find(temp);
			if((temp = strtok(NULL, "\0")) == NULL)
				return;

			if(strcmp(temp, getNick().c_str()) != 0) // Check nick... is RCTM really for me ? ;-)
				return;

			if(i == users.end()) {
				return;
			}

			u = i->second;
			if(!u->isSet(User::PASSIVE)) {
				u->setFlag(User::PASSIVE);
				up = true;
			}
		}

		if(u) {
			if(SETTING(CONNECTION_TYPE) == SettingsManager::CONNECTION_ACTIVE) {
				connectToMe(u);
				Speaker<NmdcHubListener>::fire(NmdcHubListener::RevConnectToMe(), this, u);
			} else {
				// Notify the user that we're passive too...
				if(up)
					revConnectToMe(u);
			}

			if(up)
				updated(u);
		}
	} else if(strcmp(cmd, "$SR") == 0) {
		if(param != NULL) {
			SearchManager::getInstance()->onNMDCSearchResult(param);
		}
	} else if(strcmp(cmd, "$HubName") == 0) {
		if(param == NULL)
			return;

		name = param;
		Speaker<NmdcHubListener>::fire(NmdcHubListener::HubName(), this);
	} else if(strcmp(cmd, "$Supports") == 0) {
		if(param == NULL)
			return;

		bool QuickList = false;
		StringList sl;
		if((temp = strtok(param, " ")) == NULL)
			return;

		while(temp != NULL) {
			sl.push_back(temp);
			if((strcmp(temp, "UserCommand")) == 0) {
				supportFlags |= SUPPORTS_USERCOMMAND;
			} else if((strcmp(temp, "NoGetINFO")) == 0) {
				supportFlags |= SUPPORTS_NOGETINFO;
			} else if((strcmp(temp, "UserIP2")) == 0) {
				supportFlags |= SUPPORTS_USERIP2;
			} else if((strcmp(temp, "QuickList")) == 0) {
				if(state == STATE_HELLO) {
					state = STATE_MYINFO;
					updateCounts(false);
					myInfo();
					getNickList();
					QuickList = true;
				}
			}
			temp = strtok(NULL, " ");
		}
		if (!QuickList) {
			validateNick(getNick());
		}
		Speaker<NmdcHubListener>::fire(NmdcHubListener::Supports(), this, sl);
	} else if(strcmp(cmd, "$UserCommand") == 0) {
		if(param == NULL)
			return;

		if((temp = strtok(param, " ")) == NULL)
			return;

		int type = Util::toInt(temp);
		if((temp = strtok(NULL, " ")) == NULL)
			return;

		if(type == UserCommand::TYPE_SEPARATOR || type == UserCommand::TYPE_CLEAR) {
			int ctx = Util::toInt(temp);
			Speaker<NmdcHubListener>::fire(NmdcHubListener::UserCommand(), this, type, ctx, Util::emptyString, Util::emptyString);
		} else if(type == UserCommand::TYPE_RAW || type == UserCommand::TYPE_RAW_ONCE) {
			int ctx = Util::toInt(temp);
			if((temp = strtok(NULL, "$")) == NULL)
				return;

			char *name = temp;
			if((temp = strtok(NULL, "\0")) == NULL)
				return;

			Speaker<NmdcHubListener>::fire(NmdcHubListener::UserCommand(), this, type, ctx, Util::validateMessage(name, true, false), Util::validateMessage(temp, true, false));
		}
	} else if(strcmp(cmd, "$Lock") == 0) {
		if(state != STATE_LOCK || param == NULL)
			return;

		state = STATE_HELLO;
		char *lock;
		if((temp = strstr(param, " Pk=")) != NULL) { // simply the best ;-)
			param[strlen(param)-strlen(temp)] = 0;
			lock = param;
		} else
			lock = param; // Pk is not usefull, don't waste cpu time to get it ;o)

		if(CryptoManager::getInstance()->isExtended(lock)) {
			StringList feat;
			feat.push_back("UserCommand");
			feat.push_back("NoGetINFO");
			feat.push_back("NoHello");
			feat.push_back("UserIP2");
			feat.push_back("TTHSearch");
			feat.push_back("QuickList");

			if(BOOLSETTING(COMPRESS_TRANSFERS))
				feat.push_back("GetZBlock");
			supports(feat);
			key(CryptoManager::getInstance()->makeKey(lock));
		} else {
			key(CryptoManager::getInstance()->makeKey(lock));
			validateNick(getNick());
		}

		Speaker<NmdcHubListener>::fire(NmdcHubListener::CLock(), this, lock, Util::emptyString);
	} else if(strcmp(cmd, "$Hello") == 0) {
		if(param == NULL)
			return;

		User::Ptr u = ClientManager::getInstance()->getUser(param, this);
		{
			Lock l(cs);
			users[param] = u;
		}
		
		if(getNick() == param) {
			setMe(u);
		
			u->setFlag(User::DCPLUSPLUS);
			if(SETTING(CONNECTION_TYPE) != SettingsManager::CONNECTION_ACTIVE)
				u->setFlag(User::PASSIVE);
			else
				u->unsetFlag(User::PASSIVE);
		}

		if(state == STATE_HELLO) {
			state = STATE_CONNECTED;
			updateCounts(false);

		string Verze = "";		
		if ((!getStealth()) && (SETTING(CLIENT_EMULATION) != SettingsManager::CLIENT_DC)) {
			Verze = DCVERSIONSTRING;
		} else { Verze = "1,0091"; }


			version(Verze);
			myInfo();
			getNickList();
		}
		Speaker<NmdcHubListener>::fire(NmdcHubListener::Hello(), this, u);

	} else if(strcmp(cmd, "$ForceMove") == 0) {
		if(param == NULL)
			return;

		disconnect();
		Speaker<NmdcHubListener>::fire(NmdcHubListener::Redirect(), this, param);
	} else if(strcmp(cmd, "$HubIsFull") == 0) {
		Speaker<NmdcHubListener>::fire(NmdcHubListener::HubFull(), this);
	} else if(strcmp(cmd, "$ValidateDenide") == 0) {
		disconnect();
		Speaker<NmdcHubListener>::fire(NmdcHubListener::ValidateDenied(), this);
	} else if(strcmp(cmd, "$UserIP") == 0) {
		if(param == NULL)
			return;

		User::List v;
		StringList l;
		if((temp = strtok(param, "$$")) == NULL)
			return;

		while(temp != NULL) {
			l.push_back(temp);
			temp = strtok(NULL, "$$");
		}
		for(StringIter it = l.begin(); it != l.end(); ++it) {
			string::size_type j = 0;
			if((j = it->find(' ')) == string::npos)
				continue;
			if((j+1) == it->length())
				continue;
			string nick = it->substr(0, j);	
			v.push_back(ClientManager::getInstance()->getUser(nick, this));
			string IP = it->substr(j+1);
			v.back()->setIp(IP);
			ClientManager::getInstance()->setIPNick(IP, nick);

		}
		Speaker<NmdcHubListener>::fire(NmdcHubListener::UserIp(), this, v);
	} else if(strcmp(cmd, "$NickList") == 0) {
		if(param == NULL)
			return;

		User::List v;
		if((temp = strtok(param, "$$")) == NULL)
			return;

		while(temp != NULL) {
			v.push_back(ClientManager::getInstance()->getUser(temp, this));
			temp = strtok(NULL, "$$");
		}

		{
			Lock l(cs);
			for(User::Iter it2 = v.begin(); it2 != v.end(); ++it2) {
				users[(*it2)->getNick()] = *it2;
			}
		}
		
		if(!(getSupportFlags() & SUPPORTS_NOGETINFO)) {
			string tmp;
			// Let's assume 10 characters per nick...
			tmp.reserve(v.size() * (11 + 10 + getNick().length())); 
			for(User::List::const_iterator i = v.begin(); i != v.end(); ++i) {
				tmp += "$GetINFO ";
				tmp += (*i)->getNick();
				tmp += ' ';
				tmp += getNick(); 
				tmp += '|';
			}
			if(!tmp.empty()) {
				send(tmp);
			}
		}

		Speaker<NmdcHubListener>::fire(NmdcHubListener::NickList(), this, v);
	} else if(strcmp(cmd, "$OpList") == 0) {
		if(param == NULL)
			return;

		User::List v;
		if((temp = strtok(param, "$$")) == NULL)
			return;

		while(temp != NULL) {
			v.push_back(ClientManager::getInstance()->getUser(temp, this));
			v.back()->setFlag(User::OP);
			temp = strtok(NULL, "$$");
		}

		{
			Lock l(cs);
			for(User::Iter it2 = v.begin(); it2 != v.end(); ++it2) {
				users[(*it2)->getNick()] = *it2;
			}
		}
		Speaker<NmdcHubListener>::fire(NmdcHubListener::OpList(), this, v);
		updateCounts(false);
		myInfo();
	} else if(strcmp(cmd, "$To:") == 0) {
		if(param == NULL)
			return;

		char *temp1, *from;
		if((temp1 = strstr(param, "From:")) != NULL) {

			if((temp = strtok(temp1+6, "$")) == NULL)
				return;

			from = temp;
			from[strlen(from)-1] = '\0';
			if((temp = strtok(NULL, "\0")) == NULL)
				return;

			Speaker<NmdcHubListener>::fire(NmdcHubListener::PrivateMessage(), this, ClientManager::getInstance()->getUser(from, this, false), Util::validateMessage(temp, true));
		}
	} else if(strcmp(cmd, "$GetPass") == 0) {
		setRegistered(true);
		Speaker<NmdcHubListener>::fire(NmdcHubListener::GetPassword(), this);
	} else if(strcmp(cmd, "$BadPass") == 0) {
		Speaker<NmdcHubListener>::fire(NmdcHubListener::BadPassword(), this);
	} else if(strcmp(cmd, "$LogedIn") == 0) {
		Speaker<NmdcHubListener>::fire(NmdcHubListener::LoggedIn(), this);
	} else {
		dcassert(cmd[0] == '$');
		if(param != NULL) {
			strcat(cmd, " ");
			strcat(cmd, param);
		}
		dcdebug("NmdcHub::onLine Unknown command %s\n", cmd);
	}
}

void NmdcHub::myInfo() {
	if(state != STATE_CONNECTED && state != STATE_MYINFO) {
		return;
	}
	
	dcdebug("MyInfo %s...\n", getNick().c_str());
	lastCounts = counts;
	char StatusMode = '\x01';
	string tmp0 = "<++";
	string tmp1 = "\x1fU9";
	string tmp2 = "+L9";
	string tmp3 = "+G9";
	string tmp4 = "+R9";
	string tmp5 = "+K9";
	string::size_type i;
	
	for(i = 0; i < 6; i++) {
		tmp1[i]++;
	}
	for(i = 0; i < 3; i++) {
		tmp2[i]++; tmp3[i]++; tmp4[i]++; tmp5[i]++;
	}
	char modeChar = '?';
	if(SETTING(CONNECTION_TYPE) == SettingsManager::CONNECTION_ACTIVE)
		modeChar = 'A';
	else if(SETTING(CONNECTION_TYPE) == SettingsManager::CONNECTION_PASSIVE)
		modeChar = 'P';
	else if(SETTING(CONNECTION_TYPE) == SettingsManager::CONNECTION_SOCKS5)
		modeChar = '5';
	
	string VERZE = DCVERSIONSTRING;
	


	if ((getStealth() == false) && (SETTING(CLIENT_EMULATION) != SettingsManager::CLIENT_DC) && (SETTING(CLIENT_EMULATION) != SettingsManager::CLIENT_CZDC)) {
		tmp0 = "<StrgDC++";
		VERZE = VERSIONSTRING CZDCVERSIONSTRING;
	}
	string extendedtag = tmp0 + tmp1 + VERZE + tmp2 + modeChar + tmp3 + getCounts() + tmp4 + Util::toString(UploadManager::getInstance()->getSlots());

	string connection = SETTING(CONNECTION);
	string speedDescription = "";

	if((getStealth() == false) && (SETTING(CLIENT_EMULATION) != SettingsManager::CLIENT_DC)) {
		if (SETTING(THROTTLE_ENABLE) && SETTING(MAX_UPLOAD_SPEED_LIMIT) != 0) {
			int tag = 0;
			tag = SETTING(MAX_UPLOAD_SPEED_LIMIT);
			extendedtag += tmp5 + Util::toString(tag);
		}

		if (UploadManager::getFireballStatus()) {
			StatusMode += 8;
		} else if (UploadManager::getFileServerStatus()) {
			StatusMode += 4;
		}

		if(Util::getAway()) {
			StatusMode += 2;
		}
	} else {
		if (connection == "Modem") { connection = "56Kbps"; }
		if (connection == "Wireless") { connection = "Satellite"; }

		if (SETTING(THROTTLE_ENABLE) && SETTING(MAX_UPLOAD_SPEED_LIMIT) != 0) {
			speedDescription = Util::toString(SETTING(MAX_UPLOAD_SPEED_LIMIT)*8) + "kbps ";
		}

	}

	extendedtag += ">";
	
	string nldetect =
	(FindWindow(NULL, "NetLimiter v1.30") || FindWindow(NULL, "NetLimiter v1.29") || FindWindow(NULL, "NetLimiter v1.25") || FindWindow(NULL, "NetLimiter v1.22"))
	? "NetLimiter " : Util::emptyString;

	string newmyinfo = ("$MyINFO $ALL " + Util::validateNick(getNick()) + " " + Util::validateMessage(speedDescription+nldetect+getDescription(), false));
	if(BOOLSETTING(SEND_EXTENDED_INFO) || (((counts.normal) + (counts.registered) + (counts.op)) > 10) ) {
		newmyinfo += extendedtag;
	}
	int64_t newbytesshared = ShareManager::getInstance()->getShareSize();
	newmyinfo += ("$ $" + connection + StatusMode + "$" + Util::validateMessage(SETTING(EMAIL), false) + '$');
	if ( (newmyinfo != lastmyinfo) || ( (newbytesshared < (lastbytesshared - 1048576) ) || (newbytesshared > (lastbytesshared + 1048576) ) ) ){
		send(newmyinfo + Util::toString(newbytesshared) + "$|");
		lastmyinfo = newmyinfo;
		lastbytesshared = newbytesshared;
	}
}

void NmdcHub::disconnect() throw() {	
	state = STATE_CONNECT;
	socket->disconnect();
	{ 
		Lock l(cs);
		clearUsers();
	}
}

void NmdcHub::search(int aSizeType, int64_t aSize, int aFileType, const string& aString){
	checkstate(); 
	char* buf;
	char c1 = (aSizeType == SearchManager::SIZE_DONTCARE || aSizeType == SearchManager::SIZE_EXACT) ? 'F' : 'T';
	char c2 = (aSizeType == SearchManager::SIZE_ATLEAST) ? 'F' : 'T';
	string tmp = aString;
	string::size_type i;
	while((i = tmp.find(' ')) != string::npos) {
		tmp[i] = '$';
	}
	int chars = 0;
		if((SETTING(CONNECTION_TYPE) == SettingsManager::CONNECTION_ACTIVE) && (!BOOLSETTING(SEARCH_PASSIVE))) {
		string x = getLocalIp();
		buf = new char[x.length() + aString.length() + 64];
		chars = sprintf(buf, "$Search %s:%d %c?%c?%s?%d?%s|", x.c_str(), SETTING(IN_PORT), c1, c2, Util::toString(aSize).c_str(), aFileType+1, tmp.c_str());
	} else {
		buf = new char[getNick().length() + aString.length() + 64];
		chars = sprintf(buf, "$Search Hub:%s %c?%c?%s?%d?%s|", getNick().c_str(), c1, c2, Util::toString(aSize).c_str(), aFileType+1, tmp.c_str());
	}
	send(buf, chars);
	delete[] buf;
}

void NmdcHub::kick(const User::Ptr& aUser, const string& aMsg) {
	checkstate(); 
	dcdebug("NmdcHub::kick\n");
	static const char str[] = 
		"$To: %s From: %s $<%s> You are being kicked because: %s|<%s> %s is kicking %s because: %s|";
	string msg2 = Util::validateMessage(aMsg, false);
	
	char* tmp = new char[sizeof(str) + 2*aUser->getNick().length() + 2*msg2.length() + 4*getNick().length()];
	const char* u = aUser->getNick().c_str();
	const char* n = getNick().c_str();
	const char* m = msg2.c_str();
	sprintf(tmp, str, u, n, n, m, n, n, u, m);
	send(tmp);
	delete[] tmp;
	
	// Short, short break to allow the message to reach the NmdcHub...
	Thread::sleep(200);
	send("$Kick " + aUser->getNick() + "|");
}

void NmdcHub::kick(const User* aUser, const string& aMsg) {
	checkstate(); 
	dcdebug("NmdcHub::kick\n");
	
	static const char str[] = 
		"$To: %s From: %s $<%s> You are being kicked because: %s|<%s> %s is kicking %s because: %s|";
	string msg2 = Util::validateMessage(aMsg, false);
	
	char* tmp = new char[sizeof(str) + 2*aUser->getNick().length() + 2*msg2.length() + 4*getNick().length()];
	const char* u = aUser->getNick().c_str();
	const char* n = getNick().c_str();
	const char* m = msg2.c_str();
	sprintf(tmp, str, u, n, n, m, n, n, u, m);
	send(tmp);
	delete[] tmp;
	
	// Short, short break to allow the message to reach the NmdcHub...
	Thread::sleep(100);
	send("$Kick " + aUser->getNick() + "|");
}

// TimerManagerListener
void NmdcHub::on(TimerManagerListener::Second, u_int32_t aTick) throw() {
	if(socket && (lastActivity + (120+Util::rand(0, 60)) * 1000) < aTick) {
		// Nothing's happened for ~120 seconds, check if we're connected, if not, try to connect...
		lastActivity = aTick;
		// Try to send something for the fun of it...
		if(isConnected()) {
			dcdebug("Testing writing...\n");
			socket->write("|", 1);
		} else {
			// Try to reconnect...
			if(reconnect && !getAddress().empty())
				connect();
		}
	}
	{
		Lock l(cs);
		
		while(!seekers.empty() && seekers.front().second + (5 * 1000) < aTick) {
			seekers.pop_front();
		}
		
		while(!flooders.empty() && flooders.front().second + (120 * 1000) < aTick) {
			flooders.pop_front();
		}
	}
}

// BufferedSocketListener
void NmdcHub::on(BufferedSocketListener::Failed, const string& aLine) throw() {
	{
		Lock l(cs);
		clearUsers();
	}
	if(state == STATE_CONNECTED)
		state = STATE_CONNECT;
	Speaker<NmdcHubListener>::fire(NmdcHubListener::Failed(), this, aLine); 
}

void NmdcHub::sendDebugMessage(const string& aLine) {
	if (BOOLSETTING(DEBUG_COMMANDS))
		DebugManager::getInstance()->SendDebugMessage("Hub:	" + aLine);
}

/**
 * @file
 * $Id$
 */
