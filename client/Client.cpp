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
#include "Client.h"
#include "BufferedSocket.h"
#include "FavoriteManager.h"
#include "TimerManager.h"
#include "DebugManager.h"
#include "ClientManager.h"

Client::Counts Client::counts;

Client::Client(const string& hubURL, char separator_, bool secure_) : 
	reconnDelay(120), lastActivity(0), registered(false), socket(NULL),
	hubUrl(hubURL), port(0), separator(separator_),
	secure(secure_), countType(COUNT_UNCOUNTED), supportFlags(0), availableBytes(0) {
	string file;
	Util::decodeUrl(hubURL, address, port, file);
}

Client::~Client() throw() {
	dcassert(!socket);
	updateCounts(true);
}

void Client::shutdown() {
	if(socket) {
		BufferedSocket::putSocket(socket);
		socket = 0;
	}
}

void Client::reloadSettings(bool updateNick) {
	FavoriteHubEntry* hub = FavoriteManager::getInstance()->getFavoriteHubEntry(getHubUrl());
	
	string speedDescription = Util::emptyString;
	if(BOOLSETTING(SHOW_DESCRIPTION_SPEED))
		speedDescription = "["+SETTING(DOWN_SPEED)+"/"+SETTING(UP_SPEED)+"]";

	if(hub) {
		if(updateNick)
			getMyIdentity().setNick(checkNick(hub->getNick(true)));
		if(!hub->getUserDescription().empty()) {
			getMyIdentity().setDescription(speedDescription + hub->getUserDescription());
		} else {
			getMyIdentity().setDescription(speedDescription + SETTING(DESCRIPTION));
		}
		setPassword(hub->getPassword());
		setStealth(hub->getStealth());
		setFavIp(hub->getIP());
	} else {
		getMyIdentity().setNick(checkNick(SETTING(NICK)));
		getMyIdentity().setDescription(speedDescription + SETTING(DESCRIPTION));
		setStealth(true);
		setFavIp(Util::emptyString);
	}
	getMyIdentity().setUser(ClientManager::getInstance()->getMe());
	getMyIdentity().setHubUrl(getHubUrl());	
}

bool Client::isActive() {
	return ClientManager::getInstance()->getMode(hubUrl) != SettingsManager::INCOMING_FIREWALL_PASSIVE;
}

void Client::connect() {
	if(socket)
		BufferedSocket::putSocket(socket);

	// TODO should this be done also on disconnect ???
	FavoriteManager::getInstance()->removeUserCommand(getHubUrl());
	availableBytes = 0;

	setReconnDelay(120 + Util::rand(0, 60));
	reloadSettings(true);
	setRegistered(false);

	try {
		socket = BufferedSocket::getSocket(separator);
		socket->addListener(this);
		socket->connect(address, port, secure, true);
	} catch(const Exception& e) {
		if(socket) {
			BufferedSocket::putSocket(socket);
			socket = NULL;
		}
		fire(ClientListener::Failed(), this, e.getError());
	}
	updateActivity();
}

void Client::disconnect(bool graceLess) {
	if(!socket)
		return;
	socket->removeListener(this);
	socket->disconnect(graceLess);
}

void Client::updateActivity() {
	lastActivity = GET_TICK();
}

void Client::updateCounts(bool aRemove) {
	// We always remove the count and then add the correct one if requested...
	if(countType == COUNT_NORMAL) {
		Thread::safeDec(counts.normal);
	} else if(countType == COUNT_REGISTERED) {
		Thread::safeDec(counts.registered);
	} else if(countType == COUNT_OP) {
		Thread::safeDec(counts.op);
	}

	countType = COUNT_UNCOUNTED;

	if(!aRemove) {
		if(getMyIdentity().isOp()) {
			Thread::safeInc(counts.op);
			countType = COUNT_OP;
		} else if(registered) {
			Thread::safeInc(counts.registered);
			countType = COUNT_REGISTERED;
		} else {
			Thread::safeInc(counts.normal);
			countType = COUNT_NORMAL;
		}
	}
}

string Client::getLocalIp() const {
	// Favorite hub Ip
	if(!getFavIp().empty())
		return getFavIp();

	// Best case - the server detected it
	if((!BOOLSETTING(NO_IP_OVERRIDE) || SETTING(EXTERNAL_IP).empty()) && !getMyIdentity().getIp().empty()) {
		return getMyIdentity().getIp();
	}

	if(!SETTING(EXTERNAL_IP).empty()) {
		return Socket::resolve(SETTING(EXTERNAL_IP));
	}

	string lip;
	if(socket)
		lip = socket->getLocalIp();

	if(lip.empty())
		return Util::getLocalIp();
	return lip;
}

/**
 * @file
 * $Id$
 */
