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

#include "stdinc.h"
#include "DCPlusPlus.h"

#include "QueueManager.h"

#include "ConnectionManager.h"
#include "SearchManager.h"
#include "ClientManager.h"
#include "DownloadManager.h"
#include "ShareManager.h"
#include "LogManager.h"
#include "ResourceManager.h"
#include "version.h"

#include "UserConnection.h"
#include "SimpleXML.h"
#include "StringTokenizer.h"
#include "DirectoryListing.h"

#include "FileChunksInfo.h"
#include "UploadManager.h"

#include <limits>

#ifdef ff
#undef ff
#endif

#ifndef _WIN32
#include <sys/types.h>
#include <dirent.h>
#include <fnmatch.h>
#endif

namespace {
	const string TEMP_EXTENSION = ".dctmp";

	string getTempName(const string& aFileName, const TTHValue& aRoot) {
		string tmp(aFileName);
		tmp += "." + aRoot.toBase32();
		tmp += TEMP_EXTENSION;
		return tmp;
	}

	/**
	* Convert space seperated number string to vector
	*
	* Created by RevConnect
	*/
	template <class T>
	void toIntList(const string& freeBlocks, vector<T>& v)
	{
		if (freeBlocks.empty())
			return;

		StringTokenizer<string> t(freeBlocks, ' ');
		StringList& sl = t.getTokens();

		v.reserve(sl.size());

		for(StringList::const_iterator i = sl.begin(); i != sl.end(); ++i) {
			if(!i->empty()) {
				int64_t offset = Util::toInt64(*i);
				if(!v.empty() && offset < v.back()){
					dcassert(0);
					v.clear();
					return;
				}else{
					v.push_back((T)offset);
				}
			}else{
				dcassert(0);
				v.clear();
				return;
			}
		}

		dcassert(!v.empty() && (v.size() % 2 == 0));
	}
}

const string& QueueItem::getTempTarget() {
	if(!isSet(QueueItem::FLAG_USER_LIST) && tempTarget.empty()) {
		if(!SETTING(TEMP_DOWNLOAD_DIRECTORY).empty() && (File::getSize(getTarget()) == -1)) {
#ifdef _WIN32
			::StringMap sm;
			if(target.length() >= 3 && target[1] == ':' && target[2] == '\\')
				sm["targetdrive"] = target.substr(0, 3);
			else
				sm["targetdrive"] = Util::getConfigPath().substr(0, 3);
			setTempTarget(Util::formatParams(SETTING(TEMP_DOWNLOAD_DIRECTORY), sm, false) + getTempName(getTargetFileName(), getTTH()));
#else //_WIN32
			setTempTarget(SETTING(TEMP_DOWNLOAD_DIRECTORY) + getTempName(getTargetFileName(), getTTH()));
#endif //_WIN32
		}
	}
	return tempTarget;
}

QueueItem* QueueManager::FileQueue::add(const string& aTarget, int64_t aSize, 
						  int aFlags, QueueItem::Priority p, const string& aTempTarget,
						  int64_t aDownloadedBytes, time_t aAdded, const string& freeBlocks/* = Util::emptyString*/, const string& verifiedBlocks /* = Util::emptyString */, const TTHValue& root) throw(QueueException, FileException)
{
	if(p == QueueItem::DEFAULT) {
		p = QueueItem::NORMAL;
		if(aSize <= SETTING(PRIO_HIGHEST_SIZE)*1024) {
		p = QueueItem::HIGHEST;
		} else if(aSize <= SETTING(PRIO_HIGH_SIZE)*1024) {
			p = QueueItem::HIGH;
		} else if(aSize <= SETTING(PRIO_NORMAL_SIZE)*1024) {
			p = QueueItem::NORMAL;
		} else if(aSize <= SETTING(PRIO_LOW_SIZE)*1024) {
			p = QueueItem::LOW;
		} else if(SETTING(PRIO_LOWEST)) {
			p = QueueItem::LOWEST;
		}
	}

	QueueItem* qi = new QueueItem(aTarget, aSize, p, aFlags, aDownloadedBytes, aAdded, root);

	if(!qi->isSet(QueueItem::FLAG_USER_LIST)) {
		if(!aTempTarget.empty()) {
			qi->setTempTarget(aTempTarget);
		}
	} else {
		qi->setPriority(QueueItem::HIGHEST);
	}

	qi->setMaxSegments(qi->isSet(QueueItem::FLAG_MULTI_SOURCE) ? getMaxSegments(qi->getSize()) : 1);

	// Create FileChunksInfo for this item
	if(qi->isSet(QueueItem::FLAG_MULTI_SOURCE)) {
		FileChunksInfo* pChunksInfo = NULL;

		dcassert(!qi->getTempTarget().empty());
		vector<int64_t> v;
		bool isMissing = false;
		
		if ( freeBlocks != Util::emptyString ){
			if(File::getSize(qi->getTempTarget()) > 0) {
				toIntList<int64_t>(freeBlocks, v);
			} else {
				aDownloadedBytes = 0;
				v.push_back(0);
				v.push_back(qi->getSize());
				isMissing = true;
			}
		} else {
			// import DC++'s download queue
			v.push_back(aDownloadedBytes);
			v.push_back(qi->getSize());
		}
		
		if(v.size() < 2 || v.size() % 2 != 0){
			dcassert(0); // wrong freeBlocks
			v.clear();
			
			TigerTree tth;
			if(HashManager::getInstance()->getTree(root, tth)){
				// mark first byte as free, finish and verify this download
				v.push_back(0);
				v.push_back(1);
			}else{
				v.push_back(0);
				v.push_back(qi->getSize());
			}
		}
		
		pChunksInfo = new FileChunksInfo(const_cast<TTHValue*>(&qi->getTTH()), qi->getSize(), &v);
		qi->chunkInfo = pChunksInfo;

		if(pChunksInfo && !isMissing && verifiedBlocks != Util::emptyString){
			vector<uint16_t> v;
			toIntList<uint16_t>(verifiedBlocks, v);

			for(vector<uint16_t>::iterator i = v.begin(); i < v.end(); i++, i++)
				pChunksInfo->markVerifiedBlock(*i, *(i+1));
		}
	}

	if((qi->getDownloadedBytes() > 0))
		qi->setFlag(QueueItem::FLAG_EXISTS);

	if(BOOLSETTING(AUTO_PRIORITY_DEFAULT) && !qi->isSet(QueueItem::FLAG_USER_LIST) && (p != QueueItem::HIGHEST) && (p != QueueItem::PAUSED)) {
		qi->setAutoPriority(true);
		qi->setPriority(qi->calculateAutoPriority());
	}

	dcassert(find(aTarget) == NULL);
	add(qi);
	return qi;
}

void QueueManager::FileQueue::add(QueueItem* qi) {
	queue.insert(make_pair(const_cast<string*>(&qi->getTarget()), qi));
	//if(lastInsert == queue.end())
	//	lastInsert = queue.insert(make_pair(const_cast<string*>(&qi->getTarget()), qi)).first;
	//else
	//	lastInsert = queue.insert(lastInsert, make_pair(const_cast<string*>(&qi->getTarget()), qi));
}

QueueItem* QueueManager::FileQueue::find(const string& target) {
	QueueItem::StringIter i = queue.find(const_cast<string*>(&target));
	return (i == queue.end()) ? NULL : i->second;
}

void QueueManager::FileQueue::find(QueueItem::List& sl, int64_t aSize, const string& suffix) {
	for(QueueItem::StringIter i = queue.begin(); i != queue.end(); ++i) {
		if(i->second->getSize() == aSize) {
			const string& t = i->second->getTarget();
			if(suffix.empty() || (suffix.length() < t.length() &&
				Util::stricmp(suffix.c_str(), t.c_str() + (t.length() - suffix.length())) == 0) )
				sl.push_back(i->second);
		}
	}
}

void QueueManager::FileQueue::find(QueueItem::List& ql, const TTHValue& tth) {
	for(QueueItem::StringIter i = queue.begin(); i != queue.end(); ++i) {
		QueueItem* qi = i->second;
		if(qi->getTTH() == tth) {
			ql.push_back(qi);
		}
	}
}

static QueueItem* findCandidate(QueueItem::StringIter start, QueueItem::StringIter end, deque<string>& recent) {
	QueueItem* cand = NULL;
	for(QueueItem::StringIter i = start; i != end; ++i) {
		QueueItem* q = i->second;

		// We prefer to search for things that are not running...
		if((cand != NULL) && !q->hasFreeSegments()) 
			continue;
		// No user lists
		if(q->isSet(QueueItem::FLAG_USER_LIST) || q->isSet(QueueItem::FLAG_TESTSUR) || q->isSet(QueueItem::FLAG_CHECK_FILE_LIST))
			continue;
        // No paused downloads
		if(q->getPriority() == QueueItem::PAUSED)
			continue;
		// No files that already have more than AUTO_SEARCH_LIMIT online sources
		if(q->countOnlineUsers() >= SETTING(AUTO_SEARCH_LIMIT))
			continue;
		// Did we search for it recently?
        if(find(recent.begin(), recent.end(), q->getTarget()) != recent.end())
			continue;

		cand = q;

		if(cand->hasFreeSegments())
			break;
	}

	//check this again, if the first item we pick is running and there are no
	//other suitable items this will be true
	if((cand != NULL) && (!cand->hasFreeSegments())) {
		cand = NULL;
	}

	return cand;
}

