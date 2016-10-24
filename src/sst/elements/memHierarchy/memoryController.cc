// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2016, Sandia Corporation
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

/* 
 * File:   memController.cc
 * Author: Caesar De la Paz III
 * Email:  caesar.sst@gmail.com
 */

#include <sst_config.h>
#include <sst/core/element.h>
#include <sst/core/params.h>
#include <sst/core/simulation.h>

#include "memoryController.h"
#include "util.h"

#include <sstream>
#include <stdio.h>
#include <fcntl.h>

#include "memEvent.h"
#include "membackend/memBackend.h"
#include "bus.h"
#include "cacheListener.h"
#include "memNIC.h"

#define NO_STRING_DEFINED "N/A"

using namespace std;
using namespace SST;
using namespace SST::MemHierarchy;


#ifdef __SST_DEBUG_OUTPUT__
#define Debug(level, fmt, ... ) dbg.debug( level, fmt, ##__VA_ARGS__  )
#else
#define Debug(level, fmt, ... )
#endif

/*************************** Memory Controller ********************/
MemController::MemController(ComponentId_t id, Params &params) : Component(id), reqId_(0) {
    int debugLevel = params.find<int>("debug_level", 0);
    
    // Output for debug
    dbg.init("--->  ", debugLevel, 0, (Output::output_location_t)params.find<int>("debug", 0));
    if (debugLevel < 0 || debugLevel > 10)
        dbg.fatal(CALL_INFO, -1, "Debugging level must be between 0 and 10. \n");
    dbg.debug(_L10_,"---");
    
    // Output for warnings
    Output out("", 1, 0, Output::STDOUT);
    
    
    // Check for deprecated parameters and warn/fatal
    // Currently deprecated - mem_size (replaced by backend.mem_size), network_num_vc, statistic, direct_link 
    bool found;
    params.find<int>("statistics", 0, found);
    if (found) {
        out.output("%s, **WARNING** ** Found deprecated parameter: statistics **  memHierarchy statistics have been moved to the Statistics API. Please see sst-info to view available statistics and update your input deck accordingly.\nNO statistics will be printed otherwise! Remove this parameter from your deck to eliminate this message.\n", getName().c_str());
    }
    params.find<std::string>("mem_size", "0B", found);
    if (found) {
        out.fatal(CALL_INFO, -1, "%s, Error - you specified memory size by the \"mem_size\" parameter, this must now be backend.mem_size WITH UNITS (e.g., 8GiB or 1024MiB), change the parameter name in your input deck.\n", getName().c_str());
    }
    params.find<int>("network_num_vc", 0, found);
    if (found) {
        out.output("%s, ** Found deprecated parameter: network_num_vc ** MemHierarchy does not use multiple virtual channels. Remove this parameter from your input deck to eliminate this message.\n", getName().c_str());
    }
    params.find<int>("direct_link", 0, found);
    if (found) {
        out.output("%s, ** Found deprecated parameter: direct_link ** The value of this parameter is now auto-detected by the link configuration in your input deck. Remove this parameter from your input deck to eliminate this message.\n", getName().c_str());
    }

    /* Check required parameters */
    string clock_freq = params.find<std::string>("clock", "", found);
    if (!found) {
        out.fatal(CALL_INFO, -1, "Param not specified (%s): clock - memory controller's clock rate (with units, e.g., MHz)\n", getName().c_str());
    }
    UnitAlgebra backendRamSize = UnitAlgebra(params.find<std::string>("backend.mem_size", "0B", found));
    if (!found) {
        out.fatal(CALL_INFO, -1, "Param not specified (%s): backend.mem_size - memory controller must have a size specified, (NEW) WITH units. E.g., 8GiB or 1024MiB. \n", getName().c_str());
    }
    if (!backendRamSize.hasUnits("B")) {
        out.fatal(CALL_INFO, -1, "Invalid param (%s): backend.mem_size - definition has CHANGED! Now requires units in 'B' (SI OK, ex: 8GiB or 1024MiB).\nSince previous definition was implicitly MiB, you may simply append 'MiB' to the existing value. You specified '%s'\n", getName().c_str(), backendRamSize.toString().c_str());
    }

    rangeStart_             = params.find<Addr>("range_start", 0);
    string ilSize           = params.find<std::string>("interleave_size", "0B");
    string ilStep           = params.find<std::string>("interleave_step", "0B");

    string memoryFile       = params.find<std::string>("memory_file", NO_STRING_DEFINED);
    cacheLineSize_          = params.find<uint32_t>("request_width", 64);
    string backendName      = params.find<std::string>("backend", "memHierarchy.simpleMem");
    string protocolStr      = params.find<std::string>("coherence_protocol", "MESI");
    string link_lat         = params.find<std::string>("direct_link_latency", "100 ns");
    doNotBack_              = params.find<bool>("do_not_back",false);

    // Requests per cycle -> limit to 1 per cycle for simpleMem, unlimited otherwise
    maxReqsPerCycle_        = params.find<int>("max_requests_per_cycle", -1, found);
    if (!found && backendName == "memHierarchy.simpleMem") {
        maxReqsPerCycle_ = 1;
    } else if (maxReqsPerCycle_ == 0) {
        maxReqsPerCycle_ = -1;
    }

    int addr = params.find<int>("network_address");
    std::string net_bw = params.find<std::string>("network_bw", "80GiB/s");
    
    const uint32_t listenerCount  = params.find<uint32_t>("listenercount", 0);
    char* nextListenerName   = (char*) malloc(sizeof(char) * 64);
    char* nextListenerParams = (char*) malloc(sizeof(char) * 64);

    for (uint32_t i = 0; i < listenerCount; ++i) {
	    sprintf(nextListenerName, "listener%" PRIu32, i);
	    string listenerMod     = params.find<std::string>(nextListenerName, "");

            if (listenerMod != "") {
		sprintf(nextListenerParams, "listener%" PRIu32 ".", i);
		Params listenerParams = params.find_prefix_params(nextListenerParams);

		CacheListener* loadedListener = dynamic_cast<CacheListener*>(loadSubComponent(listenerMod, this, listenerParams));
		listeners_.push_back(loadedListener);
	    }
    }

    free(nextListenerName);
    free(nextListenerParams);
    string traceFileLoc     = params.find<std::string>("trace_file", "");
     if ("" != traceFileLoc) {
        traceFP = fopen(traceFileLoc.c_str(), "wt");
    } else {
        traceFP = NULL;
    }


    // Check protocol string - note this is only used if directory controller is not present in system to ensure LLC gets the right permissions
    if (protocolStr != "MESI" && protocolStr != "mesi" && protocolStr != "msi" && protocolStr != "MSI" && protocolStr != "none" && protocolStr != "NONE") {
        dbg.fatal(CALL_INFO, -1, "Invalid param(%s): protocol - must be one of 'MESI', 'MSI', or 'NONE'. You specified '%s'\n", getName().c_str(), protocolStr.c_str());
    }
    // Convert into MBs
    memSize_ = backendRamSize.getRoundedValue();
    if (memSize_ % cacheLineSize_ != 0) {
        dbg.fatal(CALL_INFO, -1, "Invalid param(%s): backend.mem_size - must be a multiple of request_size. Note: use 'MB' for base-10 and 'MiB' for base-2. Please change one of these parameters. You specified backend.mem_size='%s' and request_size='%" PRIu32 "' B\n", getName().c_str(), backendRamSize.toString().c_str(), cacheLineSize_);
    }

    // Check interleave parameters
    fixByteUnits(ilSize);
    fixByteUnits(ilStep);
    interleaveSize_ = UnitAlgebra(ilSize).getRoundedValue();
    interleaveStep_ = UnitAlgebra(ilStep).getRoundedValue();
    if (!UnitAlgebra(ilSize).hasUnits("B") || interleaveSize_ % cacheLineSize_ != 0) {
        dbg.fatal(CALL_INFO, -1, "Invalid param(%s): interleave_size - must be specified in bytes with units (SI units OK) and must also be a multiple of request_size. This definition has CHANGED. Example: If you used to set this      to '1', change it to '1KiB'. You specified %s\n",
                getName().c_str(), ilSize.c_str());
    }
    if (!UnitAlgebra(ilStep).hasUnits("B") || interleaveStep_ % cacheLineSize_ != 0) {
        dbg.fatal(CALL_INFO, -1, "Invalid param(%s): interleave_step - must be specified in bytes with units (SI units OK) and must also be a multiple of request_size. This definition has CHANGED. Example: If you used to set this      to '4', change it to '4KiB'. You specified %s\n",
                getName().c_str(), ilStep.c_str());
    }

    requestWidth_           = cacheLineSize_;
	requestSize_            = cacheLineSize_;
    numPages_               = (interleaveStep_ > 0 && interleaveSize_ > 0) ? memSize_ / interleaveSize_ : 0;
    protocol_               = (protocolStr == "mesi" || protocolStr == "MESI") ? 1 : 0;
   

    // Ensure we can extract backend parameters for memH.
    Params backendParams = params.find_prefix_params("backend.");
    backend_                = dynamic_cast<MemBackend*>(loadSubComponent(backendName, this, backendParams));

    if (isPortConnected("direct_link")) {
        cacheLink_   = configureLink( "direct_link", link_lat, new Event::Handler<MemController>(this, &MemController::handleEvent));
        networkLink_ = NULL;
    } else {
        if (!isPortConnected("network")) {
            dbg.fatal(CALL_INFO,-1,"%s, Error: No connected port detected. Connect 'direct_link' or 'network' port.\n", getName().c_str());
        }
        MemNIC::ComponentInfo myInfo;
        myInfo.link_port        = "network";
        myInfo.link_bandwidth   = net_bw;
        myInfo.num_vcs          = 1;
        myInfo.name             = getName();
        myInfo.network_addr     = addr;
        myInfo.type             = MemNIC::TypeMemory;
        myInfo.link_inbuf_size  = params.find<std::string>("network_input_buffer_size", "1KiB");
        myInfo.link_outbuf_size = params.find<std::string>("network_output_buffer_size", "1KiB");
        networkLink_ = new MemNIC(this, &dbg, -1, myInfo, new Event::Handler<MemController>(this, &MemController::handleEvent));

        MemNIC::ComponentTypeInfo typeInfo;
        typeInfo.rangeStart       = rangeStart_;
        typeInfo.rangeEnd         = memSize_;
        typeInfo.interleaveSize   = interleaveSize_;
        typeInfo.interleaveStep   = interleaveStep_;
        networkLink_->addTypeInfo(typeInfo);
        cacheLink_ = NULL;
    }

    // Set up backing store if needed
    if (doNotBack_ && memoryFile != NO_STRING_DEFINED) {
        dbg.fatal(CALL_INFO, -1, "%s, Error - conflicting parameters. 'do_not_back_memory' cannot be true if 'memory_file' is specified. You specified: do_not_back_memory = %d, memory_file = %s\n",
                getName().c_str(), doNotBack_, memoryFile.c_str());
    }
    if (!doNotBack_) {
        int mmap_flags          = setBackingFile(memoryFile);
        memBuffer_              = (uint8_t*)mmap(NULL, memSize_, PROT_READ|PROT_WRITE, mmap_flags, backingFd_, 0);
        if (memBuffer_ == MAP_FAILED) {
            int err = errno;
            dbg.fatal(CALL_INFO,-1,"Failed to MMAP backing store for memory: %s, errno = %d\n", strerror(err), err);
        }
    }

    if (!backend_)          dbg.fatal(CALL_INFO,-1,"Unable to load Module %s as backend\n", backendName.c_str());

    stat_GetSReqReceived    = registerStatistic<uint64_t>("requests_received_GetS");
    stat_GetSExReqReceived  = registerStatistic<uint64_t>("requests_received_GetSEx");
    stat_GetXReqReceived    = registerStatistic<uint64_t>("requests_received_GetX");
    stat_PutMReqReceived    = registerStatistic<uint64_t>("requests_received_PutM");
    stat_outstandingReqs    = registerStatistic<uint64_t>("outstanding_requests");
    stat_GetSLatency        = registerStatistic<uint64_t>("latency_GetS");
    stat_GetSExLatency      = registerStatistic<uint64_t>("latency_GetSEx");
    stat_GetXLatency        = registerStatistic<uint64_t>("latency_GetX");
    stat_PutMLatency        = registerStatistic<uint64_t>("latency_PutM");

    cyclesWithIssue = registerStatistic<uint64_t>( "cycles_with_issue" );
    cyclesAttemptIssueButRejected = registerStatistic<uint64_t>(
	"cycles_attempted_issue_but_rejected" );
    totalCycles     = registerStatistic<uint64_t>( "total_cycles" );

    /* Clock Handler */
    registerClock(clock_freq, new Clock::Handler<MemController>(this, &MemController::clock));
    registerTimeBase("1 ns", true);
}


