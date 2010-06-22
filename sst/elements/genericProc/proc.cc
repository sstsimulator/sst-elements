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

#include <sst_config.h>

#include "proc.h"
#include "FE/thread.h"
//#include "sst/boost.h"
#include <sst/core/cpunicEvent.h>
#include <algorithm>

int gproc_debug;


extern "C" {
  proc* genericProcAllocComponent( SST::ComponentId_t id,
				   SST::Component::Params_t& params )
  {
    proc* ret = new proc( id, params );
    return ret;
  }
}

// dummy constructor, called in deserialization. 
proc::proc(ComponentId_t idC, Params_t& paramsC, int dummy) :
  processor(idC, paramsC), id(idC), params(paramsC) {
  _GPROC_DBG(1, "Dummy Constructor\n");
}

proc::proc(ComponentId_t idC, Params_t& paramsC) : 
  processor(idC, paramsC), myThread(0), ssBackEnd(0),
    externalMem(0), maxMMOut(-1), cores(1), 
  onDeckInst(NULL),
  id(idC), params(paramsC)
{
  _GPROC_DBG(1, "Constructor\n");
  tSource.init(this, paramsC);
  myThread = 0;
  needThreadSwap = 0;
  coherent = 1;
  maxOutstandingSpecReq = 8;

#if TIME_MEM
  totalMemReq = 0;
  totalSpecialMemReq = 0;
  numMemReq = 0;
  memReqLat = 0;
  memSpecialReqLat = 0;
  memStores = 0;
#endif

  memory_t* tmp = new memory_t( );
  // construct the params for the memory device and prefetcher
  Params_t memParams, prefInit;
  for (Params_t::iterator pi = paramsC.begin(); pi != paramsC.end(); ++pi) {
    if (pi->first.find("mem.") == 0) {
      string memS(pi->first, 4);
      memParams[memS] = pi->second;
    }
    if (pi->first.find("pref.") == 0) {
      prefInit[pi->first] = pi->second;
    }
  }
  /* This does not work right with the new link behavior, because it connects and sends messages
   * tmp->devAdd(  new memory_t::dev_t( *this, memParams, "mem0" ), 0,
		0x100000000ULL );*/

#if 0
  tmp->map( 0x0, 0x0, 0x100000 );
  tmp->map( 0x100000, 0x200000, 0x1000 );
  tmp->map( 0x300000, 0x300000, 0x100000000 - 0x300000 );
#endif
  memory.push_back( tmp );

  // add links
  NICeventHandler = new EventHandler<proc, bool, Event *>
    (this, &proc::handle_nic_events);

  // turned off network until we can check for it
  Link *nl = LinkAdd("net0", NICeventHandler);
  if (nl) netLinks.push_back(nl);


  clockHandler = new EventHandler< proc, bool, Cycle_t>
    ( this, &proc::preTic );

  // find config parameters
  std::string clockSpeed = "1GHz";
  string ssConfig;
  Params_t::iterator it = params.begin();  
  while( it != params.end() ) {
    _GPROC_DBG(1, "key=%s value=%s\n",
	       it->first.c_str(),it->second.c_str());
    if ( ! it->first.compare("clock") ) {
//       sscanf( it->second.c_str(), "%f", &clockSpeed );
      clockSpeed = it->second;
    }
    if (!it->first.compare("debug"))   {
      sscanf(it->second.c_str(), "%d", &gproc_debug);
    }
    if (!it->first.compare("cores"))   {
      sscanf(it->second.c_str(), "%d", &cores);
    }
    if (!it->first.compare("ssBackEnd"))   {
      sscanf(it->second.c_str(), "%d", &ssBackEnd);
    }
    if (!it->first.compare("externalMem"))   {
      sscanf(it->second.c_str(), "%d", &externalMem);
    }
    if (!it->first.compare("maxMMOut"))   {
      sscanf(it->second.c_str(), "%d", &maxMMOut);
    }
    if (!it->first.compare("ssConfig"))   {
      ssConfig = it->second;
    }
    if (!it->first.compare("coherent"))   {
      sscanf(it->second.c_str(), "%d", &coherent);
    }
    if (!it->first.compare("maxSpecReq"))   {
      sscanf(it->second.c_str(), "%d", &maxOutstandingSpecReq);
    }
    ++it;
  }

  if (cores > 1 && !ssBackEnd) {
    printf("Multicore is currently only allowed with the SimpleScalar "
	   "backend\n");
    exit(-1);
  }

  // if we are using the simple scalar backend, initialize it
  if (ssBackEnd) {
    for (int c = 0; c < cores; ++c) {
      mProcs.push_back(new mainProc(ssConfig, tSource, maxMMOut, this, c, 
				    prefInit, this));
      outstandingSpecReq.push_back(0);
    }
  } else {
    mProcs.clear();
  }

  _GPROC_DBG(1, " Registering clockHandler %p @ %s\n", clockHandler, 
	     clockSpeed.c_str());
  //   ClockRegister( clockSpeed, clockHandler );
  registerClock( clockSpeed, clockHandler );
}

