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
#include <sst/core/interfaces/stdMem.h>

#include <map>
#include <queue>
#include <vector>
#include <bitset>
#include <string>
#include <cstdint>
#include <sstream>


#include "../graph/graph.h"
#include "../lsQueue.h"
#include "../llyrTypes.h"

namespace SST {
namespace Llyr {

typedef struct alignas(uint64_t) {
    bool forward_;
    std::queue< LlyrData >* data_queue_;
} LlyrQueue;

class ProcessingElement
{
public:
    ProcessingElement(opType op_binding, uint32_t processor_id, LlyrConfig* llyr_config)  :
                    op_binding_(op_binding), processor_id_(processor_id),
                    pending_op_(0), llyr_config_(llyr_config)
    {
        //setup up i/o for messages
        char prefix[256];
        sprintf(prefix, "[t=@t][ProcessingElement-%u]: ", processor_id_);
        output_ = new SST::Output(prefix, llyr_config_->verbosity_, 0, Output::STDOUT);

        pending_op_ = 0;
        lsqueue_ = llyr_config->lsqueue_;
        mem_interface_ = llyr_config->mem_interface_;
    }

    virtual ~ProcessingElement() {};

    uint32_t bindInputQueue(ProcessingElement* src)
    {
        uint32_t queueId = input_queues_->size();

        output_->verbose(CALL_INFO, 4, 0, ">> Binding Input Queue-%" PRIu32 " on PE-%" PRIu32 " to PE-%" PRIu32 "\n",
                        queueId, processor_id_, src->getProcessorId() );

        auto retVal = input_queue_map_.emplace( queueId, src );
        if( retVal.second == false ) {
            return 0;
        }

        //if insert succeeded need to create the queue
//         std::queue< LlyrData >* tempQueue = new std::queue< LlyrData >;
//         input_queues_->push_back(tempQueue);

        LlyrQueue* tempQueue = new LlyrQueue;
        tempQueue->data_queue_ = new std::queue< LlyrData >;
        input_queues_->push_back(tempQueue);

        return queueId;
    }

    uint32_t bindOutputQueue(ProcessingElement* dst)
    {
        uint32_t queueId = output_queues_->size();

        output_->verbose(CALL_INFO, 4, 0, ">> Binding Output Queue-%" PRIu32 " on PE-%" PRIu32 " to PE-%" PRIu32 "\n",
                        queueId, processor_id_, dst->getProcessorId() );

        auto retVal = output_queue_map_.emplace( queueId, dst );
        if( retVal.second == false ) {
            return 0;
        }

        //if insert succeeded need to create the queue
//         std::queue< LlyrData >* tempQueue = new std::queue< LlyrData >;
//         output_queues_->push_back(tempQueue);

        LlyrQueue* tempQueue = new LlyrQueue;
        tempQueue->data_queue_ = new std::queue< LlyrData >;
        output_queues_->push_back(tempQueue);

        return queueId;
    }

    void pushInputQueue(uint32_t id, uint64_t &inVal )
    {
        LlyrData newValue = LlyrData(inVal);
        input_queues_->at(id)->data_queue_->push(newValue);
    }

    void pushInputQueue(uint32_t id, LlyrData &inVal )
    {
        input_queues_->at(id)->data_queue_->push(inVal);
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

    ProcessingElement* getProcInputQueueBinding(uint32_t id) const
    {
        auto it = input_queue_map_.begin();
        for( ; it != input_queue_map_.end(); ++it ) {
            if( it->first == id ) {
                return it->second;
            }
        }

        return NULL;
    }

    uint32_t getNumInputQueues() const { return input_queues_->size(); }
    uint32_t getNumOutputQueues() const { return output_queues_->size(); }

    void     setOpBinding(opType binding) { op_binding_ = binding; }
    opType   getOpBinding() const { return op_binding_; }

    void     setProcessorId(uint32_t id) { processor_id_ = id; }
    uint32_t getProcessorId() const { return processor_id_; }

    bool     getPendingOp() const { return pending_op_; }

    void printInputQueue()
    {
        for( uint32_t i = 0; i < input_queues_->size(); ++i ) {
            std::cout << "i:" << i << ": " << input_queues_->at(i)->data_queue_->size();
            if( input_queues_->at(i)->data_queue_->size() > 0 ) {
                std::cout << ":" << input_queues_->at(i)->data_queue_->front().to_ullong() << ":" << input_queues_->at(i)->data_queue_->front() << "\n";
            } else {
                std::cout << ":x" << ":x" << "\n";
            }
        }
    }

    void printOutputQueue()
    {
        for( uint32_t i = 0; i < output_queues_->size(); ++i ) {
            std::cout << "o:" << i << ": " << output_queues_->at(i)->data_queue_->size();
            if( output_queues_->at(i)->data_queue_->size() > 0 ) {
                std::cout << ":" << output_queues_->at(i)->data_queue_->back().to_ullong() << ":" << output_queues_->at(i)->data_queue_->front() << "\n";
            } else {
                std::cout << ":x" << ":x" << "\n";
            }
        }
    }

    virtual bool doSend() = 0;
    virtual bool doReceive(LlyrData data) = 0;
    virtual bool doCompute() = 0;

    //TODO for testing only
    virtual void queueInit() = 0;

protected:
    opType op_binding_;

    uint32_t cycles_;
    uint32_t processor_id_;

    // input and output queues per PE
    uint32_t queue_depth_;
    std::vector< LlyrQueue* >* input_queues_;
    std::vector< LlyrQueue* >* output_queues_;

/*    std::vector< std::queue< LlyrData >* >* input_queues_;
    std::vector< std::queue< LlyrData >* >* output_queues_;*/

    // need to connect PEs to queues -- queue_id, src/dst
    std::map< uint32_t, ProcessingElement* > input_queue_map_;
    std::map< uint32_t, ProcessingElement* > output_queue_map_;

    // track outstanding L/S requests (passed from top-level)
    LSQueue* lsqueue_;

    Interfaces::StandardMem*  mem_interface_;
    SST::Output* output_;

    // used to stall execution - waiting on mem/queues full
    bool pending_op_;

    // bundle of configuration parameters
    LlyrConfig* llyr_config_;

private:

};


}//SST
}//Llyr

#endif //_LLYR_PE_H
