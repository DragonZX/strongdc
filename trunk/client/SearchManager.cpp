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

#include "stdinc.h"
#include "DCPlusPlus.h"

#include "SearchManager.h"
#include "UploadManager.h"

#include "ClientManager.h"
#include "ShareManager.h"
#include "AdcCommand.h"
#include "FileChunksInfo.h"
#include "QueueManager.h"

#include "../pme-1.0.4/pme.h"

SearchResult::SearchResult(Client* aClient, Types aType, int64_t aSize, const string& aFile, const TTHValue* aTTH, bool aUtf8) :
file(aFile), hubName(aClient->getName()), hubIpPort(aClient->getIpPort()), user(aClient->getMe()), 
size(aSize), type(aType), slots(UploadManager::getInstance()->getSlots()), freeSlots(UploadManager::getInstance()->getFreeSlots()),  
tth(aTTH == NULL ? NULL : new TTHValue(*aTTH)), utf8(aUtf8), ref(1) { }

SearchResult::SearchResult(Types aType, int64_t aSize, const string& aFile, const TTHValue* aTTH) :
	file(aFile), size(aSize), type(aType), slots(UploadManager::getInstance()->getSlots()), freeSlots(UploadManager::getInstance()->getFreeSlots()),  
	tth(aTTH == NULL ? NULL : new TTHValue(*aTTH)), utf8(true), ref(1) { }

string SearchResult::toSR() const {
	// File:		"$SR %s %s%c%s %d/%d%c%s (%s)|"
	// Directory:	"$SR %s %s %d/%d%c%s (%s)|"
	string tmp;
	tmp.reserve(128);
	tmp.append("$SR ", 4);
	tmp.append(Text::utf8ToAcp(user->getNick()));
	tmp.append(1, ' ');
	string acpFile = utf8 ? Text::utf8ToAcp(file) : file;
	if(type == TYPE_FILE) {
		tmp.append(acpFile);
		tmp.append(1, '\x05');
		tmp.append(Util::toString(size));
	} else {
		tmp.append(acpFile, 0, acpFile.length() - 1);
	}
	tmp.append(1, ' ');
	tmp.append(Util::toString(freeSlots));
	tmp.append(1, '/');
	tmp.append(Util::toString(slots));
	tmp.append(1, '\x05');
	if(getTTH() == NULL) {
		tmp.append(Text::utf8ToAcp(hubName));
	} else {
		tmp.append("TTH:" + getTTH()->toBase32());
	}
	tmp.append(" (", 2);
	tmp.append(hubIpPort);
	tmp.append(")|", 2);
	return tmp;
}

AdcCommand SearchResult::toRES(char type) const {
	AdcCommand cmd(AdcCommand::CMD_RES, type);
	cmd.addParam("SI", Util::toString(size));
	cmd.addParam("SL", Util::toString(freeSlots));
	cmd.addParam("FN", Util::toAdcFile(utf8 ? file : Text::acpToUtf8(file)));
	if(getTTH() != NULL) {
		cmd.addParam("TR", getTTH()->toBase32());
	}
	return cmd;
}

void SearchManager::search(const string& aName, int64_t aSize, TypeModes aTypeMode /* = TYPE_ANY */, SizeModes aSizeMode /* = SIZE_ATLEAST */, const string& aToken /* = Util::emptyString */, int* aWindow /* = NULL */, tstring aSearch /*= _T("")*/) {
	SearchQueueItem sqi(aSizeMode, aSize, aTypeMode, aName, aWindow, aSearch, aToken);
	if(aWindow != NULL) {
		bool added = false;
		if(searchQueue.empty()) {
			searchQueue.insert(searchQueue.begin(), sqi);
			added = true;
		} else {
			// Insert before the automatic searches (manual search) 
			for(SearchQueueIter qi = searchQueue.begin(); qi != searchQueue.end(); qi++) {
				if(qi->getWindow() == NULL) {
					searchQueue.insert(qi, sqi);
					added = true;
					break;
				}
			}
		}
		if (!added) {
			searchQueue.push_back(sqi);
		}
	} else {
		// Insert last (automatic search)
		searchQueue.push_back(sqi);
	}

}

