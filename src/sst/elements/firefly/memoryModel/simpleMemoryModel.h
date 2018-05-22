// Copyright 2013-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef COMPONENTS_FIREFLY_SIMPLE_MEMORY_MODEL_H
#define COMPONENTS_FIREFLY_SIMPLE_MEMORY_MODEL_H

#define CALL_INFO_LAMBDA     __LINE__, __FILE__

class SimpleMemoryModel : SubComponent {


#define BUS_WIDGET_MASK 1<<1
#define CACHE_MASK      1<<2
#define LOAD_MASK       1<<3
#define MEM_MASK        1<<4
#define MUX_MASK        1<<5
#define STORE_MASK      1<<6
#define THREAD_MASK     1<<7
#define BUS_BRIDGE_MASK 1<<8
#define TLB_MASK        1<<9
#define SM_MASK        1<<10
#define SHARED_TLB_MASK 1<<11
 public:

#include "memOp.h"

 private:

   typedef std::function<void()> Callback;

#include "cache.h"
#include "memReq.h"
#include "sharedTlb.h"
#include "unit.h"
#include "thread.h"
#include "tlbUnit.h"
#include "sharedTlbUnit.h"
#include "nicUnit.h"
#include "busBridgeUnit.h"
#include "loadUnit.h"
#include "storeUnit.h"
#include "memUnit.h"
#include "cacheUnit.h"

    class SelfEvent : public SST::Event {
      public:
        SelfEvent( int slot, Work* work = NULL ) : callback(NULL), unit(NULL), slot(slot), work(work) {}
        SelfEvent( Callback callback ) : callback(callback), unit(NULL), work(NULL) {}
        SelfEvent( UnitBase* unit, UnitBase* srcUnit = NULL ) : callback(NULL), unit(unit), srcUnit(srcUnit), work(NULL) {}
		Callback callback;
		UnitBase* unit;
		UnitBase* srcUnit;
		int slot;
		Work* work;

        NotSerializable(SelfEvent)
    };

  public:
	enum NIC_Thread { Send, Recv };

    SimpleMemoryModel( Component* comp, Params& params, int id, int numCores, int numNicUnits ) : 
		SubComponent( comp ), m_numNicThreads(numNicUnits)
	{
    	char buffer[100];
    	snprintf(buffer,100,"@t:%d:SimpleMemoryModel::@p():@l ",id);

    	m_dbg.init(buffer, params.find<uint32_t>("verboseLevel",0), params.find<uint32_t>("verboseMask",-1), Output::STDOUT);

		int memReadLat_ns = params.find<int>( "memReadLat_ns", 150 );
		int memWriteLat_ns = params.find<int>( "memWriteLat_ns", 150 );
		int memNumSlots = params.find<int>( "memNumSlots", 10 );

		int nicNumLoadSlots = params.find<int>( "nicNumLoadSlots", 32 );
		int nicNumStoreSlots = params.find<int>( "nicNumStoreSlots", 32 );
		int hostNumLoadSlots = params.find<int>( "hostNumLoadSlots", 32 );
		int hostNumStoreSlots = params.find<int>( "hostNumStoreSlots", 32 );

		double busBandwidth = params.find<double>("busBandwidth_Gbs", 7.8 );
		int busNumLinks = params.find<double>("busNumLinks", 16 );
		int busLatency = params.find<double>("busLatency", 0 );
		int DLL_bytes = params.find<int>( "DLL_bytes", 16 );
		int TLP_overhead = params.find<int>( "TLP_overhead", 30 );

		int hostCacheUnitSize = params.find<int>( "hostCacheUnitSize", 32 );
		int hostCacheNumMSHR = params.find<int>( "hostCacheNumMSHR", 10 );
		int hostCacheLineSize = params.find<int>( "hostCacheLineSize", 64 );
		int widgetSlots = params.find<int>( "widgetSlots", 64 );


		int tlbPageSize = params.find<int>( "tlbPageSize", 1024*1024*4 );
		int tlbSize = params.find<int>( "tlbSize", 0 );
		int tlbMissLat_ns = params.find<int>( "tlbMissLat_ns", 0 );
		int numWalkers = params.find<int>( "numWalkers", 1 );
		int numTlbSlots = params.find<int>( "numTlbSlots", 1 );
        int nicToHostMTU = params.find<int>( "nicToHostMTU", 256 );

		m_memUnit = new MemUnit( *this, m_dbg, id, memReadLat_ns, memWriteLat_ns, memNumSlots );
		m_hostCacheUnit = new CacheUnit( *this, m_dbg, id, m_memUnit, hostCacheUnitSize, hostCacheLineSize, hostCacheNumMSHR,  "Host" );
	    m_muxUnit = new MuxUnit( *this, m_dbg, id, m_hostCacheUnit, "HostCache" );

		m_busBridgeUnit = new BusBridgeUnit( *this, m_dbg, id, m_muxUnit, busBandwidth, busNumLinks, busLatency,
                                                                TLP_overhead, DLL_bytes, hostCacheLineSize, widgetSlots );

		
        m_sharedTlb = new SharedTlb( *this, m_dbg, id, tlbSize, tlbPageSize, tlbMissLat_ns, numWalkers );
		
		m_nicUnit = new NicUnit( *this, m_dbg, id );

		std::stringstream tlbName;
		std::stringstream unitName;
		std::stringstream threadName; 
		for ( int i = 0; i < m_numNicThreads; i++ ) {
		
			unitName.str("");
			unitName.clear();
			unitName << "Nic" << i;

            SharedTlbUnit* tlb = new SharedTlbUnit( *this, m_dbg, id, unitName.str().c_str(), m_sharedTlb, 
					new LoadUnit( *this, m_dbg, id,
						m_busBridgeUnit,
						nicNumLoadSlots, unitName.str().c_str() ),

					new StoreUnit( *this, m_dbg, id,
						m_busBridgeUnit,
						nicNumStoreSlots, unitName.str().c_str() ),
                        numTlbSlots, numTlbSlots 
                        );
#if 0
            Tlb* tlb = new Tlb( *this, m_dbg, id, unitName.str().c_str(), 
					new LoadUnit( *this, m_dbg, id,
						m_busBridgeUnit,
						nicNumLoadSlots, unitName.str().c_str() ),

					new StoreUnit( *this, m_dbg, id,
						m_busBridgeUnit,
						nicNumStoreSlots, unitName.str().c_str() ),
                        tlbSize, tlbPageSize, tlbMissLat_ns, numWalkers, numTlbSlots, numTlbSlots 
                        );
#endif

			m_threads.push_back( 
				new Thread( *this, unitName.str(), m_dbg, id, nicToHostMTU, tlb, tlb )	
 			); 
		}
		for ( int i = 0; i < numCores; i++ ) {
			threadName.str("");
			threadName.clear();
			threadName << "HostThread" << i;
			unitName.str("");
			unitName.clear();
			unitName << "Host" << i;

			m_threads.push_back( 
				new Thread( *this, threadName.str(), m_dbg, id, 64,
						new LoadUnit( *this, m_dbg, id, m_muxUnit, hostNumLoadSlots, unitName.str().c_str() ),
						new StoreUnit( *this, m_dbg, id, m_muxUnit, hostNumStoreSlots, unitName.str().c_str() ) 
				) 
			);
		}

		m_selfLink = comp->configureSelfLink("Nic::SimpleMemoryModel", "1 ns",
        new Event::Handler<SimpleMemoryModel>(this,&SimpleMemoryModel::handleSelfEvent));
	}