// handle incoming NIC events. Just put them on the list for the
// user to pick up
// bool proc::handle_nic_events( Time_t t, Event *event) {
bool proc::handle_nic_events(Event *event) {
  _GPROC_DBG(4, "CPU %lu got a NIC event at time FIXME\n", Id());
  this->addNICevent(static_cast<CPUNicEvent *>(event));
  return false;
}

void proc::processMemDevResp( ) {
  instruction* inst;
  while ( memory[0]->popCookie( inst ) ) {

    if (mProcs.empty()) {
      continue;
    }

    //INFO("recived mem for addr=%#lx (type=?, tag=%p)\n", 
    //(unsigned long) 0, inst);
    if ((inst) == (instruction*)(~0)) {
      // if it is a returning writeback, just give it to the first
      // core
      mProcs[0]->handleMemEvent(inst);
    } else {
      // Check the standard memory request map
      memReqMap_t::iterator mi = memReqMap.find(inst);
      if (mi != memReqMap.end()) {
#if TIME_MEM
	numMemReq++;
	uint64_t startTime = mi->second.second;
	uint64_t lat = getCurrentSimTimeNano() - startTime;
	memReqLat += lat;
	static int c = 0; c++;
	if ((c % 0xff) == 0) printf("lat %llu\n", lat);
#endif
        // hand it off
	int owningProc = mi->second.first;
	mProcs[owningProc]->handleMemEvent(inst);

	// remove the reference
	memReqMap.erase(mi);
      } else {
	// check the special memory request map
	memReqMap_t::iterator mi2 = memSpecReqMap.find(inst);
	if (mi != memSpecReqMap.end()) {
#if TIME_MEM
	  uint64_t startTime = mi2->second.second;
	  uint64_t lat = getCurrentSimTimeNano() - startTime;
	  memSpecialReqLat += lat;
	  static int c = 0; c++;
	  if ((c % 0xff) == 0) printf("slat %llu\n", lat);
#endif
	  // decrement outstanding request count
	  int owningProc = mi2->second.first;
	  outstandingSpecReq[owningProc]--;

	  //remove the ref
	  memSpecReqMap.erase(mi2);	  
	} else {
	  ERROR("Got back memory req for instruction we never issued\n");
	  exit(-1);
	}
      }
    }
  }  
}

int proc::Setup() {
    _GPROC_DBG(1,"\n");
  registerExit();
  if (!mProcs.empty()) {
    for (int c = 0; c < cores; ++c) {
      mProcs[c]->setup();
    }
  } else {
    myThread = tSource.getFirstThread(0);
    INFO("proc got Thread %p\n", myThread);
    myThread->assimilate(this);
  }
  return 0;
}

int proc::Finish() {
    _GPROC_DBG(1,"\n");
  if (!mProcs.empty()) {
    for (int c = 0; c < cores; ++c) {
      mProcs[c]->finish();
    }
  }
#if TIME_MEM
  printf("%llu Total Memory Requests\n", totalMemReq);
  printf("%llu Timed Memory Requests, avg lat. %.2fns %llu stores\n", numMemReq, 
	 double(memReqLat)/double(numMemReq), memStores);
#endif
  printf("proc finished at %llu ns\n", getCurrentSimTimeNano());
  return false;
}

// if we have extra threads, try to swap them in 
void proc::swapThreads(bool quanta, bool refill) {
  // check if there is anyone to swap in...
  if (threadQ.empty()) {
    return;
  } else {
    // check if this is a normal thread quanta, or if we have already
    // told cores to flush
    if (quanta || refill) {
      // try to add threads until we have added them all, tried to, or
      // adding a thread failed
      bool done = false;
      int tries = threadQ.size();
      while (!done && (tries > 0)) {
	if (threadQ.empty()) {
	  done = true;
	} else {
	  thread *t = threadQ.front();
	  threadQ.pop_front();
	  if (addThread(t) == false) {
	    done = true;
	  }
	}
	tries--;
      }
      // if we weren't able to add them all, tell some processors to
      // flush, unless we are refilling already opened slots
      if (!threadQ.empty() && !refill) {
	int numToFlush = std::max(threadQ.size(), mProcs.size());
	int tellFlush = std::min(numToFlush, int(mProcs.size()));
	for (int i = 0; i < tellFlush; ++i) {
	  // tell the processor to start clearing its pipe
	  mProcs[i]->setClearPipe(1);
	}
	needThreadSwap = 1;
      } else {
	// make sure no one is left trying to clear
	for (vector<mainProc *>::iterator i = mProcs.begin();
	   i != mProcs.end(); ++i) {
	  (*i)->setClearPipe(0);
	}
	needThreadSwap = 0;
      }
    } else {
      // we have asked processors to clear their pipelines.
      // check which processors are available
      int cleared = 0;
      for (vector<mainProc *>::iterator i = mProcs.begin();
	   i != mProcs.end(); ++i) {
	mainProc *p = *i;
	if ((p->getThread() != NULL) && p->pipeClear()) {
	  // found a processor waiting for a new thread.
	  // save the old thread
	  threadQ.push_back(p->getThread());
	  // reset the processor to take new threads
	  p->setThread(NULL);
	  p->setClearPipe(0);
	  cleared++;
	}
      }
      // try to reschedual waiting threads, but don't try to clear
      // processors again if we can't
      swapThreads(0,1);
    }
  }
}