void SearchManager::search(StringList& who, const string& aName, int64_t aSize /* = 0 */, TypeModes aTypeMode /* = TYPE_ANY */, SizeModes aSizeMode /* = SIZE_ATLEAST */, const string& aToken /* = Util::emptyString */, int* aWindow /* = NULL */, tstring aSearch /*= _T("")*/) {
	SearchQueueItem sqi(who, aSizeMode, aSize, aTypeMode, aName, aWindow, aSearch, aToken);
	if(aWindow != NULL) {
		bool added = false;
		if(searchQueue.empty()) {
			searchQueue.push_front(sqi);
			added = true;
		} else {
			// Insert before the automatic searches (manual search) 
			for(SearchQueueIter qi = searchQueue.begin(); qi != searchQueue.end(); qi++) {
				if(qi->getWindow() == NULL) {
					if(qi != NULL)
						searchQueue.insert(qi, sqi);
					else
						searchQueue.push_front(sqi);
					added = true;
					break;
				}
			}
		}
		if (!added) {
			searchQueue.push_back(sqi);
		}
	} else {
		// Insert last (automatic search)
		searchQueue.push_back(sqi);
	}
}

string SearchResult::getFileName() const { 
	if(getType() == TYPE_FILE) 
		return Util::getFileName(getFile()); 

	if(getFile().size() < 2)
		return getFile();

	string::size_type i = getFile().rfind('\\', getFile().length() - 2);
	if(i == string::npos)
		return getFile();

	return getFile().substr(i + 1);
}

void SearchManager::setPort(short aPort) throw(SocketException) {
	port = aPort;
	if(socket != NULL) {
		disconnect();
	} else {
		socket = new Socket();
	}

	socket->create(Socket::TYPE_UDP, true);
	socket->bind(aPort);
	start();
}

void SearchManager::disconnect() throw() {
	if(socket != NULL) {
		stop = true;
		socket->disconnect();
#ifdef _WIN32
		join();
#endif
		stop = false;
	}
}

#define BUFSIZE 8192
int SearchManager::run() {
	
	AutoArray<u_int8_t> buf(BUFSIZE);
	int len;

	while(true) {

		string remoteAddr;
		try {
			while( (len = socket->read((u_int8_t*)buf, BUFSIZE, remoteAddr)) != 0) {
				onData(buf, len, remoteAddr);
			}
		} catch(const SocketException& e) {
			dcdebug("SearchManager::run Error: %s\n", e.getError().c_str());
		}
		if(stop) {
			return 0;
		}

		try {
			socket->disconnect();
			socket->create(Socket::TYPE_UDP, true);
			socket->bind(port);
		} catch(const SocketException& e) {
			// Oops, fatal this time...
			dcdebug("SearchManager::run Stopped listening: %s\n", e.getError().c_str());
			return 1;
		}
	}
	
	return 0;
}

