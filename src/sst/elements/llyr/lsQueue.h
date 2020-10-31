// Copyright 2009-2020 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2020, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _LLYR_LSQ
#define _LLYR_LSQ

#include <sst/core/output.h>
#include <sst/core/interfaces/simpleMem.h>

#include <map>
#include <queue>
#include <cstdint>

using namespace SST::Interfaces;

namespace SST {
namespace Llyr {


class LSEntry
{
public:
    LSEntry(const SimpleMem::Request::id_t reqId, uint32_t targetProc) :
            req_id_(reqId), target_proc_(targetProc) {}
    ~LSEntry() {}

    uint32_t getTargetPe() { return target_proc_; }
    SimpleMem::Request::id_t getReqId() { return req_id_; }

protected:
    SimpleMem::Request::id_t req_id_;
    uint32_t target_proc_;

private:

};

class LSQueue
{
public:
    LSQueue(SST::Output* output) :
            output_(output) {}
    ~LSQueue() {}

    void addEntry( LSEntry* entry )
    {
        memory_queue_.push( entry->getReqId() );
//         pending_.insert( std::pair< SimpleMem::Request::id_t, LSEntry* >( entry->getReqId(), entry ) );
        pending_.emplace( entry->getReqId(), entry );
    }

    uint32_t lookupEntry( SimpleMem::Request::id_t id )
    {
        auto entry = pending_.find( id );

        if( entry == pending_.end() )
        {
            fprintf(stderr, "Error: response from memory could not be found.\n");
            exit(-1);
        }

        return entry->second->getTargetPe();
    }

    void removeEntry( SimpleMem::Request::id_t id )
    {
        auto entry = pending_.find( id );

        if( entry != pending_.end() )
        {
            pending_.erase(entry);
        }
    }


protected:


private:
    SST::Output* output_;

    std::queue< SimpleMem::Request::id_t > memory_queue_;
    std::map< SimpleMem::Request::id_t, LSEntry* > pending_;

};


}
}

#endif
