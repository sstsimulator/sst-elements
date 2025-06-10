// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.
//

#ifndef _H_SST_MESSIER_CACHE
#define _H_SST_MESSIER_CACHE

#include <sst/core/sst_types.h>
#include <sst/core/event.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>
#include <sst/core/output.h>

#include <cstring>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>

#include <stdio.h>
#include <stdint.h>
#include <poll.h>

using namespace SST;

namespace SST{
namespace MessierComponent {

    // This defines the internal inside the NVM-based DIMM memory
    class NVM_CACHE
    {
        int32_t size; // The size in KB
        int32_t latency; // Latency
        int32_t assoc; // Associativity
        int32_t bs; // This is the block size of the internal cache
        int32_t ** lru; // This indicates the LRU information

        bool ** valid;
        bool ** dirty;

        uint64_t ** tag_array; // This indicates the tag array
        uint64_t num_sets; // This inidicates the number of sets

        public:


        NVM_CACHE(int32_t Size, int32_t Assoc, int32_t Latency, int32_t BS)
        {
            size = Size;
            assoc = Assoc;
            latency = Latency;
            bs = BS;

            num_sets = ((long long int) size*1024)/(bs*assoc); // Assigning the number of sets

            tag_array = new uint64_t *[num_sets];
            lru =  new int32_t * [num_sets];
            valid = new bool * [num_sets];
            dirty = new bool * [num_sets];

            for(int32_t i = 0; i < num_sets; i++)
            {
                tag_array[i] = new uint64_t [assoc];
                lru[i] = new int32_t [assoc];
                valid[i] = new bool [assoc];
                dirty[i] = new bool [assoc];

                for(int32_t j=0; j<assoc; j++)
                {
                    tag_array[i][j] = -1;
                    lru[i][j] = j;
                    valid[i][j] = false;
                    dirty[i][j] = false;
                }
            }
        }

        // Check if a hit or miss
        bool check_hit(uint64_t add)
        {
            uint64_t set = (add/bs)%num_sets;

            for(int32_t i=0; i < assoc; i++)
                if(valid[set][i] && (tag_array[set][i]/bs == add/bs))
                    return true;

            return false;
        }

        // This adds the block to the cache
        uint64_t insert_block(uint64_t add, bool clean)
        {
            uint64_t set = (add/bs)%num_sets;
            uint64_t evicted = 0;

            for(int32_t i=0; i < assoc; i++)
                if(lru[set][i]==(assoc-1))
                {
                    evicted = tag_array[set][i];
                    valid[set][i] = true;
                    tag_array[set][i] = add;
                    dirty[set][i] = !clean;
                    break;
                }

            return evicted;
        }

        bool dirty_eviction(uint64_t add)
        {

            uint64_t set = (add/bs)%num_sets;

            for(int32_t i=0; i < assoc; i++) {
                if(lru[set][i]==(assoc-1))
                {
                    //   valid[set][i] = true;
                    //  tag_array[set][i] = add;
                    if(dirty[set][i] && valid[set][i])
                        return true;
                    else
                        return false;
                }
            }

            return false;
        }


        void set_dirty(uint64_t add)
        {
            uint64_t set = (add/bs)%num_sets;
            for(int32_t i=0; i < assoc; i++) {
                if(tag_array[set][i]/bs==add/bs)
                {
                    dirty[set][i] = true;
                    break;
                }
            }
        }


        // This function updates the LRU position of the block
        void update_lru(uint64_t add)
        {

            uint64_t set = (add/bs)%num_sets;

            int32_t lru_pos = -1;
            int32_t lru_ind = -1;

            for(int32_t i=0; i < assoc; i++)
            {

                if(tag_array[set][i]/bs==add/bs)
                {
                    lru_pos = lru[set][i];
                    lru_ind  = i;
                    break;
                }
            }

            if(lru_pos == -1)
                return;

            for(int32_t i=0; i < assoc; i++)
                if(lru[set][i] < lru_pos)
                    lru[set][i]++;

            lru[set][lru_ind] = 0;
        }

        // This function invalidates a cache block
        void invalidate(uint64_t add)
        {
            uint64_t set = (add/bs)%num_sets;

            for(int32_t i=0; i < assoc; i++)
            {

                if(tag_array[set][i]/bs==add/bs)
                {
                    valid[set][i] = false;
                    break;
                }
            }

        }
    };


}
}

#endif
