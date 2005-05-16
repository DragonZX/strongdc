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

#if !defined(QUEUE_MANAGER_H)
#define QUEUE_MANAGER_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "TimerManager.h"

#include "CriticalSection.h"
#include "Exception.h"
#include "User.h"
#include "File.h"
#include "QueueItem.h"
#include "Singleton.h"
#include "DirectoryListing.h"
#include "MerkleTree.h"

#include "QueueManagerListener.h"
#include "SearchManagerListener.h"
#include "ClientManagerListener.h"
#include "LogManager.h"
#include "DownloadManager.h"

STANDARD_EXCEPTION(QueueException);

class UserConnection;

class DirectoryItem {
public:
	typedef DirectoryItem* Ptr;
	typedef HASH_MULTIMAP<User::Ptr, Ptr, User::HashFunction> DirectoryMap;
	typedef DirectoryMap::iterator DirectoryIter;
	typedef pair<DirectoryIter, DirectoryIter> DirectoryPair;
	
	typedef vector<Ptr> List;
	typedef List::iterator Iter;

	DirectoryItem() : priority(QueueItem::DEFAULT) { };
	DirectoryItem(const User::Ptr& aUser, const string& aName, const string& aTarget, 
		QueueItem::Priority p) : name(aName), target(aTarget), priority(p), user(aUser) { };
	~DirectoryItem() { };
	
	User::Ptr& getUser() { return user; };
	void setUser(const User::Ptr& aUser) { user = aUser; };
	
	GETSET(string, name, Name);
	GETSET(string, target, Target);
	GETSET(QueueItem::Priority, priority, Priority);
private:
	User::Ptr user;
};

class ConnectionQueueItem;
class QueueLoader;

