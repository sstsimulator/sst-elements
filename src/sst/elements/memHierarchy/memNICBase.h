// Copyright 2013-2024 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2024, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _MEMHIERARCHY_MEMNICBASE_SUBCOMPONENT_H_
#define _MEMHIERARCHY_MEMNICBASE_SUBCOMPONENT_H_

#include <string>
#include <unordered_map>
#include <queue>

#include <sst/core/event.h>
#include <sst/core/output.h>
#include <sst/core/subcomponent.h>
#include <sst/core/interfaces/simpleNetwork.h>

#include "sst/elements/memHierarchy/memEventBase.h"
#include "sst/elements/memHierarchy/util.h"
#include "sst/elements/memHierarchy/memLinkBase.h"

namespace SST {
namespace MemHierarchy {

/* MemNIC Base class
 *  Base class to handle initialization and endpoint management for different NICs
 */
class MemNICBase : public MemLinkBase {
    public:

#define MEMNICBASE_ELI_PARAMS MEMLINKBASE_ELI_PARAMS, \
        { "group",                       "(int) Group ID. See params 'sources' and 'destinations'. If not specified, the parent component will guess.", "1"},\
        { "sources",                     "(comma-separated list of ints) List of group IDs that serve as sources for this component. If not specified, defaults to 'group - 1'.", "group-1"},\
        { "destinations",                "(comma-separated list of ints) List of group IDs that serve as destinations for this component. If not specified, defaults to 'group + 1'.", "group+1"},\
        { "range_check",                 "(int) Enable initial check for overlapping memory ranges. 0=Disabled 1=Enabled", "1"}

        SST_ELI_REGISTER_SUBCOMPONENT_DERIVED_API(SST::MemHierarchy::MemNICBase, SST::MemHierarchy::MemLinkBase)

        /* Constructor */
        MemNICBase(ComponentId_t id, Params &params, TimeConverter* tc) : MemLinkBase(id, params, tc) {
            build(params);
        }

        /* Destructor */
        virtual ~MemNICBase() { }

        // Router events
        class MemRtrEvent : public SST::Event {
            protected:
                MemEventBase * event;
            public:
                MemRtrEvent() : Event(), event(nullptr) { }
                MemRtrEvent(MemEventBase * ev) : Event(), event(ev) { }
                ~MemRtrEvent() {
                    if (event) {
                        delete event;
                    }
                }

                virtual Event* clone(void) override {
                    MemRtrEvent *mre = new MemRtrEvent(*this);
                    if (this->event != nullptr)
                        mre->event = this->event->clone();
                    else
                        mre->event = nullptr;
                    return mre;
                }

                void putEvent(MemEventBase* ev) {
                    event = ev;
                }

                MemEventBase* takeEvent() {
                    MemEventBase* tmp = event;
                    event = nullptr;
                    return tmp;
                }

                MemEventBase* inspectEvent() {
                    return event;
                }

                virtual bool hasClientData() const { return true; }

                virtual std::string toString() const override {
                    return event->toString();
                }

                void serialize_order(SST::Core::Serialization::serializer &ser) override {
                    Event::serialize_order(ser);
                    ser & event;
                }

                ImplementSerializable(SST::MemHierarchy::MemNICBase::MemRtrEvent);
        };

        class InitMemRtrEvent : public MemRtrEvent {
            public:
                EndpointInfo info;

                InitMemRtrEvent() {}
                InitMemRtrEvent(EndpointInfo info) : MemRtrEvent(), info(info) { }

                virtual Event* clone(void) override {
                    InitMemRtrEvent * imre = new InitMemRtrEvent(*this);
                    if (this->event != nullptr)
                        imre->event = this->event->clone();
                    else
                        imre->event = nullptr;
                    return imre;
                }

                virtual bool hasClientData() const override { return false; }
                
                virtual std::string toString() const override {
                    return info.toString();
                }

                void serialize_order(SST::Core::Serialization::serializer & ser) override {
                    MemRtrEvent::serialize_order(ser);
                    ser & info.name;
                    ser & info.addr;
                    ser & info.id;
                    ser & info.region.start;
                    ser & info.region.end;
                    ser & info.region.interleaveSize;
                    ser & info.region.interleaveStep;
                }