QueueItem* QueueManager::FileQueue::findAutoSearch(deque<string>& recent) {
	// We pick a start position at random, hoping that we will find something to search for...
	QueueItem::StringMap::size_type start = (QueueItem::StringMap::size_type)Util::rand((uint32_t)queue.size());

	QueueItem::StringIter i = queue.begin();
	advance(i, start);

	QueueItem* cand = findCandidate(i, queue.end(), recent);
	if(cand == NULL) {
		cand = findCandidate(queue.begin(), i, recent);
	} else if(!cand->hasFreeSegments()) {
		QueueItem* cand2 = findCandidate(queue.begin(), i, recent);
		if(cand2 != NULL && cand2->hasFreeSegments()) {
			cand = cand2;
		}
	}
	return cand;
}

void QueueManager::FileQueue::move(QueueItem* qi, const string& aTarget) {
	//if(lastInsert != queue.end() && Util::stricmp(*lastInsert->first, qi->getTarget()) == 0)
	//	lastInsert = queue.end();
	queue.erase(const_cast<string*>(&qi->getTarget()));
	qi->setTarget(aTarget);
	add(qi);
}

void QueueManager::UserQueue::add(QueueItem* qi) {
	for(QueueItem::SourceConstIter i = qi->getSources().begin(); i != qi->getSources().end(); ++i) {
		add(qi, i->getUser());
	}
}

void QueueManager::UserQueue::add(QueueItem* qi, const User::Ptr& aUser) {
	if(!qi->isSet(QueueItem::FLAG_MULTI_SOURCE) && (qi->getStatus() == QueueItem::STATUS_RUNNING)) {
		return;
	}

	QueueItem::List& l = userQueue[qi->getPriority()][aUser];
	if(qi->isSet(QueueItem::FLAG_EXISTS)) {
		l.push_front(qi);
		//l.insert(l.begin(), qi);
	} else {
		l.push_back(qi);
	}
}

QueueItem* QueueManager::UserQueue::getNextAll(const User::Ptr& aUser, QueueItem::Priority minPrio) {
	int p = QueueItem::LAST - 1;

	do {
		QueueItem::UserListIter i = userQueue[p].find(aUser);
		if(i != userQueue[p].end()) {
			dcassert(!i->second.empty());
			return i->second.front();
		}
		p--;
	} while(p >= minPrio);

	return NULL;
}

QueueItem* QueueManager::UserQueue::getNext(const User::Ptr& aUser, QueueItem::Priority minPrio, QueueItem* pNext /* = NULL */) {
	int p = QueueItem::LAST - 1;
	bool fNext = false;

	do {
		QueueItem::UserListIter i = userQueue[p].find(aUser);
		if(i != userQueue[p].end()) {
			dcassert(!i->second.empty());
			QueueItem* found = i->second.front();

			bool freeSegments = found->hasFreeSegments();

			if(freeSegments && (pNext == NULL || fNext)) {
				return found;
			}else{
				if(!freeSegments) {
					if(fNext || (pNext == NULL)) {
						pNext = found;
					}
				}

				QueueItem::Iter iQi = find(i->second.begin(), i->second.end(), pNext);

				while(iQi != i->second.end()) {
	                fNext = true;   // found, next is target
	
					iQi++;
					if((iQi != i->second.end()) && (*iQi)->hasFreeSegments()) {
						return *iQi;
					}
				}
			}
		}
		p--;
	} while(p >= minPrio);

	return NULL;
}

void QueueManager::UserQueue::setRunning(QueueItem* qi, const User::Ptr& aUser) {

	dcassert(qi->isSource(aUser));
	if(qi->isSet(QueueItem::FLAG_MULTI_SOURCE)) {
		QueueItem::UserListMap& ulm = userQueue[qi->getPriority()];
		QueueItem::UserListMap::iterator j = ulm.find(aUser);
		dcassert(j != ulm.end());
		QueueItem::List& l = j->second;
		dcassert(find(l.begin(), l.end(), qi) != l.end());
		l.erase(find(l.begin(), l.end(), qi));
	
		if(l.empty()) {
			ulm.erase(j);
		}
	} else {
		// Remove the download from the userQueue...
		remove(qi);
	}

	// Set the flag to running...
	qi->setStatus(QueueItem::STATUS_RUNNING);
    qi->addCurrent(aUser);
    
	// Move the download to the running list...
	dcassert(running.find(aUser) == running.end());
	running[aUser] = qi;
}

void QueueManager::UserQueue::setWaiting(QueueItem* qi, const User::Ptr& aUser) {
	// This might have been set to wait by removesource already...
	if (running.find(aUser) == running.end()) {
		QueueItem::UserListMap& ulm = userQueue[qi->getPriority()];
		QueueItem::UserListIter j = ulm.find(aUser);
		if((j == ulm.end()) && qi->isSource(aUser))
			add(qi, aUser);
		return;
	}

	dcassert(qi->getStatus() == QueueItem::STATUS_RUNNING);

	// Remove the download from running
	running.erase(aUser);
	qi->removeCurrent(qi->isSet(QueueItem::FLAG_MULTI_SOURCE) ? aUser : qi->getCurrents()[0]);

	// Set flag to waiting
	if(qi->getCurrents().empty() || !qi->isSet(QueueItem::FLAG_MULTI_SOURCE)){
		qi->setStatus(QueueItem::STATUS_WAITING);
		qi->setAverageSpeed(0);
	}
	qi->setCurrentDownload(0);	

   	// Add to the userQueue
	if(qi->isSet(QueueItem::FLAG_MULTI_SOURCE)) {
		add(qi, aUser);
	} else {
		add(qi);
	}
}

QueueItem* QueueManager::UserQueue::getRunning(const User::Ptr& aUser) {
	QueueItem::UserIter i = running.find(aUser);
	return (i == running.end()) ? 0 : i->second;
}

void QueueManager::UserQueue::remove(QueueItem* qi) {
	for(QueueItem::SourceConstIter i = qi->getSources().begin(); i != qi->getSources().end(); ++i) {
		remove(qi, i->getUser());
	}
}

void QueueManager::UserQueue::remove(QueueItem* qi, const User::Ptr& aUser) {
	bool isCurrent = qi->isCurrent(aUser);
	if((!qi->isSet(QueueItem::FLAG_MULTI_SOURCE) && (qi->getStatus() == QueueItem::STATUS_RUNNING)) || isCurrent){
		if(isCurrent) {
			// Remove from running...
			dcassert(running.find(aUser) != running.end());
			running.erase(aUser);
		
			if(qi->isSet(QueueItem::FLAG_MULTI_SOURCE)) {
				qi->removeCurrent(aUser);

				if(qi->getCurrents().empty())
					qi->setStatus(QueueItem::STATUS_WAITING);
			}
		}
	} else {
		dcassert(qi->isSource(aUser));
		QueueItem::UserListMap& ulm = userQueue[qi->getPriority()];
		QueueItem::UserListMap::iterator j = ulm.find(aUser);
		dcassert(j != ulm.end());
		QueueItem::List& l = j->second;
		dcassert(find(l.begin(), l.end(), qi) != l.end());
		l.erase(std::remove(l.begin(), l.end(), qi), l.end());

		if(l.empty()) {
			ulm.erase(j);
		}
	}
}

QueueManager::QueueManager() : lastSave(0), queueFile(Util::getConfigPath() + "Queue.xml"), dirty(true), nextSearch(0) { 
	TimerManager::getInstance()->addListener(this); 
	SearchManager::getInstance()->addListener(this);
	ClientManager::getInstance()->addListener(this);

	File::ensureDirectory(Util::getListPath());
}

QueueManager::~QueueManager() throw() { 
	SearchManager::getInstance()->removeListener(this);
	TimerManager::getInstance()->removeListener(this); 
	ClientManager::getInstance()->removeListener(this);

	saveQueue();

	if(!BOOLSETTING(KEEP_LISTS)) {
		string path = Util::getListPath();

#ifdef _WIN32
		WIN32_FIND_DATA data;
		HANDLE hFind;
		
		hFind = FindFirstFile(Text::toT(path + "\\*.xml.bz2").c_str(), &data);
		if(hFind != INVALID_HANDLE_VALUE) {
			do {
				File::deleteFile(path + Text::fromT(data.cFileName));			
			} while(FindNextFile(hFind, &data));
			
			FindClose(hFind);
		}
		
		hFind = FindFirstFile(Text::toT(path + "\\*.DcLst").c_str(), &data);
		if(hFind != INVALID_HANDLE_VALUE) {
			do {
				File::deleteFile(path + Text::fromT(data.cFileName));			
			} while(FindNextFile(hFind, &data));
			
			FindClose(hFind);
		}

#else
		DIR* dir = opendir(path.c_str());
		if (dir) {
			while (struct dirent* ent = readdir(dir)) {
				if (fnmatch("*.xml.bz2", ent->d_name, 0) == 0 ||
					fnmatch("*.DcLst", ent->d_name, 0) == 0) {
					File::deleteFile(path + ent->d_name);	
				}
			}
			closedir(dir);
		}
#endif
	}
}

