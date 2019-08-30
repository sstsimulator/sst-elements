// -*- mode: c++ -*-

// Copyright 2009-2019 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2019, NTESS
// All rights reserved.
// 
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef COMPONENTS_MERLIN_ARBITRATION_OUTPUT_ARB_QOS_MULTI_H
#define COMPONENTS_MERLIN_ARBITRATION_OUTPUT_ARB_QOS_MULTI_H

#include <sst/elements/merlin/router.h>
#include <sst/elements/merlin/arbitration/single_arb.h>

namespace SST {
namespace Merlin {

class output_arb_qos_multi : public PortInterface::OutputArbitration {

public:

    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
        output_arb_qos_multi,
        "merlin",
        "arb.output.qos.multi",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Multi-level output arbitration with quality of service for PortControl",
        SST::Merlin::PortInterface::OutputArbitration)
    
    SST_ELI_DOCUMENT_PARAMS(
        {"arb_vns",      "Tyoe of arbitration to use for virtual networks","merlin.arb.base.single.lru"},
        {"arb_vcs",      "Tyoe of arbitration to use with virtual network for VCs","merlin.arb.base.single.roundrobin"},
        {"qos_settings", "Array of qos setting for each VN", ""},
    )

    
private:

    static const int window = 100;
    int num_vns;
    int num_counters;
    int* vcs_per_vn;
    int* vn_offset;
    std::string arb_vns_name;
    std::string arb_vcs_name;
    SingleArbitration* arb;
    SingleArbitration** vn_arb;

    int16_t* qos_settings;
    int16_t* counters;
    int16_t* which_counter;

    Cycle_t last_cycle;
    
public:
    output_arb_qos_multi(Component* parent, Params& params) :
        OutputArbitration(parent)
    {}

    output_arb_qos_multi(ComponentId_t cid, Params& params) :
        OutputArbitration(cid),
        last_cycle(0)
    {
        // Get the name of the main arbitration unit
        arb_vns_name = params.find<std::string>("arb_vns","merlin.arb.base.single.lru");

        // Get the name of the arbitration unit for use inside a VN
        arb_vcs_name = params.find<std::string>("arb_vcs","merlin.arb.base.single.roundrobin");

        // Get the qos settings
        std::vector<int16_t> settings;
        params.find_array<int16_t>("qos_settings",settings);
        if ( settings.size() == 0 ) {
            merlin_abort.fatal(CALL_INFO_LONG,1,"output_arb_qos_multi: must specify qos_settings\n");
        }
        
        num_vns = settings.size();
        num_counters = 0;
        for ( int i = 0; i < num_vns; ++i ) {
            if ( settings[i] != -1 ) num_counters++;
        }
        if ( num_counters != num_vns ) {
            // Not all VNs were given an allocation.  All VNs without
            // allocation will share the same counter that will just
            // have what's left of the bnadwidth
            num_counters++;
        }
        
        qos_settings = new int16_t[num_counters];
        counters = new int16_t[num_counters];
        which_counter = new int16_t[num_vns];

        int count = 0;
        // For now, the window will be "window" flit cycles.  Will make
        // this settable later.
        int16_t remainder = window;
        for ( int i = 0; i < num_vns; ++i ) {
            // The numbers represent a percent of the total bandwidth
            // allocated to that VC.
            if ( settings[i] == -1 ) {
                // This will use the "extra" counter
                which_counter[i] = num_counters - 1;
            }
            else {
                which_counter[i] = count;
                qos_settings[count] = window * settings[i] / 100;
                counters[count] = qos_settings[count];
                remainder -= qos_settings[count];
                count++;
            }
        }
        if ( num_counters != num_vns ) {
            qos_settings[num_counters - 1] = remainder;
            counters[num_counters - 1] = remainder;
        }


    }

    ~output_arb_qos_multi() {
        delete arb;
        for ( int i = 0; i < num_vns; ++ i ) delete vn_arb[i];
        delete[] vn_arb;
        delete[] qos_settings;
        delete[] counters;
        delete[] which_counter;
        delete[] vcs_per_vn;
        delete[] vn_offset;
    }
    