void MemController::handleEvent(SST::Event* event) {
    MemEvent *ev = static_cast<MemEvent*>(event);

    Debug(_L10_,"\n\n----------------------------------------------------------------------------------------\n");
    Debug(_L10_,"Memory Controller: %s - Event Received. Cmd = %s\n", getName().c_str(), CommandString[ev->getCmd()]);
    Debug(_L10_,"Event info: Addr: 0x%" PRIx64 ", dst = %s, src = %s, rqstr = %s, size = %d, prefetch = %d, vAddr = 0x%" PRIx64 ", instPtr = %" PRIx64 "\n",
        ev->getBaseAddr(), ev->getDst().c_str(), ev->getSrc().c_str(), ev->getRqstr().c_str(), ev->getSize(), ev->isPrefetch(), ev->getVirtualAddress(), ev->getInstructionPointer());

    Command cmd = ev->getCmd();
    ev->setDeliveryTime(getCurrentSimTimeNano());

    // Notify our listeners that we have received an event
    switch (cmd) {
        case GetS:
        case GetX:
        case GetSEx:
        case PutM:
            // Notify listeners that we have equiv. of a read
            if ( ! listeners_.empty()) {
		CacheListenerNotification notify(ev->getAddr(),	ev->getVirtualAddress(),
			ev->getInstructionPointer(), ev->getSize(), READ, HIT);

		for (unsigned long int i = 0; i < listeners_.size(); ++i) {
	        	listeners_[i]->notifyAccess(notify);
	    	}
	    }
            
            // Update statistics
            if (cmd == GetS)         stat_GetSReqReceived->addData(1);
            else if (cmd == GetX)    stat_GetXReqReceived->addData(1);
            else if (cmd == GetSEx)  stat_GetSExReqReceived->addData(1);
            else if (cmd == PutM)    stat_PutMReqReceived->addData(1);

            addRequest(ev);
            break;
        case PutS:
        case PutE:
            break;
        default:
            dbg.fatal(CALL_INFO,-1,"Memory controller received unrecognized command: %s", CommandString[cmd]);
    }
}


