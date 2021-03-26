// Copyright 2013-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2021, NTESS
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

#include <math.h>
#include <sst/core/interfaces/simpleMem.h>
#include "ioVec.h"
#include "memoryModel/memoryModel.h"
#include "memoryModel/detailedInterface.h"
#include "memReq.h"

#include <queue>
#include "../thingHeap.h"

#define CALL_INFO_LAMBDA     __LINE__, __FILE__

namespace SST {
namespace Firefly {

class SimpleMemoryModel : public MemoryModel {

public:
   SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
        SimpleMemoryModel,
        "firefly",
        "SimpleMemory",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "",
       	SST::Firefly::SimpleMemoryModel
    )

    SST_ELI_DOCUMENT_PARAMS(
	    {"id",                  "ID of the router."},
	    {"numCores",            "number of memory operation units for the host.","0"},
	    {"numNicUnits",         "number of memory operation units for the nic.","0"},
		{"verboseLevel",        "Sets the output level","0"},
		{"verboseMask",         "Sets the output mask","-1"},
		{"memReadLat_ns",       "Sets the latency for a read of host memory","150"},
		{"memWriteLat_ns",     	"Sets the latency for a write of host memory","150"},
		{"memNumSlots",        	"Sets the number of operations the memory control can do in parallel","10"},
		{"nicNumLoadSlots",     "Sets the number of loads the NIC can do in parallel","32"},
		{"nicNumStoreSlots",    "Sets the number of stores the NIC can do in parallel","32"},
		{"hostNumLoadSlots",    "Sets the number of loads the Host can do in parallel","32"},
		{"hostNumStoreSlots",   "Sets the number of stores the Host can do in parallel","32"},
		{"busBandwidth_Gbs",    "Sets the bandwidth of a PCIe link","7.8"},
		{"busNumLinks",         "Sets the number of PCIe links","16"},
		{"busLatency",          "Sets the latency of the PCIe bus","0"},
		{"DLL_bytes",           "Sets the PCIe DLL bytes","16"},
        {"TLP_overhead",        "Sets the PCIe TLP overhead ","30"},
		{"hostCacheUnitSize",   "Sets the host cache size","32"},
		{"hostCacheNumMSHR",    "Sets the host cache number of MSHRs","10"},
		{"hostCacheLineSize",   "Sets the host cache line size ","64"},
		{"widgetSlots",         "Sets the number of loads and stores the Bus can do in parallel","64"},
		{"tlbPageSize",         "Sets the TLB page size","4194304"},
		{"tlbSize",             "Sets the TLB size","0"},
		{"tlbMissLat_ns",       "Sets the TLB miss latency","0"},
		{"numWalkers",          "Sets the number of TLB page walkers","1"},
		{"numTlbSlots",         "Sets the number of loads and store the TLB can do in parallel","1"},
		{"nicToHostMTU",        "Set the size of the PCIe MTU","256"},
		{"useHostCache",        "Sets whether or not to use a host cache","yes"},
		{"useDetailedModel",    "Sets whether or not a detailed memory model is used","no"},
		{"useBusBridge",        "Sets whether or not a bus is used between the NIC and host","yes"},
		{"printConfig",         "Print the config","no"},
    )

    SST_ELI_DOCUMENT_STATISTICS(
        { "nic_thread_work_Q_depth",           "number of entries in queue", "entries", 1},
        { "host_thread_work_Q_depth",          "number of entries in queue", "entries", 1},
        { "nic_thread_load_latency",           "latency to complete", "nanoseconds", 1},
        { "nic_thread_load_pending_Q_depth",   "number of entries in queue", "entries", 1},
        { "nic_thread_store_pending_Q_depth",  "number of entries in queue", "entries", 1},

        { "host_thread_load_latency",          "latency to complete", "nanoseconds", 1},
        { "host_thread_load_pending_Q_depth",  "number of entries in queue", "entries", 1},
        { "host_thread_store_pending_Q_depth", "number of entries in queue", "entries", 1},
        { "hostCache_mux_blocked_ns",          "latency blocked", "nanoseconds", 1},
        { "nic_mux_blocked_ns",                "latency blocked", "nanoseconds", 1},
        { "bus_blocked_ns",                    "latency blocked", "nanoseconds", 1},
        { "bus_load_widget_pending_Q_depth",   "number of entries in queue", "entries", 1},
        { "bus_store_widget_pending_Q_depth",  "number of entries in queue", "entries", 1},

        { "nic_TLB_hits",                      "number of TLB hits", "count", 1},
        { "nic_TLB_total",                     "total number of TLB requests", "count", 1},
        { "host_cache_hits",                   "number of TLB hits", "count", 1},
        { "host_cache_total",                  "total number of TLB requests", "count", 1},
        { "mem_blocked_time",                  "time memory requests were blocked", "nanseconds", 1},
        { "mem_num_loads",                     "total number of loads", "count", 1},
        { "mem_num_stores",                    "total number of stores", "count", 1},
        { "mem_addrs",                         "addresses accesed", "value", 1},
	)


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
#define DETAILED_MASK   1<<12

#include "cache.h"
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
#include "detailedUnit.h"