void SearchManager::onData(const u_int8_t* buf, size_t aLen, const string& address) {
	string x((char*)buf, aLen);
	if(x.compare(0, 4, "$SR ") == 0) {
		string::size_type i, j;
		// Directories: $SR <nick><0x20><directory><0x20><free slots>/<total slots><0x05><Hubname><0x20>(<Hubip:port>)
		// Files:       $SR <nick><0x20><filename><0x05><filesize><0x20><free slots>/<total slots><0x05><Hubname><0x20>(<Hubip:port>)
		i = 4;
		if( (j = x.find(' ', i)) == string::npos) {
			return;
		}
		string nick = Text::acpToUtf8(x.substr(i, j-i));
		i = j + 1;

		// A file has 2 0x05, a directory only one
		size_t cnt = count(x.begin() + j, x.end(), 0x05);
	
		SearchResult::Types type = SearchResult::TYPE_FILE;
		string file;
		int64_t size = 0;

		if(cnt == 1) {
			// We have a directory...find the first space beyond the first 0x05 from the back 
			// (dirs might contain spaces as well...clever protocol, eh?)
			type = SearchResult::TYPE_DIRECTORY;
			// Get past the hubname that might contain spaces
			if((j = x.rfind(0x05)) == string::npos) {
				return;
			}
			// Find the end of the directory info
			if((j = x.rfind(' ', j-1)) == string::npos) {
				return;
			}
			if(j < i + 1) {
				return;
			}	
			file = x.substr(i, j-i) + '\\';
		} else if(cnt == 2) {
			if( (j = x.find((char)5, i)) == string::npos) {
				return;
			}
			file = x.substr(i, j-i);
			i = j + 1;
			if( (j = x.find(' ', i)) == string::npos) {
				return;
			}
			size = Util::toInt64(x.substr(i, j-i));
		}	
		i = j + 1;
		
		if( (j = x.find('/', i)) == string::npos) {
			return;
		}
		int freeSlots = Util::toInt(x.substr(i, j-i));
		i = j + 1;
		if( (j = x.find((char)5, i)) == string::npos) {
			return;
		}
		int slots = Util::toInt(x.substr(i, j-i));
		i = j + 1;
		if( (j = x.rfind(" (")) == string::npos) {
			return;
		}
		// the hub's name will get replaced later (with a UTF-8 version) if there's a TTH in the result
		string hubName = Text::acpToUtf8(x.substr(i, j-i));
		i = j + 2;
		if( (j = x.rfind(')')) == string::npos) {
			return;
		}
		string hubIpPort = x.substr(i, j-i);
		User::Ptr user = ClientManager::getInstance()->getUser(nick, hubIpPort);

		SearchResult* sr = new SearchResult(user, type, slots, freeSlots, size,
			file, hubName, hubIpPort, address, false);

		{
			Lock l(cs);
			seznam.push_back(sr);
		}	
	} else if(x.compare(1, 4, "RES ") == 0 && x[x.length() - 1] == 0x0a) {
		AdcCommand c(x.substr(0, x.length()-1));
		if(c.getParameters().empty())
			return;

		User::Ptr p = ClientManager::getInstance()->getUser(c.getFrom(), false);
		if(!p)
			return;

		int freeSlots = -1;
		int64_t size = -1;
		string name;

		for(StringIter i = c.getParameters().begin(); i != c.getParameters().end(); ++i) {
			string& str = *i;
			if(str.compare(0, 2, "FN") == 0) {
				name = Util::toNmdcFile(str.substr(2));
			} else if(str.compare(0, 2, "SL") == 0) {
				freeSlots = Util::toInt(str.substr(2));
			} else if(str.compare(0, 2, "SI") == 0) {
				size = Util::toInt64(str.substr(2));
			}
		}

		if(!name.empty() && freeSlots != -1 && size != -1) {
			SearchResult::Types type = (name[name.length() - 1] == '\\' ? SearchResult::TYPE_DIRECTORY : SearchResult::TYPE_FILE);
			SearchResult* sr = new SearchResult(p, type, p->getSlots(), freeSlots, size, name, p->getClientName(), "0.0.0.0", NULL, true);
			fire(SearchManagerListener::SR(), sr);
			sr->decRef();
		}
	} else if(x.compare(1, 4, "SCH ") == 0 && x[x.length() - 1] == 0x0a) {
		try {
			respond(AdcCommand(x.substr(0, x.length()-1)));
		} catch(ParseException& ) {
		}
	} else if(x.compare(0, 5, "$PSR ") == 0) {
		string::size_type i, j;
		// Syntax: $PSR <nick>$<UdpPort>$<Hubip:port>$<TTH>$<PartialCount>$<PartialInfo>$|
		i = 5;
		if( (j = x.find('$', i)) == string::npos) {
			return;
		}
		string nick = Text::acpToUtf8(x.substr(i, j-i));
		i = j + 1;

		if( (j = x.find('$', i)) == string::npos) {
			return;
		}
		u_int16_t UdpPort = Util::toInt(x.substr(i, j-i));
		i = j + 1;

		if( (j = x.find('$', i)) == string::npos) {
			return;
		}
		string hubIpPort = x.substr(i, j-i);
		i = j + 1;

		if( (j = x.find('$', i)) == string::npos) {
			return;
		}
		string tth = x.substr(i, j-i);
		i = j + 1;

		if( (j = x.find('$', i)) == string::npos) {
			return;
		}
		u_int32_t partialCount = Util::toUInt32(x.substr(i, j-i)) * 2;
		i = j + 1;

		if( (j = x.find('$', i)) == string::npos) {
			return;
		}
		string partialInfoBlocks = x.substr(i, j-i);

		PartsInfo partialInfo;
		i = 0; j = 0;
		while(j != string::npos) {
			j = partialInfoBlocks.find(',', i);
			u_int16_t block = Util::toInt(partialInfoBlocks.substr(i, j-i));			
			partialInfo.push_back(block);
			i = j + 1;
		}
		dcdebug(("PartialInfo Size = "+Util::toString(partialInfo.size())+"\n").c_str());
		dcassert(partialInfo.size() == partialCount);

		User::Ptr user = ClientManager::getInstance()->getUser(nick, hubIpPort);
		PartsInfo outPartialInfo;
		QueueManager::getInstance()->handlePartialResult(user, TTHValue(tth), partialInfo, outPartialInfo);

		if((UdpPort > 0) && (!outPartialInfo.empty())) {
			char buf[1024];
			Client* aClient = user->getClient();
			string me = Text::utf8ToAcp(aClient->getMe()->getNick());
			_snprintf(buf, 1023, "$PSR %s$%d$%s$%s$%d$%s$|", me.c_str(), 0, hubIpPort.c_str(), tth.c_str(), outPartialInfo.size() / 2, GetPartsString(outPartialInfo));
			buf[1023] = NULL;
			Socket s; s.writeTo(Socket::resolve(address), UdpPort, buf);
		}
	}
}

