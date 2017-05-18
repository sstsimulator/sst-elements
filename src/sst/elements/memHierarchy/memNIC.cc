// Copyright 2013-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2017, Sandia Corporation
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>
#include "memNIC.h"

#include <algorithm>

#include <sst/core/params.h>
#include <sst/core/simulation.h>
#include <sst/core/timeLord.h>



using namespace SST;
using namespace SST::MemHierarchy;
using namespace SST::Interfaces;

/* Translates a MemEvent string destination to a network address (integer) */
int MemNIC::addrForDest(const std::string &target) const
{
  std::map<std::string, int>::const_iterator addrIter = addrMap.find(target);
  if ( addrIter == addrMap.end() )
      dbg->fatal(CALL_INFO, -1, "Address for target %s not found in addrMap.\n", target.c_str());
  return addrIter->second;
}

bool MemNIC::isValidDestination(std::string target) {
    return (addrMap.find(target) != addrMap.end());
}

void MemNIC::setMinPacketSize(unsigned int bytes) {
    packetHeaderBytes = bytes;
}

/* Get size in bytes for a MemEvent */
int MemNIC::getSizeInBits(MemEvent *ev)
{
    /* addr (8B) + cmd (1B) + size */
    return 8 * (packetHeaderBytes + ev->getPayloadSize());
}

/* Given a destination, convert a global address to a local address (assuming local addresses are contiguous) */
Addr MemNIC::convertToDestinationAddress(const std::string &dst, Addr addr) {
    std::unordered_map<std::string, ComponentTypeInfo>::iterator cit = peerAddrs.find(dst);
    if (cit == peerAddrs.end()) dbg->fatal(CALL_INFO, -1, "Peer %s not found in peerAddrs\n", dst.c_str());

    if (0 == (*cit).second.interleaveSize) return addr - (*cit).second.rangeStart;
    
    Addr a = addr - (*cit).second.rangeStart;
    Addr step = a / (*cit).second.interleaveStep;
    Addr offset = a % (*cit).second.interleaveStep;
    Addr retAddr = (step * (*cit).second.interleaveSize) + offset;
    dbg->debug(_L10_, "%s converted source address %" PRIu64 " to destination address %" PRIu64 " for destination: %s\n", comp->getName().c_str(), addr, retAddr, dst.c_str());
    return retAddr;
}


void MemNIC::moduleInit(ComponentInfo &ci, Event::HandlerBase *handler)
{
    this->ci = ci;
    this->recvHandler = handler;

    num_vcs = ci.num_vcs;

    last_recv_vc = 0;

    Params params; // LinkControl doesn't actually use the params
    link_control = (SimpleNetwork*)comp->loadSubComponent("merlin.linkcontrol", comp, params);
    link_control->initialize(ci.link_port, UnitAlgebra(ci.link_bandwidth), num_vcs, UnitAlgebra(ci.link_inbuf_size), UnitAlgebra(ci.link_outbuf_size));

    recvNotifyHandler = new SimpleNetwork::Handler<MemNIC>(this, &MemNIC::recvNotify);
    link_control->setNotifyOnReceive(recvNotifyHandler);
}


MemNIC::MemNIC(Component *comp, Output* output, Addr dAddr, ComponentInfo &ci, Event::HandlerBase *handler) :
    typeInfoSent(false), comp(comp)
{
    dbg = output;
    DEBUG_ADDR = dAddr;
    DEBUG_ALL = (dAddr == -1);
    moduleInit(ci, handler);
}