bool QueueManager::getTTH(const string& name, TTHValue& tth) throw() {
	Lock l(cs);
	QueueItem* qi = fileQueue.find(name);
	if(qi) {
		tth = qi->getTTH();
		return true;
	}
	return false;
}

void QueueManager::on(TimerManagerListener::Minute, uint32_t aTick) throw() {
	string searchString;
	bool online = false;

	{
		Lock l(cs);
		QueueItem::UserMap& um = userQueue.getRunning();

		for(QueueItem::UserIter j = um.begin(); j != um.end(); ++j) {
			QueueItem* q = j->second;
			if(!q->isSet(QueueItem::FLAG_MULTI_SOURCE)) {
				dcassert(q->getCurrentDownload() != NULL);
				q->setDownloadedBytes(q->getCurrentDownload()->getPos());
			}
			if(q->getAutoPriority()) {
				QueueItem::Priority p1 = q->getPriority();
				if(p1 != QueueItem::PAUSED) {
					QueueItem::Priority p2 = q->calculateAutoPriority();
					if(p1 != p2)
						setPriority(q->getTarget(), p2);
				}
			}
		}
		if(!um.empty())
			setDirty();

		if(BOOLSETTING(AUTO_SEARCH) && (aTick >= nextSearch) && (fileQueue.getSize() > 0)) {
			// We keep 30 recent searches to avoid duplicate searches
			while((recent.size() >= fileQueue.getSize()) || (recent.size() > 30)) {
				recent.pop_front();
			}

			QueueItem* qi;
			while((qi = fileQueue.findAutoSearch(recent)) == NULL && !recent.empty()) { // TEST how does this work
				recent.pop_front();
			}
			if(qi != NULL) {
				searchString = qi->getTTH().toBase32();
				online = qi->hasOnlineUsers();
				recent.push_back(qi->getTarget());
				nextSearch = aTick + (SETTING(SEARCH_TIME) * (online ? 24000 : 60000));
				if(BOOLSETTING(REPORT_ALTERNATES))
					LogManager::getInstance()->message(CSTRING(ALTERNATES_SEND) + Util::getFileName(qi->getTargetFileName()));		
			}
		}
	}

	if(!searchString.empty()) {
		SearchManager::getInstance()->search(searchString, 0, SearchManager::TYPE_TTH, SearchManager::SIZE_DONTCARE, "auto");
	}
}

void QueueManager::addList(const User::Ptr& aUser, int aFlags) throw(QueueException, FileException) {
	string target = Util::getListPath() + Util::validateFileName(Util::cleanPathChars(aUser->getFirstNick())) + "." + aUser->getCID().toBase32();

	add(target, -1, TTHValue(), aUser, QueueItem::FLAG_USER_LIST | aFlags);
}

void QueueManager::addPfs(const User::Ptr& aUser, const string& aDir) throw(QueueException) {
	if(aUser == ClientManager::getInstance()->getMe()) {
		throw QueueException(STRING(NO_DOWNLOADS_FROM_SELF));
	}

	if(!aUser->isOnline())
		return;

	{
		Lock l(cs);
		pair<PfsIter, PfsIter> range = pfsQueue.equal_range(aUser->getCID());
		if(find_if(range.first, range.second, CompareSecond<CID, string>(aDir)) == range.second) {
			pfsQueue.insert(make_pair(aUser->getCID(), aDir));
		}
	}

	ConnectionManager::getInstance()->getDownloadConnection(aUser);
}

void QueueManager::add(const string& aTarget, int64_t aSize, const TTHValue& root, User::Ptr aUser,
					   int aFlags /* = QueueItem::FLAG_RESUME */, bool addBad /* = true */) throw(QueueException, FileException)
{
	bool wantConnection = true;
	bool newItem = false;

	// Check that we're not downloading from ourselves...
	if(aUser == ClientManager::getInstance()->getMe()) {
		throw QueueException(STRING(NO_DOWNLOADS_FROM_SELF));
	}

	// Check if we're not downloading something already in our share
	if (BOOLSETTING(DONT_DL_ALREADY_SHARED)){
		if (ShareManager::getInstance()->isTTHShared(root)){
			throw QueueException(STRING(TTH_ALREADY_SHARED));
		}
	}
    
	string target = checkTarget(aTarget, aSize, aFlags);

	// Check if it's a zero-byte file, if so, create and return...
	if(aSize == 0) {
		if(!BOOLSETTING(SKIP_ZERO_BYTE)) {
			File::ensureDirectory(target);
			File f(target, File::WRITE, File::CREATE);
		}
		return;
	}
	
	if(aUser->isSet(User::PASSIVE) && !ClientManager::getInstance()->isActive(ClientManager::getInstance()->getHubUrl(aUser))) {
		throw QueueException(STRING(NO_DOWNLOADS_FROM_PASSIVE));
	}
	
	{
		Lock l(cs);

		QueueItem* q = fileQueue.find(target);
		
		if(q == NULL && (aSize > 2097153) ){
			QueueItem::List ql;
			fileQueue.find(ql, root);
			if(!ql.empty()){
				dcassert(ql.size() == 1);
				q = ql[0];
			}
		}
				
		if(q == NULL) {
			aFlags |= BOOLSETTING(MULTI_CHUNK) ? QueueItem::FLAG_MULTI_SOURCE : 0;
			q = fileQueue.add(target, aSize, aFlags, QueueItem::DEFAULT, Util::emptyString, 0, GET_TIME(), Util::emptyString, Util::emptyString, root);
			fire(QueueManagerListener::Added(), q);

			newItem = !q->isSet(QueueItem::FLAG_USER_LIST) && !q->isSet(QueueItem::FLAG_TESTSUR);
		} else {
			if(q->getSize() != aSize) {
				throw QueueException(STRING(FILE_WITH_DIFFERENT_SIZE));
			}
			if(!(root == q->getTTH())) {
				throw QueueException(STRING(FILE_WITH_DIFFERENT_TTH));
			}

			q->setFlag(aFlags);

			// We don't add any more sources to user list downloads...
			if(q->isSet(QueueItem::FLAG_USER_LIST))
				return;
		}

		wantConnection = addSource(q, aUser, addBad ? QueueItem::Source::FLAG_MASK : 0);
	}

	if(wantConnection && aUser->isOnline())
		ConnectionManager::getInstance()->getDownloadConnection(aUser);

	// auto search, prevent DEADLOCK
	if(newItem && BOOLSETTING(AUTO_SEARCH)){
		SearchManager::getInstance()->search(TTHValue(root).toBase32(), 0, SearchManager::TYPE_TTH, SearchManager::SIZE_DONTCARE, "auto");
	}
	
}

void QueueManager::readd(const string& target, User::Ptr& aUser) throw(QueueException) {
	bool wantConnection = false;
	{
		Lock l(cs);
		QueueItem* q = fileQueue.find(target);
		if(q != NULL && q->isBadSource(aUser)) {
			wantConnection = addSource(q, aUser, QueueItem::Source::FLAG_MASK);
		}
	}
	if(wantConnection && aUser->isOnline())
		ConnectionManager::getInstance()->getDownloadConnection(aUser);
}

string QueueManager::checkTarget(const string& aTarget, int64_t aSize, int& flags) throw(QueueException, FileException) {
#ifdef _WIN32
	if(aTarget.length() > MAX_PATH) {
		throw QueueException(STRING(TARGET_FILENAME_TOO_LONG));
	}
	// Check that target starts with a drive or is an UNC path
	if( (aTarget[1] != ':' || aTarget[2] != '\\') &&
		(aTarget[0] != '\\' && aTarget[1] != '\\') ) {
		throw QueueException(STRING(INVALID_TARGET_FILE));
	}
#else
	if(aTarget.length() > PATH_MAX) {
		throw QueueException(STRING(TARGET_FILENAME_TOO_LONG));
	}
	// Check that target contains at least one directory...we don't want headless files...
	if(aTarget[0] != '/') {
		throw QueueException(STRING(INVALID_TARGET_FILE));
	}
#endif

	string target = Util::validateFileName(aTarget);

	// Check that the file doesn't already exist...
	int64_t sz = File::getSize(target);
	if( (aSize != -1) && (aSize <= sz) )  {
		throw FileException(STRING(LARGER_TARGET_FILE_EXISTS));
	}
	if(sz > 0)
		flags |= QueueItem::FLAG_EXISTS;

	return target;
}

