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
#include "memEvent.h"
#include "util.h"

#include <sst/elements/merlin/linkControl.h>


namespace SST {
namespace MemHierarchy {

class MemNIC {

public:
    enum ComponentType {
        TypeCache,
        TypeDirectoryCtrl,
        TypeDMAEngine,
        TypeOther
    };

    union ComponentTypeInfo {
        struct {
            uint32_t blocksize;
            uint32_t num_blocks;
        } cache;
        struct {
            uint64_t rangeStart;
            uint64_t rangeEnd;
            uint64_t interleaveSize;
            uint64_t interleaveStep;
        } dirctrl;
    };

    struct ComponentInfo {
        std::string link_port;
		int num_vcs;
        std::string link_bandwidth;
        std::string name;
        int network_addr;
        ComponentType type;
        ComponentTypeInfo typeInfo;
        
        ComponentInfo() :
            link_port(""), link_bandwidth(""), name(""),
            network_addr(0)
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
        friend class boost::serialization::access;
        template<class Archive>
            void
            serialize(Archive & ar, const unsigned int version )
            {
                ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Merlin::RtrEvent);
                ar & BOOST_SERIALIZATION_NVP(event);
            }
    };

    class InitMemRtrEvent : public Merlin::RtrEvent {
    public:
        std::string name;
        int address;
        ComponentType compType;
        ComponentTypeInfo compInfo;

        InitMemRtrEvent() {}
        InitMemRtrEvent(const std::string &name, int addr, ComponentType type, ComponentTypeInfo info) :
            Merlin::RtrEvent(), name(name), address(addr), compType(type), compInfo(info)
        {
            src = addr;
        }

        virtual RtrEvent* clone(void) {
            return new InitMemRtrEvent(*this);
        }
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
            case TypeCache:
                ar & BOOST_SERIALIZATION_NVP(compInfo.cache.blocksize);
                ar & BOOST_SERIALIZATION_NVP(compInfo.cache.num_blocks);
                break;
            case TypeDirectoryCtrl:
                ar & BOOST_SERIALIZATION_NVP(compInfo.dirctrl.rangeStart);
                ar & BOOST_SERIALIZATION_NVP(compInfo.dirctrl.rangeEnd);
                ar & BOOST_SERIALIZATION_NVP(compInfo.dirctrl.interleaveSize);
                ar & BOOST_SERIALIZATION_NVP(compInfo.dirctrl.interleaveStep);
                break;
            default:
                _abort(MemNIC, "Don't know how to serialize this type [%d].\n", compType);
            }
        }
    };

private:

    Output dbg;
    int num_vcs;
    size_t flitSize;

    Component *comp;
    ComponentInfo ci;
    Event::HandlerBase *recvHandler;
    Merlin::LinkControl *link_control;

    std::deque<MemRtrEvent*> initQueue;
    std::deque<MemRtrEvent *> sendQueue;
    int last_recv_vc;
    std::map<std::string, int> addrMap;
    /* Built during init -> available in Setup and later */
    std::vector<ComponentInfo> peers;
    /* Built during init -> available for lookups later */
    std::vector<MemNIC::ComponentInfo> directories;


    /* Translates a MemEvent string destination to an network address
       (integer) */
    int addrForDest(const std::string &target) const;

    /* Get size in flits for a MemEvent */
    int getFlitSize(MemEvent *ev);


public:
    MemNIC(Component *comp, ComponentInfo &ci, Event::HandlerBase *handler = NULL);


    /* Call these from their respective calls in the component */
    void setup(void);
    void init(unsigned int phase);
    void finish(void);
    bool clock(void);


    void send(MemEvent *ev);
    MemEvent* recv(void);
    void sendInitData(MemEvent *ev);
    MemEvent* recvInitData(void);
    const std::vector<ComponentInfo>& getPeerInfo(void) const { return peers; }
    // translate a memory address to a network target (string)
    std::string findTargetDirectory(Addr addr);
    // NOTE: does not clear the listing of directories which are used for address lookups
    void clearPeerInfo(void) { peers.clear(); }

};

} //namespace memHierarchy
} //namespace SST

#endif
