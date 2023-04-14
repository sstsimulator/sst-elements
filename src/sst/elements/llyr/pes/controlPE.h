// Copyright 2013-2023 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2023, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _CONTROL_PE_H
#define _CONTROL_PE_H

#include "pes/processingElement.h"

namespace SST {
namespace Llyr {

/**
 * @todo write docs
 */
class ControlProcessingElement : public ProcessingElement
{
public:
    ControlProcessingElement(opType op_binding, uint32_t processor_id, LlyrConfig* llyr_config) :
                    ProcessingElement(op_binding, processor_id, llyr_config)
    {
        do_forward_ = 0;
        first_touch_ = 1;
    }

    virtual bool doSend()
    {
        uint32_t queueId;
        LlyrData sendVal;
        ProcessingElement* dstPe;

        for(auto it = output_queue_map_.begin() ; it != output_queue_map_.end(); ++it ) {
            queueId = it->first;
            dstPe = it->second;

            if( output_queues_->at(queueId)->data_queue_->size() > 0 ) {
                output_->verbose(CALL_INFO, 8, 0, ">> Sending...%" PRIu32 "-%" PRIu32 " to %" PRIu32 "\n",
                                processor_id_, queueId, dstPe->getProcessorId());

                sendVal = output_queues_->at(queueId)->data_queue_->front();
                output_queues_->at(queueId)->data_queue_->pop();

                dstPe->pushInputQueue(dstPe->getInputQueueId(this->getProcessorId()), sendVal);
            }
        }

        if( output_->getVerboseLevel() >= 10 ) {
            output_->verbose(CALL_INFO, 10, 0, "Queue Contents (2)\n");
            printInputQueue();
            printOutputQueue();
        }

        return true;
    }

    virtual bool doReceive(LlyrData data) { return 0; };

