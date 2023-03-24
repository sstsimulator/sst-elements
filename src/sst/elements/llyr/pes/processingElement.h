// Copyright 2013-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2022, NTESS
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
#include <algorithm>

#include "../graph/graph.h"
#include "../lsQueue.h"
#include "../llyrTypes.h"
#include "../llyrHelpers.h"

namespace SST {
namespace Llyr {

typedef struct alignas(uint64_t) {
    bool forwarded_;
    int32_t argument_;
    std::string* routing_arg_;
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

        input_queues_= new std::vector< LlyrQueue* >();
        output_queues_ = new std::vector< LlyrQueue* >();
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

        LlyrQueue* tempQueue = new LlyrQueue;
        tempQueue->forwarded_ = 0;
        tempQueue->argument_  = 0;
        tempQueue->routing_arg_ = new std::string("");
        tempQueue->data_queue_ = new std::queue< LlyrData >;
        input_queues_->push_back(tempQueue);

        return queueId;
    }

    uint32_t bindInputQueue(ProcessingElement* src, uint32_t queueId, int32_t argument)
    {
        output_->verbose(CALL_INFO, 4, 0, ">> Binding Input Queue-%" PRIu32 " on PE-%" PRIu32 " to PE-%" PRIu32 "\n",
                         queueId, processor_id_, src->getProcessorId() );

        auto retVal = input_queue_map_.emplace( queueId, src );
        if( retVal.second == false ) {
            return 0;
        }

        while( input_queues_->size() <= queueId ) {
            LlyrQueue* tempQueue    = new LlyrQueue;
            tempQueue->forwarded_   = 0;
            tempQueue->routing_arg_ = new std::string("");
            tempQueue->argument_    = 0;
            tempQueue->data_queue_  = new std::queue< LlyrData >;
            input_queues_->push_back(tempQueue);
        }

        input_queues_->at(queueId)->argument_ = argument;

        return queueId;
    }

    uint32_t bindInputQueue(ProcessingElement* src, uint32_t queueId, int32_t argument, std::string* routing_arg)
    {
        output_->verbose(CALL_INFO, 4, 0, ">> Binding Input Queue-%" PRIu32 " on PE-%" PRIu32 " to PE-%" PRIu32 "\n",
                         queueId, processor_id_, src->getProcessorId() );

        auto retVal = input_queue_map_.emplace( queueId, src );
        if( retVal.second == false ) {
            return 0;
        }

        while( input_queues_->size() <= queueId ) {
            LlyrQueue* tempQueue    = new LlyrQueue;
            tempQueue->forwarded_   = 0;
            tempQueue->routing_arg_ = new std::string("");
            tempQueue->argument_    = 0;
            tempQueue->data_queue_  = new std::queue< LlyrData >;
            input_queues_->push_back(tempQueue);
        }

        input_queues_->at(queueId)->argument_    = argument;
        input_queues_->at(queueId)->routing_arg_ = routing_arg;

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

        LlyrQueue* tempQueue = new LlyrQueue;
        tempQueue->forwarded_ = 0;
        tempQueue->argument_  = 0;
        tempQueue->routing_arg_ = new std::string("");
        tempQueue->data_queue_ = new std::queue< LlyrData >;
        output_queues_->push_back(tempQueue);

        return queueId;
    }

    uint32_t bindOutputQueue(ProcessingElement* dst, uint32_t queueId)
    {
        output_->verbose(CALL_INFO, 4, 0, ">> Binding Output Queue-%" PRIu32 " on PE-%" PRIu32 " to PE-%" PRIu32 "\n",
                         queueId, processor_id_, dst->getProcessorId() );

        auto retVal = output_queue_map_.emplace( queueId, dst );
        if( retVal.second == false ) {
            return 0;
        }

        while( output_queues_->size() <= queueId ) {
            LlyrQueue* tempQueue = new LlyrQueue;
            tempQueue->forwarded_ = 0;
            tempQueue->argument_  = 0;
            tempQueue->routing_arg_ = new std::string("");
            tempQueue->data_queue_ = new std::queue< LlyrData >;
            output_queues_->push_back(tempQueue);
        }

        return queueId;
    }

