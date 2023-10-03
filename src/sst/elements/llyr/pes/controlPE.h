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

// valid, queue id, data
typedef std::tuple< bool, int32_t, LlyrData > HelperReturn;

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
        timeout_ = 5;
    }

    virtual bool doReceive(LlyrData data) { return 0; };

    virtual bool doCompute()
    {
        // TraceFunction trace(CALL_INFO_LONG);
        output_->verbose(CALL_INFO, 4, 0, ">> Compute %s\n", getOpString(op_binding_).c_str());

        uint64_t intResult = 0x0F;

        std::vector< QueueData > argList(3);
        std::vector< bool > forwarded(input_queues_->size(), 0);
        LlyrData retVal;
        uint32_t queue_id;
        bool valid_return;

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
        bool routed = doRouting( total_num_inputs );

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
        if( num_ready < num_inputs && num_ready > 0) {
            pending_op_ = 1;
        } else {
            pending_op_ = 0 | routed;
        }

        // if all inputs are available pull from queue and add to arg list
        // exception is MERGE, which will forward the first data token to arrive
        // exception is REPEATER, which is the bane of my existance
        if( op_binding_ == MERGE && num_ready > 0 ) {
            output_->verbose(CALL_INFO, 4, 0, "+Inputs %" PRIu32 " Ready %" PRIu32 "\n", num_inputs, num_ready);
            for( uint32_t i = 0; i < total_num_inputs; ++i) {
                if( input_queues_->at(i)->argument_ > -1 ) {
                    if( input_queues_->at(i)->data_queue_->size() > 0 ) {
                        argList[i].valid_ = 1;
                        argList[i].data_  = input_queues_->at(i)->data_queue_->front();
                    } else {
                        argList[i].valid_ = 0;
                    }
                }
            }
        } else if( op_binding_ == REPEATER ) {
            for( uint32_t i = 0; i < total_num_inputs; ++i) {
                if( input_queues_->at(i)->argument_ > -1 ) {
                    if( input_queues_->at(i)->data_queue_->size() > 0 ) {
                        argList[i].valid_ = 1;
                        argList[i].data_  = input_queues_->at(i)->data_queue_->front();

                        forwarded[i] = input_queues_->at(i)->forwarded_;
                        input_queues_->at(i)->forwarded_ = 0;
                        input_queues_->at(i)->data_queue_->pop();

                    } else {
                        argList[i].valid_ = 0;
                    }
                }
            }

            if( argList[0].valid_ != 1 ) {
                output_->verbose(CALL_INFO, 4, 0, "-Inputs %" PRIu32 " Ready %" PRIu32 "\n", num_inputs, num_ready);
                for( uint32_t i = 0; i < argList.size(); ++i ) {
                    if( argList[i].valid_ ==  1 ) {
                        input_queues_->at(i)->forwarded_ = forwarded[i];
                        input_queues_->at(i)->data_queue_->push(argList[i].data_);
                    }
                }
                return false;
            } else {
                output_->verbose(CALL_INFO, 4, 0, "+Inputs %" PRIu32 " Ready %" PRIu32 "\n", num_inputs, num_ready);
            }

        } else if( num_inputs == 0 || num_ready < num_inputs ) {
            output_->verbose(CALL_INFO, 4, 0, "-Inputs %" PRIu32 " Ready %" PRIu32 "\n", num_inputs, num_ready);
            return false;
        } else {
            output_->verbose(CALL_INFO, 4, 0, "+Inputs %" PRIu32 " Ready %" PRIu32 "\n", num_inputs, num_ready);
            for( uint32_t i = 0; i < total_num_inputs; ++i) {
                if( input_queues_->at(i)->argument_ > -1 ) {
                    if( input_queues_->at(i)->data_queue_->size() > 0 ) {
                        argList[i].valid_ = 1;
                        argList[i].data_  = input_queues_->at(i)->data_queue_->front();

                        forwarded[i] = input_queues_->at(i)->forwarded_;
                        input_queues_->at(i)->forwarded_ = 0;
                        input_queues_->at(i)->data_queue_->pop();
                    } else {
                        argList[i].valid_ = 0;
                    }
                }
            }
        }

        // If data tokens in output queue then simulation cannot end
        pending_op_ = 1;

        // variables set in helper function
        queue_id = 0;
        valid_return = 0;
        HelperReturn tempReturn;

        switch( op_binding_ ) {
            case SEL :
            case ROZ :
            case ROO :
            case ONEONAND :
            case GATED_ONE :
            case MERGE :
            case REPEATER :
                tempReturn = helperFunction(op_binding_, argList[0], argList[1], argList[2]);
                retVal = std::get<2>(tempReturn);
                queue_id = std::get<1>(tempReturn);
                valid_return = std::get<0>(tempReturn);
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

        // for now push the result to all output queues that need this result
        if( op_binding_ == MERGE ) {
            input_queues_->at(queue_id)->forwarded_ = forwarded[queue_id];
            input_queues_->at(queue_id)->data_queue_->pop();

            for( uint32_t i = 0; i < output_queues_->size(); ++i ) {
                if( *output_queues_->at(i)->routing_arg_ == "" ) {
                    output_queues_->at(i)->data_queue_->push(retVal);
                }
            }
        } else if( op_binding_ == REPEATER ) {
            if( valid_return == 1 ) {
                for( uint32_t i = 0; i < output_queues_->size(); ++i ) {
                    if( *output_queues_->at(i)->routing_arg_ == "" ) {
                        output_queues_->at(i)->data_queue_->push(retVal);
                    }
                }

                // need to keep the arg-1 value if it wasn't reset, using queueId of 2 for this
                if( argList[0].valid_ == 1 && argList[0].data_ == 0 && queue_id == 2 ) {
                    if( argList[1].valid_ == 1 ) {
                        input_queues_->at(1)->forwarded_ = forwarded[1];
                        input_queues_->at(1)->data_queue_->push(argList[1].data_);
                    }
                }
            } else if( queue_id == 2 ) {
                for( uint32_t i = 0; i < argList.size(); ++i ) {
                    if( argList[i].valid_ ==  1 ) {
                        input_queues_->at(i)->forwarded_ = forwarded[1];
                        input_queues_->at(i)->data_queue_->push(argList[i].data_);
                    }
                }
            }
        } else if( op_binding_ == GATED_ONE ) {
            // need to keep the arg-0 value if the data stream didn't gnom it up
            if( argList[1].valid_ == 1 && argList[1].data_ == 0 ) {
                if( argList[0].valid_ == 1 ) {
                    input_queues_->at(0)->forwarded_ = forwarded[0];
                    input_queues_->at(0)->data_queue_->push(argList[0].data_);
                }
            }

           if( valid_return == 1 ) {
                for( uint32_t i = 0; i < output_queues_->size(); ++i ) {
                    if( *output_queues_->at(i)->routing_arg_ == "" ) {
                        output_queues_->at(i)->data_queue_->push(retVal);
                    }
                }
            }
        } else {
            if( valid_return == 1 ) {
                for( uint32_t i = 0; i < output_queues_->size(); ++i ) {
                    if( *output_queues_->at(i)->routing_arg_ == "" ) {
                        output_queues_->at(i)->data_queue_->push(retVal);
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

protected:
    uint16_t do_forward_;

    HelperReturn helperFunction( opType op, QueueData arg0, QueueData arg1, QueueData arg2 )
    {

        std::cout << "ARG[0]:" << arg0.valid_ << "::" << arg0.data_ << "-> " << arg0.data_.to_ullong() << std::endl;
        std::cout << "ARG[1]:" << arg1.valid_ << "::" << arg1.data_ << "-> " << arg1.data_.to_ullong() << std::endl;
        std::cout << "ARG[2]:" << arg2.valid_ << "::" << arg2.data_ << "-> " << arg2.data_.to_ullong() << std::endl;

        // SEL - Select: If control (arg2) is 0, send arg0, else send arg1
        // ROS - Route on Signal: Push stored token out when input-1 is high
        // ROZ - Route on 0: Forward data if control (arg0) == 0
        // ROO - Route on 1: Forward data if control (arg0) == 1
        // ONEONAND - Test & Set: Emit a '1' if both inputs are 1, else drop the inputs and emit nothing TODO move to logic
        // MERGE - Choose One: Forward first token to arrive
        // REPEATER - Repeater: If ctrl = 0, fwd buffer; if buffer empty, fill buffer and fwd; if ctrl = 1, fill buffer
        // FILTER - Filter: Filter based on value (e.g. if filter 0s, forward all but 0s)
        if( op == SEL ) {
            if( arg2.valid_ == 1 ) {
                switch( arg2.data_.to_ullong() ) {
                    case 0 :
                        return std::make_tuple(1, 0, arg0.data_);
                        break;
                    case 1 :
                        return std::make_tuple(1, 1, arg1.data_);;
                        break;
                    default :
                        output_->verbose( CALL_INFO, 0, 0, "Error: invalid select signal.\n" );
                        exit(-1);
                }
            } else {
                output_->verbose( CALL_INFO, 0, 0, "Error: invalid select signal.\n" );
                exit(-1);
            }
        } else if( op == ROS ) {
            if( arg1.valid_ == 1 ) {
                do_forward_ = 1;
                return std::make_tuple(1, 0, arg0.data_);
            } else {
                do_forward_ = 0;
                return std::make_tuple(0, 0, LlyrData(0x00));
            }
        } else if( op == RNE ) {
            if( arg1.valid_ == 1 ) {
                if( arg0.data_ == arg1.data_ ) {
                    return std::make_tuple(0, 0, LlyrData(0x00));
                } else {
                    return std::make_tuple(1, 1, LlyrData(arg1.data_));
                }
            } else {
                output_->verbose( CALL_INFO, 0, 0, "Error: invalid select signal.\n" );
                exit(-1);
            }
        } else if( op == ROZ ) {
            if( arg0.valid_ == 1 && arg0.data_[0] == 0 ) {
                return std::make_tuple(1, 1, arg1.data_);
            } else if( arg0.valid_ == 1 && arg0.data_[0] == 1 ) {
                return std::make_tuple(0, 0, LlyrData(0xFF));
            } else {
                output_->verbose( CALL_INFO, 0, 0, "Error: invalid select signal.\n" );
                exit(-1);
            }
        } else if( op == ROO ) {
            if( arg0.valid_ == 1 && arg0.data_[0] == 1 ) {
                return std::make_tuple(1, 1, arg1.data_);
            } else if( arg0.valid_ == 1 && arg0.data_[0] == 0 ) {
                return std::make_tuple(0, 0, LlyrData(0xFF));
            } else {
                output_->verbose( CALL_INFO, 0, 0, "Error: invalid select signal.\n" );
                exit(-1);
            }
        } else if( op == ONEONAND ) {
            if( arg0.valid_ == 1 && arg1.valid_ == 1 ) {
                if( arg0.data_[0] && arg1.data_[0] ) {
                    return std::make_tuple(1, 0, LlyrData(0x01));
                } else {
                    return std::make_tuple(0, 0, LlyrData(0xFF));
                }
            } else {
                output_->verbose( CALL_INFO, 0, 0, "Error: invalid data signals.\n" );
                exit(-1);
            }
        } else if( op == GATED_ONE ) {
            if( arg0.valid_ == 1 && arg1.valid_ == 1 ) {
                if( arg0.data_[0] == 0 && arg1.data_[0] == 0 ) {
                    return std::make_tuple(1, 0, LlyrData(0x00));
                } else if( arg0.data_[0] == 0 && arg1.data_[0] == 1 ) {
                    return std::make_tuple(0, 0, LlyrData(0xFF));
                } else if( arg0.data_[0] == 1 && arg1.data_[0] == 0 ) {
                    return std::make_tuple(1, 0, LlyrData(0x00));
                } else if( arg0.data_[0] == 1 && arg1.data_[0] == 1 ) {
                    return std::make_tuple(1, 0, LlyrData(0x01));
                } else {
                    return std::make_tuple(0, 0, LlyrData(0xFF));
                }
            } else {
                output_->verbose( CALL_INFO, 0, 0, "Error: invalid data signals.\n" );
                exit(-1);
            }
        } else if( op == MERGE ) {
            if( arg0.valid_ == 1 ) {
                return std::make_tuple(1, 0, arg0.data_);
            } else if( arg1.valid_ == 1 ) {
                return std::make_tuple(1, 1, arg1.data_);
            } else {
                return std::make_tuple(0, 0, LlyrData(0xFF));
            }
        }  else if( op == FILTER ) {
            if( arg0.data_ == 0 && arg1.data_ == 0 ) {
                do_forward_ = 0;
                return std::make_tuple(0, 0, LlyrData(0xFF));
            } else if( arg0.data_ == 1 && arg1.data_ == 1 ) {
                do_forward_ = 0;
                return std::make_tuple(0, 0, LlyrData(0x00));
            } else {
                return std::make_tuple(1, 1, LlyrData(arg1.data_));
            }
        } else if( op == REPEATER ) {
            if( arg0.valid_ == 1 && arg0.data_[0] == 0 ) {
                if( control_buffer_.size() > 0 ) {
                    output_->verbose( CALL_INFO, 0, 0, "RPTR AAAAAA\n" );
                    return std::make_tuple(1, 2, control_buffer_.front());
                } else if( arg1.valid_ == 1 ) {
                    output_->verbose( CALL_INFO, 0, 0, "RPTR XXXXXXX\n" );
                    control_buffer_.push(arg1.data_);
                    return std::make_tuple(1, 1, control_buffer_.front());
                } else {
                    return std::make_tuple(0, 2, 0xFF);
                }
            } else if( arg0.valid_ == 1 && arg0.data_[0] == 1 ) {
                if( arg1.valid_ == 1 ) {
                    output_->verbose( CALL_INFO, 0, 0, "RPTR NNNNNNN\n" );
                    control_buffer_.pop();
                    control_buffer_.push(arg1.data_);
                    return std::make_tuple(0, 0, 0xFF);
                } else {
                    output_->verbose( CALL_INFO, 0, 0, "RPTR QQQQQQQQ\n" );
                    return std::make_tuple(0, 2, 0xFF);
                }
            } else {
                output_->verbose( CALL_INFO, 0, 0, "Error-2: invalid data signals.\n" );
                exit(-1);
            }
        } else {
            output_->verbose( CALL_INFO, 0, 0, "Error: could not find corresponding op-%" PRIu32 ".\n", op );
            exit(-1);
        }
    }

private:
    std::queue< LlyrData > control_buffer_;
};

class ControlConstProcessingElement : public ControlProcessingElement
{
public:
    ControlConstProcessingElement(opType op_binding, uint32_t processor_id, LlyrConfig* llyr_config,
                              QueueArgMap* arguments)  :
    ControlProcessingElement(op_binding, processor_id, llyr_config)
    {
        first_touch_ = 1;

        // iterate through the arguments and set initial queue values
        for( auto it = arguments->begin(); it != arguments->end(); ++it ) {
            auto retVal = input_queues_init_.emplace( it->first, it->second );
            if( retVal.second == false ) {
                ///TODO
            }
        }
    }

    virtual bool doCompute()
    {
        // TraceFunction trace(CALL_INFO_LONG);
        output_->verbose(CALL_INFO, 4, 0, ">> Compute %s\n", getOpString(op_binding_).c_str());

        uint64_t intResult = 0x0F;

        std::vector< QueueData > argList(3);
        LlyrData retVal;

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
        bool routed = doRouting( total_num_inputs );

        //check to see if all of the input queues have data
        for( uint32_t i = 0; i < total_num_inputs; ++i) {
            if( input_queues_->at(i)->argument_ > -1 ) {
                if( input_queues_->at(i)->data_queue_->size() > 0 ) {
                    num_ready = num_ready + 1;
                }
            }
        }

        pending_op_ = 0 | routed;
        //if there are values waiting on any of the inputs (queue-0 is a const), this PE could still fire
        for( uint32_t i = 1; i < total_num_inputs; ++i ) {
            if( input_queues_->at(i)->data_queue_->size() > 0 ) {
                pending_op_ = 1;
            } else {
                pending_op_ = 0 | routed;
            }
        }

        // if all inputs are available pull from queue and add to arg list
        if( num_inputs == 0 || num_ready < num_inputs ) {
            output_->verbose(CALL_INFO, 4, 0, "-Inputs %" PRIu32 " Ready %" PRIu32 "\n", num_inputs, num_ready);
            return false;
        } else {
            output_->verbose(CALL_INFO, 4, 0, "+Inputs %" PRIu32 " Ready %" PRIu32 "\n", num_inputs, num_ready);
            for( uint32_t i = 0; i < total_num_inputs; ++i) {
                if( input_queues_->at(i)->argument_ > -1 ) {
                    if( input_queues_->at(i)->data_queue_->size() > 0 ) {
                        argList[i].valid_ = 1;
                        argList[i].data_  = input_queues_->at(i)->data_queue_->front();

                        if( op_binding_ != ROS ) {
                            input_queues_->at(i)->forwarded_ = 0;
                            input_queues_->at(i)->data_queue_->pop();
                        }
                    } else {
                        argList[i].valid_ = 0;
                    }
                }
            }
        }

        // If data tokens in output queue then simulation cannot end
        pending_op_ = 1;

        // variables set in helper function
        bool valid_return = 0;
        HelperReturn tempReturn;

        switch( op_binding_ ) {
            case ROS :
            case RNE :
            case FILTER :
                tempReturn = helperFunction(op_binding_, argList[0], argList[1], argList[2]);
                retVal = std::get<2>(tempReturn);
                valid_return = std::get<0>(tempReturn);
                break;
            default :
                output_->verbose(CALL_INFO, 0, 0, "Error: could not find corresponding op-%" PRIu32 ".\n", op_binding_);
                exit(-1);
        }

        output_->verbose(CALL_INFO, 32, 0, "intResult = %" PRIu64 "\n", intResult);
        output_->verbose(CALL_INFO, 32, 0, "retVal = %s\n", retVal.to_string().c_str());

        //for now push the result to all output queues that need this result
        if( op_binding_ == ROS ) {
            if( do_forward_ == 1 ) {
                input_queues_->at(1)->data_queue_->pop();

                for( uint32_t i = 0; i < output_queues_->size(); ++i ) {
                    if( *output_queues_->at(i)->routing_arg_ == "" ) {
                        output_queues_->at(i)->data_queue_->push(retVal);
                    }
                }
            }
        } else if( op_binding_ == FILTER ) {
            // this is so hacky -- need to preserve the const
            input_queues_->at(0)->data_queue_->push(argList[0].data_);
            if( valid_return == 1 ) {
                for( uint32_t i = 0; i < output_queues_->size(); ++i ) {
                    if( *output_queues_->at(i)->routing_arg_ == "" ) {
                        output_queues_->at(i)->data_queue_->push(retVal);
                    }
                }
            }
        } else if( op_binding_ == RNE ) {
            if( argList[0].valid_ == 1 ) {
                input_queues_->at(0)->data_queue_->push(argList[0].data_);
            }

            if( valid_return == 1 ) {
                for( uint32_t i = 0; i < output_queues_->size(); ++i ) {
                    if( *output_queues_->at(i)->routing_arg_ == "" ) {
                        output_queues_->at(i)->data_queue_->push(retVal);
                    }
                }
            }
        } else {
            if( valid_return == 1 ) {
                for( uint32_t i = 0; i < output_queues_->size(); ++i ) {
                    if( *output_queues_->at(i)->routing_arg_ == "" ) {
                        output_queues_->at(i)->data_queue_->push(retVal);
                    }
                }
            }
        }

        if( output_->getVerboseLevel() >= 10 ) {
            printInputQueue();
            printOutputQueue();
        }

        return true;
    }// doCompute

    virtual void inputQueueInit()
    {
        output_->verbose(CALL_INFO, 4, 0, ">> Fake Init Input Queue(%" PRIu32 "), Op %" PRIu32 " \n",
                        processor_id_, op_binding_ );

        while( input_queues_->size() < input_queues_init_.size() ) {
            LlyrQueue* tempQueue = new LlyrQueue;
            tempQueue->forwarded_ = 0;
            tempQueue->argument_ = 0;
            tempQueue->routing_arg_ = new std::string("");
            tempQueue->data_queue_ = new std::queue< LlyrData >;
            input_queues_->push_back(tempQueue);
        }

        //TODO Need a more elegant way to initialize these queues
        uint32_t queue_id = 0;
        if( input_queues_init_.size() > 0 ) {
            for( auto it = input_queues_init_.begin(); it != input_queues_init_.end(); ++it ) {
                if( it->first == queue_id ) {
                    int64_t init_value = std::stoll(it->second);
                    LlyrData temp = LlyrData(init_value);
                    input_queues_->at(queue_id)->data_queue_->push(temp);
                }
                queue_id = queue_id + 1;
            }
        }
    }

private:
    bool first_touch_;
    std::map< uint32_t, Arg > input_queues_init_;

};// ControlConstProcessingElement

}//SST
}//Llyr

#endif // _LOGIC_PE_H

