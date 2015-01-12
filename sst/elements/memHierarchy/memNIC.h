// Copyright 2013-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2014, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _MEMHIERARCHY_MEMNIC_H_
#define _MEMHIERARCHY_MEMNIC_H_

#include <string>
#include <deque>


#include <sst/core/component.h>
#include <sst/core/debug.h>
#include <sst/core/event.h>
#include <sst/core/output.h>
#include <sst/core/module.h>
#include "memEvent.h"
#include "util.h"

#include <sst/elements/merlin/linkControl.h>


namespace SST {
namespace MemHierarchy {

class MemNIC : public Module {

public:
    enum ComponentType {
        TypeCache,              // cache - connected to network below & talks to dir 
        TypeNetworkCache,       // cache - connected to network above & below, talks to cache above, dir below; associated with a particular set of addresses
        TypeCacheToCache,       // cache - connected to network below & talks to cache 
        TypeDirectoryCtrl,      // directory - connected to network above & talks to cache; associated with a particular set of addresses
        TypeDMAEngine,
        TypeOther
    };

    union ComponentTypeInfo {
        struct {
            uint32_t blocksize;
            uint32_t num_blocks;
        } cache;
        struct AddrRange {
            uint64_t rangeStart;
            uint64_t rangeEnd;
            uint64_t interleaveSize;
            uint64_t interleaveStep;
            bool contains(uint64_t addr) const {
                if ( addr >= rangeStart && addr < rangeEnd ) {
                    if ( interleaveSize == 0 ) return true;
                    uint64_t offset = (addr - rangeStart) % interleaveStep;
                    return (offset < interleaveSize);
                }
                return false;
            }
            bool operator<(const AddrRange &o) const {
                return (rangeStart < o.rangeStart);
            }
        } addrRange;
    };

    struct ComponentInfo {
        std::string link_port;
	int num_vcs;
        std::string link_bandwidth;
        std::string name;
        int network_addr;
        ComponentType type;

        ComponentInfo() :
            link_port(""), num_vcs(0), link_bandwidth(""), name(""),
            network_addr(0), type(TypeOther)
        { }
    };


public:
    class MemRtrEvent : public Merlin::RtrEvent {
    public:
        MemEvent *event;

        MemRtrEvent() {}
        MemRtrEvent(MemEvent *ev) :
            Merlin::RtrEvent(), event(ev)
        { }

        virtual RtrEvent* clone(void) {
            MemRtrEvent *mre = new MemRtrEvent(*this);
            mre->event = new MemEvent(*event);
            return mre;
        }

        virtual bool hasClientData() const { return true; }

        friend class boost::serialization::access;
        template<class Archive>
            void
            serialize(Archive & ar, const unsigned int version )
            {
                ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Merlin::RtrEvent);
                ar & BOOST_SERIALIZATION_NVP(event);
            }
    };

    class InitMemRtrEvent : public MemRtrEvent {
    public:
        std::string name;
        int address;
        ComponentType compType;
        ComponentTypeInfo compInfo;

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

        virtual RtrEvent* clone(void) {
            return new InitMemRtrEvent(*this);
        }

        virtual bool hasClientData() const { return false; }

        friend class boost::serialization::access;
        template<class Archive>
        void
        serialize(Archive & ar, const unsigned int version )
        {
            ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Merlin::RtrEvent);
            ar & BOOST_SERIALIZATION_NVP(compType);
            ar & BOOST_SERIALIZATION_NVP(address);
            ar & BOOST_SERIALIZATION_NVP(name);
            switch ( compType ) {
            case TypeCacheToCache:
            case TypeCache:
                ar & BOOST_SERIALIZATION_NVP(compInfo.cache.blocksize);
                ar & BOOST_SERIALIZATION_NVP(compInfo.cache.num_blocks);
                break;
            case TypeNetworkCache:
            case TypeDirectoryCtrl:
                ar & BOOST_SERIALIZATION_NVP(compInfo.addrRange.rangeStart);
                ar & BOOST_SERIALIZATION_NVP(compInfo.addrRange.rangeEnd);
                ar & BOOST_SERIALIZATION_NVP(compInfo.addrRange.interleaveSize);
                ar & BOOST_SERIALIZATION_NVP(compInfo.addrRange.interleaveStep);
                break;
            default:
                _abort(MemNIC, "Don't know how to serialize this type [%d].\n", compType);
            }
        }
    };


    typedef std::pair<ComponentInfo, ComponentTypeInfo> PeerInfo_t;

private:

    Output dbg;
    int num_vcs;
    size_t flitSize;
    bool typeInfoSent; // True if TypeInfo has already been sent

    Component *comp;
    ComponentInfo ci;
    std::vector<ComponentTypeInfo> typeInfoList;
    Event::HandlerBase *recvHandler;
    Merlin::LinkControl *link_control;

    std::deque<MemRtrEvent*> initQueue;
    std::deque<MemRtrEvent *> sendQueue;
    int last_recv_vc;
    std::map<std::string, int> addrMap;
    /* Built during init -> available in Setup and later */
    std::vector<PeerInfo_t> peers;
    /* Built during init -> available for lookups later */
    std::map<MemNIC::ComponentTypeInfo::AddrRange, std::string> destinations;


    /* Translates a MemEvent string destination to an network address
       (integer) */
    int addrForDest(const std::string &target) const;

    /* Get size in flits for a MemEvent */
    int getFlitSize(MemEvent *ev);

    /* Used during run to send updated address ranges */
    void sendNewTypeInfo(const ComponentTypeInfo &cti);

public:
    MemNIC(Component *comp, ComponentInfo &ci, Event::HandlerBase *handler = NULL);
    /** Constructor to be used when loading as a module.
     */
    MemNIC(Component *comp);
    /** To be used when loading MemNIC as a module.  Not necessary to call
     * when using the full-featured constructor
     */
    void moduleInit(ComponentInfo &ci, Event::HandlerBase *handler = NULL);

    void addTypeInfo(const ComponentTypeInfo &cti) {
        typeInfoList.push_back(cti);
        if ( typeInfoSent ) sendNewTypeInfo(cti);
    }

    /* Call these from their respective calls in the component */
    void setup(void);
    void init(unsigned int phase);
    void finish(void);
    bool clock(void);


    void send(MemEvent *ev);
    MemEvent* recv(void);
    void sendInitData(MemEvent *ev);
    MemEvent* recvInitData(void);
    const std::vector<PeerInfo_t>& getPeerInfo(void) const { return peers; }
    // translate a memory address to a network target (string)
    std::string findTargetDestination(Addr addr);
    // NOTE: does not clear the listing of destinations which are used for address lookups
    void clearPeerInfo(void) { peers.clear(); }

};

} //namespace memHierarchy
} //namespace SST

#endif