    void setOutputQueueRoute(uint32_t queueID, std::string* routing_arg)
    {
        if( output_queues_->size() < queueID ) {
            output_->fatal(CALL_INFO, -1, "Error: Output Size %" PRIu64 " Smaller Than ID(%" PRIu32 "\n",
                           output_queues_->size(), queueID);
        }

        output_queues_->at(queueID)->routing_arg_ = routing_arg;
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

    int32_t getQueueOutputProcBinding(ProcessingElement* pe) const
    {
        auto it = output_queue_map_.begin();
        for( ; it != output_queue_map_.end(); ++it ) {
            if( it->second == pe ) {
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

    int32_t getQueueInputProcBinding(ProcessingElement* pe) const
    {
        auto it = input_queue_map_.begin();
        for( ; it != input_queue_map_.end(); ++it ) {
            if( it->second == pe ) {
                return it->first;
            }
        }

        return -1;
    }

    uint32_t getNumInputQueues() const
    {
        if( input_queues_ != nullptr) {
            return input_queues_->size();
        } else {
            return 0;
        }
    }
    uint32_t getNumOutputQueues() const
    {
        if( output_queues_ != nullptr) {
            return output_queues_->size();
        } else {
            return 0;
        }
    }

    void     setOpBinding(opType binding) { op_binding_ = binding; }
    opType   getOpBinding() const { return op_binding_; }

    void     setProcessorId(uint32_t id) { processor_id_ = id; }
    uint32_t getProcessorId() const { return processor_id_; }

    bool     getPendingOp() const { return pending_op_; }

    void printInputQueue()
    {
        for( uint32_t i = 0; i < input_queues_->size(); ++i ) {
            std::cout << "i:" << i << "(" << input_queues_->at(i)->argument_ << ")";
            std::cout << ": " << input_queues_->at(i)->data_queue_->size();
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
    virtual void inputQueueInit() = 0;
    virtual void outputQueueInit() = 0;

protected:
    opType   op_binding_;
    uint32_t processor_id_;

    uint16_t latency_;
    uint16_t cycles_to_fire_;

    // input and output queues per PE
    uint32_t queue_depth_;
    std::vector< LlyrQueue* >* input_queues_;
    std::vector< LlyrQueue* >* output_queues_;

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

    // Make sure that anything that needs to be routed gets routed
    virtual void doRouting( uint32_t total_num_inputs )
    {
        for( uint32_t i = 0; i < total_num_inputs; ++i) {
            bool routed = 0;
            const std::string rtr_arg = *input_queues_->at(i)->routing_arg_;
            std::cout << "\trtr_arg " << i << " -- " << rtr_arg << " (" << input_queues_->at(i)->forwarded_ << ")" << std::endl;
            if( rtr_arg == "" || input_queues_->at(i)->forwarded_ == 1 ) {
                std::cout << "continue" << std::endl;
                continue;
            }

            std::cout << "num output queues " << output_queues_->size() << std::endl;
            for( uint32_t j = 0; j < output_queues_->size(); ++j) {
                std::cout << "output queue arg (" << j << ") " << *output_queues_->at(j)->routing_arg_ << std::endl;
                if( *output_queues_->at(j)->routing_arg_ == rtr_arg && input_queues_->at(i)->data_queue_->size() > 0) {
                    std::cout << "Now I'm hereherehere_3 -- " << input_queues_->at(i)->data_queue_->front() << std::endl;

                    routed = 1;
                    output_queues_->at(j)->data_queue_->push(input_queues_->at(i)->data_queue_->front());
                    std::cout << "data type arg " << input_queues_->at(i)->argument_ << std::endl;
                    output_->verbose(CALL_INFO, 4, 0, "+Routing %s from %" PRIu32 "\n", rtr_arg.c_str(), i);
                }
            }

            if( routed == 1 ) {
                if( input_queues_->at(i)->argument_ == -1 ) {
                    input_queues_->at(i)->data_queue_->pop();
                } else {
                    input_queues_->at(i)->forwarded_ = 1;
                }
            }

        }
    }

private:

};


}//SST
}//Llyr

#endif //_LLYR_PE_H
