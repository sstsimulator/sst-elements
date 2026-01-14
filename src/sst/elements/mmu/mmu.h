// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
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

    typedef std::function<void(RequestID, /*link*/uint32_t, /*core*/uint32_t, /*hw_thread*/uint32_t,
                 /*pid*/uint32_t,/*vpn*/uint32_t,/*perms*/uint32_t,/*inst_ptr*/uint64_t,/*mem_addr*/ uint64_t )> Callback;

    SST_ELI_REGISTER_SUBCOMPONENT_API(SST::MMU_Lib::MMU)
    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS()
    SST_ELI_DOCUMENT_PARAMS(
        { "num_cores", "Number of cores managed by this MMU", "1" },
        { "num_threads", "Number of hardware threads per core", "1" },
        { "page_size", "Memory page size in bytes", "4096" },
        { "use_nic_tlb" , "Whether to use the NIC TLB", "false" },
        { "useNicTlb", "DEPRECATED. Use 'use_nic_tlb' instead", "false" }
    )
    SST_ELI_DOCUMENT_PORTS(
        { "core%(cores)d.dtlb", "Link to each core's data TLB", {} },
        { "core%(cores)d.itlb", "Link to each core's instruction TLB", {} },
        { "nicTlb", "", {} },
        { "ostlb", "Link to the OS's TLB", {} },

    )
    SST_ELI_DOCUMENT_STATISTICS()

    MMU(SST::ComponentId_t id, SST::Params& params);
    virtual ~MMU() {}
    virtual void snapshot( std::string ) = 0;
    virtual void snapshotLoad( std::string ) = 0;

    virtual void init(unsigned int phase) override;
    void registerPermissionsCallback( Callback& callback ) { permissions_callback_ = callback; }

    virtual void dup( uint32_t from_pid, uint32_t to_pid ) = 0;
    virtual void removeWrite( uint32_t pid ) = 0;
    virtual void flushTlb( uint32_t core, uint32_t hw_thread ) = 0;
    virtual void unmap( uint32_t pid, uint32_t vpn, size_t num_pages ) = 0;
    virtual void map( uint32_t pid, uint32_t vpn, std::vector<uint32_t>& ppns, uint32_t page_size, uint64_t flags ) = 0;
    virtual void map( uint32_t pid, uint32_t vpn, uint32_t ppn, uint32_t page_size, uint64_t flags ) = 0;
    virtual void faultHandled( RequestID, uint32_t link, uint32_t pid, uint32_t vpn, bool success = false ) = 0;
    virtual void initPageTable( uint32_t pid ) = 0;
    virtual void setCoreToPageTable( uint32_t core, uint32_t hw_thread, uint32_t pid ) = 0;
    virtual uint32_t virtToPhys( uint32_t pid, uint32_t vpn ) = 0;
    virtual uint32_t getPerms( uint32_t pid, uint32_t vpn ) = 0;

  protected:

    void sendEvent( uint32_t link, Event* ev ) {
        if ( std::numeric_limits<uint32_t>::max() == link ) {
            assert( nic_tlb_link_ );
            nic_tlb_link_->send(ev);
        } else if ( 0 == link % 2 ) {
            core_links_[link/2]->dtlb->send(ev);
        } else if ( 1 == link % 2 ) {
            core_links_[link/2]->itlb->send(ev);
        } else {
            assert(0);
        }
    }

    uint32_t getTlbCore( uint32_t link ) {
        return link/2;
    }

    uint32_t getLink( uint32_t core, const std::string type ) {
        uint32_t link = core * 2;
        if ( 0 == type.compare("dtlb") ) {
        } else if ( 0 == type.compare("itlb") ) {
            ++link;
        } else {
            assert(0);
        }
        return link;
    }


    std::string getTlbName( uint32_t link ) {
        uint32_t core = getTlbCore( link );
        if ( 0 == link % 2 ) {
            return "core" + std::to_string(core) + ".dtlb";
        } else if ( 1 == link % 2 ) {
            return "core" + std::to_string(core) + ".itlb";
        }
        assert(0);
    }

    virtual void handleTlbEvent( Event*, int link ) = 0;
    virtual void handleNicTlbEvent( Event* ){ assert(0); };
    Output dbg_;

    uint32_t num_cores_;
    uint32_t num_hw_threads_;

    uint32_t page_shift_;

    struct CoreTlbLinks {
        CoreTlbLinks( Link* data, Link* inst ) : dtlb(data), itlb(inst) {}
        Link*  dtlb;
        Link*  itlb;
    };
    std::vector<CoreTlbLinks*> core_links_;
    Link*   nic_tlb_link_;
    Callback permissions_callback_;
};

} //namespace MMU_Lib
} //namespace SST

#endif /* MMU_H */