class QueueManager : public Singleton<QueueManager>, public Speaker<QueueManagerListener>, private TimerManagerListener, 
	private SearchManagerListener, private ClientManagerListener
{
public:
	/** Add a file with hash to the queue. */
	bool add(const string& aFile, int64_t aSize, const string& tth) throw(QueueException, FileException); 
	/** Add a file to the queue. */
	void add(const string& aTarget, int64_t aSize, const TTHValue* root, User::Ptr aUser, const string& aSourceFile, 
		bool utf8 = true, int aFlags = QueueItem::FLAG_RESUME, bool addBad = true) throw(QueueException, FileException);
		/** Add a user's filelist to the queue. */
	void addList(const User::Ptr& aUser, int aFlags) throw(QueueException, FileException);
	/** Queue a partial file list download */
	void addPfs(const User::Ptr aUser, const string& aDir) throw();

	void addTestSUR(User::Ptr aUser, bool checkList = false) throw(QueueException, FileException) {
		string fileName = "TestSUR" + Util::validateFileName(aUser->getNick());
		string file = Util::getAppPath() + "TestSURs\\" + fileName;
		add(file, -1, NULL, aUser, fileName, true, (checkList ? QueueItem::FLAG_CHECK_FILE_LIST : 0) | QueueItem::FLAG_TESTSUR);
		aUser->setHasTestSURinQueue(true);
	}

	void removeTestSUR(User::Ptr aUser) {
		try {
			remove(Util::getAppPath() + "TestSURs\\TestSUR" + aUser->getNick());
			aUser->setHasTestSURinQueue(false);
		} catch(...) {
			// exception
		}
		return;
	}

	/** Readd a source that was removed */
	void readd(const string& target, User::Ptr& aUser) throw(QueueException);

	/** Add a directory to the queue (downloads filelist and matches the directory). */
	void addDirectory(const string& aDir, const User::Ptr& aUser, const string& aTarget, QueueItem::Priority p = QueueItem::DEFAULT) throw();
	
	int matchListing(const DirectoryListing& dl) throw();

	/** Move the target location of a queued item. Running items are silently ignored */
	void move(const string& aSource, const string& aTarget) throw();

	void remove(const string& aTarget) throw();
	void removeSource(const string& aTarget, User::Ptr aUser, int reason, bool removeConn = true) throw();
	void removeSource(User::Ptr aUser, int reason) throw();

	void setPriority(const string& aTarget, QueueItem::Priority p) throw();
	void setAutoPriority(const string& aTarget, bool ap) throw();

	void getTargetsBySize(StringList& sl, int64_t aSize, const string& suffix) throw();
	void getTargetsByRoot(StringList& sl, const TTHValue& tth);
	QueueItem::StringMap& lockQueue() throw() { cs.enter(); return fileQueue.getQueue(); } ;
	void unlockQueue() throw() { cs.leave(); };

	bool getQueueInfo(User::Ptr& aUser, string& aTarget, int64_t& aSize, int& aFlags) throw();
	Download* getDownload(User::Ptr& aUser, bool supportsTrees, bool supportsChunks, string &message, string aTarget = Util::emptyString) throw();
	void putDownload(Download* aDownload, bool finished, bool removeSegment = true) throw();

	bool hasDownload(const User::Ptr& aUser, QueueItem::Priority minPrio = QueueItem::LOWEST) throw() {
		Lock l(cs);
		return (pfsQueue.find(aUser->getCID()) != pfsQueue.end()) || (userQueue.getNext(aUser, minPrio) != NULL);
	}
	
	void loadQueue() throw();
	void saveQueue() throw();

	bool handlePartialSearch(const string& aSearchString, int64_t aSize, const TTHValue& tth, PartsInfo& _outPartsInfo);
	bool handlePartialResult(const User::Ptr& aUser, const TTHValue& tth, PartsInfo& partialInfo, PartsInfo& outPartialInfo);
	
	QueueItem* getRunning(const User::Ptr& aUser);
	bool setActiveSegment(const User::Ptr& aUser, bool& isAlreadyActive, u_int16_t& SegmentsCount);
	bool dropSource(Download* d, const User::Ptr& aUser);
	bool autoDropSource(const User::Ptr& aUser);
	int64_t setQueueItemSpeed(const User::Ptr& aUser, int64_t speed, u_int16_t& activeSegments);

	inline u_int16_t getRunningCount(const User::Ptr& aUser, const string& aTarget, bool currents, int64_t& size, QueueItem::Status& status) {
		u_int16_t value;
		{
			Lock l(cs);
			QueueItem* qi = currents ? fileQueue.find(aTarget) : userQueue.getRunning(aUser);
			size = -1;
			status = QueueItem::STATUS_WAITING;

			if(!qi)
				return 0;

			size = qi->getSize();
			value  = qi->getActiveSegments().size();
			status = qi->getStatus();
			if(currents) {
				if(find(qi->getActiveSegments().begin(), qi->getActiveSegments().end(), *qi->getSource(aUser)) != qi->getActiveSegments().end()) {
					value -= 1;
				}
			}
		}
		return value;
	}

	QueueItem::List getRunningFiles() throw() {
		QueueItem::List ql;
		for(QueueItem::StringIter i = fileQueue.getQueue().begin(); i != fileQueue.getQueue().end(); ++i) {
			QueueItem* q = i->second;
			if(q->getStatus() == QueueItem::STATUS_RUNNING) {
				ql.push_back(q);
			}
		}
		return ql;
	}

	bool getTargetByRoot(const TTHValue& tth, string& target, string& tempTarget) {
		Lock l(cs);
		QueueItem::List ql;
		fileQueue.find(ql, tth);

		if(ql.empty()) return false;

		target = ql.front()->getTarget();
		tempTarget = ql.front()->getTempTarget();
		return true;
		
	}
	
	GETSET(u_int32_t, lastSave, LastSave);
	GETSET(string, queueFile, QueueFile);

	typedef HASH_MAP_X(CID, string, CID::Hash, equal_to<CID>, less<CID>) PfsQueue;
	typedef PfsQueue::iterator PfsIter;

	/** All queue items by target */
	class FileQueue {
	public:
		FileQueue() : lastInsert(queue.end()) { };
		~FileQueue() {
			for(QueueItem::StringIter i = queue.begin(); i != queue.end(); ++i)
				delete i->second;
			}
		void add(QueueItem* qi);
		QueueItem* add(const string& aTarget, int64_t aSize, 
			int aFlags, QueueItem::Priority p, const string& aTempTarget, int64_t aDownloaded,
			u_int32_t aAdded, const string& freeBlocks = Util::emptyString, const string& verifiedBlocks = Util::emptyString , const TTHValue* root = NULL) throw(QueueException, FileException);

		QueueItem* find(const string& target);
		void find(QueueItem::List& sl, int64_t aSize, const string& ext);
		int getMaxSegments(string filename, int64_t filesize);
		void find(StringList& sl, int64_t aSize, const string& ext);
		void find(QueueItem::List& ql, const TTHValue& tth);

		QueueItem* findAutoSearch(StringList& recent);
		size_t getSize() { return queue.size(); };
		QueueItem::StringMap& getQueue() { return queue; };
		void move(QueueItem* qi, const string& aTarget);
		void remove(QueueItem* qi) {
			if(lastInsert != queue.end() && Util::stricmp(*lastInsert->first, qi->getTarget()) == 0)
				lastInsert = queue.end();
			queue.erase(const_cast<string*>(&qi->getTarget()));

			if(qi->isSet(QueueItem::FLAG_MULTI_SOURCE)) {
				FileChunksInfo::Free(qi->getTempTarget());
			}

			delete qi;
		}

	private:
		QueueItem::StringMap queue;
		/** A hint where to insert an item... */
		QueueItem::StringIter lastInsert;
	};

	/** QueueItems by target */
	FileQueue fileQueue;

private:
	/** All queue items indexed by user (this is a cache for the FileQueue really...) */
	class UserQueue {
	public:
		void add(QueueItem* qi);
		void add(QueueItem* qi, const User::Ptr& aUser);
		QueueItem* getNext(const User::Ptr& aUser, QueueItem::Priority minPrio = QueueItem::LOWEST, QueueItem* pNext = NULL);
		QueueItem* getRunning(const User::Ptr& aUser);
		void setRunning(QueueItem* qi, const User::Ptr& aUser);
		void setWaiting(QueueItem* qi, const User::Ptr& aUser, bool removeSegment = true);
		QueueItem::UserListMap& getList(int p) { return userQueue[p]; };
		void remove(QueueItem* qi);
		void remove(QueueItem* qi, const User::Ptr& aUser);

		QueueItem::UserMap& getRunning() { return running; };
		bool isRunning(const User::Ptr& aUser) const { 
			return (running.find(aUser) != running.end());
		};
	private:
		/** QueueItems by priority and user (this is where the download order is determined) */
		QueueItem::UserListMap userQueue[QueueItem::LAST];
		/** Currently running downloads, a QueueItem is always either here or in the userQueue */
		QueueItem::UserMap running;
	};

	friend class QueueLoader;
	friend class Singleton<QueueManager>;
	
	QueueManager();
	virtual ~QueueManager() throw();
	
	CriticalSection cs;
	
	/** Partial file list queue */
	PfsQueue pfsQueue;
	/** QueueItems by user */
	UserQueue userQueue;
	/** Directories queued for downloading */
	DirectoryItem::DirectoryMap directories;
	/** Recent searches list, to avoid searching for the same thing too often */
	StringList recent;
	/** The queue needs to be saved */
	bool dirty;
	/** Next search */
	u_int32_t nextSearch;
	
	static const string USER_LIST_NAME;

	/** Sanity check for the target filename */
	static string checkTarget(const string& aTarget, int64_t aSize, int& flags) throw(QueueException, FileException);
	/** Add a source to an existing queue item */
	bool addSource(QueueItem* qi, const string& aFile, User::Ptr aUser, Flags::MaskType addBad, bool utf8) throw(QueueException, FileException);

	int matchFiles(const DirectoryListing::Directory* dir) throw();
	void processList(const string& name, User::Ptr& user, int flags);

	void load(const SimpleXML& aXml);

	void setDirty() {
		if(!dirty) {
			dirty = true;
			lastSave = GET_TICK();
		}
	}

	// TimerManagerListener
	virtual void on(TimerManagerListener::Second, u_int32_t aTick) throw();
	virtual void on(TimerManagerListener::Minute, u_int32_t aTick) throw();
	
	// SearchManagerListener
	virtual void on(SearchManagerListener::SR, SearchResult*) throw();

	// ClientManagerListener
	virtual void on(ClientManagerListener::UserUpdated, const User::Ptr& aUser) throw();
};

#endif // !defined(QUEUE_MANAGER_H)

/**
 * @file
 * $Id$
 */
