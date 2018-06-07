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


#ifndef COMPONENTS_KINGSLEY_LRU_UNIT_H
#define COMPONENTS_KINGSLEY_LRU_UNIT_H

#include <vector>

using namespace SST;

namespace SST {
namespace Kingsley {

template<typename T>
class lru_unit {

    std::vector<T> priority[2];
    T* current_list;
    T* sat_list;
    T* unsat_list;
    bool finalized;
    int current;
    

    void init_lists() {
        int next = (current + 1) & 0x1;

        current_list = priority[next].data();
        unsat_list = priority[current].data();
        sat_list = unsat_list + (priority[current].size() - 1);
        current = next;

    }
    
public:
    lru_unit() : finalized(false), current(0)
    {
    }

    // Called once all the entries have been initialized
    void finalize() {
        finalized = true;
        priority[1].resize(priority[0].size());
        current = 1;
        init_lists();
    }

    void insert(T data) {
        if ( finalized ) throw std::string("lru_unit: Attempt to insert into a finalized unit.\n");
        priority[0].push_back(data);
    }

    // Gets the highest priority entry.
    const T& top() {
        if ( !finalized ) throw std::string("lru_unit: Attempt to access top element before finalizing unit.\n");
        return *current_list;
    }

    // Declares whether the top entry was satisfied or not.  Moves
    // next entry to top.
    void satisfied(bool sat) {
        if ( !finalized ) throw std::string("lru_unit: Attempt to call satisfied() before finalizing unit.\n");
        if ( sat ) {
            *sat_list = *current_list;
            sat_list--;
            current_list++;
        }
        else {
            *unsat_list = *current_list;
            unsat_list++;
            current_list++;
        }
        if ( sat_list < unsat_list ) {
            init_lists();
        }
    }

    size_t size() {
        return priority[0].size();
    }

    // void print() {
    //     for ( unsigned int i = 0; i < priority[current].size(); ++i ) {
    //         std::cout << priority[current][i] << std::endl;
    //     }
    // }
    
};

}
}

#endif // COMPONENTS_KINGSLEY_LRU_UNIT_H