/** Add a source to an existing queue item */
bool QueueManager::addSource(QueueItem* qi, User::Ptr aUser, Flags::MaskType addBad) throw(QueueException, FileException) {
	bool wantConnection = (qi->getPriority() != QueueItem::PAUSED) && qi->hasFreeSegments();

	if(qi->isSource(aUser)) {
		throw QueueException(STRING(DUPLICATE_SOURCE));
	}

	if(qi->isBadSourceExcept(aUser, addBad)) {
		throw QueueException(STRING(DUPLICATE_SOURCE));
	}

	qi->addSource(aUser);

	if(aUser->isSet(User::PASSIVE) && !ClientManager::getInstance()->isActive(ClientManager::getInstance()->getHubUrl(aUser))) {
		qi->removeSource(aUser, QueueItem::Source::FLAG_PASSIVE);
		wantConnection = false;
	} else {
		if ((!SETTING(SOURCEFILE).empty()) && (!BOOLSETTING(SOUNDS_DISABLED)))
			PlaySound(Text::toT(SETTING(SOURCEFILE)).c_str(), NULL, SND_FILENAME | SND_ASYNC);
		userQueue.add(qi, aUser);
	}

	fire(QueueManagerListener::SourcesUpdated(), qi);
	setDirty();

	return wantConnection;
}

void QueueManager::addDirectory(const string& aDir, const User::Ptr& aUser, const string& aTarget, QueueItem::Priority p /* = QueueItem::DEFAULT */) throw() {
	bool needList;
	{
		Lock l(cs);
		
		DirectoryItem::DirectoryPair dp = directories.equal_range(aUser);
		
		for(DirectoryItem::DirectoryIter i = dp.first; i != dp.second; ++i) {
			if(Util::stricmp(aTarget.c_str(), i->second->getName().c_str()) == 0)
				return;
		}
		
		// Unique directory, fine...
		directories.insert(make_pair(aUser, new DirectoryItem(aUser, aDir, aTarget, p)));
		needList = (dp.first == dp.second);
		setDirty();
	}

	if(needList) {
		try {
			addList(aUser, QueueItem::FLAG_DIRECTORY_DOWNLOAD);
		} catch(const Exception&) {
			// Ignore, we don't really care...
		}
	}
}

QueueItem::Priority QueueManager::hasDownload(const User::Ptr& aUser) throw() {
	Lock l(cs);
	if(pfsQueue.find(aUser->getCID()) != pfsQueue.end()) {
		return QueueItem::HIGHEST;
	}
	QueueItem* qi = userQueue.getNext(aUser, QueueItem::LOWEST);
	if(!qi) {
		return QueueItem::PAUSED;
	}
	return qi->getPriority();
}
namespace {
typedef HASH_MAP_X(TTHValue, const DirectoryListing::File*, TTHValue::Hash, equal_to<TTHValue>, less<TTHValue>) TTHMap;

// *** WARNING *** 
// Lock(cs) makes sure that there's only one thread accessing this
static TTHMap tthMap;

void buildMap(const DirectoryListing::Directory* dir) throw() {
	for(DirectoryListing::Directory::List::const_iterator j = dir->directories.begin(); j != dir->directories.end(); ++j) {
		if(!(*j)->getAdls())
			buildMap(*j);
	}

	for(DirectoryListing::File::List::const_iterator i = dir->files.begin(); i != dir->files.end(); ++i) {
		const DirectoryListing::File* df = *i;
		tthMap.insert(make_pair(df->getTTH(), df));
	}
}
}
int QueueManager::matchListing(const DirectoryListing& dl) throw() {
	int matches = 0;
	{
		Lock l(cs);
		tthMap.clear();
		buildMap(dl.getRoot());

		for(QueueItem::StringMap::const_iterator i = fileQueue.getQueue().begin(); i != fileQueue.getQueue().end(); ++i) {
			QueueItem* qi = i->second;
			if(qi->isSet(QueueItem::FLAG_USER_LIST))
				continue;
			TTHMap::iterator j = tthMap.find(qi->getTTH());
			if(j != tthMap.end() && i->second->getSize() == qi->getSize()) {
				try {
					addSource(qi, dl.getUser(), QueueItem::Source::FLAG_FILE_NOT_AVAILABLE);
				} catch(...) {
					// Ignore...
				}
				matches++;
			}
		}
	}
	if(matches > 0)
		ConnectionManager::getInstance()->getDownloadConnection(dl.getUser());
		return matches;
}

void QueueManager::move(const string& aSource, const string& aTarget) throw() {
	string target = Util::validateFileName(aTarget);
	if(aSource == target)
		return;

	bool delSource = false;

	Lock l(cs);
	QueueItem* qs = fileQueue.find(aSource);
	if(qs != NULL) {
		// Don't move running downloads
		if(qs->getStatus() == QueueItem::STATUS_RUNNING) {
			return;
		}
		// Don't move file lists
		if(qs->isSet(QueueItem::FLAG_USER_LIST))
			return;

		// Let's see if the target exists...then things get complicated...
		QueueItem* qt = fileQueue.find(target);
		if(qt == NULL || Util::stricmp(aSource, target) == 0) {
			// Good, update the target and move in the queue...
			fileQueue.move(qs, target);
			fire(QueueManagerListener::Moved(), qs, aSource);
			setDirty();
		} else {
			// Don't move to target of different size
			if(qs->getSize() != qt->getSize())
				return;

			try {
				for(QueueItem::SourceConstIter i = qs->getSources().begin(); i != qs->getSources().end(); ++i) {
					addSource(qt, i->getUser(), QueueItem::Source::FLAG_MASK);
				}
			} catch(const Exception&) {
			}
			delSource = true;
		}
	}

	if(delSource) {
		remove(aSource);
	}
}

bool QueueManager::getQueueInfo(User::Ptr& aUser, string& aTarget, int64_t& aSize, int& aFlags, bool& aFileList, bool& aSegmented) throw() {
    Lock l(cs);
    QueueItem* qi = userQueue.getNextAll(aUser);
	if(qi == NULL)
		return false;

	aTarget = qi->getTarget();
	aSize = qi->getSize();
	aFlags = qi->getFlags();
	aFileList = qi->isSet(QueueItem::FLAG_USER_LIST) || qi->isSet(QueueItem::FLAG_TESTSUR);
	aSegmented = qi->isSet(QueueItem::FLAG_MULTI_SOURCE);

	return true;
}

uint8_t QueueManager::FileQueue::getMaxSegments(int64_t filesize) {
	uint8_t MaxSegments = 1;

	if(BOOLSETTING(SEGMENTS_MANUAL)) {
		MaxSegments = min((uint8_t)SETTING(NUMBER_OF_SEGMENTS), (uint8_t)10);
	} else {
		if((filesize >= 2*1048576) && (filesize < 15*1048576)) {
			MaxSegments = 2;
		} else if((filesize >= (int64_t)15*1048576) && (filesize < (int64_t)30*1048576)) {
			MaxSegments = 3;
		} else if((filesize >= (int64_t)30*1048576) && (filesize < (int64_t)60*1048576)) {
			MaxSegments = 4;
		} else if((filesize >= (int64_t)60*1048576) && (filesize < (int64_t)120*1048576)) {
			MaxSegments = 5;
		} else if((filesize >= (int64_t)120*1048576) && (filesize < (int64_t)240*1048576)) {
			MaxSegments = 6;
		} else if((filesize >= (int64_t)240*1048576) && (filesize < (int64_t)480*1048576)) {
			MaxSegments = 7;
		} else if((filesize >= (int64_t)480*1048576) && (filesize < (int64_t)960*1048576)) {
			MaxSegments = 8;
		} else if((filesize >= (int64_t)960*1048576) && (filesize < (int64_t)1920*1048576)) {
			MaxSegments = 9;
		} else if(filesize >= (int64_t)1920*1048576) {
			MaxSegments = 10;
		}
	}

#ifdef _DEBUG
	return 200;
#else
	return MaxSegments;
#endif
}

void QueueManager::getTargets(const TTHValue& tth, StringList& sl) {
	Lock l(cs);
	QueueItem::List ql;
	fileQueue.find(ql, tth);
	for(QueueItem::Iter i = ql.begin(); i != ql.end(); ++i) {
		sl.push_back((*i)->getTarget());
	}
}

