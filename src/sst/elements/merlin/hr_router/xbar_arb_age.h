// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2016, Sandia Corporation
// All rights reserved.
// 
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef COMPONENTS_HR_ROUTER_XBAR_ARB_AGE_H
#define COMPONENTS_HR_ROUTER_XBAR_ARB_AGE_H

#include <sst/core/component.h>
#include <sst/core/event.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>

#include <vector>
#include <queue>

#include "sst/elements/merlin/router.h"
#include "sst/elements/merlin/portControl.h"

namespace SST {
namespace Merlin {

    
class xbar_arb_age : public XbarArbitration {

private:
    /**
       Structure for sorting priority based on age
     */
    struct priority_entry_t {
        uint16_t port;
        uint16_t vc;
        uint16_t next_port;
        uint16_t next_vc;
        SimTime_t injection_time;
        int size_in_flits;

        // priority_entry_t(uint16_t port, uint16_t vc, SimTime_t injection_time) :
        //     port(port),
        //     vc(vc),
        //     injection_time(injection_time)
        // {}

        priority_entry_t() :
            port(0),
            vc(0),
            next_port(0),
            next_vc(0),
            injection_time(0),
            size_in_flits(0)
        {}
        
        priority_entry_t(uint16_t port, uint16_t vc) :
            port(port),
            vc(vc),
            next_port(0),
            next_vc(0),
            injection_time(0),
            size_in_flits(0)
        {}
    };
    
    /** To use with STL priority queues, that order in reverse. */
    class time_priority {
    public:
        /** Compare based off pointers */
        inline bool operator()(const priority_entry_t* lhs, const priority_entry_t* rhs) const {
            return lhs->injection_time > rhs->injection_time;
        }

        /** Compare based off references */
        inline bool operator()(const priority_entry_t& lhs, const priority_entry_t& rhs) {
            return lhs.injection_time > rhs.injection_time;
        }
    };

    class port_priority {
    public:
        /** Compare based off pointers */
        inline bool operator()(const priority_entry_t* lhs, const priority_entry_t* rhs) const {
            return lhs->port > rhs->port;
        }

        /** Compare based off references */
        inline bool operator()(const priority_entry_t& lhs, const priority_entry_t& rhs) {
            return lhs.port > rhs.port;
        }
    };

    typedef std::priority_queue<priority_entry_t*, std::vector<priority_entry_t*>, xbar_arb_age::time_priority> age_queue_t;
    age_queue_t age_queue;

    priority_entry_t* entries;
    
    int num_ports;
    int num_vcs;

    int total_entries;
    
    internal_router_event** vc_heads;

    // PortControl** ports;
    
public:
    xbar_arb_age(Component* parent) :
        XbarArbitration(parent)
    {
    }

    ~xbar_arb_age() {
        delete[] entries;
    }

    void setPorts(int num_ports_s, int num_vcs_s) {
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
                   PortControl** ports, int* in_port_busy, int* out_port_busy, int* progress_vc, bool clocking
#else
                   PortControl** ports, int* in_port_busy, int* out_port_busy, int* progress_vc
#endif
                   )
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
                    entries[index].injection_time = vc_heads[j]->getEncapsulatedEvent()->getInjectionTime();
                    entries[index].size_in_flits = vc_heads[j]->getFlitCount();

                    age_queue.push(&entries[index]);
                }
                index++;
            }
            
        }

        while ( !age_queue.empty() ) {

            priority_entry_t* entry = age_queue.top();
            age_queue.pop();
            
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

#endif // COMPONENTS_HR_ROUTER_XBAR_ARB_AGE_H