void MemController::addRequest(MemEvent* ev) {
    if ( !isRequestAddressValid(ev) ) {
        dbg.fatal(CALL_INFO, 1, "MemoryController \"%s\" received request from \"%s\" with invalid address.\n"
                "\t\tRequested Address:   0x%" PRIx64 "\n"
                "\t\tMC Range Start:      0x%" PRIx64 "\n"
                "\t\tMC Range End:        0x%" PRIx64 "\n"
                "\t\tMC Interleave Step:  0x%" PRIx64 "\n"
                "\t\tMC Interleave Size:  0x%" PRIx64 "\n",
                getName().c_str(),
                ev->getSrc().c_str(), ev->getAddr(),
                rangeStart_, (rangeStart_ + memSize_),
                interleaveStep_, interleaveSize_);
    }

#if 0
	if( cacheLineSize_ != ev->getSize() ) {
        dbg.fatal(CALL_INFO, -1, "CacheLineSize %d does not match request size %d\n",
													cacheLineSize_, ev->getSize() );
	}		
#endif

	uint32_t id = genReqId();
	MemReq* req = new MemReq( ev, id );
	requestQueue_.push_back( req );
	pendingRequests_[id] = req;
    
    Debug(_L10_,"Creating MemReq. BaseAddr = %" PRIx64 ", Size: %" PRIu64 ", %s\n", 
						req->baseAddr(), req->getSize(), CommandString[ev->getCmd()]);
}