void SearchManager::respond(const AdcCommand& adc) {
	// Filter own searches
	if(adc.getFrom().toBase32() == SETTING(CLIENT_ID))
		return;

	User::Ptr p = ClientManager::getInstance()->getUser(adc.getFrom(), false);
	if(!p)
		return;

	SearchResult::List results;
	ShareManager::getInstance()->search(results, adc.getParameters(), 10);

	string token;

	adc.getParam("TO", 0, token);

	if(results.empty())
		return;

	if(!p->getIp().empty() && p->getUDPPort() != 0) {
		Socket s;
		try {
			s.create(Socket::TYPE_UDP);
			for(SearchResult::Iter i = results.begin(); i != results.end(); ++i) {
				if(token.empty())
					s.writeTo(p->getIp(), p->getUDPPort(), (*i)->toRES(AdcCommand::TYPE_UDP).toString());
				else
					s.writeTo(p->getIp(), p->getUDPPort(), (*i)->toRES(AdcCommand::TYPE_UDP).addParam("TO", token).toString());
				(*i)->decRef();
			}
		} catch(const SocketException&) {
			dcdebug("Search caught error\n");
		}
	} else {
		for(SearchResult::Iter i = results.begin(); i != results.end(); ++i) {
			AdcCommand cmd = (*i)->toRES(AdcCommand::TYPE_DIRECT);
			cmd.setTo(adc.getFrom());
			if(!token.empty())
				cmd.addParam("TO", token);
			p->send(cmd.toString());
			(*i)->decRef();
		}
	}
}

string SearchManager::clean(const string& aSearchString) {
	static const char* badChars = "$|.[]()-_+";
	string::size_type i = aSearchString.find_first_of(badChars);
	if(i == string::npos)
		return aSearchString;

	string tmp = aSearchString;
	// Remove all strange characters from the search string
	do {
		tmp[i] = ' ';
	} while ( (i = tmp.find_first_of(badChars, i)) != string::npos);

	return tmp;
}

void SearchManager::on(TimerManagerListener::Second, u_int32_t aTick) throw() {
	bool resort = false;
	{
		Lock l(cs);

		for(SearchResult::List::iterator i = seznam.begin(); i < seznam.end(); i++) {
			SearchResult* sr = *i;
			if (!sr->getIP().empty()) {
				ClientManager::getInstance()->setIPNick(sr->getIP(), sr->getUser());
			}

			if((sr->user->getVersion() == "0.403") && (sr->user->getClientType() != "rmDC++ 0.403D[1]")) {
				string path(sr->file);
				path = path.substr(0, path.find("\\"));
				PME reg("([A-Z])");
				if(reg.match(path)) {
					sr->user->setCheat("rmDC++ 0.403D[1] with DC++ emulation" , true);
					sr->user->setClientType("rmDC++ 0.403D[1]");
					sr->user->setBadClient(true);
				}
			}
			fire(SearchManagerListener::SR(), sr);
			sr->decRef();
		}
		resort = !seznam.empty();
		seznam.clear();
	}

	if(resort)
		fire(SearchManagerListener::Resort());

	if(!searchQueue.empty() && ((getLastSearch() + (SETTING(MINIMUM_SEARCH_INTERVAL)*1000)) < aTick)) {
		SearchQueueItem sqi = searchQueue.front();
		searchQueue.erase(searchQueue.begin());
		if(sqi.getHubs().empty()) {
			ClientManager::getInstance()->search(sqi.getSizeMode(), sqi.getSize(), sqi.getTypeMode(), sqi.getTarget(), sqi.getToken());
			fire(SearchManagerListener::Searching(), &sqi);
		} else {
			ClientManager::getInstance()->search(sqi.getHubs(), sqi.getSizeMode(), sqi.getSize(), sqi.getTypeMode(), sqi.getTarget(), sqi.getToken());
			fire(SearchManagerListener::Searching(), &sqi);
		}
		setLastSearch( GET_TICK() );
	}
}

u_int32_t SearchManager::getLastSearch() {
	return lastSearch; 
}

int SearchManager::getSearchQueueNumber(int* aWindow) {
	if(!searchQueue.empty()){
		int queueNumber = 0;
		for(SearchQueueIter sqi = searchQueue.begin(); sqi != searchQueue.end(); ++sqi) {
			if(sqi->getWindow() == aWindow) {
				return queueNumber;
			}
		queueNumber++;
		}
	} else {
		// should never get here but just in case...
		return 0;
	}
	return 0;
}

/**
 * @file
 * $Id$
 */