    void setVCs(int n_vns, int* vcs_per_vn_in) {
        if ( n_vns != num_vns ) {
            // Error, this means the original qos_settings were wrong
            merlin_abort.fatal(CALL_INFO_LONG,1,"output_arb_qos_multi: size of qos_settings does not match number of vns in network\n");
        }
        vcs_per_vn = new int[num_vns];
        vn_offset = new int[num_vns];
        Params empty;
        arb = loadModule<SingleArbitration>(arb_vns_name,empty,num_vns);
        vn_arb = new SingleArbitration*[num_vns];
        int offset = 0;
        for ( int i = 0; i < num_vns; ++i ) {
            vcs_per_vn[i] = vcs_per_vn_in[i];
            vn_offset[i] = offset;
            offset += vcs_per_vn[i];
            vn_arb[i] = loadModule<SingleArbitration>(arb_vcs_name,empty,vcs_per_vn[i]);
        }
    }

    int arbitrate(Cycle_t cycle, PortInterface::port_queue_t* out_q, int* port_out_credits, bool isHostPort, bool& have_packets) {
        // First see if we need to see if we need to reest the
        // counters. We reset if we cross over a boundary of window
        // cycles
        Cycle_t next_reset = ((last_cycle + window) / window ) * window;
        if ( cycle >= next_reset ) {
            for ( int i = 0; i < num_counters; ++i ) {
                counters[i] = qos_settings[i];
            }
        }
        last_cycle = cycle;
        
        int vc_to_send = -1;
        bool found = false;
        internal_router_event* send_event = NULL;
        have_packets = false;

        // First, we will look through all the VNs to find packets.  A
        // packet can send as long as there are "qos credits" left.
        for ( int i = 0; i < num_vns; ++i ) {
            if ( found ) {
                arb->satisfied();
                break;
            }
            int vn = arb->next();

            // Now look through all this VNs VCs
            for ( int j = 0; j < vcs_per_vn[vn]; ++j ) {
                int vc = vn_arb[vn]->next();
                int actual_vc = vn_offset[vn] + vc;
                if ( out_q[actual_vc].empty() ) continue;
                have_packets = true;
                send_event = out_q[actual_vc].front();
                // Check to see if there are qos credits available.  We'll
                // send if there are any credits left (there doesn't need
                // to be enough for the whole packet)
                if ( counters[which_counter[vn]] <= 0 ) continue;
                // Check to see if there are enough port credits
                if ( port_out_credits[isHostPort ? send_event->getVN() : actual_vc] < send_event->getFlitCount() ) continue;
                vc_to_send = actual_vc;
                found = true;
                vn_arb[vn]->satisfied();
                counters[which_counter[vn]] -= send_event->getFlitCount();
                break;
            }
        }
        if ( found ) {
            return vc_to_send;
        }

        // If we found packets, but didn't send anything, we will try
        // again, this time without the qos constraints (this will
        // allow service levels that have used all their bandwidth to
        // use more if there are no other service levels that can
        // progress
        if ( !have_packets ) {
            return -1;
        }
        for ( int i = 0; i < num_vns; ++i ) {
            if ( found ) {
                arb->satisfied();
                break;
            }
            int vn = arb->next();

            // Now look through all this VN's VCs
            for ( int j = 0; j < vcs_per_vn[vn]; ++j ) {
                int vc = vn_arb[vn]->next();
                int actual_vc = vn_offset[vn] + vc;
                if ( out_q[actual_vc].empty() ) continue;
                send_event = out_q[actual_vc].front();
                // Check to see if there are enough port credits
                if ( port_out_credits[isHostPort ? send_event->getVN() : actual_vc] < send_event->getFlitCount() ) continue;
                vc_to_send = actual_vc;
                found = true;
                vn_arb[vn]->satisfied();
                break;
            }
        }
        
        return vc_to_send;
    }

    void dumpState(std::ostream& stream) {
    }

};

}
}

#endif // COMPONENTS_MERLIN_ROUTER_H
