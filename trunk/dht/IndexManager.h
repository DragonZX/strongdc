/*
 * Copyright (C) 2008-2009 Big Muscle, http://strongdc.sf.net
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

#pragma once

#include "Constants.h"
#include "KBucket.h"

#include "../client/ShareManager.h"
#include "../client/Singleton.h"

namespace dht
{

struct File
{
	File() { };
	File(const TTHValue& _tth, int64_t _size) :
		tth(_tth), size(_size) { }
		
	/** File hash */
	TTHValue tth;
	
	/** File size in bytes */
	int64_t size;
};

struct Source
{
	GETSET(CID, cid, CID);
	GETSET(string, ip, Ip);
	GETSET(uint64_t, expires, Expires);
	GETSET(uint64_t, size, Size);
	GETSET(uint16_t, udpPort, UdpPort);
};

class IndexManager :
	public Singleton<IndexManager>
{
public:
	IndexManager(void);
	~IndexManager(void);

	typedef std::deque<Source> SourceList;
		
	/** Finds TTH in known indexes and returns it */
	bool findResult(const TTHValue& tth, SourceList& sources) const;
	
	/** Try to publish next file in queue */
	void publishNextFile();
	
	/** Create publish queue from local file list */
	void createPublishQueue(ShareManager::HashFileMap& tthIndex);
	
	/** Publishes all files in queue */
	bool publishFiles();
	
	/** Loads existing indexes from disk */
	void loadIndexes(SimpleXML& xml);
	
	/** Save all indexes to disk */
	void saveIndexes(SimpleXML& xml);
		
	/** How many files is currently being published */
	void setPublishing(uint8_t _pub) { publishing = _pub; }
	uint8_t getPublishing() const { return publishing; }
	
	/** Processes incoming request to publish file */
	void processPublishRequest(const Node::Ptr& node, const AdcCommand& cmd);
	
private:

	/** Contains known hashes in the network and their sources */
	typedef std::tr1::unordered_map<TTHValue, SourceList> TTHMap;
	TTHMap tthList;
	
	/** Queue of files prepared for publishing */
	typedef std::deque<File> FileQueue;
	FileQueue publishQueue;
		
	/** How many files is currently being published */
	uint8_t publishing;
	
	/** Synchronizes access to tthList */
	mutable CriticalSection cs;
	
	/** Add new source to tth list */
	void addIndex(const TTHValue& tth, const Node::Ptr& node, uint64_t size);

};

} // namespace dht