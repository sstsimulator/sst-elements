// Copyright 2013-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2025, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _COMPLEX_PE_H
#define _COMPLEX_PE_H

#include <cmath>

#include "pes/processingElement.h"

namespace SST {
namespace Llyr {

/**
 * @todo write docs
 */
class ComplexProcessingElement : public ProcessingElement
{
public:
    ComplexProcessingElement(opType op_binding, uint32_t processor_id, LlyrConfig* llyr_config) :
                    ProcessingElement(op_binding, processor_id, llyr_config)
    {
        latency_ = llyr_config->complex_latency_;
        cycles_to_fire_ = latency_;
    }

    virtual bool doReceive(LlyrData data) { return 0; };

    virtual bool doCompute()
    {
        output_->verbose(CALL_INFO, 4, 0, ">> Compute %s\n", getOpString(op_binding_).c_str());

        uint64_t intResult = 0x0F;

        std::vector< LlyrData > argList;
        LlyrData retVal;

        if( output_->getVerboseLevel() >= 10 ) {
            output_->verbose(CALL_INFO, 10, 0, "Queue Contents (0)\n");
            printInputQueue();
            printOutputQueue();
        }

        uint32_t num_ready = 0;
        uint32_t num_inputs = 0;
        uint32_t total_num_inputs = input_queues_->size();

        // discover which of the input queues are used for the compute
        for( uint32_t i = 0; i < total_num_inputs; ++i) {
            if( input_queues_->at(i)->argument_ > -1 ) {
                num_inputs = num_inputs + 1;
            }
        }

        // FIXME check to see of there are any routing jobs -- should be able to do this without waiting to fire
        bool routed = doRouting( total_num_inputs );

        //check to see if all of the input queues have data
        for( uint32_t i = 0; i < total_num_inputs; ++i) {
            if( input_queues_->at(i)->argument_ > -1 ) {
                if( input_queues_->at(i)->data_queue_->size() > 0 ) {
                    num_ready = num_ready + 1;
                }
            }
        }

        //if there are values waiting on any of the inputs, this PE could still fire
        if( num_ready < num_inputs && num_ready > 0) {
            pending_op_ = 1;
        } else {
            pending_op_ = 0 | routed;
        }

        // make sure all of the output queues have room for new data
        for( uint32_t i = 0; i < output_queues_->size(); ++i) {
            // std::cout << " Queue " << i << " Size " << output_queues_->at(i)->data_queue_->size() << " Max " << queue_depth_ << std::endl;
             if( output_queues_->at(i)->data_queue_->size() >= queue_depth_ && *output_queues_->at(i)->routing_arg_ == "" ) {
                output_->verbose(CALL_INFO, 4, 0, "-Inputs %" PRIu32 " Ready %" PRIu32 " -- No room in output queue %" PRIu32 ", cannot fire\n", num_inputs, num_ready, i);
                return false;
            }
        }

        //if all inputs are available pull from queue and add to arg list
        if( num_inputs == 0 || num_ready < num_inputs ) {
            output_->verbose(CALL_INFO, 4, 0, "-Inputs %" PRIu32 " Ready %" PRIu32 " Fire %" PRIu16 "\n", num_inputs, num_ready, cycles_to_fire_);
            return false;
        } else if( cycles_to_fire_ > 0 ) {
            output_->verbose(CALL_INFO, 4, 0, "+Inputs %" PRIu32 " Ready %" PRIu32 " Fire %" PRIu16 "\n", num_inputs, num_ready, cycles_to_fire_);
            cycles_to_fire_ = cycles_to_fire_ - 1;
            pending_op_ = 1;
            return false;
        } else {
            output_->verbose(CALL_INFO, 4, 0, "+Inputs %" PRIu32 " Ready %" PRIu32 " Fire %" PRIu16 "\n", num_inputs, num_ready, cycles_to_fire_);
            for( uint32_t i = 0; i < total_num_inputs; ++i) {
                if( input_queues_->at(i)->argument_ > -1 ) {
                    argList.push_back(input_queues_->at(i)->data_queue_->front());
                    input_queues_->at(i)->forwarded_ = 0;
                    input_queues_->at(i)->data_queue_->pop();
                }
            }
            cycles_to_fire_ = latency_;
        }

        // If data tokens in output queue then simulation cannot end
        pending_op_ = 1;

        switch( op_binding_ ) {
            case TSIN :
                intResult = sin(argList[0].to_ullong());
                break;
            case TCOS :
                intResult = cos(argList[0].to_ullong());
                break;
            case TTAN :
                intResult = tan(argList[0].to_ullong());
                break;
            default :
                output_->verbose(CALL_INFO, 0, 0, "Error: could not find corresponding op-%" PRIu32 ".\n", op_binding_);
                exit(-1);
        }

        retVal = LlyrData(intResult);

        output_->verbose(CALL_INFO, 32, 0, "intResult = %" PRIu64 "\n", intResult);
        output_->verbose(CALL_INFO, 32, 0, "retVal = %s\n", retVal.to_string().c_str());

        //for now push the result to all output queues
        for( uint32_t i = 0; i < output_queues_->size(); ++i) {
            output_queues_->at(i)->data_queue_->push(retVal);
        }

        if( output_->getVerboseLevel() >= 10 ) {
            output_->verbose(CALL_INFO, 10, 0, "Queue Contents (1)\n");
            printInputQueue();
            printOutputQueue();
        }

        return true;
    }

    //TODO for testing only
    virtual void inputQueueInit() {};
    virtual void outputQueueInit() {};

};

}//SST
}//Llyr

#endif // _LOGIC_PE_H
