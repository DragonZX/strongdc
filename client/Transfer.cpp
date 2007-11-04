/*
 * Copyright (C) 2001-2007 Jacek Sieka, arnetheduck on gmail point com
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

#include "Transfer.h"

#include "UserConnection.h"
#include "ResourceManager.h"
#include "ClientManager.h"

const string Transfer::names[] = {
	"file", "file", "list", "tthl", "file"
};

const string Transfer::USER_LIST_NAME = "files.xml";
const string Transfer::USER_LIST_NAME_BZ = "files.xml.bz2";

Transfer::Transfer(UserConnection& conn, const string& path_, const TTHValue& tth_) : start(0), type(TYPE_FILE),
	path(path_), tth(tth_), actual(0), pos(0), startPos(0), size(-1), fileSize(-1), userConnection(conn),
	lastTick(GET_TICK()) { }

void Transfer::tick() {
	Lock l(cs);
	while(samples.size() >= SAMPLES) {
		samples.pop_front();
	}
	samples.push_back(std::make_pair(GET_TICK(), pos - startPos));
}

double Transfer::getAverageSpeed() const {
	Lock l(cs);
	if(samples.size() < 2) {
		return 0;
	}
	uint64_t ticks = samples.back().first - samples.front().first;
	int64_t bytes = samples.back().second - samples.front().second;

	return ticks > 0 ? (static_cast<double>(bytes) / ticks) * 1000.0 : 0;
}

void Transfer::getParams(const UserConnection& aSource, StringMap& params) const {
	params["userNI"] = Util::toString(ClientManager::getInstance()->getNicks(aSource.getUser()->getCID()));
	params["userI4"] = aSource.getRemoteIp();
	StringList hubNames = ClientManager::getInstance()->getHubNames(aSource.getUser()->getCID());
	if(hubNames.empty())
		hubNames.push_back(STRING(OFFLINE));
	params["hub"] = Util::toString(hubNames);
	StringList hubs = ClientManager::getInstance()->getHubs(aSource.getUser()->getCID());
	if(hubs.empty())
		hubs.push_back(STRING(OFFLINE));
	params["hubURL"] = Util::toString(hubs);
	params["fileSI"] = Util::toString(getSize());
	params["fileSIshort"] = Util::formatBytes(getSize());
	params["fileSIchunk"] = Util::toString(getTotal());
	params["fileSIchunkshort"] = Util::formatBytes(getTotal());
	params["fileSIactual"] = Util::toString(getActual());
	params["fileSIactualshort"] = Util::formatBytes(getActual());
	params["speed"] = Util::formatBytes(static_cast<int64_t>(getAverageSpeed())) + "/s";
	params["time"] = Text::fromT(Util::formatSeconds((GET_TICK() - getStart()) / 1000));
	params["fileTR"] = getTTH().toBase32();
}

UserPtr Transfer::getUser() {
	return getUserConnection().getUser();
}
const UserPtr Transfer::getUser() const {
	return getUserConnection().getUser();
}