bool MemController::clock(Cycle_t cycle) {
    totalCycles->addData(1);
    if (networkLink_) networkLink_->clock();

    int reqsThisCycle = 0;
    while ( !requestQueue_.empty()) {
        if (reqsThisCycle == maxReqsPerCycle_) {
            break;
        }

		MemReq* req = requestQueue_.front();

        bool issued = backend_->issueRequest( req->id(), req->addr(), req->isWrite(), requestSize_ );
        if (issued) {
    	    cyclesWithIssue->addData(1);
        } else {
    	    cyclesAttemptIssueButRejected->addData(1);
	        break;
        }

        reqsThisCycle++;
        req->increment( requestSize_ );		

        if ( req->processed() >= cacheLineSize_ ) {
            Debug(_L10_, "Completed issue of request\n");
            req->setResponse( performRequest( req->getMemEvent() ) );
            requestQueue_.pop_front();
        }
    }

    stat_outstandingReqs->addData(pendingRequests_.size());
    
    backend_->clock();

    return false;
}


MemEvent* MemController::performRequest(MemEvent* event) {
	bool noncacheable  = event->queryFlag(MemEvent::F_NONCACHEABLE);
    Addr localBaseAddr = convertAddressToLocalAddress(event->getBaseAddr());
    Addr localAddr;

	MemEvent* respEvent = NULL;

    if (event->getCmd() == PutM) {  /* Write request to memory */
        Debug(_L10_,"WRITE.  Addr = %" PRIx64 ", Request size = %i , Noncacheable Req = %s\n",localBaseAddr, event->getSize(), noncacheable ? "true" : "false");

        if ( ! doNotBack_) {
        	for ( size_t i = 0 ; i < event->getSize() ; i++)
            	memBuffer_[localBaseAddr + i] = event->getPayload()[i];
		}

    } else {
        Addr localAbsAddr = convertAddressToLocalAddress(event->getAddr());

        if (noncacheable && event->getCmd()== GetX) {
            Debug(_L10_,"WRITE. Noncacheable request, Addr = %" PRIx64 ", Request size = %i\n", localAbsAddr, event->getSize());

            if (!doNotBack_) {
                for ( size_t i = 0 ; i < event->getSize() ; i++)
                    memBuffer_[localAbsAddr + i] = event->getPayload()[i];
            }
        }

        if (noncacheable)   localAddr = localAbsAddr;
        else                localAddr = localBaseAddr;

        respEvent = event->makeResponse();

    if (respEvent->getSize() != event->getSize()) {
        dbg.fatal(CALL_INFO, -1, "Request and response sizes do not match: %" PRIu32 " != %" PRIu32 "\n",
            (uint32_t) respEvent->getSize(), (uint32_t) event->getSize());
        }

        Debug(_L10_, "READ.  Addr = %" PRIx64 ", Request size = %i\n", localAddr, event->getSize());

        for ( size_t i = 0 ; i < respEvent->getSize() ; i++)
            respEvent->getPayload()[i] = doNotBack_ ? 0 : memBuffer_[localAddr + i];

        if (noncacheable) respEvent->setFlag(MemEvent::F_NONCACHEABLE);

        if (event->getCmd() == GetX) respEvent->setGrantedState(M);
        else {
            if (protocol_) respEvent->setGrantedState(E); // Directory controller supersedes this; only used if DirCtrl does not exist
            else respEvent->setGrantedState(S);
        }
    }
	return respEvent;
}


