// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef MMU_H
#define MMU_H

#include <sst/core/sst_types.h>
#include <sst/core/subcomponent.h>
#include <sst/core/link.h>
#include "mmuTypes.h"

namespace SST {

namespace MMU_Lib {

class MMU : public SubComponent {

  public:

    typedef std::function<void(RequestID,/*link*/unsigned,/*core*/ unsigned ,/*hwThread*/ unsigned ,
                 /*pid*/unsigned,/*vpn*/uint32_t,/*perms*/uint32_t,/*instPtr*/uint64_t,/*memAddr*/ uint64_t )> Callback;

    SST_ELI_REGISTER_SUBCOMPONENT_API(SST::MMU_Lib::MMU)
    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS()
    SST_ELI_DOCUMENT_PARAMS()
    SST_ELI_DOCUMENT_PORTS(
        { "core%(cores)d.dtlb", "", {} },
        { "core%(cores)d.itlb", "", {} },
        { "nicTlb", "", {} },
        { "ostlb", "", {} },
    )
    SST_ELI_DOCUMENT_STATISTICS()

    MMU(SST::ComponentId_t id, SST::Params& params);
    virtual ~MMU() {}

    virtual void init(unsigned int phase);
    void registerPermissionsCallback( Callback& callback ) { m_permissionsCallback = callback; }

    virtual void dup( unsigned fromPid, unsigned toPid ) = 0;
    virtual void removeWrite( unsigned pid ) = 0;
    virtual void flushTlb( unsigned core, unsigned hwThread ) = 0;
    virtual void map( unsigned pid, uint32_t vpn, std::vector<uint32_t>& ppns, int pageSize, uint64_t flags ) = 0;
    virtual void map( unsigned pid, uint32_t vpn, uint32_t ppn, int pageSize, uint64_t flags ) = 0;
    virtual int getPerms( unsigned pid, uint32_t vpn ) = 0;
    virtual void faultHandled( RequestID, unsigned link, unsigned pid, unsigned vpn, bool success = false ) = 0;
    virtual void initPageTable( unsigned pid ) = 0;
    virtual void setCoreToPageTable( unsigned core, unsigned hwThread, unsigned pid ) = 0; 
    virtual uint32_t virtToPhys( unsigned pid, uint64_t vpn ) = 0;
    virtual uint32_t getPerms( unsigned pid, uint64_t vpn ) = 0;

  protected:

    void sendEvent( int link, Event* ev ) {
        if ( -1 == link ) {
            assert( m_nicTlbLink );
            m_nicTlbLink->send(0,ev);
        } else if ( 0 == link % 2 ) {
            m_coreLinks[link/2]->dtlb->send(0,ev);
        } else if ( 1 == link % 2 ) {
            m_coreLinks[link/2]->itlb->send(0,ev);
        } else {
            assert(0);
        }
    }

    int getTlbCore( int link ) { 
        return link/2; 
    }

    int getLink( int core, const std::string type ) {
        int link = core * 2;
        if ( 0 == type.compare("dtlb") ) {
        } else if ( 0 == type.compare("itlb") ) {
            ++link;
        } else {
            assert(0);
        }
        return link;
    }


    std::string getTlbName( int link ) {
        int core = getTlbCore( link );
        if ( 0 == link % 2 ) { 
            return "core" + std::to_string(core) + ".dtlb"; 
        } else if ( 1 == link % 2 ) { 
            return "core" + std::to_string(core) + ".itlb"; 
        } 
        assert(0);
    }

    virtual void handleTlbEvent( Event*, int link ) = 0;
    virtual void handleNicTlbEvent( Event* ){ assert(0); };
    Output m_dbg;

    unsigned m_numCores;
    unsigned m_numHwThreads;

    int m_pageShift;

    struct CoreTlbLinks {
        CoreTlbLinks( Link* data, Link* inst ) : dtlb(data), itlb(inst) {}
        Link*  dtlb;
        Link*  itlb;
    };
    std::vector<CoreTlbLinks*> m_coreLinks;
    Link*   m_nicTlbLink;
    Callback m_permissionsCallback;
};

} //namespace MMU_Lib
} //namespace SST

#endif /* MMU_H */