Download* QueueManager::getDownload(UserConnection& aSource, string& message) throw() {
	Lock l(cs);

	User::Ptr& aUser = aSource.getUser();
	// First check PFS's...
	PfsIter pi = pfsQueue.find(aUser->getCID());
	if(pi != pfsQueue.end()) {
		Download* d = new Download(aSource);
		d->setFlag(Download::FLAG_PARTIAL_LIST);
		d->setSource(pi->second);
		return d;
	}

	QueueItem* q = userQueue.getNext(aUser);

again:

	if(!q)
		return 0;

	if((SETTING(FILE_SLOTS) != 0) && (q->getStatus() == QueueItem::STATUS_WAITING) && !q->isSet(QueueItem::FLAG_TESTSUR) &&
		!q->isSet(QueueItem::FLAG_USER_LIST) && (getRunningFiles().size() >= (size_t)SETTING(FILE_SLOTS))) {
		message = STRING(ALL_FILE_SLOTS_TAKEN);
		q = userQueue.getNext(aUser, QueueItem::LOWEST, q);
		goto again;
	}
	
	int64_t freeBlock = 0;

	QueueItem::SourceConstIter source = q->getSource(aUser);
	bool useChunks = true;
	if(q->isSet(QueueItem::FLAG_MULTI_SOURCE) && q->chunkInfo) {
		if(source->isSet(QueueItem::Source::FLAG_PARTIAL)) {
			freeBlock = q->chunkInfo->getChunk(source->getPartialInfo(), aUser->getLastDownloadSpeed());
		} else {
			freeBlock = q->chunkInfo->getChunk(useChunks, aUser->getLastDownloadSpeed());
		}

		if(freeBlock < 0) {
			if(freeBlock == -2) {
				dcassert(source->isSet(QueueItem::Source::FLAG_PARTIAL));
				userQueue.remove(q, aUser);
				q->removeSource(aUser, QueueItem::Source::FLAG_NO_NEED_PARTS);
				message = STRING(NO_NEEDED_PART);
			} else {
				message = STRING(NO_FREE_BLOCK);
			}
			
			q = userQueue.getNext(aUser, QueueItem::LOWEST, q);
			goto again;
		}
	}

	userQueue.setRunning(q, aUser);

	Download* d = new Download(aSource, *q, source);
	
	if(d->getSize() != -1) {
		if(HashManager::getInstance()->getTree(d->getTTH(), d->getTigerTree())) {
			d->setTreeValid(true);
		} else if(aSource.isSet(UserConnection::FLAG_SUPPORTS_TTHL) && !source->isSet(QueueItem::Source::FLAG_NO_TREE) && d->getSize() > HashManager::MIN_BLOCK_SIZE) {
			// Get the tree unless the file is small (for small files, we'd probably only get the root anyway)
			d->setFlag(Download::FLAG_TREE_DOWNLOAD);
			d->getTigerTree().setFileSize(d->getSize());
			d->setPos(0);
			d->setSize(-1);
			d->unsetFlag(Download::FLAG_RESUME);
			
			if(q->chunkInfo) {
				q->chunkInfo->putChunk(freeBlock);
			}
		} else if (!q->isSet(QueueItem::FLAG_MULTI_SOURCE)) {
			// Use the root as tree to get some sort of validation at least...
			d->getTigerTree() = TigerTree(d->getSize(), d->getSize(), d->getTTH());
			d->setTreeValid(true);
		}
	}

	if(q->isSet(QueueItem::FLAG_MULTI_SOURCE) && !d->isSet(Download::FLAG_TREE_DOWNLOAD)) {
		bool supportsChunks = !aSource.isSet(UserConnection::FLAG_STEALTH) && (aSource.isSet(UserConnection::FLAG_SUPPORTS_ADCGET) || aSource.isSet(UserConnection::FLAG_SUPPORTS_GETZBLOCK) || aSource.isSet(UserConnection::FLAG_SUPPORTS_XML_BZLIST));
		d->setStartPos(freeBlock);
		q->chunkInfo->setDownload(freeBlock, d, supportsChunks && useChunks);
	} else {
		if(!d->isSet(Download::FLAG_TREE_DOWNLOAD) && BOOLSETTING(ANTI_FRAG) ) {
			d->setStartPos(q->getDownloadedBytes());
		}		
		q->setCurrentDownload(d);
	}

	fire(QueueManagerListener::StatusUpdated(), q);
	return d;
}

void QueueManager::putDownload(Download* aDownload, bool finished, bool connectSources /* = true */) throw() {
	User::List getConn;
	string fname;
	User::Ptr up;
	int flag = 0;
	bool checkList = false;
	User::Ptr user;
	TTHValue* aTTH = aDownload->isSet(Download::FLAG_MULTI_CHUNK) ? new TTHValue(aDownload->getTTH().toBase32()) : NULL;

	{
		Lock l(cs);

		if(aDownload->isSet(Download::FLAG_PARTIAL_LIST)) {
			pair<PfsIter, PfsIter> range = pfsQueue.equal_range(aDownload->getUser()->getCID());
			PfsIter i = find_if(range.first, range.second, CompareSecond<CID, string>(aDownload->getSource()));
			if(i != range.second) {
				pfsQueue.erase(i);
				fire(QueueManagerListener::PartialList(), aDownload->getUser(), aDownload->getPFS());
			}
		} else {
			QueueItem* q = fileQueue.find(aDownload->getTarget());

			if(q) {
				if(aDownload->isSet(Download::FLAG_USER_LIST)) {
					if(aDownload->getSource() == Transfer::USER_LIST_NAME_BZ) {
						q->setFlag(QueueItem::FLAG_XML_BZLIST);
					} else {
						q->unsetFlag(QueueItem::FLAG_XML_BZLIST);
					}
				}

				if(finished) {
					if(aDownload->isSet(Download::FLAG_TREE_DOWNLOAD)) {
						// Got a full tree, now add it to the HashManager
						dcassert(aDownload->getTreeValid());
						HashManager::getInstance()->addTree(aDownload->getTigerTree());

						if(q->getStatus() == QueueItem::STATUS_RUNNING) {
							userQueue.setWaiting(q, aDownload->getUser());
							fire(QueueManagerListener::StatusUpdated(), q);
						}
					} else {
						// Now, let's see if this was a directory download filelist...
						if( (q->isSet(QueueItem::FLAG_DIRECTORY_DOWNLOAD) && directories.find(q->getCurrents()[0]) != directories.end()) ||
							(q->isSet(QueueItem::FLAG_MATCH_QUEUE)) ) 
						{
							fname = q->getListName();
							up = q->getCurrents()[0];
							flag = (q->isSet(QueueItem::FLAG_DIRECTORY_DOWNLOAD) ? QueueItem::FLAG_DIRECTORY_DOWNLOAD : 0)
								| (q->isSet(QueueItem::FLAG_MATCH_QUEUE) ? QueueItem::FLAG_MATCH_QUEUE : 0);
						} 

						fire(QueueManagerListener::Finished(), q, aDownload->getAverageSpeed());
						fire(QueueManagerListener::Removed(), q);

						userQueue.remove(q);
						fileQueue.remove(q);
						setDirty();
					}
				} else {
					if(!aDownload->isSet(Download::FLAG_TREE_DOWNLOAD)) {
						if(!q->isSet(QueueItem::FLAG_MULTI_SOURCE))
							q->setDownloadedBytes(aDownload->getPos());
	
						if(q->getDownloadedBytes() > 0) {
							q->setFlag(QueueItem::FLAG_EXISTS);
						} else {
							q->setTempTarget(Util::emptyString);
						}
						if(q->isSet(QueueItem::FLAG_USER_LIST)) {
							// Blah...no use keeping an unfinished file list...
							File::deleteFile(q->getListName());
						}
					}

					if(/*(connectSources || (q->getCurrents().size() <= 2)) &&*/ (q->getPriority() != QueueItem::PAUSED)) {
						q->getOnlineUsers(getConn);
					}
	
					if(!q->isSet(QueueItem::FLAG_MULTI_SOURCE)) {
						// This might have been set to wait elsewhere already...
						if(q->getStatus() == QueueItem::STATUS_RUNNING) {
							userQueue.setWaiting(q, aDownload->getUser());
							fire(QueueManagerListener::StatusUpdated(), q);						
						}
					} else {
						userQueue.setWaiting(q, aDownload->getUser());
						if(q->getStatus() != QueueItem::STATUS_RUNNING) {
							fire(QueueManagerListener::StatusUpdated(), q);
						}
					}
				}
			} else if(!aDownload->isSet(Download::FLAG_TREE_DOWNLOAD)) {
				if(!aDownload->getTempTarget().empty() && (aDownload->isSet(Download::FLAG_USER_LIST) || aDownload->getTempTarget() != aDownload->getTarget())) {
					File::deleteFile(aDownload->getTempTarget() + Download::ANTI_FRAG_EXT);
					File::deleteFile(aDownload->getTempTarget());
				}
			}
		}
	
		if(aDownload->isSet(Download::FLAG_MULTI_CHUNK) && 
			!aDownload->isSet(Download::FLAG_TREE_DOWNLOAD)) {
			
			FileChunksInfo::Ptr fileChunks = FileChunksInfo::Get(aTTH);
			if(!(fileChunks == (FileChunksInfo*)NULL)){
				fileChunks->putChunk(aDownload->getStartPos());
				/*if(aDownload->getPos() > aDownload->getStartPos()) {
					fileChunks->verifyBlock(aDownload->getPos() - 1, aDownload->getTigerTree(), aDownload->getTempTarget());				
				}*/
			}
		}

		int64_t speed = aDownload->getAverageSpeed();
		if(speed > 0 && aDownload->getTotal() > 32768 && speed < 10485760){
			aDownload->getUser()->setLastDownloadSpeed((size_t)speed);
		}
		
		checkList = aDownload->isSet(Download::FLAG_CHECK_FILE_LIST) && aDownload->isSet(Download::FLAG_TESTSUR);
		user = aDownload->getUser();

		delete aDownload;
	}
	delete aTTH;

	for(User::Iter i = getConn.begin(); i != getConn.end(); ++i) {
		ConnectionManager::getInstance()->getDownloadConnection(*i);
	}

	if(!fname.empty()) {
		processList(fname, up, flag);
	}

	if(checkList) {
		try {
			QueueManager::getInstance()->addList(user, QueueItem::FLAG_CHECK_FILE_LIST);
		} catch(const Exception&) {}
	}
}