    class SelfEvent : public SST::Event {
      public:
        void init( int _slot, Work* _work = NULL ) { callback = NULL;  unit = NULL; slot = _slot; work = _work; }
        void init( Callback* _callback ) { callback= _callback; unit = NULL; work = NULL; }
        void init( UnitBase* _unit, UnitBase* _srcUnit = NULL ) { callback = NULL; unit = _unit; srcUnit = _srcUnit; work = NULL; }

		Callback* callback;
		UnitBase* unit;
		UnitBase* srcUnit;
		Work* work;
		int slot;

        NotSerializable(SelfEvent)
    };

  public:
	enum NIC_Thread { Send, Recv };

    SimpleMemoryModel( ComponentId_t compId, Params& params ) :
		MemoryModel( compId ), m_hostCacheUnit(NULL), m_busBridgeUnit(NULL)
	{
		int id = params.find<int32_t>( "id", -1 );
		assert( id > -1 );
		int numCores = params.find<uint32_t>("numCores",0);
		m_numNicThreads = params.find<uint32_t>("numNicUnits",0);

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
		std::string tmp = params.find<std::string>( "useHostCache", "yes" );
		bool useHostCache;
		if ( 0 == tmp.compare("yes" ) ) {
			useHostCache = true;
		} else if ( 0 == tmp.compare("no" ) ) {
			useHostCache = false;
		} else {
			m_dbg.fatal(CALL_INFO,0,"unknown value for parameter useHostCache '%s'\n",tmp.c_str());
		}

		m_detailedUnit = NULL;
		tmp = params.find<std::string>( "useDetailedModel", "no" );
		if ( 0 == tmp.compare("yes" ) ) {
			m_detailedUnit = new DetailedUnit( *this, m_dbg, id );
			useHostCache = false;
		}

		bool useBusBridge;
		tmp = params.find<std::string>( "useBusBridge", "yes" );
		if ( 0 == tmp.compare("yes" ) ) {
			useBusBridge = true;
		} else if ( 0 == tmp.compare("no" ) ) {
			useBusBridge = false;
		} else {
			m_dbg.fatal(CALL_INFO,0,"unknown value for parameter useBusBridge '%s'\n",tmp.c_str());
		}

		if ( 0 == params.find<std::string>( "printConfig", "no" ).compare("yes" ) ) {
			m_dbg.output("Node id=%d is using SimpleMemoryModel, useBusBridge=%d, useHostCache=%d\n", id, useBusBridge, useHostCache);
		}

		if ( ! m_detailedUnit ) {
			m_memUnit = new MemUnit( *this, m_dbg, id, memReadLat_ns, memWriteLat_ns, memNumSlots );
		} else {
			m_memUnit = static_cast<MemUnit*>(m_detailedUnit);
		}

		MuxUnit* nicMuxUnit;
        if ( useHostCache ) {
		    m_hostCacheUnit = new CacheUnit( *this, m_dbg, id, m_memUnit, hostCacheUnitSize, hostCacheLineSize, hostCacheNumMSHR,  "host" );
	        m_muxUnit = new MuxUnit( *this, m_dbg, id, m_hostCacheUnit, "hostCache" );
        } else {
	        m_muxUnit = new MuxUnit( *this, m_dbg, id, m_memUnit, "hostCache" );
        }

        if ( useBusBridge ) {

			m_busBridgeUnit = new BusBridgeUnit( *this, m_dbg, id, m_muxUnit, busBandwidth, busNumLinks, busLatency,
                                                                TLP_overhead, DLL_bytes, hostCacheLineSize, widgetSlots );
	    	nicMuxUnit = new MuxUnit( *this, m_dbg, id, m_busBridgeUnit, "nic" );

		} else {
	    	nicMuxUnit = m_muxUnit;
		}

        m_sharedTlb = new SharedTlb( *this, m_dbg, id, tlbSize, tlbPageSize, tlbMissLat_ns, numWalkers );

		m_nicUnit = new NicUnit( *this, m_dbg, id );

		std::stringstream tlbName;
		std::stringstream threadName;
		for ( int i = 0; i < m_numNicThreads; i++ ) {

            SharedTlbUnit* tlb = new SharedTlbUnit( *this, m_dbg, id, "nic_thread", m_sharedTlb,
					new LoadUnit( *this, m_dbg, id, i,
                        nicMuxUnit,
						nicNumLoadSlots, "nic_thread" ),

					new StoreUnit( *this, m_dbg, id, i,
                        nicMuxUnit,
						nicNumStoreSlots, "nic_thread" ),
                        numTlbSlots, numTlbSlots
                        );

			m_threads.push_back(
				new Thread( *this, "nic", m_dbg, id, i, nicToHostMTU, tlb, tlb )
 			);
		}
		for ( int i = 0; i < numCores; i++ ) {

			m_threads.push_back(
				new Thread( *this, "host", m_dbg, id, i, 64,
						new LoadUnit( *this, m_dbg, id, i, m_muxUnit, hostNumLoadSlots, "host_thread" ),
						new StoreUnit( *this, m_dbg, id, i, m_muxUnit, hostNumStoreSlots, "host_thread" )
				)
			);
		}

		m_selfLink = configureSelfLink("Nic::SimpleMemoryModel", "1 ns",
        new Event::Handler<SimpleMemoryModel>(this,&SimpleMemoryModel::handleSelfEvent));
	}