                ImplementSerializable(SST::MemHierarchy::MemNICBase::InitMemRtrEvent);
        };

        // Init functions
        virtual void sendUntimedData(MemEventInit* ev, bool broadcast = true) {
            DISABLE_WARN_DEPRECATED_DECLARATION
            sendInitData(ev, broadcast);
            REENABLE_WARNING
        }

        [[deprecated("sendInitData() has been deprecated and will be removed in SST 14.  Please use sendUntimedData().")]]
        virtual void sendInitData(MemEventInit * ev, bool broadcast = true) {
            if (!broadcast) {
                std::string dst = findTargetDestination(ev->getRoutingAddress());
                if (dst == "") {
                    // Hold this request until we know the right address
                    initWaitForDst.insert(ev);
                    return;
                }
                ev->setDst(dst);
            }
            MemRtrEvent * mre = new MemRtrEvent(ev);
            SST::Interfaces::SimpleNetwork::Request* req = new SST::Interfaces::SimpleNetwork::Request();
            req->dest = SST::Interfaces::SimpleNetwork::INIT_BROADCAST_ADDR;
            req->givePayload(mre);
            initSendQueue.push(req);
        }

        virtual MemEventInit* recvUntimedData() {
            DISABLE_WARN_DEPRECATED_DECLARATION
            auto ret = recvInitData();
            REENABLE_WARNING
                return ret;
        }
    
        [[deprecated("recvInitData() has been deprecated and will be removed in SST 14.  Please use recvUntimedData().")]]
        virtual MemEventInit* recvInitData() {
            if (initQueue.size()) {
                MemRtrEvent * mre = initQueue.front();
                initQueue.pop();
                MemEventInit * ev = static_cast<MemEventInit*>(mre->takeEvent());
                delete mre;
                return ev;
            }
            return nullptr;
        }

        virtual bool isSource(std::string str) { /* Note this is only used during init so doesn't need to be fast */
            for (std::set<EndpointInfo>::iterator it = sourceEndpointInfo.begin(); it != sourceEndpointInfo.end(); it++) {
                if (it->name == str) return true;
            }
            return false;
        }

        virtual bool isDest(std::string str) { /* Note this is only used during init so doesn't need to be fast */
            for (std::set<EndpointInfo>::iterator it = destEndpointInfo.begin(); it != destEndpointInfo.end(); it++) {
                if (it->name == str) return true;
            }
            return false;
        }

        virtual bool isClocked() { return true; } // Tell parent to trigger our clock

        virtual std::set<EndpointInfo>* getSources() { return &sourceEndpointInfo; }
        virtual std::set<EndpointInfo>* getDests() { return &destEndpointInfo; }
        
        virtual std::string findTargetDestination(Addr addr) {
            for (std::set<EndpointInfo>::const_iterator it = destEndpointInfo.begin(); it != destEndpointInfo.end(); it++) {
                if (it->region.contains(addr)) return it->name;
            }
            return "";
        }

        virtual std::string getTargetDestination(Addr addr) {
            std::string dst = findTargetDestination(addr);
            if (dst != "") {
                return dst;
            }

            stringstream error;
            error << getName() + " (MemNICBase) cannot find a destination for address " << std::hex << addr << endl;
            error << "Known destinations: " << endl;
            for (std::set<EndpointInfo>::const_iterator it = destEndpointInfo.begin(); it != destEndpointInfo.end(); it++) {
                error << it->name << " " << it->region.toString() << endl;
            }
            dbg.fatal(CALL_INFO, -1, "%s", error.str().c_str());
            return "";
        }

        virtual bool isReachable(std::string dst) {
            return reachableNames.find(dst) != reachableNames.end();
        }
        
        virtual std::string getAvailableDestinationsAsString() {
            stringstream str;
            for (std::set<EndpointInfo>::const_iterator it = destEndpointInfo.begin(); it != destEndpointInfo.end(); it++) {
                str << it->name << " " << it->region.toString() << endl;
            }
            return str.str();
        }


    protected:
        virtual void addSource(EndpointInfo info) { 
            sourceEndpointInfo.insert(info);
            reachableNames.insert(info.name);
        }
        virtual void addDest(EndpointInfo info) { 
            destEndpointInfo.insert(info); 
            reachableNames.insert(info.name);
        }

