// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
// 
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef COMPONENTS_HR_ROUTER_XBAR_ARB_LRU_INFX_H
#define COMPONENTS_HR_ROUTER_XBAR_ARB_LRU_INFX_H

#include <sst/core/component.h>
#include <sst/core/elementinfo.h>
#include <sst/core/event.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>

#include <vector>

#include "sst/elements/merlin/router.h"
#include "sst/elements/merlin/portControl.h"

namespace SST {
namespace Merlin {

class xbar_arb_lru_infx : public XbarArbitration {

public:

    SST_ELI_REGISTER_SUBCOMPONENT(
        xbar_arb_lru_infx,
        "merlin",
        "xbar_arb_lru_infx",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Least recently used arbitration unit with \"infiinite crossbar\" for hr_router",
        "SST::Merlin::XbarArbitration")
    
    
private:
    int num_ports;
    int num_vcs;
        
#if VERIFY_DECLOCKING    
    int rr_port_shadow;
#endif

    typedef std::pair<uint16_t,uint16_t> priority_entry_t;
    priority_entry_t* priority[2];
    priority_entry_t* cur_list;
    priority_entry_t* next_list;
    
    int total_entries;
    
    internal_router_event** vc_heads;

    // PortControl** ports;
    
public:
    xbar_arb_lru_infx(Component* parent, Params& params) :
        XbarArbitration(parent)
    {
    }

    ~xbar_arb_lru_infx() {
    }

    void setPorts(int num_ports_s, int num_vcs_s) {
        num_ports = num_ports_s;
        num_vcs = num_vcs_s;

        total_entries = num_ports * num_vcs;

        priority[0] = new priority_entry_t[total_entries];
        priority[1] = new priority_entry_t[total_entries];
        cur_list = priority[0];
        next_list = priority[1];
        
        int index = 0;
        for ( int i = 0; i < num_ports; i++ ) {
            for ( int j = 0; j < num_vcs; j++ ) {
                cur_list[index++] = priority_entry_t(i,j);
            }
        }

        
        vc_heads = new internal_router_event*[num_vcs];
    }
    
    // Naming convention is from point of view of the xbar.  So,
    // in_port_busy is >0 if someone is writing to that xbar port and
    // out_port_busy is >0 if that xbar port being read.
    void arbitrate(
#if VERIFY_DECLOCKING
                   PortControl** ports, int* in_port_busy, int* out_port_busy, int* progress_vc, bool clocking
#else
                   PortControl** ports, int* in_port_busy, int* out_port_busy, int* progress_vc
#endif
                   )
    {
        // TraceFunction trace(CALL_INFO_LONG);
        
        for ( int i = 0; i < num_ports; i++ ) progress_vc[i] = -1;
        

        priority_entry_t* sat_list = &next_list[total_entries-1];
        priority_entry_t* unsat_list = next_list;
        
        for ( int i = 0; i < total_entries; i++ ) {
                    
            const priority_entry_t& check = cur_list[i];

            /* std::cout << check.first << ", " << check.second << std::endl; */
            
            int port = check.first;
            int vc = check.second;

            // std::cout << check.first << ", " << check.second << std::endl;
            
            vc_heads = ports[port]->getVCHeads();
            
            internal_router_event* src_event = vc_heads[vc];

            // trace.getOutput().output("Got to here 1\n");
            
            // If there's an event, see if we can progress it
            if ( src_event != NULL) {
                int next_port = src_event->getNextPort();
                int next_vc = src_event->getVC();
            
                // Move the packet as long as there is space in the output buffer
                if ( ports[next_port]->spaceToSend(next_vc, src_event->getFlitCount()) ) {

                    // We just go ahead and do the move.  The
                    // progress_vc vector will be set to all -1's so
                    // hr_router won't try to progress anything.
                    // trace.getOutput().output("Got to here 2\n");
                    internal_router_event* ev = ports[port]->recv(vc);
                    // trace.getOutput().output("%p\n",ev);
                    ports[ev->getNextPort()]->send(ev,ev->getVC());
                    // trace.getOutput().output("Got to here 3\n");
                    
                    
                    // Copy data to new list.  This goes at the bottom since it was satisfied
                    *sat_list = check;
                    --sat_list;
                }
                else {
                    // Not satisfied, put at the top of the next list
                    *unsat_list = check;
                    ++unsat_list;
                }
            }
            else {
                // std::cout << "putting at top of list" << std::endl;
                // next_list[unsat_index++] = check;
                *unsat_list = check;
                ++unsat_list;
            }
            // trace.getOutput().output("Got to here 4\n");
        }

        // std::cout << "+++++++++" << std::endl;
        // for ( int i = 0; i < total_entries; i++ ) {
        //     std::cout << priority[next_list][i].first << ", " << priority[cur_list][i].second << std::endl;
        // }
        // std::cout << "+++++++++" << std::endl;

        // cur_list ^= next_list;
        // next_list ^= cur_list;
        // cur_list ^= next_list;
        priority_entry_t* tmp = cur_list;
        cur_list = next_list;
        next_list = tmp;
        return;
    }
    
    void reportSkippedCycles(Cycle_t cycles) {
    }

    void dumpState(std::ostream& stream) {
        /* stream << "Current round robin port: " << rr_port << std::endl; */
        /* stream << "  Current round robin VC by port:" << std::endl; */
        /* for ( int i = 0; i < num_ports; i++ ) { */
        /*     stream << i << ": " << rr_vcs[i] << std::endl; */
        /* } */
    }
    
};
 
}
}

#endif // COMPONENTS_HR_ROUTER_XBAR_ARB_LRU_INFX_H