void QueueManager::processList(const string& name, User::Ptr& user, int flags) {
	DirectoryListing dirList(user);
	try {
		dirList.loadFile(name);
	} catch(const Exception&) {
		LogManager::getInstance()->message(STRING(UNABLE_TO_OPEN_FILELIST) + name);
		return;
	}

	if(flags & QueueItem::FLAG_DIRECTORY_DOWNLOAD) {
		DirectoryItem::List dl;
		{
			Lock l(cs);
			DirectoryItem::DirectoryPair dp = directories.equal_range(user);
			for(DirectoryItem::DirectoryIter i = dp.first; i != dp.second; ++i) {
				dl.push_back(i->second);
			}
			directories.erase(user);
		}

		for(DirectoryItem::Iter i = dl.begin(); i != dl.end(); ++i) {
			DirectoryItem* di = *i;
			dirList.download(di->getName(), di->getTarget(), false);
			delete di;
		}
	}
	if(flags & QueueItem::FLAG_MATCH_QUEUE) {
		const size_t BUF_SIZE = STRING(MATCHED_FILES).size() + 16;
		AutoArray<char> tmp(BUF_SIZE);
		snprintf(tmp, BUF_SIZE, CSTRING(MATCHED_FILES), matchListing(dirList));
		LogManager::getInstance()->message(user->getFirstNick() + ": " + string(tmp));			
	}
}

void QueueManager::remove(const string& aTarget) throw() {
	string x;

	{
		Lock l(cs);

		QueueItem* q = fileQueue.find(aTarget);
		if(!q)
			return;

		if(q->isSet(QueueItem::FLAG_DIRECTORY_DOWNLOAD)) {
			dcassert(q->getSources().size() == 1);
			DirectoryItem::DirectoryPair dp = directories.equal_range(q->getSources()[0].getUser());
			for(DirectoryItem::DirectoryIter i = dp.first; i != dp.second; ++i) {
				delete i->second;
			}
			directories.erase(q->getSources()[0].getUser());
		}

		string temptarget = q->getTempTarget();

		// For partial-share
		UploadManager::getInstance()->abortUpload(temptarget);

		if(q->getStatus() == QueueItem::STATUS_RUNNING) {
			x = q->getTarget();
		} else if(!q->getTempTarget().empty() && q->getTempTarget() != q->getTarget()) {
			File::deleteFile(q->getTempTarget() + Download::ANTI_FRAG_EXT);
			File::deleteFile(q->getTempTarget());
		}

		fire(QueueManagerListener::Removed(), q);

		userQueue.remove(q);
		fileQueue.remove(q);

		setDirty();
	}
	if(!x.empty()) {
		DownloadManager::getInstance()->abortDownload(x);
	}
}

void QueueManager::removeSource(const string& aTarget, User::Ptr& aUser, int reason, bool removeConn /* = true */) throw() {
	bool isRunning = false;
	bool removeCompletely = false;
	{
		Lock l(cs);
		QueueItem* q = fileQueue.find(aTarget);
		if(!q)
			return;

		if(!q->isSource(aUser))
			return;
	
		if(q->isSet(QueueItem::FLAG_USER_LIST)) {
			removeCompletely = true;
			goto endCheck;
		}

		if(reason == QueueItem::Source::FLAG_NO_TREE) {
			q->getSource(aUser)->setFlag(reason);
			return;
		}

		if(reason == QueueItem::Source::FLAG_CRC_WARN) {
			// Already flagged?
			QueueItem::SourceIter s = q->getSource(aUser);
			if(s->isSet(QueueItem::Source::FLAG_CRC_WARN)) {
				reason = QueueItem::Source::FLAG_CRC_FAILED;
			} else {
				s->setFlag(reason);
				return;
			}
		}

		if((q->getStatus() == QueueItem::STATUS_RUNNING) && q->isCurrent(aUser)) {
			isRunning = true;
			userQueue.setWaiting(q, aUser);
			fire(QueueManagerListener::StatusUpdated(), q);
		}

		userQueue.remove(q, aUser);
		q->removeSource(aUser, reason);
		
		fire(QueueManagerListener::SourcesUpdated(), q);
		setDirty();
	}
endCheck:
	if(isRunning && removeConn) {
		ConnectionManager::getInstance()->disconnect(aUser, true);
	}
	if(removeCompletely) {
		remove(aTarget);
	}	
}

void QueueManager::removeSource(User::Ptr& aUser, int reason) throw() {
	bool isRunning = false;
	string removeRunning;
	{
		Lock l(cs);
		QueueItem* qi = NULL;
		while( (qi = userQueue.getNextAll(aUser, QueueItem::PAUSED)) != NULL) {
			if(qi->isSet(QueueItem::FLAG_USER_LIST)) {
				remove(qi->getTarget());
			} else {
				userQueue.remove(qi, aUser);
				qi->removeSource(aUser, reason);
				fire(QueueManagerListener::SourcesUpdated(), qi);
				setDirty();
			}
		}
		
		qi = userQueue.getRunning(aUser);
		if(qi) {
			if(qi->isSet(QueueItem::FLAG_USER_LIST)) {
				removeRunning = qi->getTarget();
			} else {
				userQueue.setWaiting(qi, aUser);
				userQueue.remove(qi, aUser);
				isRunning = true;
				qi->removeSource(aUser, reason);
				fire(QueueManagerListener::StatusUpdated(), qi);
				fire(QueueManagerListener::SourcesUpdated(), qi);
				setDirty();
			}
		}
	}

	if(isRunning) {
		ConnectionManager::getInstance()->disconnect(aUser, true);
	}
	if(!removeRunning.empty()) {
		remove(removeRunning);
	}	
}

void QueueManager::setPriority(const string& aTarget, QueueItem::Priority p) throw() {
	User::List ul;
	bool running = false;

	{
		Lock l(cs);
	
		QueueItem* q = fileQueue.find(aTarget);
		if( (q != NULL) && (q->getPriority() != p) ) {
			if( q->getStatus() == QueueItem::STATUS_WAITING ) {
				if(q->getPriority() == QueueItem::PAUSED || p == QueueItem::HIGHEST) {
					// Problem, we have to request connections to all these users...
					q->getOnlineUsers(ul);
				}

				userQueue.remove(q);
				q->setPriority(p);
				userQueue.add(q);
			} else {
				running = true;
				if(q->isSet(QueueItem::FLAG_MULTI_SOURCE)) {
					for(QueueItem::SourceConstIter i = q->getSources().begin(); i != q->getSources().end(); ++i) {
						if(!q->isCurrent(i->getUser())) userQueue.remove(q, i->getUser());
					}
					q->setPriority(p);
					for(QueueItem::SourceConstIter i = q->getSources().begin(); i != q->getSources().end(); ++i) {
						if(!q->isCurrent(i->getUser())) userQueue.add(q, i->getUser());
					}
				} else {
					q->setPriority(p);
				}
			}
			setDirty();
			fire(QueueManagerListener::StatusUpdated(), q);
		}
	}

	if(p == QueueItem::PAUSED) {
		if(running)
			DownloadManager::getInstance()->abortDownload(aTarget);
	} else {
		for(User::Iter i = ul.begin(); i != ul.end(); ++i) {
			ConnectionManager::getInstance()->getDownloadConnection(*i);
		}
	}
}

void QueueManager::setAutoPriority(const string& aTarget, bool ap) throw() {
	User::List ul;

	{
		Lock l(cs);
	
		QueueItem* q = fileQueue.find(aTarget);
		if( (q != NULL) && (q->getAutoPriority() != ap) ) {
			q->setAutoPriority(ap);
			if(ap) {
				QueueManager::getInstance()->setPriority(q->getTarget(), q->calculateAutoPriority());
			}
			setDirty();
			fire(QueueManagerListener::StatusUpdated(), q);
		}
	}
}