        virtual void addEndpoint(EndpointInfo info) { endpointInfo.insert(info); }

        virtual InitMemRtrEvent* createInitMemRtrEvent() {
            return new InitMemRtrEvent(info);
        }

        virtual void processInitMemRtrEvent(InitMemRtrEvent* imre) {

            if (sourceIDs.find(imre->info.id) != sourceIDs.end()) {
                addSource(imre->info);
                dbg.debug(_L10_, "%s (memNICBase) received source imre. Name: %s, Addr: %" PRIu64 ", ID: %" PRIu32 ", start: %" PRIu64 ", end: %" PRIu64 ", size: %" PRIu64 ", step: %" PRIu64 "\n",
                        getName().c_str(), imre->info.name.c_str(), imre->info.addr, imre->info.id, imre->info.region.start, imre->info.region.end, imre->info.region.interleaveSize, imre->info.region.interleaveStep);
            }
            
            if (destIDs.find(imre->info.id) != destIDs.end()) {
                addDest(imre->info);
                dbg.debug(_L10_, "%s (memNICBase) received dest imre. Name: %s, Addr: %" PRIu64 ", ID: %" PRIu32 ", start: %" PRIu64 ", end: %" PRIu64 ", size: %" PRIu64 ", step: %" PRIu64 "\n",
                        getName().c_str(), imre->info.name.c_str(), imre->info.addr, imre->info.id, imre->info.region.start, imre->info.region.end, imre->info.region.interleaveSize, imre->info.region.interleaveStep);
            }
        }

        /* NIC initialization so that subclasses don't have to do this. Subclasses should call this during init() */
        virtual void nicInit(SST::Interfaces::SimpleNetwork * linkcontrol, unsigned int phase) {
            bool networkReady = linkcontrol->isNetworkInitialized();

            // After we've set up network and exchanged params, drain the send queue
            if (networkReady && initMsgSent) {
                while (!initSendQueue.empty()) {
                    linkcontrol->sendUntimedData(initSendQueue.front());
                    initSendQueue.pop();
                }

                for (auto it = initWaitForDst.begin(); it != initWaitForDst.end();) {
                    std::string dst = findTargetDestination((*it)->getRoutingAddress());
                    if (dst != "") {
                        (*it)->setDst(dst);
                        MemRtrEvent * mre = new MemRtrEvent(*it);
                        SST::Interfaces::SimpleNetwork::Request* req = new SST::Interfaces::SimpleNetwork::Request();
                        req->dest = SST::Interfaces::SimpleNetwork::INIT_BROADCAST_ADDR;
                        req->givePayload(mre);
                        linkcontrol->sendUntimedData(req);
                        it = initWaitForDst.erase(it);
                    } else {
                        it++;
                    }
                }
            }

            // On first init round, send our region out to all others
            if (networkReady && !initMsgSent) {
                info.addr = linkcontrol->getEndpointID();
                InitMemRtrEvent *ev = createInitMemRtrEvent();

                SST::Interfaces::SimpleNetwork::Request * req = new SST::Interfaces::SimpleNetwork::Request();
                req->dest = SST::Interfaces::SimpleNetwork::INIT_BROADCAST_ADDR;
                req->src = info.addr;
                req->givePayload(ev);
                linkcontrol->sendUntimedData(req);
                initMsgSent = true;
            }

            // Expect different kinds of init events
            // 1. MemNIC - record these as needed and do not inform parent
            // 2. MemEventBase - only notify parent if sender is a src or dst for us
            // We should know since network is in order and NIC does its init before the
            // parents do
            while (SST::Interfaces::SimpleNetwork::Request *req = linkcontrol->recvUntimedData()) {
                Event * payload = req->takePayload();
                InitMemRtrEvent * imre = dynamic_cast<InitMemRtrEvent*>(payload);
                if (imre) {
                    // Record name->address map for all other endpoints
                    networkAddressMap.insert(std::make_pair(imre->info.name, imre->info.addr));
                    processInitMemRtrEvent(imre);
                    delete imre;
                } else {
                    MemRtrEvent * mre = static_cast<MemRtrEvent*>(payload);
                    MemEventInit *ev = static_cast<MemEventInit*>(mre->takeEvent()); // mre no longer has a copy of its event
                    dbg.debug(_L10_, "%s (memNICBase) received mre during init. %s\n", getName().c_str(), ev->getVerboseString(dlevel).c_str());

                    /*
                     * Event is for us if:
                     *  1. We are the dst
                     *  2. Broadcast (dst = "") and:
                     *      src is a src/dst and a coherence init message
                     *      src is a src/dst?
                     */
                    if (ev->getInitCmd() == MemEventInit::InitCommand::Region) {
                        delete ev;
                        delete mre;
                    } else if (ev->getInitCmd() == MemEventInit::InitCommand::Endpoint) {
                        // Intercept and record so that we know how to find this endpoint if we need to
                        // for noncacheable accesses. We don't need to record whether a particular region
                        // is noncacheable because the StandardMem interfaces will enforce
                        MemEventInitEndpoint * mEvEndPt = static_cast<MemEventInitEndpoint*>(ev);
                        if (!isDest(mEvEndPt->getSrc())) {
                            delete ev;
                            delete mre;
                        } else {
                            dbg.debug(_L10_, "%s received init message: %s\n", getName().c_str(), mEvEndPt->getVerboseString(dlevel).c_str());
                            std::vector<std::pair<MemRegion,bool>> regions = mEvEndPt->getRegions();
                            for (auto it = regions.begin(); it != regions.end(); it++) {
                                EndpointInfo epInfo;
                                epInfo.name = mEvEndPt->getSrc();
                                epInfo.addr = 0; // Not on a network so don't need it
                                epInfo.id = 0; // Not on a network so don't need it
                                epInfo.region = it->first;
                                addEndpoint(epInfo);
                            }
                            mre->putEvent(ev); // If we did not delete the Event, give it back to the MemRtrEvent
                            initQueue.push(mre); // Our component will forward on all its other ports
                        }
                    } else if ((ev->getCmd() == Command::NULLCMD && (isSource(ev->getSrc()) || isDest(ev->getSrc()))) || ev->getDst() == info.name) {
                        dbg.debug(_L10_, "\tInserting in initQueue\n");
                        mre->putEvent(ev); // If we did not delete the Event, give it back to the MemRtrEvent
                        initQueue.push(mre);
                    } else {
			delete mre;
			delete ev;
		    }
                }
                delete req;
            }
        }
        