    virtual bool doCompute()
    {
        TraceFunction trace(CALL_INFO_LONG);
        output_->verbose(CALL_INFO, 4, 0, ">> Compute %s\n", getOpString(op_binding_).c_str());

        uint64_t intResult = 0x0F;

        std::vector< LlyrData > argList;
        LlyrData retVal;

        // FIXME lazy way to do a post-init so that the first token is free
        if( op_binding_ == ROS && first_touch_ == 1 ) {
            input_queues_->at(0)->data_queue_->push(LlyrData(0x00));
            first_touch_ = 0;
        }

        if( output_->getVerboseLevel() >= 10 ) {
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
        doRouting( total_num_inputs );

        //check to see if all of the input queues have data
        for( uint32_t i = 0; i < total_num_inputs; ++i) {
            if( input_queues_->at(i)->argument_ > -1 ) {
                if( input_queues_->at(i)->data_queue_->size() > 0 ) {
                    num_ready = num_ready + 1;
                }
            }
        }

        //FIXME do control PEs need to know if they're waiting on data?
        //if there are values waiting on any of the inputs, this PE could still fire
//         if( num_ready < num_inputs && num_ready > 0) {
//             pending_op_ = 1;
//         } else {
//             pending_op_ = 0;
//         }

        //if all inputs are available pull from queue and add to arg list
        if( num_inputs == 0 || num_ready < num_inputs ) {
            output_->verbose(CALL_INFO, 4, 0, "-Inputs %" PRIu32 " Ready %" PRIu32 "\n", num_inputs, num_ready);
            return false;
        } else {
            output_->verbose(CALL_INFO, 4, 0, "+Inputs %" PRIu32 " Ready %" PRIu32 "\n", num_inputs, num_ready);
            for( uint32_t i = 0; i < total_num_inputs; ++i) {
                if( input_queues_->at(i)->argument_ > -1 ) {
                    argList.push_back(input_queues_->at(i)->data_queue_->front());
                    if( op_binding_ != ROS && op_binding_ != REPEATER ) {
                        input_queues_->at(i)->data_queue_->pop();
                    }
                }
            }
        }

        switch( op_binding_ ) {
            case SEL :
            case ROS :
            case REPEATER :
                retVal = helperFunction(op_binding_, argList[0], argList[1], argList[2]);
                break;
            case ROUTE :
                retVal = LlyrData(0x00);
                break;
            case RET :
                retVal = LlyrData(0x00);
                break;
            default :
                output_->verbose(CALL_INFO, 0, 0, "Error: could not find corresponding op-%" PRIu32 ".\n", op_binding_);
                exit(-1);
        }

        output_->verbose(CALL_INFO, 32, 0, "intResult = %" PRIu64 "\n", intResult);
        output_->verbose(CALL_INFO, 32, 0, "retVal = %s\n", retVal.to_string().c_str());

        //for now push the result to all output queues that need this result
        if( op_binding_ == SEL ) {
            for( uint32_t i = 0; i < output_queues_->size(); ++i ) {
                if( *output_queues_->at(i)->routing_arg_ == "" ) {
                    output_queues_->at(i)->data_queue_->push(retVal);
                }
            }
        } else if( op_binding_ == ROS ) {
            if( do_forward_ == 1 ) {
                for( uint32_t i = 0; i < total_num_inputs; ++i) {
                    if( input_queues_->at(i)->argument_ > -1 ) {
                        input_queues_->at(i)->data_queue_->pop();
                    }
                }

                for( uint32_t i = 0; i < output_queues_->size(); ++i ) {
                    if( *output_queues_->at(i)->routing_arg_ == "" ) {
                        output_queues_->at(i)->data_queue_->push(retVal);
                    }
                }
            } else {
                input_queues_->at(0)->data_queue_->pop();
            }
        } else if( op_binding_ == REPEATER ) {
            if( do_forward_ > 0 ) {
                for( uint32_t i = 0; i < output_queues_->size(); ++i ) {
                    if( *output_queues_->at(i)->routing_arg_ == "" ) {
                        output_queues_->at(i)->data_queue_->push(retVal);
                    }
                }
            }

            if( do_forward_ == 1 ) {
                input_queues_->at(0)->data_queue_->pop();
            } else {
                for( uint32_t i = 0; i < total_num_inputs; ++i) {
                    if( input_queues_->at(i)->argument_ > -1 ) {
                        input_queues_->at(i)->data_queue_->pop();
                    }
                }
            }
        }

        if( output_->getVerboseLevel() >= 10 ) {
            printInputQueue();
            printOutputQueue();
        }

        return true;
    }

    //TODO for testing only
    virtual void inputQueueInit() {};
    virtual void outputQueueInit() {};

private:
    bool first_touch_;
    uint16_t do_forward_;

    LlyrData helperFunction( opType op, LlyrData arg0, LlyrData arg1, LlyrData arg2 )
    {

//         std::cout << "ARG[0]:" << arg0 << "::" << arg0.to_ullong() << std::endl;
//         std::cout << "ARG[1]:" << arg1 << "::" << arg1.to_ullong() << std::endl;
//         std::cout << "ARG[2]:" << arg2 << "::" << arg2.to_ullong() << std::endl;

        uint32_t select_signal = arg2.to_ullong();
        if( op == SEL ) {
            switch( select_signal ) {
                case 0 :
                    return arg0;
                    break;
                case 1 :
                    return arg1;
                    break;
                default :
                    output_->verbose( CALL_INFO, 0, 0, "Error: invalid select signal.\n" );
                    exit(-1);
            }
        } else if( op == ROS ) {
            if( arg0 == 0 ) {
                do_forward_ = 1;
                return arg1;
            } else {
                do_forward_ = 0;
                return LlyrData(0x00);
            }
        } else if( op == REPEATER ) {
            if( arg0 == 1 ) {
                do_forward_ = 1;
                return arg1;
            } else if( do_forward_ == 1 ) {
                do_forward_ = 2;
                return arg1;
            } else {
                do_forward_ = 0;
                return LlyrData(0x00);
            }
        } else {
            output_->verbose( CALL_INFO, 0, 0, "Error: could not find corresponding op-%" PRIu32 ".\n", op );
            exit(-1);
        }
    }

};

}//SST
}//Llyr

#endif // _LOGIC_PE_H