    virtual ~SimpleMemoryModel() {
        if ( m_hostCacheUnit ) {
            delete m_hostCacheUnit;
        }
        for ( unsigned i = 0; i < m_threads.size(); i++ ) {
            delete m_threads[i];
        }
		delete m_sharedTlb;
		delete m_nicUnit;
    }

	ThingHeap<SelfEvent> m_eventHeap;

	void setDetailedInterface( SST::Firefly::DetailedInterface* ptr ) {
		assert( m_detailedUnit );
		static_cast<DetailedUnit*>(m_detailedUnit)->setDetailedInterface( ptr );
	}

	void schedCallback( SimTime_t delay, Callback* callback ){
		SelfEvent* ev = m_eventHeap.alloc( );
		ev->init( callback );
		m_selfLink->send( delay , ev );
	}
	void schedResume( SimTime_t delay, UnitBase* unit, UnitBase* srcUnit = NULL ){
		SelfEvent* ev = m_eventHeap.alloc( );
		ev->init( unit, srcUnit );
		m_selfLink->send( delay , ev );
	}

	void handleSelfEvent( Event* ev ) {

		SimTime_t now = getCurrentSimTimeNano();
		SelfEvent* event = static_cast<SelfEvent*>(ev);
		if ( event->callback ) {
			m_dbg.debug(CALL_INFO,3,SM_MASK,"callback\n");
			(*event->callback)();
			delete event->callback;
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
		m_eventHeap.free( event );
	};

	void addWork( int slot, Work* work ) {
		// we send an event to ourselves to break the call chain, we will eventually call a
		// callback provided by the caller of this function, this call back may re-enter here
		if ( m_threads[slot]->isIdle() ) {
			SelfEvent* ev = m_eventHeap.alloc();
			ev->init( slot, work );
		    m_selfLink->send( 0 , ev );
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

	bool busUnitWrite( UnitBase* src, MemReq* req, Callback* callback ) {
		if ( m_busBridgeUnit ) {
			return m_busBridgeUnit->write( src, req, callback );
		} else {
			schedCallback( 0, callback );
			return false;
		}
	}

    void printStatus( Output& out, int id ) {
        for ( unsigned i = 0; i < m_threads.size(); i++ ) {
            m_threads[i]->printStatus( out, id );
        }
    }

	void init(unsigned int phase) {
		if ( m_detailedUnit ) {
			m_detailedUnit->init(phase);
		}
	}



  private:

	Link* m_selfLink;

	Unit*			m_detailedUnit;
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

} // namespace Firefly
} // namespace SST
#endif
