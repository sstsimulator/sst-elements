// Copyright 2009-2026 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2026, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
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

namespace SST {
namespace Merlin {

class xbar_arb_lru : public XbarArbitration {

public:

    SST_ELI_REGISTER_SUBCOMPONENT(
        xbar_arb_lru,
        "merlin",
        "xbar_arb_lru",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Least recently used arbitration unit for hr_router",
        SST::Merlin::XbarArbitration
    )


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

    xbar_arb_lru() : XbarArbitration() {}

    xbar_arb_lru(ComponentId_t cid, Params& param) :
        XbarArbitration(cid)
    {
    }

    ~xbar_arb_lru() {
    }

    void serialize_order(SST::Core::Serialization::serializer& ser) override {
        XbarArbitration::serialize_order(ser);
        SST_SER(num_ports);
        SST_SER(num_vcs);
        SST_SER(total_entries);

        // Need to serialize each of the priority arrays separately as there is no way to pass the fact that that the
        // data in the array are c-style arrays
        SST_SER(SST::Core::Serialization::array(priority[0], total_entries));
        SST_SER(SST::Core::Serialization::array(priority[1], total_entries));

        // Serialize which buffer is current vs next (as index 0 or 1)
        int cur_idx = (cur_list == priority[0]) ? 0 : 1;
        SST_SER(cur_idx);
        if ( ser.mode() == SST::Core::Serialization::serializer::UNPACK ) {
            cur_list = priority[cur_idx];
            next_list = priority[1 - cur_idx];
        }
        // vc_heads is a non-owning scratch buffer, re-allocated on UNPACK
        if ( ser.mode() == SST::Core::Serialization::serializer::UNPACK ) {
            vc_heads = new internal_router_event*[num_vcs];
        }
    }
    ImplementSerializable(SST::Merlin::xbar_arb_lru)

    void setPorts(int num_ports_s, int num_vcs_s) override
    {
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
                   PortInterface** ports, int* in_port_busy, int* out_port_busy, int* progress_vc, bool clocking
#else
                   PortInterface** ports, int* in_port_busy, int* out_port_busy, int* progress_vc
#endif
                   ) override
    {

        for ( int i = 0; i < num_ports; i++ ) progress_vc[i] = -1;

        // std::cout << "---------" << std::endl;
        // for ( int i = 0; i < total_entries; i++ ) {
        //     std::cout << priority[cur_list][i].first << ", " << priority[cur_list][i].second << std::endl;
        // }
        // std::cout << "---------" << std::endl;

        // Run through the priority list
        // for ( priority_list_t::iterator it = priority.begin(); it != priority.end(); ) {
        // int sat_index = total_entries - 1;
        // int unsat_index = 0;

        priority_entry_t* sat_list = &next_list[total_entries-1];
        priority_entry_t* unsat_list = next_list;

        for ( int i = 0; i < total_entries; i++ ) {

            // const priority_entry_t& check = priority[cur_list][i];
            const priority_entry_t& check = cur_list[i];

            /* std::cout << check.first << ", " << check.second << std::endl; */

            int port = check.first;
            int vc = check.second;

            // std::cout << check.first << ", " << check.second << std::endl;

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


                    // Copy data to new list
                    // std::cout << "putting at bottom of list" << std::endl;
                    // next_list[sat_index--] = check;
                    *sat_list = check;
                    --sat_list;
                }
                else {
                    // std::cout << "putting at top of list" << std::endl;
                    // next_list[unsat_index++] = check;
                    *unsat_list = check;
                    ++unsat_list;
                    progress_vc[port] = -2;
                }
            }
            else {
                // std::cout << "putting at top of list" << std::endl;
                // next_list[unsat_index++] = check;
                *unsat_list = check;
                ++unsat_list;
            }
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

    void reportSkippedCycles(Cycle_t cycles) override
    {}

    void dumpState(std::ostream& stream) override
    {
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