        // Setup
        // Clean up state generated during init() and perform some sanity checks
        virtual void setup() {
            /* Limit destinations to the memory regions reported by endpoint messages that came through them */
            
            std::set<std::string> names;
            std::set<EndpointInfo> newDests;
            for (auto it = endpointInfo.begin(); it != endpointInfo.end(); it++) {
                names.insert(it->name);
            }
            
            dbg.debug(_L10_, "Routing information for %s\n", getName().c_str());
            for (auto it = destEndpointInfo.begin(); it != destEndpointInfo.end(); it++) {
                //dbg.debug(_L10_, "    Orig Dest: %s\n", it->toString().c_str());
                if (names.find(it->name) != names.end()) {
                    for (auto et = endpointInfo.begin(); et != endpointInfo.end(); et++) {
                        if (it->name == et->name) {
                            std::set<MemRegion> reg = (it->region).intersect(et->region);
                            for (auto mt = reg.begin(); mt != reg.end(); mt++) {
                                EndpointInfo epInfo;
                                epInfo.name = it->name;
                                epInfo.addr = it->addr;
                                epInfo.id = it->id;
                                epInfo.region = (*mt);
                                newDests.insert(epInfo);
                            }
                        }
                    }
                } else {
                    newDests.insert(*it); // Copy into the new set
                }
            }
            destEndpointInfo = newDests;

            // This algorithm can take an extremely long time for some memory configurations.
            if (range_check > 0) {
                int stopAfter = 20; // This is error checking, if it takes too long, stop
                for (auto et = destEndpointInfo.begin(); et != destEndpointInfo.end(); et++) {
                    for (auto it = std::next(et,1); it != destEndpointInfo.end(); it++) {
                        if (it->name == et->name) continue; // Not a problem
                        if ((it->region).doesIntersect(et->region)) {
                            dbg.fatal(CALL_INFO, -1, "%s, Error: Found destinations on the network with overlapping address regions. Cannot generate routing table."
                                    "\n  Destination 1: %s\n  Destination 2: %s\n", 
                                    getName().c_str(), it->toString().c_str(), et->toString().c_str());
                        }
                        stopAfter--;
                        if (stopAfter == 0) {
                            stopAfter = -1;
                            break;
                        }
                    }
                    if (stopAfter <= 0) {
                        stopAfter = -1;
                        break;
                    }
                }
                if (stopAfter == -1)
                    dbg.debug(_L2_, "%s, Notice: Too many regions to complete error check for overlapping destination regions. Checked first 20 pairs. To disable this check set range_check parameter to 0\n",
                            getName().c_str());
            }

            for (auto it = networkAddressMap.begin(); it != networkAddressMap.end(); it++) {
                dbg.debug(_L10_, "    Address: %s -> %" PRIu64 "\n", it->first.c_str(), it->second);
            }
            for (auto it = sourceEndpointInfo.begin(); it != sourceEndpointInfo.end(); it++) {
                dbg.debug(_L10_, "    Source: %s\n", it->toString().c_str()); 
            }
            for (std::set<EndpointInfo>::const_iterator it = destEndpointInfo.begin(); it != destEndpointInfo.end(); it++) {
                dbg.debug(_L10_, "    Dest: %s\n", it->toString().c_str()); 
            }
            for (auto it = endpointInfo.begin(); it != endpointInfo.end(); it++) {
                dbg.debug(_L10_, "    Endpoint: %s\n", it->toString().c_str()); 
            }

            if (!initWaitForDst.empty()) {
                dbg.fatal(CALL_INFO, -1, "%s, Error: Unable to find destination for init event %s\n",
                        getName().c_str(), (*initWaitForDst.begin())->getVerboseString(dlevel).c_str());
            }
        }