void MemController::sendResponse( MemReq* req) {

	MemEvent* event = req->getMemEvent();
	MemEvent* resp = req->getResponse();
    if ( resp ) {
        if ( networkLink_ ) {
			networkLink_->send( resp );
		} else {
			cacheLink_->send( resp );
		}
	}
    
    uint64_t latency = getCurrentSimTimeNano() - event->getDeliveryTime(); 

    switch (event->getCmd()) {
        case GetS:
            stat_GetSLatency->addData(latency);
            break;
        case GetSEx:
            stat_GetSExLatency->addData(latency);
            break;
        case GetX:
            stat_GetXLatency->addData(latency);
            break;
        case PutM:
            stat_PutMLatency->addData(latency);
            break;
        default:
            break;
    }

	// MemReq deletes it's MemEvent
    delete req;
}


void MemController::handleMemResponse( ReqId reqId ) {

	uint32_t id = MemReq::getBaseId(reqId);

	if ( pendingRequests_.find( id ) == pendingRequests_.end() ) {
        dbg.fatal(CALL_INFO, -1, "memory request not found\n");
	}

	MemReq* req = pendingRequests_[id];
	
    Debug(_L10_, "Finishing processing MemReq. Addr = %" PRIx64 "\n", req->addr( ) );

	req->decrement( );

    if ( req->isDone() ) {
    	Debug(_L10_, "Finishing processing MemEvent. Addr = %" PRIx64 "\n", req->baseAddr( ) );
		sendResponse( req );
		pendingRequests_.erase(id);
	}
}