void QueueManager::saveQueue() throw() {
	if(!dirty)
		return;
		
	Lock l(cs);	
		
	try {
		
		File ff(getQueueFile() + ".tmp", File::WRITE, File::CREATE | File::TRUNCATE);
		BufferedOutputStream<false> f(&ff);
		
		f.write(SimpleXML::utf8Header);
		f.write(LIT("<Downloads Version=\"" VERSIONSTRING "\">\r\n"));
		string tmp;
		string b32tmp;
		for(QueueItem::StringIter i = fileQueue.getQueue().begin(); i != fileQueue.getQueue().end(); ++i) {
			QueueItem* qi = i->second;
			if(!qi->isSet(QueueItem::FLAG_USER_LIST) && !qi->isSet(QueueItem::FLAG_TESTSUR)) {
				f.write(LIT("\t<Download Target=\""));
				f.write(SimpleXML::escape(qi->getTarget(), tmp, true));
				f.write(LIT("\" Size=\""));
				f.write(Util::toString(qi->getSize()));
				f.write(LIT("\" Priority=\""));
				f.write(Util::toString((int)qi->getPriority()));
				if(qi->isSet(QueueItem::FLAG_MULTI_SOURCE) && qi->chunkInfo) {
					f.write(LIT("\" FreeBlocks=\""));
					f.write(qi->chunkInfo->getFreeChunksString());
					f.write(LIT("\" VerifiedParts=\""));
					f.write(qi->chunkInfo->getVerifiedBlocksString());
				}
				f.write(LIT("\" Added=\""));
				f.write(Util::toString(qi->getAdded()));
				b32tmp.clear();
				f.write(LIT("\" TTH=\""));
				f.write(qi->getTTH().toBase32(b32tmp));
				if(qi->getDownloadedBytes() > 0) {
					f.write(LIT("\" TempTarget=\""));
					f.write(SimpleXML::escape(qi->getTempTarget(), tmp, true));
					f.write(LIT("\" Downloaded=\""));
					f.write(Util::toString(qi->getDownloadedBytes()));
				}
				f.write(LIT("\" AutoPriority=\""));
				f.write(Util::toString(qi->getAutoPriority()));
				if(qi->isSet(QueueItem::FLAG_MULTI_SOURCE)) {
					f.write(LIT("\" MaxSegments=\""));
					f.write(Util::toString(qi->getMaxSegments()));
				}

				f.write(LIT("\">\r\n"));

				for(QueueItem::SourceConstIter j = qi->sources.begin(); j != qi->sources.end(); ++j) {
					if(j->isSet(QueueItem::Source::FLAG_PARTIAL)) continue;
					f.write(LIT("\t\t<Source CID=\""));
					f.write(j->getUser()->getCID().toBase32());
					f.write(LIT("\" Nick=\""));
					f.write(SimpleXML::escape(j->getUser()->getFirstNick(), tmp, true));
					f.write(LIT("\"/>\r\n"));
				}

				f.write(LIT("\t</Download>\r\n"));
			}
		}
		
		f.write("</Downloads>\r\n");
		f.flush();
		ff.close();

		File::deleteFile(getQueueFile() + ".bak");
		CopyFile(Text::toT(getQueueFile()).c_str(), Text::toT(getQueueFile() + ".bak").c_str(), FALSE);
		File::deleteFile(getQueueFile());
		File::renameFile(getQueueFile() + ".tmp", getQueueFile());

		dirty = false;
	} catch(const FileException&) {
		// ...
	}
	// Put this here to avoid very many saves tries when disk is full...
	lastSave = GET_TICK();
}

class QueueLoader : public SimpleXMLReader::CallBack {
public:
	QueueLoader() : cur(NULL), inDownloads(false) { }
	virtual ~QueueLoader() { }
	virtual void startTag(const string& name, StringPairList& attribs, bool simple);
	virtual void endTag(const string& name, const string& data);
private:
	string target;

	QueueItem* cur;
	bool inDownloads;
};

void QueueManager::loadQueue() throw() {
	try {
		QueueLoader l;
		SimpleXMLReader(&l).fromXML(File(getQueueFile(), File::READ, File::OPEN).read());
		dirty = false;
	} catch(const Exception&) {
		// ...
	}
}

static const string sDownload = "Download";
static const string sTempTarget = "TempTarget";
static const string sTarget = "Target";
static const string sSize = "Size";
static const string sDownloaded = "Downloaded";
static const string sPriority = "Priority";
static const string sSource = "Source";
static const string sNick = "Nick";
static const string sDirectory = "Directory";
static const string sAdded = "Added";
static const string sTTH = "TTH";
static const string sCID = "CID";
static const string sFreeBlocks = "FreeBlocks";
static const string sVerifiedBlocks = "VerifiedParts";
static const string sAutoPriority = "AutoPriority";
static const string sMaxSegments = "MaxSegments";

void QueueLoader::startTag(const string& name, StringPairList& attribs, bool simple) {
	QueueManager* qm = QueueManager::getInstance();	
	if(!inDownloads && name == "Downloads") {
		inDownloads = true;
	} else if(inDownloads) {
		if(cur == NULL && name == sDownload) {
			int flags = QueueItem::FLAG_RESUME;
			int64_t size = Util::toInt64(getAttrib(attribs, sSize, 1));
			if(size == 0)
				return;
			try {
				const string& tgt = getAttrib(attribs, sTarget, 0);
				target = QueueManager::checkTarget(tgt, size, flags);
				if(target.empty())
					return;
			} catch(const Exception&) {
				return;
			}
			const string& freeBlocks = getAttrib(attribs, sFreeBlocks, 1);
			const string& verifiedBlocks = getAttrib(attribs, sVerifiedBlocks, 2);

			QueueItem::Priority p = (QueueItem::Priority)Util::toInt(getAttrib(attribs, sPriority, 3));
			time_t added = static_cast<time_t>(Util::toInt(getAttrib(attribs, sAdded, 4)));
			const string& tthRoot = getAttrib(attribs, sTTH, 5);
			if(tthRoot.empty())
				return;

			string tempTarget = getAttrib(attribs, sTempTarget, 5);
			int64_t downloaded = Util::toInt64(getAttrib(attribs, sDownloaded, 5));
			uint8_t maxSegments = (uint8_t)Util::toInt(getAttrib(attribs, sMaxSegments, 5));
			if (downloaded > size || downloaded < 0)
				downloaded = 0;

			if(added == 0)
				added = GET_TIME();

			QueueItem* qi = qm->fileQueue.find(target);

			if(qi == NULL) {
				if((maxSegments > 1) || !freeBlocks.empty()) {
					flags |= QueueItem::FLAG_MULTI_SOURCE;
				}
				qi = qm->fileQueue.add(target, size, flags, p, tempTarget, downloaded, added, freeBlocks, verifiedBlocks, TTHValue(tthRoot));

				bool ap = Util::toInt(getAttrib(attribs, sAutoPriority, 6)) == 1;
				qi->setAutoPriority(ap);
				qi->setMaxSegments(max((uint8_t)1, maxSegments));
				
				qm->fire(QueueManagerListener::Added(), qi);
			}
			if(!simple)
				cur = qi;
		} else if(cur != NULL && name == sSource) {
			const string& cid = getAttrib(attribs, sCID, 0);
			if(cid.length() != 39) {
				// Skip loading this source - sorry old users
				return;
			}
			User::Ptr user = ClientManager::getInstance()->getUser(CID(cid));
			const string& nick = getAttrib(attribs, sNick, 1);
			user->setFirstNick(nick);

			try {
				if(qm->addSource(cur, user, 0) && user->isOnline())
				ConnectionManager::getInstance()->getDownloadConnection(user);
			} catch(const Exception&) {
				return;
			}
		}
	}
}

void QueueLoader::endTag(const string& name, const string&) {
	if(inDownloads) {
		if(name == sDownload)
			cur = NULL;
		else if(name == "Downloads")
			inDownloads = false;
	}
}

// SearchManagerListener
void QueueManager::on(SearchManagerListener::SR, SearchResult* sr) throw() {
	bool added = false;
	bool wantConnection = false;
	int users = 0;

	if(BOOLSETTING(AUTO_SEARCH)) {
		Lock l(cs);
		QueueItem::List matches;

		fileQueue.find(matches, sr->getTTH());

		for(QueueItem::Iter i = matches.begin(); i != matches.end(); ++i) {
			QueueItem* qi = *i;

			// Size compare to avoid popular spoof
			if(qi->getSize() == sr->getSize() && !qi->isSource(sr->getUser())) {
				try {
					users = qi->countOnlineUsers();
					if(!BOOLSETTING(AUTO_SEARCH_AUTO_MATCH) || (users >= SETTING(MAX_AUTO_MATCH_SOURCES)))
						wantConnection = addSource(qi, sr->getUser(), 0);
					added = true;
				} catch(const Exception&) {
					// ...
				}
				break;
			}
		}
	}

	if(added && BOOLSETTING(AUTO_SEARCH_AUTO_MATCH) && (users < SETTING(MAX_AUTO_MATCH_SOURCES))) {
		try {
			addList(sr->getUser(), QueueItem::FLAG_MATCH_QUEUE);
		} catch(const Exception&) {
			// ...
		}
	}
	if(added && sr->getUser()->isOnline() && wantConnection)
		ConnectionManager::getInstance()->getDownloadConnection(sr->getUser());

}