        // Lookup the network address for a given endpoint
        virtual uint64_t lookupNetworkAddress(const std::string &dst) const {
            std::unordered_map<std::string,uint64_t>::const_iterator it = networkAddressMap.find(dst);
            if (it == networkAddressMap.end()) {
                dbg.fatal(CALL_INFO, -1, "%s (MemNICBase), Network address for destination '%s' not found in networkAddressMap.\n", getName().c_str(), dst.c_str());
            }
            return it->second;
        }

        /*
         * Some helper functions to avoid needing to repeat code everywhere
         */

        // Get a packet header size parameter & error check it
        size_t extractPacketHeaderSize(Params &params, std::string pname, std::string defsize = "8B") {
            UnitAlgebra size = UnitAlgebra(params.find<std::string>(pname, defsize));
            if (!size.hasUnits("B"))
                dbg.fatal(CALL_INFO, -1, "Invalid param(%s): %s - must have units of bytes (B). SI units OK. You specified '%s'\n.",
                        getName().c_str(), pname.c_str(), size.toString().c_str());
            return size.getRoundedValue();
        }

        // Drain a send queue
        void drainQueue(std::queue<SST::Interfaces::SimpleNetwork::Request*>* queue, SST::Interfaces::SimpleNetwork* linkcontrol) {
            while (!(queue->empty())) {
                SST::Interfaces::SimpleNetwork::Request* head = queue->front();
#ifdef __SST_DEBUG_OUTPUT__
                MemEventBase* ev = (static_cast<MemRtrEvent*>(head->inspectPayload()))->inspectEvent();
                std::string debugEvStr = ev ? ev->getBriefString() : "";
                uint64_t dst = head->dest;
                bool doDebug = ev ? is_debug_event(ev) : false;
#endif
                if (linkcontrol->spaceToSend(0, head->size_in_bits) && linkcontrol->send(head, 0)) {

#ifdef __SST_DEBUG_OUTPUT__
                    if (!debugEvStr.empty() && doDebug) {
                        dbg.debug(_L4_, "E: %-20" PRIu64 " %-20" PRIu64 " %-20s Event:Send    (%s), Dst: %" PRIu64 "\n",
                                getCurrentSimCycle(), 0, getName().c_str(), debugEvStr.c_str(), dst);
                    }
#endif
                    queue->pop();
                } else {
                    break;
                }
            }
        }

