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
        { "destinations",                "(comma-separated list of ints) List of group IDs that serve as destinations for this component. If not specified, defaults to 'group + 1'.", "group+1"}

        SST_ELI_REGISTER_SUBCOMPONENT_DERIVED_API(SST::MemHierarchy::MemNICBase, SST::MemHierarchy::MemLinkBase)

        /* Constructor */
        MemNICBase(ComponentId_t id, Params &params, TimeConverter* tc) : MemLinkBase(id, params, tc) {
            build(params);
        }

        /* Destructor */
        virtual ~MemNICBase() { }

        // Router events
        class MemRtrEvent : public SST::Event {
            public:
                MemEventBase * event;
                MemRtrEvent() : Event(), event(nullptr) { }
                MemRtrEvent(MemEventBase * ev) : Event(), event(ev) { }

                virtual Event* clone(void) override {
                    MemRtrEvent *mre = new MemRtrEvent(*this);
                    if (this->event != nullptr)
                        mre->event = this->event->clone();
                    else
                        mre->event = nullptr;
                    return mre;
                }

                virtual bool hasClientData() const { return true; }

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
        virtual void sendInitData(MemEventInit * ev) {
            MemRtrEvent * mre = new MemRtrEvent(ev);
            SST::Interfaces::SimpleNetwork::Request* req = new SST::Interfaces::SimpleNetwork::Request();
            req->dest = SST::Interfaces::SimpleNetwork::INIT_BROADCAST_ADDR;
            req->givePayload(mre);
            initSendQueue.push(req);
        }

        virtual MemEventInit* recvInitData() {
            if (initQueue.size()) {
                MemRtrEvent * mre = initQueue.front();
                initQueue.pop();
                MemEventInit * ev = static_cast<MemEventInit*>(mre->event);
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

            stringstream error;
            error << getName() + " (MemNICBase) cannot find a destination for address " << std::hex << addr << endl;
            error << "Known destinations: " << endl;
            for (std::set<EndpointInfo>::const_iterator it = destEndpointInfo.begin(); it != destEndpointInfo.end(); it++) {
                error << it->name << " " << it->region.toString() << endl;
            }
            dbg.fatal(CALL_INFO, -1, "%s", error.str().c_str());
            return "";
        }

    protected:
        virtual void addSource(EndpointInfo info) { sourceEndpointInfo.insert(info); }
        virtual void addDest(EndpointInfo info) { destEndpointInfo.insert(info); }

        virtual InitMemRtrEvent* createInitMemRtrEvent() {
            return new InitMemRtrEvent(info);
        }

        virtual void processInitMemRtrEvent(InitMemRtrEvent* imre) {
            dbg.debug(_L10_, "%s (memNICBase) received imre. Name: %s, Addr: %" PRIu64 ", ID: %" PRIu32 ", start: %" PRIu64 ", end: %" PRIu64 ", size: %" PRIu64 ", step: %" PRIu64 "\n",
                    getName().c_str(), imre->info.name.c_str(), imre->info.addr, imre->info.id, imre->info.region.start, imre->info.region.end, imre->info.region.interleaveSize, imre->info.region.interleaveStep);

            if (sourceIDs.find(imre->info.id) != sourceIDs.end()) {
                addSource(imre->info);
                dbg.debug(_L10_, "\tAdding to sourceEndpointInfo. %zu sources found\n", sourceEndpointInfo.size());
            } else if (destIDs.find(imre->info.id) != destIDs.end()) {
                addDest(imre->info);
                dbg.debug(_L10_, "\tAdding to destEndpointInfo. %zu destinations found\n", destEndpointInfo.size());
            }
        }

        /* NIC initialization so that subclasses don't have to do this. Subclasses should call this during init() */
        virtual void nicInit(SST::Interfaces::SimpleNetwork * linkcontrol, unsigned int phase) {
            bool networkReady = linkcontrol->isNetworkInitialized();

            // After we've set up network and exchanged params, drain the send queue
            if (networkReady && initMsgSent) {
                while (!initSendQueue.empty()) {
                    linkcontrol->sendInitData(initSendQueue.front());
                    initSendQueue.pop();
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
                linkcontrol->sendInitData(req);
                initMsgSent = true;
            }

            // Expect different kinds of init events
            // 1. MemNIC - record these as needed and do not inform parent
            // 2. MemEventBase - only notify parent if sender is a src or dst for us
            // We should know since network is in order and NIC does its init before the
            // parents do
            while (SST::Interfaces::SimpleNetwork::Request *req = linkcontrol->recvInitData()) {
                Event * payload = req->takePayload();
                InitMemRtrEvent * imre = dynamic_cast<InitMemRtrEvent*>(payload);
                if (imre) {
                    // Record name->address map for all other endpoints
                    networkAddressMap.insert(std::make_pair(imre->info.name, imre->info.addr));
                    processInitMemRtrEvent(imre);
                    delete imre;
                } else {
                    MemRtrEvent * mre = static_cast<MemRtrEvent*>(payload);
                    MemEventInit *ev = static_cast<MemEventInit*>(mre->event);
                    dbg.debug(_L10_, "%s (memNICBase) received mre during init. %s\n", getName().c_str(), mre->event->getVerboseString().c_str());

                    /*
                     * Event is for us if:
                     *  1. We are the dst
                     *  2. Broadcast (dst = "") and:
                     *      src is a src/dst and a coherence init message
                     *      src is a src/dst?
                     */
                    if (ev->getInitCmd() == MemEventInit::InitCommand::Region) {
                        if (ev->getDst() == info.name) {
                            MemEventInitRegion * rEv = static_cast<MemEventInitRegion*>(ev);
                            if (rEv->getSetRegion() && acceptRegion) {
                                info.region = rEv->getRegion();
                                dbg.debug(_L10_, "\tUpdating local region\n");
                            }
                        }
                        delete ev;
                        delete mre;
                    } else if ((ev->getCmd() == Command::NULLCMD && (isSource(mre->event->getSrc()) || isDest(mre->event->getSrc()))) || ev->getDst() == info.name) {
                        dbg.debug(_L10_, "\tInserting in initQueue\n");
                        initQueue.push(mre);
                    }
                }
                delete req;
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
                MemEventBase* ev = (static_cast<MemRtrEvent*>(head->inspectPayload()))->event;
                std::string debugEvStr = ev ? ev->getBriefString() : "";
                uint64_t dst = head->dest;
                bool doDebug = ev ? is_debug_event(ev) : false;
#endif
                if (linkcontrol->spaceToSend(0, head->size_in_bits) && linkcontrol->send(head, 0)) {

#ifdef __SST_DEBUG_OUTPUT__
                    if (!debugEvStr.empty() && doDebug) {
                        dbg.debug(_L9_, "%s (memNICBase), Sending message %s to dst addr %" PRIu64 "\n",
                                getName().c_str(), debugEvStr.c_str(), dst);
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
                    } else if (destIDs.find(imre->info.id) != destIDs.end()) {
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

        // Init queues
        std::queue<MemRtrEvent*> initQueue; // Queue for received init events
        std::queue<SST::Interfaces::SimpleNetwork::Request*> initSendQueue; // Queue of events waiting to be sent after network (linkcontrol) initializes

        // Other parameters
        std::unordered_set<uint32_t> sourceIDs, destIDs; // IDs which this endpoint cares about

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