// ClientManagerListener
void QueueManager::on(ClientManagerListener::UserConnected, const User::Ptr& aUser) throw() {
	bool hasDown = false;
	{
		Lock l(cs);
		for(int i = 0; i < QueueItem::LAST; ++i) {
			QueueItem::UserListIter j = userQueue.getList(i).find(aUser);
			if(j != userQueue.getList(i).end()) {
				for(QueueItem::Iter m = j->second.begin(); m != j->second.end(); ++m)
					fire(QueueManagerListener::StatusUpdated(), *m);
				if(i != QueueItem::PAUSED)
					hasDown = true;
			}
		}

		if(pfsQueue.find(aUser->getCID()) != pfsQueue.end()) {
			hasDown = true;
		}		
	}

	if(hasDown)	
		ConnectionManager::getInstance()->getDownloadConnection(aUser);
}

void QueueManager::on(ClientManagerListener::UserDisconnected, const User::Ptr& aUser) throw() {
	bool hasTestSURinQueue = false;
	{
		Lock l(cs);
		for(int i = 0; i < QueueItem::LAST; ++i) {
			QueueItem::UserListIter j = userQueue.getList(i).find(aUser);
			if(j != userQueue.getList(i).end()) {
				for(QueueItem::Iter m = j->second.begin(); m != j->second.end(); ++m) {
					if((*m)->isSet(QueueItem::FLAG_TESTSUR))  hasTestSURinQueue = true;
					fire(QueueManagerListener::StatusUpdated(), *m);
				}
			}
		}
	}
	
	if(hasTestSURinQueue)
		removeTestSUR(aUser);
}

void QueueManager::on(TimerManagerListener::Second, uint32_t aTick) throw() {
	if(dirty && ((lastSave + 10000) < aTick)) {
		saveQueue();
	}
	if(BOOLSETTING(REALTIME_QUEUE_UPDATE) && (Speaker<QueueManagerListener>::listeners.size() > 1)) {
		Lock l(cs);
		QueueItem::List um = getRunningFiles();
		for(QueueItem::Iter j = um.begin(); j != um.end(); ++j) {
			QueueItem* q = *j;
			if(!q->isSet(QueueItem::FLAG_MULTI_SOURCE)) {
				dcassert(q->getCurrentDownload() != NULL);
				q->setDownloadedBytes(q->getCurrentDownload()->getPos());
			}
			fire(QueueManagerListener::StatusUpdated(), q);
		}
	}
}

void QueueItem::addSource(const User::Ptr& aUser) {
	dcassert(!isSource(aUser));
	SourceIter i = getBadSource(aUser);
	if(i != badSources.end()) {
		sources.push_back(*i);
		badSources.erase(i);
	} else {
		sources.push_back(Source(aUser));
	}
}

void QueueItem::removeSource(const User::Ptr& aUser, int reason) {
	SourceIter i = getSource(aUser);
	dcassert(i != sources.end());
	i->setFlag(reason);
	badSources.push_back(*i);
	sources.erase(i);
}
	
bool QueueManager::add(const string& aFile, int64_t aSize, const string& tth) throw(QueueException, FileException) 
{	
	if(aFile == Transfer::USER_LIST_NAME_BZ || aFile == Transfer::USER_LIST_NAME) return false;
	if(aSize == 0) return false;

	string target = SETTING(DOWNLOAD_DIRECTORY) + aFile;
	string tempTarget = Util::emptyString;

	int flag = QueueItem::FLAG_RESUME;

	try {
		target = checkTarget(target, aSize, flag);
		if(target.empty()) return false;
	} catch(const Exception&) {
		return false;
	}
	
	{
		Lock l(cs);

		if(fileQueue.find(target)) return false;

		TTHValue root(tth);
		QueueItem::List ql;
		fileQueue.find(ql, root);
		if(ql.size()) return false;

		QueueItem* q = fileQueue.add(target, aSize, flag | (BOOLSETTING(MULTI_CHUNK) ? QueueItem::FLAG_MULTI_SOURCE : 0), QueueItem::DEFAULT, Util::emptyString, 0, GET_TIME(), Util::emptyString, Util::emptyString, root);
		fire(QueueManagerListener::Added(), q);
	}

	if(BOOLSETTING(AUTO_SEARCH)){
		SearchManager::getInstance()->search(tth, 0, SearchManager::TYPE_TTH, SearchManager::SIZE_DONTCARE, Util::emptyString);
	}

	return true;
}

bool QueueManager::dropSource(Download* d, bool autoDrop) {
	int iHighSpeed = SETTING(H_DOWN_SPEED);

	unsigned int activeSegments, onlineUsers;
	int64_t overallSpeed;
	User::Ptr aUser = d->getUser();

	{
	    Lock l(cs);

		QueueItem* q = userQueue.getRunning(aUser);

		if(!q) return false;

   		dcassert(q->isSource(aUser));

		if(autoDrop) {
			// Don't drop only downloading source
			if(q->getCurrents().size() < 2) return false;
			if((q->getAverageSpeed() > 0) && (2*d->getRunningAverage() > q->getAverageSpeed())) return false;

			aUser->setLastDownloadSpeed((size_t)d->getRunningAverage());

		    userQueue.setWaiting(q, aUser);
			userQueue.remove(q, aUser);

			q->removeSource(aUser, QueueItem::Source::FLAG_SLOW);

			fire(QueueManagerListener::SourcesUpdated(), q);
			setDirty();

			return true;
		}

		if(!q->isSet(QueueItem::FLAG_AUTODROP)) return true;

		activeSegments = q->getCurrents().size();
		onlineUsers = q->countOnlineUsers();
		overallSpeed = q->getAverageSpeed();
	}

	if(!SETTING(DROP_MULTISOURCE_ONLY) || (activeSegments >= 2)) {
		if((overallSpeed > (iHighSpeed*1024)) || (iHighSpeed == 0)) {
			if(onlineUsers > 2) {
				aUser->setLastDownloadSpeed((size_t)d->getRunningAverage());
				if(d->getRunningAverage() < SETTING(DISCONNECT)*1024) {
					removeSource(d->getTarget(), aUser, QueueItem::Source::FLAG_SLOW);
				} else {
					d->getUserConnection().disconnect();
				}
				return false;
			}
		}
	}
	return true;
}

bool QueueManager::handlePartialResult(const User::Ptr& aUser, const TTHValue& tth, PartsInfo& partialInfo, PartsInfo& outPartialInfo) {
	bool wantConnection = false;
	dcassert(outPartialInfo.empty());

	{
		Lock l(cs);

		// Locate target QueueItem in download queue
		QueueItem::List ql;
		fileQueue.find(ql, tth);

		if(ql.empty()){
			dcdebug("Not found in download queue\n");
			return false;
		}

		// Check min size
		QueueItem::Ptr qi = ql[0];
		if(qi->getSize() < PARTIAL_SHARE_MIN_SIZE){
			dcassert(0);
			return false;
		}

		// Get my parts info
		FileChunksInfo::Ptr chunksInfo = FileChunksInfo::Get(&qi->getTTH());
		if(!chunksInfo){
			return false;
		}
		chunksInfo->getPartialInfo(outPartialInfo);
		
		// Any parts for me?
		wantConnection = chunksInfo->isSource(partialInfo);

		// If this user isn't a source and has no parts needed, ignore it
		QueueItem::SourceIter si = qi->getSource(aUser);
		if(si == qi->getSources().end()){
			si = qi->getBadSource(aUser);

			if(si != qi->getBadSources().end() && si->isSet(QueueItem::Source::FLAG_TTH_INCONSISTENCY))
				return false;

			if(!wantConnection){
				if(si == qi->getBadSources().end())
					return false;
			}else{
				// add this user as partial file sharing source
				qi->addSource(aUser);
				si = qi->getSource(aUser);
				si->setFlag(QueueItem::Source::FLAG_PARTIAL);
				userQueue.add(qi, aUser);
				dcassert(si != qi->getSources().end());
				fire(QueueManagerListener::SourcesUpdated(), qi);
			}
		}

		// Update source's parts info
		si->setPartialInfo(partialInfo);
	}
	
	// Connect to this user
	if(wantConnection)
		ConnectionManager::getInstance()->getDownloadConnection(aUser);

	return true;
}

bool QueueManager::handlePartialSearch(const TTHValue& tth, PartsInfo& _outPartsInfo) {
	{
		Lock l(cs);

		// Locate target QueueItem in download queue
		QueueItem::List ql;
		fileQueue.find(ql, tth);

		if(ql.empty()){
			return false;
		}

		QueueItem::Ptr qi = ql[0];
		if(qi->getSize() < PARTIAL_SHARE_MIN_SIZE){
			return false;
		}

		FileChunksInfo::Ptr chunksInfo = FileChunksInfo::Get(&qi->getTTH());
		if(!chunksInfo){
			return false;
		}

		chunksInfo->getPartialInfo(_outPartsInfo);
	}

	return !_outPartsInfo.empty();
}
/**
 * @file
 * $Id$
 */
