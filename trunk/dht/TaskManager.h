#pragma once

#include "..\client\Singleton.h"
#include "..\client\TimerManager.h"

namespace dht
{

	class TaskManager :
		public Singleton<TaskManager>, private TimerManagerListener
	{
	public:
		TaskManager(void);
		~TaskManager(void);
		
	private:
	
		/** Time of publishing next file in queue */
		uint64_t nextPublishTime;
		
		/** Time when our files will be republished */
		uint64_t nextRepublishTime;
		
		/** When running searches will be processed */
		uint64_t nextSearchTime;
		
		/** When initiate searching for myself */
		uint64_t nextSelfLookup;
		
		/** When request next firewall check */
		uint64_t nextFirewallCheck;
		
		uint64_t lastBootstrap;
		
		// TimerManagerListener
		void on(TimerManagerListener::Second, uint64_t aTick) throw();	
		void on(TimerManagerListener::Minute, uint64_t aTick) throw();
	};

}