// processor preTic(). Runs some tests
bool proc::preTic(Cycle_t c) {

  processMemDevResp(); 

  if (!mProcs.empty()) {
    // Note: need to be able to adjust thread schedualer quantum
    if (((c & 0x1ffff) == 0))  {
      swapThreads(1,0);
    } else if (needThreadSwap && ((c & 0xf) == 0)) {
      swapThreads(0,0);
    }

    for (int c = 0; c < cores; ++c) {
      // sigh... we use this so we know which core is currently
      // running. 
      currentRunningCore = c;
      mProcs[c]->preTic();
      currentRunningCore = -1;
    }
  } else {
    if (myThread) {

      if (myThread->isDead()) {
	    tSource.deleteThread(myThread);
	    myThread = 0;
	    //_GPROC_DBG(1, " Dead Thread\n");
	    unregisterExit();
	    return false;
      }

      if ( onDeckInst == NULL ) {
        _GPROC_DBG(1,"getNextInstruction\n");
        onDeckInst = myThread->getNextInstruction(); 
      
        onDeckInst->fetch(this);
        onDeckInst->issue(this);
      }
 
      if (( onDeckInst->op() == LOAD || onDeckInst->op() == STORE) 
            && externalMemoryModel()) 
      {
	if ( sendMemoryReq( LOAD, 
			    (uint64_t)onDeckInst->memEA(), 0, 0) )
        {
          if (onDeckInst->commit(this)) {
	        myThread->retire(onDeckInst);
          } else {
	        WARN("instruction exception!!!");
          }
          onDeckInst = NULL;
        } else {
            _GPROC_DBG(1,"memory stalled\n");
        }
      } else {

        if (onDeckInst->commit(this)) {
	      myThread->retire(onDeckInst);
        } else {
	      WARN("instruction exception!!!");
        }
        onDeckInst = NULL;
      }
    }
  }

  return false;
}

bool proc::addThread(thread *t) {
  if (mProcs.size() < 2) {
    ERROR("adding threads to running process only allowed with multiple cores");
  } else {
    // find a processor with an open thread slot, add thread
    for (vector<mainProc *>::iterator i = mProcs.begin();
	 i != mProcs.end(); ++i) {
      mainProc *p = *i;
      // clean up dead threads
      thread *pt = p->getThread();
      if (pt && pt->isDead()) {
	_GPROC_DBG(2, "Removing Dead Thread from processor core\n");
	p->setThread(NULL);
	delete pt;
      }
      // check if processor is available
      if (p->getThread() == NULL) {
	p->setThread(t);
	return true;
      }
    }
    // couldn't find a slot
    _GPROC_DBG(2, "all cores full, queueing thread %p\n", t);
    threadQ.push_back(t);
    return false;
  }
}

int proc::postMessage(msgType t, simAddress addr, mainProc* poster) {
  if (!isCoherent())
    return 0;
  
  int nP = mProcs.size();
  for(int i=0; i < nP; ++i) {
    mainProc* targP = mProcs[i];
    if (targP != poster) {
      switch(t) {
      case READ_MISS:
	targP->coher.busReadMiss(addr); break;
      case WRITE_MISS:
	targP->coher.busWriteMiss(addr); break;
      case WRITE_HIT:
	targP->coher.busWriteHit(addr); break;
      default:
	printf("unknown bus message tyoe %d\n", t);
      }
    }
  }
  registerPost();
  return 0;
}

bool proc::insertThread(thread* t) {
  addThread(t);
  return true;
}

bool proc::isLocal(const simAddress, const simPID) {
  WARN("%s not supported\n", __FUNCTION__);
  return false;
}

bool proc::spawnToCoProc(const PIM_coProc, thread* t, simRegister hint) {
  addThread(t);
  return true;
}

bool proc::switchAddrMode(PIM_addrMode) {
  WARN("%s not supported\n", __FUNCTION__);
  return false;
}

exceptType proc::writeSpecial(const PIM_cmd cmd, const int nargs, 
			      const uint *args) {
  WARN("%s %d not supported\n", __FUNCTION__, cmd);
  return NO_EXCEPTION;
}

