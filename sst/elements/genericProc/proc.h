// Copyright 2009-2010 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2010, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef PROC_H_
#define PROC_H_

#include "sst/core/eventFunctor.h"
#include "sst/core/component.h"
#include <sst/core/link.h>
#include <sst/core/cpunicEvent.h>
#include <memory.h>
#include "fe_memory.h"

#include "ssBackEnd/ssb_mainProc.h"
#include "FE/thread.h"
#include "FE/processor.h"

using namespace SST;

#define DBG_GPROC 1
#if DBG_GPROC
#define _GPROC_DBG( lvl, fmt, args...)\
    if (gproc_debug >= lvl)   { \
	printf( "%d:genericProc::%s():%d: "fmt, _debug_rank,__FUNCTION__,__LINE__, ## args ); \
    }
#else
#define _GPROC_DBG( lvl, fmt, args...)
#endif

#define TIME_MEM 1

extern int gproc_debug;

// "test" class
class tester : public Component {
    public:
	tester(ComponentId_t id, Params_t &params) :
	    Component(id)
	{
	    ClockHandler_t *handler = new EventHandler<tester,bool,Cycle_t>(this, &tester::clock);
	    std::string frequency = params["clock"];
	    if (frequency.length() == 0) {
		INFO("Using default frequency for tester (1.0 GHz)");
		frequency = "1.0 GHz";
	    }
	    printf("generating tester with freq '%s' (len:%i)\n", frequency.c_str(), (int)frequency.length());
	    registerClock(frequency, handler);
	}
	bool clock(Cycle_t thisCycle)
	{
	    printf("cycle is now: %llu\n", (unsigned long long)getCurrentSimTime());
	    return false;
	}
};

// "memory" class
class mem : public Component {
    private:
	MemoryChannel<uint64_t> *memchan;
    public:
	mem(ComponentId_t id, Params_t& params) :
	    Component(id)
	{
	    memchan = new MemoryChannel<uint64_t>(*this, params, "bus");
	    ClockHandler_t *handler = new EventHandler<mem,bool,Cycle_t>(this, &mem::clock);
	    std::string frequency = params["clock"];
	    if (frequency.length() == 0) {
		INFO("Using default frequency for genericMem (2.0 GHz)");
		frequency = "2.0 GHz";
	    }
	    registerClock(frequency, handler);
	}
	bool clock(Cycle_t thisCycle)
	{
	    MemoryChannel<uint64_t>::event_t *event;
	    while (memchan->recv(&event)) {
		event->msgType = MemoryChannel<uint64_t>::event_t::RESPONSE;
		if (!memchan->send(event)) {
		    abort(); // this is ugly
		}
	    }
	    return false;
	}
};

// "processor" class
class proc : public processor {

#if WANT_CHECKPOINT_SUPPORT
BOOST_SERIALIZE {
    _GPROC_DBG(1, "begin\n");
    BOOST_VOID_CAST_REGISTER(proc*,processor*);
    _GPROC_DBG(1, "base\n");
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP( processor );
    _GPROC_DBG(1, "links\n");
    ar & BOOST_SERIALIZATION_NVP( memLinks );
    ar & BOOST_SERIALIZATION_NVP( netLinks );
    _GPROC_DBG(1, "tSource\n");
    ar & BOOST_SERIALIZATION_NVP( tSource );
    _GPROC_DBG(1, "thread\n");
    ar & BOOST_SERIALIZATION_NVP( myThread );
    _GPROC_DBG(1, "ss\n");
    ar & BOOST_SERIALIZATION_NVP( ssBackEnd );
    //ar & BOOST_SERIALIZATION_NVP( mProc );
    _GPROC_DBG(1, "end\n");
  }
  SAVE_CONSTRUCT_DATA( proc ) {
    _GPROC_DBG(1, "begin\n");
    ar << BOOST_SERIALIZATION_NVP( t->id );
    ar << BOOST_SERIALIZATION_NVP( t->params );
    _GPROC_DBG(1, "end\n");
  }
  LOAD_CONSTRUCT_DATA( proc ) {
    _GPROC_DBG(1, "begin\n");
    ComponentId_t     id;
    Params_t          params;
    ar >> BOOST_SERIALIZATION_NVP( id );
    ar >> BOOST_SERIALIZATION_NVP( params );
    // call the dummy constructor which dosn't initialize the thread source
    ::new(t)proc( id, params, 0 );
  }
#endif

public:
  //  typedef MemoryDev< uint64_t, instruction* > memDev_t; 
  typedef Memory< uint64_t, instruction* > memory_t; 
private:
  // links to memory
  std::vector< memory_t* > memory;
  // links to other processors
  std::vector<Link*> netLinks;
  // thread source
  threadSource tSource;
  // thread of execution
  thread *myThread;
  // use a Simple-scalar-based backend timing model
  int ssBackEnd;
  // use an external memory model
  int externalMem;
  // maximum main memory references outstanding
  int maxMMOut;
  // number of cores
  int cores; 
  // coherency
  int coherent;
  // maximum number of outstanding special memory requests per core
  int maxOutstandingSpecReq;
  // the simple-scalar-based processor model
  vector<mainProc *> mProcs;
  // count of outstanding special memory requests
  vector <int> outstandingSpecReq;
  // record of who a memory request belongs to, and when it was issued
  typedef pair<int, uint64_t> memReqRec_t;
  // map of outgoing memory request instructions to cores
  typedef map<instruction *, memReqRec_t> memReqMap_t;
  // map of outgoing normal (load, store) memory requests
  //
  // maps instructions to cores
  memReqMap_t memReqMap;
  // map of special memory requests
  //
  // e.g. AMOs
  memReqMap_t memSpecReqMap;
  instruction* onDeckInst;

#if TIME_MEM
  uint64_t totalMemReq;
  uint64_t totalSpecialMemReq;
  uint64_t numMemReq;
  uint64_t memReqLat;
  uint64_t memSpecialReqLat;
  uint64_t memStores;
#endif

  ClockHandler_t* clockHandler;
  EventHandler_t *NICeventHandler;

  ComponentId_t id;
  Params_t& params;

  bool addThread(thread *);
  void swapThreads(bool quanta, bool refill);
  // flag to determine if we are flushing pipes and need to check for
  // thread swaps
  bool needThreadSwap;
  std::deque<thread*> threadQ;

public:
  /* shared memory coherency protocol handling */  
  //: Bus Message types
  typedef enum {READ_MISS, WRITE_MISS, WRITE_HIT} msgType;
  // Post a message to the bus
  int postMessage(msgType t, simAddress addr, mainProc* poster);
  //: add in contention on the bus (if any)
  void registerPost() {;};
  const bool isCoherent() const {return coherent;}

  virtual int Setup();
  virtual int Finish();
  virtual bool preTic( Cycle_t );
  void processMemDevResp( );
  virtual bool handle_nic_events( Event* );
  proc(ComponentId_t id, Params_t& params);
  proc(ComponentId_t id, Params_t& params, int dummy);

  //: Return num of cores
  virtual int getNumCores() const {return mProcs.size();}
  virtual bool insertThread(thread*);
  virtual bool isLocal(const simAddress, const simPID);
  virtual bool spawnToCoProc(const PIM_coProc, thread* t, simRegister hint);
  virtual bool switchAddrMode(PIM_addrMode);
  virtual exceptType writeSpecial(const PIM_cmd, const int nargs, 
				  const uint *args);
  virtual bool forwardToNetsimNIC(int call_num, char *params,
				  const size_t params_length,
				  char *buf, const size_t buf_len);
  virtual bool pickupNetsimNIC(CPUNicEvent **event);
  virtual bool externalMemoryModel();
  virtual bool sendMemoryReq( instType, uint64_t address, 
			       instruction*, int mProcID);
  virtual bool sendMemoryParcel(uint64_t address, instruction *inst, 
				int mProcID);
};

#endif