const std::string& MemController::getRequestor( ReqId reqId )
{
	uint32_t id = MemReq::getBaseId(reqId);
	if ( pendingRequests_.find( id ) == pendingRequests_.end() ) {
        dbg.fatal(CALL_INFO, -1, "memory request not found\n");
	}

    return pendingRequests_[id]->getMemEvent()->getRqstr();
}


MemController::~MemController() {
    while ( requestQueue_.size()) {
        delete requestQueue_.front();
		requestQueue_.pop_front();
    }
	PendingRequests::iterator iter = pendingRequests_.begin();	
	for ( ; iter != pendingRequests_.end(); ++ iter ) {
		delete iter->second; 
	}
}


void MemController::init(unsigned int phase) {
    if (!networkLink_) {
        SST::Event *ev = NULL;
        while (NULL != (ev = cacheLink_->recvInitData())) {
            MemEvent *me = dynamic_cast<MemEvent*>(ev);
            if (!me) {
                delete ev;
                return;
            }
            /* Push data to memory */
            if (GetX == me->getCmd()) {
                Debug(_L10_,"Memory init %s - Received GetX for %" PRIx64 "\n", getName().c_str(), me->getAddr());
                if (isRequestAddressValid(me) && !doNotBack_) {
                    Addr localAddr = convertAddressToLocalAddress(me->getAddr());
                    for ( size_t i = 0 ; i < me->getSize() ; i++) {
                        memBuffer_[localAddr + i] = me->getPayload()[i];
                    }
                }
            } else {
                Output out("", 0, 0, Output::STDERR);
                out.debug(_L10_,"Memory received unexpected Init Command: %d\n", me->getCmd());
            }
            delete ev;
        }
    } else {
        networkLink_->init(phase);

        while (MemEvent *ev = networkLink_->recvInitData()) {
            /* Push data to memory */
            if (ev->getDst() == getName()) {
                if (GetX == ev->getCmd()) {
                    Debug(_L10_,"Memory init %s - Received GetX for %" PRIx64 "\n", getName().c_str(), ev->getAddr());
                    if (isRequestAddressValid(ev) && !doNotBack_) {
                        Addr localAddr = convertAddressToLocalAddress(ev->getAddr());
                        for ( size_t i = 0 ; i < ev->getSize() ; i++) {
                            memBuffer_[localAddr + i] = ev->getPayload()[i];
                        }
                    }
                } else {
                    Output out("", 0, 0, Output::STDERR);
                    out.debug(_L10_,"Memory received unexpected Init Command: %d\n", ev->getCmd());
                }
            }
            delete ev; 
        }
    }
}


void MemController::setup(void) {
    backend_->setup();
    if (networkLink_) networkLink_->setup();
}


void MemController::finish(void) {
    if (!doNotBack_) munmap(memBuffer_, memSize_);
    if (-1 != backingFd_) close(backingFd_);

    // Close the trace file IF it is opened
    if (NULL != traceFP) {
        fclose(traceFP);
    }

    backend_->finish();
    if (networkLink_) networkLink_->finish();
}


void MemController::printMemory(Addr localAddr) {
#ifdef __SST_DEBUG_OUTPUT__
    dbg.debug(_L10_,"Resp. Data: ");
    for (unsigned int i = 0; i < cacheLineSize_; i++) dbg.debug(_L10_,"%d",(int)memBuffer_[localAddr + i]);
#endif
}


int MemController::setBackingFile(string memoryFile) {
	int mmap_flags = MAP_PRIVATE;
	if (NO_STRING_DEFINED != memoryFile) {
		backingFd_ = open(memoryFile.c_str(), O_RDWR);
		if (backingFd_ < 0) dbg.fatal(CALL_INFO,-1,"Failed to open backing file\n");
	} else {
		backingFd_  = -1;
		mmap_flags  |= MAP_ANON;
	}
    
    return mmap_flags;

}

