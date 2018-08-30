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


#ifndef COMPONENTS_HR_ROUTER_XBAR_ARB_RR_H
#define COMPONENTS_HR_ROUTER_XBAR_ARB_RR_H

#include <sst/core/component.h>
#include <sst/core/elementinfo.h>
#include <sst/core/event.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>

#include <vector>

#include "sst/elements/merlin/router.h"
#include "sst/elements/merlin/portControl.h"

using namespace SST;

namespace SST {
namespace Merlin {

class xbar_arb_rr : public XbarArbitration {

public:

    SST_ELI_REGISTER_SUBCOMPONENT(
        xbar_arb_rr,
        "merlin",
        "xbar_arb_rr",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Round robin arbitration unit for hr_router",
        "SST::Merlin::XbarArbitration")
    
    
private:
    int num_ports;
    int num_vcs;
    
    int *rr_vcs;
    int rr_port;
    
#if VERIFY_DECLOCKING    
    int rr_port_shadow;
#endif
    
    internal_router_event** vc_heads;

    // PortControl** ports;
    
public:
    xbar_arb_rr(Component* parent, Params& params) :
        XbarArbitration(parent),
        rr_vcs(NULL)
    {
    }

    ~xbar_arb_rr() {
        if ( rr_vcs != NULL ) delete [] rr_vcs;
    }

    void setPorts(int num_ports_s, int num_vcs_s) {
        num_ports = num_ports_s;
        num_vcs = num_vcs_s;

        rr_vcs = new int[num_ports];
        for ( int i = 0; i < num_ports; i++ ) {
            rr_vcs[i] = 0;
        }
	
        rr_port = 0;
#if VERIFY_DECLOCKING
        rr_port_shadow = 0;
#endif
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
        // Run through each of the ports, giving first pick in a round robin fashion
        // for ( int port = rr_port, pcount = 0; pcount < num_ports; port = (port+1) % num_ports, pcount++ ) {
        for ( int port = rr_port, pcount = 0; pcount < num_ports; port = ((port != num_ports-1) ? port+1 : 0), pcount++ ) {

            vc_heads = ports[port]->getVCHeads();
	    
            // Overwrite old data
            progress_vc[port] = -1;
            // if the output of this port is busy, nothing to do.
            if ( in_port_busy[port] > 0 ) {
                continue;
            }
	    
            // See what we should progress for this port
            // for ( int vc = rr_vcs[port], vcount = 0; vcount < num_vcs; vc = (vc+1) % num_vcs, vcount++ ) {
            for ( int vc = rr_vcs[port], vcount = 0; vcount < num_vcs; vc = ((vc != num_vcs-1) ? (vc+1) : 0), vcount++ ) {
		
                // If there is no event, move to next VC
                internal_router_event* src_event = vc_heads[vc];
                if ( src_event == NULL ) continue;
		
                // Have an event, see if it can be progressed
                int next_port = src_event->getNextPort();
		
                // We can progress if the next port's input is not
                // busy and there are enough credits.
                if ( out_port_busy[next_port] > 0 ) continue;
                
                // Need to see if the VC has enough credits
                int next_vc = src_event->getVC();

                // See if there is enough space
                if ( !ports[next_port]->spaceToSend(next_vc, src_event->getFlitCount()) ) continue;
		
                // Tell the router what to move
                progress_vc[port] = vc;
		
                // Need to set the busy values
                in_port_busy[port] = src_event->getFlitCount();
                out_port_busy[next_port] = src_event->getFlitCount();
                break;  // Go to next port;
            }
            // Increemnt rr_vcs for next time 
            rr_vcs[port] = (rr_vcs[port] + 1) % num_vcs;
        }
        rr_port = (rr_port + 1) % num_ports;

#if VERIFY_DECLOCKING
        if ( clocking ) {
            rr_port_shadow = rr_port;
        }
#endif
    
        return;
    }
    
    void reportSkippedCycles(Cycle_t cycles) {
#if VERIFY_DECLOCKING
        rr_port_shadow = (rr_port_shadow + cycles) % num_ports;
        if ( rr_port_shadow != rr_port ) std::cout << "  PROBLEM:  rr_port = "
                         << rr_port << ", rr_port_shadow = " << rr_port_shadow <<
                         ", cycles = " << cycles << std::endl;
#else
        rr_port = (rr_port + cycles) % num_ports;
#endif
    }

    void dumpState(std::ostream& stream) {
        stream << "Current round robin port: " << rr_port << std::endl;
        stream << "  Current round robin VC by port:" << std::endl;
        for ( int i = 0; i < num_ports; i++ ) {
            stream << i << ": " << rr_vcs[i] << std::endl;
        }
    }

};
 
}
}

#endif // COMPONENTS_HR_ROUTER_XBAR_ARB_RR_H
