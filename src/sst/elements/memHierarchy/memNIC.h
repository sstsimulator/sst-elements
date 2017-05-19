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

#ifndef _MEMHIERARCHY_MEMNIC_H_
#define _MEMHIERARCHY_MEMNIC_H_

#include <string>
#include <deque>


#include <sst/core/component.h>
#include <sst/core/event.h>
#include <sst/core/output.h>
#include <sst/core/module.h>
#include <sst/core/interfaces/simpleNetwork.h>

#include "sst/elements/memHierarchy/memEvent.h"
#include "sst/elements/memHierarchy/util.h"

//#include <sst/elements/merlin/linkControl.h>


namespace SST {
namespace MemHierarchy {

class MemNIC : public Module {

public:
    enum ComponentType {
        TypeCache,              // cache - connected to network below & talks to dir 
        TypeNetworkCache,       // cache - connected to network above & below, talks to cache above, dir below; associated with a particular set of addresses
        TypeCacheToCache,       // cache - connected to network below & talks to cache 
        TypeDirectoryCtrl,      // directory - connected to cache via network, direct link to memory; associated with a particular set of addresses
        TypeNetworkDirectory,   // directory - connected to cache and memory via network; associated with a particular set of addresses
        TypeMemory,             // memory - connected to directory or cache via network
        TypeDMAEngine,
        TypeSmartMemory,        // Sender and Receiver
        TypeScratch,
        TypeOther
    };

    struct ComponentTypeInfo {
        uint64_t rangeStart;
        uint64_t rangeEnd;
        uint64_t interleaveSize;
        uint64_t interleaveStep;
        uint32_t blocksize;    
        CoherenceProtocol coherenceProtocol;
        std::string cacheType;
        bool contains(uint64_t addr) const {
            if ( addr >= rangeStart && addr < rangeEnd ) {
                if ( interleaveSize == 0 ) return true;
                uint64_t offset = (addr - rangeStart) % interleaveStep;
                return (offset < interleaveSize);
            }
            return false;
        }
        bool operator<(const ComponentTypeInfo &o) const {
            return (rangeStart < o.rangeStart);
        }
    };

    struct ComponentInfo {
        std::string link_port;
	int num_vcs;
        std::string link_bandwidth;
        std::string name;
        int network_addr;
        ComponentType type;
        std::string link_inbuf_size;
        std::string link_outbuf_size;

        ComponentInfo() :
            link_port(""), num_vcs(0), link_bandwidth(""), name(""),
            network_addr(0), type(TypeOther), link_inbuf_size(""), link_outbuf_size("")
        { }
    };


public:
    class MemRtrEvent : public Event {
    public:
        MemEvent *event;

        MemRtrEvent() :
            Event(), event(NULL)
        {}
        MemRtrEvent(MemEvent *ev) :
            Event(), event(ev)
        { }

        virtual Event* clone(void)  override {
            MemRtrEvent *mre = new MemRtrEvent(*this);
            mre->event = new MemEvent(*event);
            return mre;
        }

        virtual bool hasClientData() const { return true; }

    public:
        void serialize_order(SST::Core::Serialization::serializer &ser)  override {
            Event::serialize_order(ser);
            ser & event;
        }
        
        ImplementSerializable(SST::MemHierarchy::MemNIC::MemRtrEvent);     
    };

    class InitMemRtrEvent : public MemRtrEvent {
    public:
        std::string name;
        int address;
        ComponentType compType;
        ComponentTypeInfo compInfo;
        int src;
        
        InitMemRtrEvent() {}
        InitMemRtrEvent(const std::string &name, int addr, ComponentType type) :
            MemRtrEvent(), name(name), address(addr), compType(type)
        {
            src = addr;
        }

        InitMemRtrEvent(const std::string &name, int addr, ComponentType type, ComponentTypeInfo info) :
            MemRtrEvent(), name(name), address(addr), compType(type), compInfo(info)
        {
            src = addr;
        }

        virtual Event* clone(void)  override {
            return new InitMemRtrEvent(*this);
        }

        virtual bool hasClientData() const override { return false; }

    public:
        void serialize_order(SST::Core::Serialization::serializer &ser)  override {
                MemRtrEvent::serialize_order(ser);
                ser & compType;
                ser & address;
                ser & name;
                ser & compInfo.rangeStart;
                ser & compInfo.rangeEnd;
                ser & compInfo.interleaveSize;
                ser & compInfo.interleaveStep;
                ser & compInfo.blocksize;
                ser & compInfo.coherenceProtocol;
                ser & compInfo.cacheType;
                ser & src;
            }
        
        ImplementSerializable(SST::MemHierarchy::MemNIC::InitMemRtrEvent);     
    };


    typedef std::pair<ComponentInfo, ComponentTypeInfo> PeerInfo_t;

private:

