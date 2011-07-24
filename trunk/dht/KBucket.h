/*
 * Copyright (C) 2009-2011 Big Muscle, http://strongdc.sf.net
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
 
#ifndef _KBUCKET_H
#define _KBUCKET_H

#include "Constants.h"

#include "../client/CID.h"
#include "../client/Client.h"
#include "../client/MerkleTree.h"
#include "../client/Pointer.h"
#include "../client/SimpleXML.h"
#include "../client/TimerManager.h"
#include "../client/User.h"

namespace dht
{
	struct UDPKey
	{
		string	ip;
		CID		key;
	};

	struct Node :
		public OnlineUser
	{
		typedef boost::intrusive_ptr<Node> Ptr;
		typedef std::map<CID, Node::Ptr> Map;
		
		Node(const UserPtr& u);
		~Node()	{ }
		
		uint8_t getType() const { return type; }
		bool isIpVerified() const { return ipVerified; }
		
		bool isOnline() const { return online; }
		void setOnline(bool _online) { online = _online; }
		
		void setAlive();
		void setIpVerified(bool verified) { ipVerified = verified; }
		void setTimeout(uint64_t now = GET_TICK());

		CID getUdpKey() const;
		void setUdpKey(const CID& _key);
		const UDPKey& getUDPKey() const { return key; }
		
	private:
	
		friend class KBucket;

		UDPKey		key;
				
		uint64_t	created;
		uint64_t	expires;
		uint8_t		type;
		bool		ipVerified;
		bool		online;	// getUser()->isOnline() returns true when node is online in any hub, we need info when he is online in DHT
	};
		
	class KBucket
	{
	public:
		KBucket(void);
		~KBucket(void);

		typedef std::deque<Node::Ptr> NodeList;
		
		/** Creates new (or update existing) node which is NOT added to our routing table */
		Node::Ptr createNode(const UserPtr& u, const string& ip, uint16_t port, bool update, bool isUdpKeyValid);

		/** Adds node to routing table */
		bool insert(const Node::Ptr& node);
		
		/** Finds "max" closest nodes and stores them to the list */
		void getClosestNodes(const CID& cid, Node::Map& closest, unsigned int max, uint8_t maxType) const;
		
		/** Return list of all nodes in this bucket */
		const NodeList& getNodes() const { return nodes; }
		
		/** Removes dead nodes */
		bool checkExpiration(uint64_t currentTime);
		
		/** Loads existing nodes from disk */
		void loadNodes(SimpleXML& xml);
		
		/** Save bootstrap nodes to disk */
		void saveNodes(SimpleXML& xml);		
		
	private:
	
		/** List of nodes in this bucket */	
		NodeList nodes;
		
		/** List of known IPs in this bucket */
		StringSet ipMap;
		
	};
	
}

#endif	// _KBUCKET_H