MemNIC::MemNIC(Component *comp, Params& params) :
    typeInfoSent(false), comp(comp)
{
    rangeStart_     = params.find<Addr>("range_start", 0);
    string ilSize   = params.find<std::string>("interleave_size", "0B");
    string ilStep   = params.find<std::string>("interleave_step", "0B");

    cacheLineSize_  =  params.find<uint32_t>("cacheLineSize", 64);

    // Check interleave parameters
    fixByteUnits(ilSize);
    fixByteUnits(ilStep);
    interleaveSize_ = UnitAlgebra(ilSize).getRoundedValue();
    interleaveStep_ = UnitAlgebra(ilStep).getRoundedValue();

    bool found;
    UnitAlgebra backendRamSize = UnitAlgebra(params.find<std::string>("mem_size", "0B", found));
    if (!found) {
        dbg->fatal(CALL_INFO, -1, "Param not specified (%s): backend.mem_size - memory controller must have a size specified, (NEW) WITH units. E.g., 8GiB or 1024MiB. \n", comp->getName().c_str());
    }
    if (!backendRamSize.hasUnits("B")) {
        dbg->fatal(CALL_INFO, -1, "Invalid param (%s): backend.mem_size - definition has CHANGED! Now requires units in 'B' (SI OK, ex: 8GiB or 1024MiB).\nSince previous definition was implicitly MiB, you may simply append 'MiB' to the existing value. You specified '%s'\n", comp->getName().c_str(), backendRamSize.toString().c_str());
    }


    if (!UnitAlgebra(ilSize).hasUnits("B") || interleaveSize_ % cacheLineSize_ != 0) {
        dbg->fatal(CALL_INFO, -1, "Invalid param(%s): interleave_size - must be specified in bytes with units (SI units OK) and must also be a multiple of request_size. This definition has CHANGED. Example: If you used to set this      to '1', change it to '1KiB'. You specified %s\n",
                comp->getName().c_str(), ilSize.c_str());
    }

    if (!UnitAlgebra(ilStep).hasUnits("B") || interleaveStep_ % cacheLineSize_ != 0) {
        dbg->fatal(CALL_INFO, -1, "Invalid param(%s): interleave_step - must be specified in bytes with units (SI units OK) and must also be a multiple of request_size. This definition has CHANGED. Example: If you used to set this      to '4', change it to '4KiB'. You specified %s\n",
                comp->getName().c_str(), ilStep.c_str());
    }

    numPages_               = (interleaveStep_ > 0 && interleaveSize_ > 0) ? memSize_ / interleaveSize_ : 0;

    int addr = params.find<int>("network_address");
    std::string net_bw = params.find<std::string>("network_bw", "80GiB/s");

    MemNIC::ComponentInfo myInfo;
    myInfo.link_port        = "network";
    myInfo.link_bandwidth   = net_bw;
    myInfo.num_vcs          = 1;
    myInfo.name             = comp->getName();
    myInfo.network_addr     = addr;
    myInfo.type             = MemNIC::TypeMemory;
    myInfo.link_inbuf_size  = params.find<std::string>("network_input_buffer_size", "1KiB");
    myInfo.link_outbuf_size = params.find<std::string>("network_output_buffer_size", "1KiB");
    moduleInit( myInfo, NULL );

    MemNIC::ComponentTypeInfo typeInfo;
    typeInfo.rangeStart       = rangeStart_;
    typeInfo.rangeEnd         = memSize_;
    typeInfo.interleaveSize   = interleaveSize_;
    typeInfo.interleaveStep   = interleaveStep_;
    addTypeInfo( typeInfo );
}


void MemNIC::setup(void)
{
    link_control->setup();
    while ( initQueue.size() ) {
        delete initQueue.front();
        initQueue.pop_front();
    }
}


void MemNIC::init(unsigned int phase)
{

    link_control->init(phase);
    if ( !phase ) {
        InitMemRtrEvent *ev;
        SimpleNetwork::Request* req;
        if ( typeInfoList.empty() ) {
            ev = new InitMemRtrEvent(comp->getName(), ci.network_addr, ci.type);
            req = new SimpleNetwork::Request();
            req->dest = SimpleNetwork::INIT_BROADCAST_ADDR;
            req->src = ci.network_addr;
            req->givePayload(ev);
            link_control->sendInitData(req);
        } else {
            for ( std::vector<ComponentTypeInfo>::iterator i = typeInfoList.begin() ;
                    i != typeInfoList.end() ; ++i ) {
                ev = new InitMemRtrEvent(comp->getName(), ci.network_addr, ci.type, *i);
                req = new SimpleNetwork::Request();
                req->dest = SimpleNetwork::INIT_BROADCAST_ADDR;
                req->src = ci.network_addr;
                req->givePayload(ev);
                link_control->sendInitData(req);
            }
        }
        typeInfoSent = true;
        dbg->debug(_L10_, "Sent init data!\n");
    }
    // while ( SST::Event *ev = link_control->recvInitData() ) {
    while ( SimpleNetwork::Request *req = link_control->recvInitData() ) {
        // InitMemRtrEvent *imre = dynamic_cast<InitMemRtrEvent*>(ev);
        Event* payload = req->takePayload();
        InitMemRtrEvent *imre = dynamic_cast<InitMemRtrEvent*>(payload);
        if ( imre ) {
            addrMap[imre->name] = imre->address;
            
            ComponentInfo peerCI;
            peerCI.name = imre->name;
            peerCI.network_addr = imre->address;
            peerCI.type = imre->compType;
            peers.push_back(std::make_pair(peerCI, imre->compInfo));
            // save peer for lookup later for address conversion
            peerAddrs[peerCI.name] = imre->compInfo; 

            // save a copy for lookups later if we should be sending requests to this entity
            if ((ci.type == MemNIC::TypeCache || ci.type == MemNIC::TypeNetworkCache) && (peerCI.type == MemNIC::TypeDirectoryCtrl || peerCI.type == MemNIC::TypeNetworkDirectory)) { // cache -> dir
                destinations[imre->compInfo] = imre->name;
            } else if (ci.type == MemNIC::TypeCacheToCache && peerCI.type == MemNIC::TypeNetworkCache) { // higher cache -> lower cache
                destinations[imre->compInfo] = imre->name;
            } else if (ci.type == MemNIC::TypeSmartMemory && (peerCI.type == MemNIC::TypeSmartMemory || peerCI.type == MemNIC::TypeDirectoryCtrl || peerCI.type == MemNIC::TypeNetworkDirectory ) ) {
                destinations[imre->compInfo] = imre->name;
            } else if (ci.type == MemNIC::TypeScratch && peerCI.type == MemNIC::TypeMemory) {
                destinations[imre->compInfo] = imre->name;
            }
        } else {
            initQueue.push_back(static_cast<MemRtrEvent*>(payload));
        }
        delete req;
    }

}