bool proc::forwardToNetsimNIC(int call_num, char *params, const size_t params_length,
    char *buf, const size_t buf_len)
{
  _GPROC_DBG(2, "%s: call_num is %d, params %p, params len %ld, buf %p, len %ld\n",
    __FUNCTION__, call_num, params, (unsigned long) params_length, buf,
    (unsigned long) buf_len);

  // This is where we create an event and send it to the NIC
  CPUNicEvent *event;
  event= new CPUNicEvent;
  event->AttachParams(params, params_length);
  event->SetRoutine(call_num);

  if (buf != NULL)   {
    // Also attach the send data
    event->AttachPayload(buf, buf_len);
  }

  // Send the event to the NIC
  CompEvent *e= static_cast<CompEvent *>(event);
  netLinks[0]->Send(e);

  return false;
}


bool proc::pickupNetsimNIC(CPUNicEvent **event) {
  if (!this->getNICresponse())   {
    _GPROC_DBG(5, "Nothing to pick-up from NIC %lu\n", Id());
    return false;
  }

  *event= this->getNICevent();
  _GPROC_DBG(4, "NIC %lu has data for the user\n", Id());
  return true;
}

bool proc::externalMemoryModel() {
  return externalMem;
}

bool proc::sendMemoryParcel(uint64_t address, instruction *inst, 
			    int mProcID) {
  // check and see if we can make another special request
  if (outstandingSpecReq[mProcID] < maxOutstandingSpecReq) {
    outstandingSpecReq[mProcID]++;
    
    // normally, we'd need a switch statement of some sort to figure out
    // what sort of request we are sending. For now, we just assume AMO
    
    // get UID
    static int count = 0; 
    // we use a unique identifier casted to a instruction * because we
    // don't actully need to track this instruction, and it is possible
    // to get multiple copies of the same poitner in the map
    int uid = (++count); if (uid == 0) uid = (++count);
    
    
    // send the memory request
    //bool ret = memory[0]->send(address, inst, memory_t::dev_t::event_t::RMW);
    // for now we can just pretend its a READ
    bool ret = memory[0]->read(address, (instruction*)uid);
    if (!ret) {
      INFO("Memory send parcel failed to Write");
    }
    // Register the instruction
    memReqMap_t::iterator mi = memSpecReqMap.find((instruction*)uid);
    if (mi != memSpecReqMap.end()) {
      WARN("UID already in use!\n");
    }
#if TIME_MEM
    memSpecReqMap[(instruction*)uid] = memReqRec_t(mProcID,getCurrentSimTimeNano());
#else
    memSpecReqMap[(instruction*)uid] = memReqRec_t(mProcID,0);
#endif
    
    static int c = 0; if ((c++ & 0xff) == 0) {
      printf("%d AMOs %ld\n", c, memSpecReqMap.size());    
    }
    
    return ret;
  } else {
    return false;
  }
}


bool proc::sendMemoryReq( instType iType,
            uint64_t address, instruction *i, int mProcID) {
    
  //note:it is not clear that a failure to send is handled anywhere...
#if TIME_MEM
  totalMemReq++;
#endif

  _GPROC_DBG(1,"instruction type %d\n", iType );
  if ( iType == STORE ) {
    if ( ! memory[0]->write(address, i ) ) {
      INFO("Memory Failed to Write");
      return false;
    }
  } else {
    if ( ! memory[0]->read(address, i ) ) {
      INFO("Memory Failed to Read");
      return false;
    }
  }   

  if (!mProcs.empty() && (i != (instruction*)(~0))) {    
    // if we need to, record the memory event
#if TIME_MEM
    memReqMap[i] = memReqRec_t(mProcID,getCurrentSimTimeNano());
#else
    memReqMap[i] = memReqRec_t(mProcID,0);
#endif
  }
  //INFO("Issuing mem req for addr=%#lx (type=%d, tag=%p) %d\n", 
  //     (unsigned long) address, iType, i, memReqMap.size());
  return true;
}

#if WANT_CHECKPOINT_SUPPORT
BOOST_CLASS_EXPORT(proc)

// // register event handler
// BOOST_CLASS_EXPORT_TEMPLATE4( SST::EventHandler,
//                                 proc, bool, SST::Time_t, SST::Event* )
// // register clock event
// BOOST_CLASS_EXPORT_TEMPLATE4( SST::EventHandler,
// 			      proc, bool, SST::Cycle_t, SST::Time_t )

BOOST_CLASS_EXPORT_TEMPLATE3( SST::EventHandler,
                                proc, bool, SST::Event* )
// register clock event
BOOST_CLASS_EXPORT_TEMPLATE3( SST::EventHandler,
			      proc, bool, SST::Cycle_t )
#endif
    