    virtual ~SimpleMemoryModel() {
        m_sharedTlb->printStats();
        delete m_hostCacheUnit;
        for ( unsigned i = 0; i < m_threads.size(); i++ ) {
            delete m_threads[i];
        }
    }

	void schedCallback( SimTime_t delay, Callback callback ){
		m_selfLink->send( delay , new SelfEvent( callback ) );
	}
	void schedResume( SimTime_t delay, UnitBase* unit, UnitBase* srcUnit = NULL ){
		m_selfLink->send( delay , new SelfEvent( unit, srcUnit ) );
	}

	void handleSelfEvent( Event* ev ) {

		SimTime_t now = getCurrentSimTimeNano();
		SelfEvent* event = static_cast<SelfEvent*>(ev); 
		if ( event->callback ) {
			m_dbg.debug(CALL_INFO,3,SM_MASK,"callback\n");
			event->callback();
		} else if ( event->unit ) {
			m_dbg.debug(CALL_INFO,3,SM_MASK,"resume %p\n",event->srcUnit);
			if ( event->srcUnit ) {
				event->unit->resume( event->srcUnit );
			} else {
				event->unit->resume( );
			}
		} else if ( event->work ) {
			m_threads[event->slot]->addWork( event->work );
		} else {

			assert(0);
		}
		delete event;
	};

	void addWork( int slot, Work* work ) {
		// we send an event to ourselves to break the call chain, we will eventually call a 
		// callback provided by the caller of this function, this call back may re-enter here 
		if ( m_threads[slot]->isIdle() ) {
		    m_selfLink->send( 0 , new SelfEvent( slot, work ) );
        } else {
		    m_threads[slot]->addWork( work );
        }
	}

	virtual void schedHostCallback( int core, std::vector< MemOp >* ops, Callback callback ) {
		SimTime_t now = getCurrentSimTimeNano();
		m_dbg.debug(CALL_INFO,3,SM_MASK,"now=%" PRIu64 "\n",now );

		int id = m_numNicThreads + core;
		addWork( id, new Work( core, ops, callback, now ) );
	}

	virtual void schedNicCallback( int unit, int pid, std::vector< MemOp >* ops, Callback callback ) { 
		SimTime_t now = getCurrentSimTimeNano();
		m_dbg.debug(CALL_INFO,3,SM_MASK,"now=%" PRIu64 " unit=%d\n", now, unit );
		assert( unit >=0 );

		addWork( unit, new Work( pid, ops, callback, now ) );
	}

	NicUnit& nicUnit() { return *m_nicUnit; }
	BusBridgeUnit& busUnit() { return *m_busBridgeUnit; }

  private:

	Link* m_selfLink;

	MuxUnit* 		m_muxUnit;
	MemUnit* 		m_memUnit;
	CacheUnit* 		m_hostCacheUnit;
	BusBridgeUnit*  m_busBridgeUnit;
	NicUnit* 		m_nicUnit;
	CacheUnit* 		m_nicCacheUnit;
    SharedTlb*      m_sharedTlb;

	std::vector<Thread*> m_threads;

	int 		m_numNicThreads;
	uint32_t    m_hostBW;
	uint32_t    m_nicBW;
	Output		m_dbg;
}; 

#endif
