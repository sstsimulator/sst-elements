// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef COMPONENTS_HR_ROUTER_XBAR_ARB_LRU_H
#define COMPONENTS_HR_ROUTER_XBAR_ARB_LRU_H

#include <sst/core/component.h>
#include <sst/core/event.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>

#include <vector>

#include "sst/elements/merlin/router.h"
#include "sst/elements/merlin/portControl.h"

namespace SST {
namespace Merlin {

class xbar_arb_lru : public XbarArbitration {

private:
    int num_ports;
    int num_vcs;
        
#if VERIFY_DECLOCKING    
    int rr_port_shadow;
#endif

    // typedef std::forward_list<std::pair<int,int> > priority_list_t;
    typedef std::vector<std::pair<int,int> > priority_list_t;
    priority_list_t priority;
    std::vector<std::pair<int,int> > used;
    
    internal_router_event** vc_heads;

    // PortControl** ports;
    
public:
    xbar_arb_lru() :
        XbarArbitration()
    {
    }

    ~xbar_arb_lru() {
    }

    void setPorts(int num_ports_s, int num_vcs_s) {
        num_ports = num_ports_s;
        num_vcs = num_vcs_s;

        // std::cout << num_ports << " " << num_vcs << std::endl;
        
        for ( int i = 0; i < num_ports; i++ ) {
            for ( int j = 0; j < num_vcs; j++ ) {
                priority.push_back(std::pair<int,int>(i,j));
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

        for ( int i = 0; i < num_ports; i++ ) progress_vc[i] = -1;

        // Run through the priority list
        for ( priority_list_t::iterator it = priority.begin(); it != priority.end(); ) {
        
            const std::pair<int,int>& check = *it;

            /* std::cout << check.first << ", " << check.second << std::endl; */
            
            int port = check.first;
            int vc = check.second;

            vc_heads = ports[port]->getVCHeads();
	    
            // if the output of this port is busy or if there is no
            // event to be processed, nothing to do.
            internal_router_event* src_event = vc_heads[vc];
            if ( in_port_busy[port] <= 0 && src_event != NULL) {
                // Have an event, see if it can be progressed
                int next_port = src_event->getNextPort();
                int next_vc = src_event->getVC();
            
                // We can progress if the next port's input is not
                // busy and there are enough credits.
                if ( out_port_busy[next_port] <= 0 &&
                     ports[next_port]->spaceToSend(next_vc, src_event->getFlitCount()) ) {
                                
                    // Tell the router what to move
                    progress_vc[port] = vc;
                    
                    // Need to set the busy values
                    in_port_busy[port] = src_event->getFlitCount();
                    out_port_busy[next_port] = src_event->getFlitCount();
                    
                    // Need to remove this port/vc pair from the priority list
                    // and set aside.
                    used.push_back(check);
                    it = priority.erase(it);
                }
                else {
                    progress_vc[port] = -2;
                    ++it;
                }
            }
            else {
                ++it;
            }
        }

        // Put the used port/vc pairs back into priority list in reverse order
        for ( int i = used.size() - 1; i >= 0; --i ) {
            priority.push_back(used[i]);
        }
        used.clear();
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

#endif // COMPONENTS_HR_ROUTER_XBAR_ARB_LRU_H