        MemRtrEvent* doRecv(SST::Interfaces::SimpleNetwork* linkcontrol) {
            SST::Interfaces::SimpleNetwork::Request* req = linkcontrol->recv(0);
            if (req != nullptr) {
                MemRtrEvent * mre = static_cast<MemRtrEvent*>(req->takePayload());
                delete req;

                if (mre->hasClientData()) {
                    return mre;
                } else {
                    InitMemRtrEvent * imre = static_cast<InitMemRtrEvent*>(mre);
                    if (networkAddressMap.find(imre->info.name) == networkAddressMap.end()) {
                        dbg.fatal(CALL_INFO, -1, "%s received information about previously unknown endpoint. This case is not handled. Endpoint name: %s\n",
                                getName().c_str(), imre->info.name.c_str());
                    }
                    if (sourceIDs.find(imre->info.id) != sourceIDs.end()) {
                        addSource(imre->info);
                    } 
                    if (destIDs.find(imre->info.id) != destIDs.end()) {
                        addDest(imre->info);
                    }
                    delete imre;
                }
            }
            return nullptr;
        }

        /*** Data Members ***/
        bool initMsgSent;

        // Data structures
        std::unordered_map<std::string,uint64_t> networkAddressMap; // Map of name -> address for each network endpoint
        std::set<EndpointInfo> sourceEndpointInfo;
        std::set<EndpointInfo> destEndpointInfo;
        std::set<EndpointInfo> endpointInfo;
        std::set<std::string> reachableNames;

        // Init queues
        std::queue<MemRtrEvent*> initQueue; // Queue for received init events
        std::queue<SST::Interfaces::SimpleNetwork::Request*> initSendQueue; // Queue of events waiting to be sent after network (linkcontrol) initializes
        std::set<MemEventInit*> initWaitForDst; // Set of events with unknown destinations    

        // Other parameters
        std::unordered_set<uint32_t> sourceIDs, destIDs; // IDs which this endpoint cares about
        uint32_t range_check = true; // Enable overlapping range check

    private:

        void build(Params& params) {
            // Get source/destination parameters
            // Each NIC has a group ID and talks to those with IDs in sources and destinations
            // If no source/destination provided, source = group ID - 1, destination = group ID + 1
            bool found;
            info.id = params.find<uint32_t>("group", 0, found);
            if (!found) {
                dbg.fatal(CALL_INFO, -1, "Param not specified(%s): group - group ID (or hierarchy level) for this NIC's component. Example: L2s in group 1, directories in group 2, memories (on network) in group 3.\n",
                        getName().c_str());
            }
            
            if (params.is_value_array("sources")) {
                std::vector<uint32_t> srcArr;
                params.find_array<uint32_t>("sources", srcArr);
                sourceIDs = std::unordered_set<uint32_t>(srcArr.begin(), srcArr.end());

            }
            if (params.is_value_array("destinations")) {
                std::vector<uint32_t> dstArr;
                params.find_array<uint32_t>("destinations", dstArr);
                destIDs = std::unordered_set<uint32_t>(dstArr.begin(), dstArr.end());
            }
            
            // range_check current is off(0) or on(1) but is using a uint32_t to 
            // allow for future selection of different algorithms.
            range_check=params.find<uint32_t>("range_check", 1);

            std::stringstream sources, destinations;
            uint32_t id;

            if (sourceIDs.empty()) {
                sources.str(params.find<std::string>("sources", ""));
                while (sources >> id) {
                    sourceIDs.insert(id);
                    while (sources.peek() == ',' || sources.peek() == ' ')
                        sources.ignore();
                }
                if (sourceIDs.empty())
                    sourceIDs.insert(info.id - 1);
            }

            if (destIDs.empty()) {
                destinations.str(params.find<std::string>("destinations", ""));
                while (destinations >> id) {
                    destIDs.insert(id);
                    while (destinations.peek() == ',' || destinations.peek() == ' ')
                        destinations.ignore();
                }
                if (destIDs.empty())
                    destIDs.insert(info.id + 1);
            }
            initMsgSent = false;

            dbg.debug(_L10_, "%s memNICBase info is: Name: %s, group: %" PRIu32 "\n",
                    getName().c_str(), info.name.c_str(), info.id);
        }
};

} //namespace memHierarchy
} //namespace SST

#endif
