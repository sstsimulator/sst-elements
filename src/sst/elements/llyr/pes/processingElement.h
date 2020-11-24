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


#ifndef _LLYR_PE_H
#define _LLYR_PE_H

#include <sst/core/output.h>
#include <sst/core/interfaces/simpleMem.h>

#include <map>
#include <queue>
#include <vector>
#include <bitset>
#include <cstdint>
#include <cstring>

#include "graph.h"
#include "lsQueue.h"
#include "llyrTypes.h"

using namespace SST::Interfaces;

namespace SST {
namespace Llyr {

class ProcessingElement
{
public:
    ProcessingElement(opType op_binding, uint32_t processor_id, uint32_t queue_depth,
                    LSQueue* lsqueue, SimpleMem*  mem_interface)  :
                    op_binding_(op_binding), processor_id_(processor_id), queue_depth_(queue_depth),
                    lsqueue_(lsqueue), mem_interface_(mem_interface), pending_op_(0)
    {
        //setup up i/o for messages
        char prefix[256];
        sprintf(prefix, "[t=@t][ProcessingElement-%u]: ", processor_id_);
        output_ = new SST::Output(prefix, 0, 0, Output::STDOUT);

        input_queues_= new std::vector< std::queue< LlyrData >* >;
        output_queues_ = new std::vector< std::queue< LlyrData >* >;
    }

    virtual ~ProcessingElement() {};

    uint32_t bindInputQueue(ProcessingElement* src)
    {
        uint32_t queueId = input_queues_->size();

        output_->verbose(CALL_INFO, 0, 0, ">> Binding Input Queue-%" PRIu32 " on PE-%" PRIu32 " to PE-%" PRIu32 "\n",
                        queueId, processor_id_, src->getProcessorId() );

        auto retVal = input_queue_map_.emplace( queueId, src );
        if( retVal.second == false ) {
            return 0;
        }

        //if insert succeeded need to create the queue
        std::queue< LlyrData >* tempQueue = new std::queue< LlyrData >;
        input_queues_->push_back(tempQueue);

        return queueId;
    }

    uint32_t bindOutputQueue(ProcessingElement* dst)
    {
        uint32_t queueId = output_queues_->size();

        output_->verbose(CALL_INFO, 0, 0, ">> Binding Output Queue-%" PRIu32 " on PE-%" PRIu32 " to PE-%" PRIu32 "\n",
                        queueId, processor_id_, dst->getProcessorId() );

        auto retVal = output_queue_map_.emplace( queueId, dst );
        if( retVal.second == false ) {
            return 0;
        }

        //if insert succeeded need to create the queue
        std::queue< LlyrData >* tempQueue = new std::queue< LlyrData >;
        output_queues_->push_back(tempQueue);

        return queueId;
    }

    void pushInputQueue(uint32_t id, uint64_t &inVal )
    {
        LlyrData newValue = LlyrData(inVal);
        input_queues_->at(id)->push(newValue);
    }

    void pushInputQueue(uint32_t id, LlyrData &inVal )
    {
        input_queues_->at(id)->push(inVal);
    }

    int32_t getInputQueueId(uint32_t id) const
    {
        auto it = input_queue_map_.begin();
        for( ; it != input_queue_map_.end(); ++it ) {
            if( it->second->getProcessorId() == id ) {
                return it->first;
            }
        }

        return -1;
    }

    int32_t getOutputQueueId(uint32_t id) const
    {
        auto it = output_queue_map_.begin();
        for( ; it != output_queue_map_.end(); ++it ) {
            if( it->second->getProcessorId() == id ) {
                return it->first;
            }
        }

        return -1;
    }

    void     setOpBinding(opType binding) { op_binding_ = binding; }
    opType   getOpBinding() const { return op_binding_; }

    void     setProcessorId(uint32_t id) { processor_id_ = id; }
    uint32_t getProcessorId() const { return processor_id_; }

    bool     getPendingOp() const { return pending_op_; }

    void printInputQueue()
    {
        for( uint32_t i = 0; i < input_queues_->size(); ++i ) {
            std::cout << "i:" << i << ": " << input_queues_->at(i)->size();
            if( input_queues_->at(i)->size() > 0 ) {
                std::cout << ":" << input_queues_->at(i)->front().to_ullong() << ":" << input_queues_->at(i)->front() << "\n";
            } else {
                std::cout << ":x" << ":x" << "\n";
            }
        }
    }

    void printOutputQueue()
    {
        for( uint32_t i = 0; i < output_queues_->size(); ++i ) {
            std::cout << "o:" << i << ": " << output_queues_->at(i)->size();
            if( output_queues_->at(i)->size() > 0 ) {
                std::cout << ":" << output_queues_->at(i)->back().to_ullong() << ":" << output_queues_->at(i)->front() << "\n";
            } else {
                std::cout << ":x" << ":x" << "\n";
            }
        }
    }

    virtual bool doSend() = 0;
    virtual bool doReceive(LlyrData data) = 0;
    virtual bool doCompute() = 0;

    //TODO for testing only
    virtual void fakeInit() = 0;

protected:

    opType   op_binding_;
    uint32_t processor_id_;

    //input and output queues per PE
    uint32_t queue_depth_;
    std::vector< std::queue< LlyrData >* >* input_queues_;
    std::vector< std::queue< LlyrData >* >* output_queues_;

    //need to connect PEs to queues -- queue_id, src/dst
    std::map< uint32_t, ProcessingElement* > input_queue_map_;
    std::map< uint32_t, ProcessingElement* > output_queue_map_;

    //track outstanding L/S requests (passed from top-level)
    LSQueue* lsqueue_;

    SimpleMem*  mem_interface_;
    SST::Output* output_;

    //used to stall execution - waiting on mem/queues full
    bool pending_op_;


private:

};


}//SST
}//Llyr

#endif //_LLYR_PE_H