void MemNIC::finish(void)
{
    link_control->finish();
}


void MemNIC::sendInitData(MemEvent *ev)
{
    MemRtrEvent *mre = new MemRtrEvent(ev);
    SimpleNetwork::Request* req = new SimpleNetwork::Request();
    /* TODO:  Better addressing */
    req->dest = SimpleNetwork::INIT_BROADCAST_ADDR;
    req->givePayload(mre);
    link_control->sendInitData(req);
}

MemEvent* MemNIC::recvInitData(void)
{
    if ( initQueue.size() ) {
        MemRtrEvent *mre = initQueue.front();
        initQueue.pop_front();
        MemEvent *ev = mre->event;
        delete mre;
        return ev;
    }
    return NULL;
}

bool MemNIC::initDataReady() {
    return (initQueue.size() > 0);
}

std::string MemNIC::findTargetDestination(Addr addr)
{
    for ( std::map<MemNIC::ComponentTypeInfo, std::string>::const_iterator i = destinations.begin() ;
            i != destinations.end() ; ++i ) {
        if ( i->first.contains(addr) ) return i->second;
    }
    dbg->fatal(CALL_INFO,-1,"MemNIC %s cannot find a target for address 0x%" PRIx64 "\n",comp->getName().c_str(),addr);
    return "";
}


bool MemNIC::clock(void)
{
    /* If stuff to send, and space to send it, send */
    bool empty = sendQueue.empty();
    while (!sendQueue.empty()) {
        SimpleNetwork::Request *head = sendQueue.front();
        if ( link_control->spaceToSend(0, head->size_in_bits) ) {
            bool sent = link_control->send(head, 0);
            if ( sent ) {
#ifdef __SST_DEBUG_OUTPUT__
                if ( static_cast<MemRtrEvent*>(head->inspectPayload())->hasClientData() ) {
                    MemEvent* event = static_cast<MemEvent*>((static_cast<MemRtrEvent*>(head->inspectPayload()))->event);
                    if (DEBUG_ALL || DEBUG_ADDR == event->getBaseAddr()) {
                        dbg->debug(_L8_, "Sent message ((%" PRIx64 ", %d) %s %" PRIx64 ") to (%" PRIu64 ") [%s]\n", 
                                event->getID().first, event->getID().second, CommandString[event->getCmd()], event->getAddr(), head->dest, event->getDst().c_str());
                    }
                }
#endif
                sendQueue.pop_front();
            } else {
                break;
            }
        } else {
            break;
        }
    }

    return empty;
}

/*  
 *  Notify MemNIC on receiving an event
 */
