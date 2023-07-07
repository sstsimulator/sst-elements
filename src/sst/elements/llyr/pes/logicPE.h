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

#ifndef _LOGIC_PE_H
#define _LOGIC_PE_H

#include "pes/processingElement.h"

namespace SST {
namespace Llyr {

/**
 * @todo write docs
 */
class LogicProcessingElement : public ProcessingElement
{
public:
    LogicProcessingElement(opType op_binding, uint32_t processor_id, LlyrConfig* llyr_config) :
                    ProcessingElement(op_binding, processor_id, llyr_config)
    {
        latency_ = llyr_config->arith_latency_;
        cycles_to_fire_ = latency_;
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
        doRouting( total_num_inputs );

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
            pending_op_ = 0;
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

        switch( op_binding_ ) {
            case AND :
                retVal = argList[0];
                retVal &= argList[1];
                break;
            case OR :
                retVal = argList[0];
                retVal |= argList[1];
                break;
            case XOR :
                retVal = argList[0];
                retVal ^= argList[1];
                break;
            case NOT :
                retVal = ~argList[0];
                break;
            case SLL :
                retVal = argList[0] << argList[1].to_ullong();
                break;
            case SLR :
                retVal = argList[0] >> argList[1].to_ullong();
                break;
            case ROL :
                retVal = (argList[0] << argList[1].to_ullong()) | (argList[0] >> (Bit_Length - argList[1].to_ullong()));
                break;
            case ROR :
                retVal = (argList[0] >> argList[1].to_ullong()) | (argList[0] << (Bit_Length - argList[1].to_ullong()));
                break;
            case EQ :
            case NE :
            case UGT :
            case UGE :
            case SGT :
            case SGE :
            case ULT :
            case ULE :
            case SLT :
            case SLE :
                retVal = helperFunction(op_binding_, argList[0], argList[1]);
                break;
            default :
                output_->verbose(CALL_INFO, 0, 0, "Error: could not find corresponding op-%" PRIu32 ".\n", op_binding_);
                exit(-1);
        }

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

protected:
    LlyrData helperFunction( opType op, LlyrData arg0, LlyrData arg1 )
    {
        //TODO might need this for data length conversion?
//         std::bitset<8> x("10000010");
//         std::bitset<64> y;
//
//         //if msb == 1, then it's negative and must sign extend
//         y = y | std::bitset<64>(x.to_ullong());
//         if( x.test(x.size() - 1) == 1 ) {
//             y |= 0xFFFFFFFFFFFFFF00;
//         }
//
//         std::cout << "x:" << x << "::" << x.size() << "\n";
//         std::cout << "y:" << y << "::" << y.size() << "\n";
//
//         int64_t boo = (int64_t)(y.to_ulong());
//         std::bitset<64> bitTestL  = boo;

//         std::cout << "ARG[0]:" << arg0 << "::" << arg0.to_ullong() << std::endl;
//         std::cout << "ARG[1]:" << arg1 << "::" << arg1.to_ullong() << std::endl;

        if( op == EQ ) {
            if( arg0.to_ullong() == arg1.to_ullong() ) {
                return 1;
            } else {
                return 0;
            }
        }
        else if( op == NE ) {
            if( arg0.to_ullong() != arg1.to_ullong() ) {
                return 1;
            } else {
                return 0;
            }
        }
        else if( op == UGT || op == UGT_IMM ) {
            if( arg0.to_ullong() > arg1.to_ullong() ) {
                return 1;
            } else {
                return 0;
            }
        }
        else if( op == UGE || op == UGE_IMM ) {
            if( arg0.to_ullong() >= arg1.to_ullong() ) {
                return 1;
            } else {
                return 0;
            }
        }
        else if( op == ULT ) {
            if( arg0.to_ullong() < arg1.to_ullong() ) {
                return 1;
            } else {
                return 0;
            }
        }
        else if( op == ULE ) {
            if( arg0.to_ullong() <= arg1.to_ullong() ) {
                return 1;
            } else {
                return 0;
            }
        }
        else if( op == SGT || op == SGT_IMM ) {
            int64_t arg0s = (int64_t)(arg0.to_ullong());
            int64_t arg1s = (int64_t)(arg1.to_ullong());

            if( arg0s > arg1s ) {
                return 1;
            } else {
                return 0;
            }
        }
        else if( op == SGE ) {
            int64_t arg0s = (int64_t)(arg0.to_ullong());
            int64_t arg1s = (int64_t)(arg1.to_ullong());

            if( arg0s >= arg1s ) {
                return 1;
            } else {
                return 0;
            }
        }
        else if( op == SLT || op == SLT_IMM ) {
            int64_t arg0s = (int64_t)(arg0.to_ullong());
            int64_t arg1s = (int64_t)(arg1.to_ullong());

            if( arg0s < arg1s ) {
                return 1;
            } else {
                return 0;
            }
        }
        else if( op == SLE ) {
            int64_t arg0s = (int64_t)(arg0.to_ullong());
            int64_t arg1s = (int64_t)(arg1.to_ullong());

            if( arg0s <= arg1s ) {
                return 1;
            } else {
                return 0;
            }
        }
        else {
            output_->verbose(CALL_INFO, 0, 0, "Error: could not find corresponding op-%" PRIu32 ".\n", op_binding_);
            exit(-1);
        }
    }

}; //LogicProcessingElement

class LogicConstProcessingElement : public LogicProcessingElement
{
public:
    LogicConstProcessingElement(opType op_binding, uint32_t processor_id, LlyrConfig* llyr_config,
                              QueueArgMap* arguments)  :
    LogicProcessingElement(op_binding, processor_id, llyr_config)
    {
        latency_ = llyr_config->arith_latency_;
        cycles_to_fire_ = latency_;

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
        for( uint32_t i = 0; i < total_num_inputs; ++i ) {
            if( input_queues_->at(i)->argument_ > -1 ) {
                num_inputs = num_inputs + 1;
            }
        }

        // FIXME check to see of there are any routing jobs -- should be able to do this without waiting to fire
        doRouting( total_num_inputs );

        //check to see if all of the input queues have data -- this no longer assumes contiguous input args
        for( uint32_t i = 0; i < total_num_inputs; ++i) {
            if( input_queues_->at(i)->argument_ > -1 ) {
                if( input_queues_->at(i)->data_queue_->size() > 0 ) {
                    num_ready = num_ready + 1;
                }
            }
        }

        //if there are values waiting on any of the inputs (queue-0 is a const), this PE could still fire
        if( input_queues_->at(1)->data_queue_->size() > 0 ) {
            pending_op_ = 1;
        } else {
            pending_op_ = 0;
        }

        std::cout << "++++++ Input Queue Size: " << input_queues_->at(0)->data_queue_->size();
        std::cout << ", Num Inputs: " << num_inputs;
        std::cout << ", Num Ready: " << num_ready << std::endl;

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
            // first queue should be const
            for( uint32_t i = 1; i < total_num_inputs; ++i) {
                if( input_queues_->at(i)->argument_ > -1 ) {
                    argList.push_back(input_queues_->at(i)->data_queue_->front());
                    input_queues_->at(i)->data_queue_->pop();
                }
            }
            cycles_to_fire_ = latency_;
        }