    Output* dbg;
    int num_vcs;
    bool typeInfoSent; // True if TypeInfo has already been sent
    bool checkRecvQueue;
    unsigned int packetHeaderBytes = 8; // default

    /* Debugging stuff */
    Addr DEBUG_ADDR;
    bool DEBUG_ALL;


    Component *comp;
    ComponentInfo ci;
    std::vector<ComponentTypeInfo> typeInfoList;
    Event::HandlerBase *recvHandler;
    //Event::HandlerBase *parentRecvNotifyHandler;
    SST::Interfaces::SimpleNetwork *link_control;
    SST::Interfaces::SimpleNetwork::Handler<MemNIC>* recvNotifyHandler;

    std::deque<MemRtrEvent*> initQueue;
    // std::deque<MemRtrEvent *> sendQueue;
    std::deque<SST::Interfaces::SimpleNetwork::Request *> sendQueue;
    int last_recv_vc;
    std::map<std::string, int> addrMap;
    /* Built during init -> available in Setup and later */
    std::vector<PeerInfo_t> peers;
    std::unordered_map<std::string, MemNIC::ComponentTypeInfo> peerAddrs;
    /* Built during init -> available for lookups later */
    std::map<MemNIC::ComponentTypeInfo, std::string> destinations;

    /* Translates a MemEvent string destination to an network address
       (integer) */
    int addrForDest(const std::string &target) const;

    /* Get size in bits for a MemEvent */
    int getSizeInBits(MemEvent *ev);

    /* Used during run to send updated address ranges */
    void sendNewTypeInfo(const ComponentTypeInfo &cti);

public:
    MemNIC(Component *comp, Output* output, Addr dAddr, ComponentInfo &ci, Event::HandlerBase *handler = NULL);
    /** Constructor to be used when loading as a module.
     */
    MemNIC(Component *comp, Params& params);
    /** To be used when loading MemNIC as a module.  Not necessary to call
     * when using the full-featured constructor
     */
    void moduleInit(ComponentInfo &ci, Event::HandlerBase *handler = NULL);

    void addTypeInfo(const ComponentTypeInfo &cti) {
        typeInfoList.push_back(cti);
        if ( typeInfoSent ) sendNewTypeInfo(cti);
    }

    /* Allow parent to register a callback function so it can de-clock itself safely */
    void registerRecvCallback(Event::HandlerBase * handler);

    /* Sets the minimum packet size/header size */
    void setMinPacketSize(unsigned int);

    /* Call these from their respective calls in the component */
    void setup(void);
    void init(unsigned int phase);
    void finish(void);
    bool clock(void);
    bool isValidDestination(std::string target);

    void send(MemEvent *ev);
    MemEvent* recv(void);
    void sendInitData(MemEvent *ev);
    MemEvent* recvInitData(void);
    bool initDataReady();
    const std::vector<PeerInfo_t>& getPeerInfo(void) const { return peers; }
    const ComponentInfo& getComponentInfo(void) const { return ci; }
    // translate a memory address to a network target (string)
    std::string findTargetDestination(Addr addr);
    // NOTE: does not clear the listing of destinations which are used for address lookups
    void clearPeerInfo(void) { peers.clear(); }
    // Assuming addresses at the target 'dst' are linear, convert addr to it's target equivalent
    Addr convertToDestinationAddress(const std::string &dst, Addr addr);

    // Callback function for linkControl
    bool recvNotify(int vn);

    void setRecvHandler( Event::HandlerBase *handler ) {
        recvHandler = handler;
    }
    void setOutput( Output* output ) { 
        dbg = output;
    }


    bool isRequestAddressValid(MemEvent *ev){
        Addr addr = ev->getAddr();
        Addr step;

        if(0 == numPages_)     return (addr >= rangeStart_ && addr < (rangeStart_ + memSize_));
        if(addr < rangeStart_) return false;

        addr = addr - rangeStart_;
        step = addr / interleaveStep_;
        if(step >= numPages_)  return false;

        Addr offset = addr % interleaveStep_;
        if (offset >= interleaveSize_) return false;

        return true;
    }
#if 0
    // A memory controller should only know about contiguous memory
    // the MemNIC should convert to a contiguous address before the MemEvent 
    // hits the memory controller 
    Addr convertAddressToLocalAddress(Addr addr){
        if (0 == numPages_) return addr - rangeStart_;

        addr = addr - rangeStart_;
        Addr step = addr / interleaveStep_;
        Addr offset = addr % interleaveStep_;
        return (step * interleaveSize_) + offset;
        return 0;
    }
#endif

    std::string name_;
    Addr        memSize_;
    Addr        rangeStart_;
    uint64_t    numPages_;
    Addr        interleaveSize_;
    Addr        interleaveStep_;
    uint32_t    cacheLineSize_;
};

} //namespace memHierarchy
} //namespace SST

#endif
