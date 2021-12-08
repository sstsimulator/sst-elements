/*
 * // Copyright 2009-2020 NTESS. Under the terms
 * // of Contract DE-NA0003525 with NTESS, the U.S.
 * // Government retains certain rights in this software.
 * //
 * // Copyright (c) 2009-2020, NTESS
 * // All rights reserved.
 * //
 * // Portions are copyright of other developers:
 * // See the file CONTRIBUTORS.TXT in the top level directory
 * // the distribution for more information.
 * //
 * // This file is part of the SST software package. For license
 * // information, see the LICENSE file in the top level directory of the
 * // distribution.
 */

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
    LogicProcessingElement(opType op_binding, uint32_t processor_id, LlyrConfig* llyr_config,
                           uint32_t cycles = 1)  :
                    ProcessingElement(op_binding, processor_id, llyr_config)
    {
        cycles_ = cycles;
        input_queues_= new std::vector< std::queue< LlyrData >* >;
        output_queues_ = new std::vector< std::queue< LlyrData >* >;
    }

    virtual bool doSend()
    {
        uint32_t queueId;
        LlyrData sendVal;
        ProcessingElement* dstPe;

        for(auto it = output_queue_map_.begin() ; it != output_queue_map_.end(); ++it ) {
            queueId = it->first;
            dstPe = it->second;

            if( output_queues_->at(queueId)->size() > 0 ) {
                output_->verbose(CALL_INFO, 8, 0, ">> Sending...%" PRIu32 "-%" PRIu32 " to %" PRIu32 "\n",
                                processor_id_, queueId, dstPe->getProcessorId());

                sendVal = output_queues_->at(queueId)->front();
                output_queues_->at(queueId)->pop();

                dstPe->pushInputQueue(dstPe->getInputQueueId(this->getProcessorId()), sendVal);
            }
        }

        if( output_->getVerboseLevel() >= 10 ) {
            printInputQueue();
            printOutputQueue();
        }

        return true;
    }

    virtual bool doReceive(LlyrData data) { return 0; };

    virtual bool doCompute()
    {
        output_->verbose(CALL_INFO, 4, 0, ">> Compute 0x%" PRIx32 "\n", op_binding_);

        uint64_t intResult = 0x0F;

        std::vector< LlyrData > argList;
        LlyrData retVal;

        if( output_->getVerboseLevel() >= 10 ) {
            printInputQueue();
            printOutputQueue();
        }

        uint32_t num_ready = 0;
        uint32_t num_inputs  = input_queues_->size();

        //check to see if all of the input queues have data
        for( uint32_t i = 0; i < num_inputs; ++i) {
            if( input_queues_->at(i)->size() > 0 ) {
                num_ready = num_ready + 1;
            }
        }

        //if there are values waiting on any of the inputs, this PE could still fire
        if( num_ready < num_inputs && num_ready > 0) {
            pending_op_ = 1;
        } else {
            pending_op_ = 0;
        }

        //if all inputs are available pull from queue and add to arg list
        if( num_ready < num_inputs ) {
            output_->verbose(CALL_INFO, 4, 0, "-Inputs %" PRIu32 " Ready %" PRIu32 "\n", num_inputs, num_ready);
            return false;
        } else {
            output_->verbose(CALL_INFO, 4, 0, "+Inputs %" PRIu32 " Ready %" PRIu32 "\n", num_inputs, num_ready);
            for( uint32_t i = 0; i < num_inputs; ++i) {
                argList.push_back(input_queues_->at(i)->front());
                input_queues_->at(i)->pop();
            }
        }

//         std::cout << "ARG[0]:" << argList[0] << std::endl;
//         std::cout << "ARG[1]:" << argList[1] << std::endl;

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
            output_queues_->at(i)->push(retVal);
        }

        if( output_->getVerboseLevel() >= 10 ) {
            printInputQueue();
            printOutputQueue();
        }

        return true;
    }

    //TODO for testing only
    virtual void queueInit() {};

private:
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
        else if( op == UGT ) {
            if( arg0.to_ullong() > arg1.to_ullong() ) {
                return 1;
            } else {
                return 0;
            }
        }
        else if( op == UGE ) {
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
        else if( op == SGT ) {
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
        else if( op == SLT ) {
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

};

}//SST
}//Llyr

#endif // _LOGIC_PE_H