        switch( op_binding_ ) {
            case AND_IMM:
                retVal = argList[0];
                retVal &= argList[1];
                break;
            case OR_IMM :
                retVal = argList[0];
                retVal |= argList[1];
                break;
            case UGT_IMM :
            case UGE_IMM :
            case SGT_IMM :
            case SLT_IMM :
                retVal = helperFunction(op_binding_, argList[0], argList[1]);
                break;
            default :
                output_->verbose(CALL_INFO, 0, 0, "Error: could not find corresponding op-%" PRIu32 ".\n", op_binding_);
                exit(-1);
        }

        output_->verbose(CALL_INFO, 32, 0, "intResult = %" PRIu64 "\n", intResult);
        output_->verbose(CALL_INFO, 32, 0, "retVal = %s\n", retVal.to_string().c_str());

        for( uint32_t i = 0; i < output_queues_->size(); ++i ) {
            if( *output_queues_->at(i)->routing_arg_ == "" ) {
                output_queues_->at(i)->data_queue_->push(retVal);
            }
        }

        if( output_->getVerboseLevel() >= 10 ) {
            output_->verbose(CALL_INFO, 10, 0, "Queue Contents (1)\n");
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
    std::map< uint32_t, Arg > input_queues_init_;

};// LogicConstProcessingElement

}//SST
}//Llyr

#endif // _LOGIC_PE_H
