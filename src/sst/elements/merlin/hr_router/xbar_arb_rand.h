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


#ifndef COMPONENTS_HR_ROUTER_XBAR_ARB_RAND_H
#define COMPONENTS_HR_ROUTER_XBAR_ARB_RAND_H

#include <sst/core/component.h>
#include <sst/core/event.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>
#include <sst/core/rng/xorshift.h>

#include <vector>
#include <queue>

#include "sst/elements/merlin/router.h"

namespace SST {
namespace Merlin {


class xbar_arb_rand : public XbarArbitration {

public:

    SST_ELI_REGISTER_SUBCOMPONENT(
        xbar_arb_rand,
        "merlin",
        "xbar_arb_rand",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Random arbitration unit for hr_router",
        SST::Merlin::XbarArbitration
    )


private:
    /**
       Structure for sorting based on random priority
     */
    struct priority_entry_t {
        uint16_t port = 0;
        uint16_t vc = 0;
        uint16_t next_port = 0;
        uint16_t next_vc = 0;
        double rand_pri = 0.0;
        int size_in_flits = 0;

        priority_entry_t() = default;

        priority_entry_t(uint16_t port, uint16_t vc) :
            port(port),
            vc(vc),
            next_port(0),
            next_vc(0),
            rand_pri(0.0),
            size_in_flits(0)
        {}

        void serialize_order(SST::Core::Serialization::serializer& ser)
        {
            SST_SER(port);
            SST_SER(vc);
            SST_SER(next_port);
            SST_SER(next_vc);
            SST_SER(rand_pri);
            SST_SER(size_in_flits);
        }
    };

    /** To use with STL priority queues, that order in reverse. */
    class rand_priority {
    public:
        /** Compare based off pointers */
        inline bool operator()(const priority_entry_t* lhs, const priority_entry_t* rhs) const {
            return lhs->rand_pri > rhs->rand_pri;
        }

        /** Compare based off references */
        inline bool operator()(const priority_entry_t& lhs, const priority_entry_t& rhs) {
            return lhs.rand_pri > rhs.rand_pri;
        }
    };


    typedef std::priority_queue<priority_entry_t*, std::vector<priority_entry_t*>, xbar_arb_rand::rand_priority> rand_queue_t;
    rand_queue_t rand_queue;

    priority_entry_t* entries = nullptr;

    int num_ports = 0;
    int num_vcs = 0;

    int total_entries = 0;

    internal_router_event** vc_heads = nullptr;

    RNG::XORShiftRNG* rng = nullptr;

    // PortControl** ports;

public:

    xbar_arb_rand() = default;

    xbar_arb_rand(ComponentId_t cid, Params& params) :
        XbarArbitration(cid)
    {
        rng = new RNG::XORShiftRNG(69);
    }

    ~xbar_arb_rand() {
        delete[] entries;
    }

    void serialize_order(SST::Core::Serialization::serializer& ser) override {
        XbarArbitration::serialize_order(ser);
        SST_SER(num_ports);
        SST_SER(num_vcs);
        SST_SER(total_entries);
        SST_SER(rng);

        SST_SER(SST::Core::Serialization::array(entries, total_entries));

        if ( ser.mode() == SST::Core::Serialization::serializer::UNPACK ) {
            vc_heads = new internal_router_event*[num_vcs];
        }
    }
    ImplementSerializable(SST::Merlin::xbar_arb_rand)

    void setPorts(int num_ports_s, int num_vcs_s) override
    {
        num_ports = num_ports_s;
        num_vcs = num_vcs_s;

        total_entries = num_ports * num_vcs;
        entries = new priority_entry_t[total_entries];

        int index = 0;
        for ( int i = 0; i < num_ports; i++ ) {
            for ( int j = 0; j < num_vcs; j++ ) {
                entries[index++] = priority_entry_t(i,j);
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


        // Find all ports that have data and who's inputs to the xbar
        // aren't busy.  Sort them by prioritizing on injection time.
        // Oldest gets top priority.
        int index = 0;
        for ( int i = 0; i < num_ports; i++ ) {
            if ( in_port_busy[i] > 0 ) {
                index += num_vcs;
                continue; // No need to consider port if input to xbar is busy
            }

            vc_heads = ports[i]->getVCHeads();
            for ( int j = 0; j < num_vcs; j++ ) {
                internal_router_event* src_event = vc_heads[j];
                if ( src_event != NULL ) {
                    entries[index].next_port = vc_heads[j]->getNextPort();
                    entries[index].next_vc = vc_heads[j]->getVC();
                    entries[index].size_in_flits = vc_heads[j]->getFlitCount();
                    entries[index].rand_pri = rng->nextUniform();

                    rand_queue.push(&entries[index]);
                }
                index++;
            }

        }

        while ( !rand_queue.empty() ) {

            priority_entry_t* entry = rand_queue.top();
            rand_queue.pop();

            int port = entry->port;
            int vc = entry->vc;

            // if the input to the xbar for this port is busy, nothing
            // to do.  This will only happen at this point if a higher
            // priority VC from this port was satisfied this cycle.
            if ( in_port_busy[port] <= 0 ) {
                // Have an event, see if it can be progressed
                int next_port = entry->next_port;
                int next_vc = entry->next_vc;

                // We can progress if the next port's output from xbar
                // is not busy and there are enough credits.
                if ( out_port_busy[next_port] <= 0 &&
                     ports[next_port]->spaceToSend(next_vc, entry->size_in_flits) ) {

                    // Tell the router what to move
                    progress_vc[port] = vc;

                    // Need to set the busy values
                    in_port_busy[port] = entry->size_in_flits;
                    out_port_busy[next_port] = entry->size_in_flits;

                }
                else {
                    progress_vc[port] = -2;
                }
            }
        }

        // std::cout << "+++++++++" << std::endl;
        // for ( int i = 0; i < total_entries; i++ ) {
        //     std::cout << priority[next_list][i].first << ", " << priority[cur_list][i].second << std::endl;
        // }
        // std::cout << "+++++++++" << std::endl;

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

#endif // COMPONENTS_HR_ROUTER_XBAR_ARB_RAND_H