bool MemNIC::recvNotify(int) {
    MemEvent * me = recv();
    if (me) {
#if 0
        if ( !isRequestAddressValid(me) ) {
            dbg->fatal(CALL_INFO, 1, "MemoryController \"%s\" received request from \"%s\" with invalid address.\n"
                "\t\tRequested Address:   0x%" PRIx64 "\n"
                "\t\tMC Range Start:      0x%" PRIx64 "\n"
                "\t\tMC Range End:        0x%" PRIx64 "\n"
                "\t\tMC Interleave Step:  0x%" PRIx64 "\n"
                "\t\tMC Interleave Size:  0x%" PRIx64 "\n",
                comp->getName().c_str(),
                me->getSrc().c_str(), me->getAddr(),
                rangeStart_, (rangeStart_ + memSize_),
                interleaveStep_, interleaveSize_);
        }
#endif

        // A memory controller should only know about a contiguous memory space
        // the MemNIC should convert to a contiguous address before the MemEvent
        // hits the memory controller and probably needs to fixup the response
        // Addr convertAddressToLocalAddress(Addr addr)
        
       
#ifdef __SST_DEBUG_OUTPUT__
        if (DEBUG_ALL || DEBUG_ADDR == me->getBaseAddr())
            dbg->debug(_L9_, "%s, memNIC recv: src: %s. (Cmd: %s, Rqst size: %u, Payload size: %u)\n", 
                    comp->getName().c_str(), me->getSrc().c_str(), CommandString[me->getCmd()], me->getSize(), me->getPayloadSize());
#endif
        
        (*recvHandler)(me);
    }
    return true;
}

MemEvent* MemNIC::recv(void)
{
    /* Check for received stuff */
    for ( int vc = 0; vc < num_vcs ; vc++ ) {
        // round-robin
        last_recv_vc = (last_recv_vc+1) % num_vcs;

        SimpleNetwork::Request* req = link_control->recv(last_recv_vc);
        if ( NULL != req ) {
            MemRtrEvent *mre = static_cast<MemRtrEvent*>(req->takePayload());
            delete req;
            if ( mre->hasClientData() ) {
                MemEvent *deliverEvent = mre->event;
                deliverEvent->setDeliveryLink(mre->getLinkId(), NULL);
                delete mre;
                return deliverEvent;
            } else {
                InitMemRtrEvent *imre = static_cast<InitMemRtrEvent*>(mre);

                // We shouldn't have any new players on the network - just updated address ranges.
                // Ideally, we should support removing of address ranges as well.  Future work...

                ComponentInfo peerCI;
                peerCI.name = imre->name;
                peerCI.network_addr = imre->address;
                peerCI.type = imre->compType;
                // If user has not cleared info...
                if ( !peers.empty() )
                    peers.push_back(std::make_pair(peerCI, imre->compInfo));

                // Save any new address ranges.
                if ((ci.type == MemNIC::TypeCache || ci.type == MemNIC::TypeNetworkCache) && (peerCI.type == MemNIC::TypeDirectoryCtrl || peerCI.type == MemNIC::TypeNetworkDirectory)) { // cache -> dir
                    destinations[imre->compInfo] = imre->name;
                } else if (ci.type == MemNIC::TypeCacheToCache && peerCI.type == MemNIC::TypeNetworkCache) { // higher cache -> lower cache
                    destinations[imre->compInfo] = imre->name;
                }
            }
        }
    }
    return NULL;
}


void MemNIC::send(MemEvent *ev)
{
    SimpleNetwork::Request* req = new SimpleNetwork::Request();
    MemRtrEvent *mre = new MemRtrEvent(ev);
    req->src = ci.network_addr;
    req->dest = addrForDest(ev->getDst());
    req->size_in_bits = getSizeInBits(ev);
    req->vn = 0;
#ifdef __SST_DEBUG_OUTPUT__
    if (DEBUG_ALL || DEBUG_ADDR == ev->getBaseAddr())
        dbg->debug(_L9_, "%s, memNIC send: dst: %s; bits: %zu. (Cmd: %s, Rqst size: %u, Payload size: %u)\n", 
                comp->getName().c_str(), ev->getDst().c_str(), req->size_in_bits, CommandString[ev->getCmd()], ev->getSize(), ev->getPayloadSize());
#endif
    req->givePayload(mre);
    
    sendQueue.push_back(req);
}


void MemNIC::sendNewTypeInfo(const ComponentTypeInfo &cti)
{
    for ( std::map<std::string, int>::const_iterator i = addrMap.begin() ; i != addrMap.end() ; ++i ) {
        InitMemRtrEvent *imre = new InitMemRtrEvent(comp->getName(), ci.network_addr, ci.type, cti);
        SimpleNetwork::Request* req = new SimpleNetwork::Request();

        req->dest = i->second;
        req->size_in_bits = 128;  // 2* 64bit address (for a range)
        req->vn = 0;
        req->givePayload(imre);
        sendQueue.push_back(req);
    }
}
