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
#include <bitset>
#include <utility>
#include <cstdint>

using namespace SST::Interfaces;

namespace SST {
namespace Llyr {


class LSEntry
{
public:
    LSEntry(const SimpleMem::Request::id_t reqId, uint32_t srcProc, uint32_t dstProc) :
            req_id_(reqId), src_proc_(srcProc), dst_proc_(dstProc) {}
    ~LSEntry() {}

    uint32_t getSourcePe() const { return src_proc_; }
    uint32_t getTargetPe() const { return dst_proc_; }
    SimpleMem::Request::id_t getReqId() const { return req_id_; }

protected:
    SimpleMem::Request::id_t req_id_;
    uint32_t src_proc_;
    uint32_t dst_proc_;

private:

};

class LSQueue
{
public:
    LSQueue()
    {
        //setup up i/o for messages
        char prefix[256];
        sprintf(prefix, "[t=@t][LSQueue]: ");
        output_ = new SST::Output(prefix, 0, 0, Output::STDOUT);
    }
    LSQueue(const LSQueue &copy)
    {
        output_ = copy.output_;
        memory_queue_ = copy.memory_queue_;
        pending_ = copy.pending_;
    }
    ~LSQueue() {}

    void addEntry( LSEntry* entry )
    {
        memory_queue_.push( entry->getReqId() );
        pending_.emplace( entry->getReqId(), entry );
    }

    bool checkEntry( SimpleMem::Request::id_t id ) const
    {
        if( memory_queue_.front() == id )
            return true;
        else
            return false;
    }

    std::pair< uint32_t, uint32_t > lookupEntry( SimpleMem::Request::id_t id )
    {
        auto entry = pending_.find( id );
        if( entry == pending_.end() )
        {
            output_->verbose(CALL_INFO, 0, 0, "Error: response from memory could not be found.\n");
            exit(-1);
        }

        return std::make_pair( entry->second->getSourcePe(), entry->second->getTargetPe() );
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
