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

#ifndef _INT_PE_H
#define _INT_PE_H

#include "pes/processingElement.h"

namespace SST {
namespace Llyr {

/**
 * @todo write docs
 */
class IntProcessingElement : public ProcessingElement
{
public:
    IntProcessingElement(opType op_binding, uint32_t processor_id, LlyrConfig* llyr_config) :
                         ProcessingElement(op_binding, processor_id, llyr_config)
    {
        if( op_binding == DIV ) {
            latency_ = llyr_config->int_div_latency_;
        } else {
            latency_ = llyr_config->int_latency_;
        }
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
            case ADD :
                intResult = argList[0].to_ullong() + argList[1].to_ullong();
                break;
            case SUB :
                intResult = argList[0].to_ullong() - argList[1].to_ullong();
                break;
            case MUL :
                intResult = argList[0].to_ullong() * argList[1].to_ullong();
                break;
            case DIV :
                intResult = argList[0].to_ullong() / argList[1].to_ullong();
                break;
            case REM :
                intResult = argList[0].to_ullong() % argList[1].to_ullong();
                break;
            default :
                output_->verbose(CALL_INFO, 0, 0, "Error: could not find corresponding op-%" PRIu32 ".\n", op_binding_);
                exit(-1);
        }

        retVal = LlyrData(intResult);

        output_->verbose(CALL_INFO, 32, 0, "intResult = %" PRIu64 "\n", intResult);
        output_->verbose(CALL_INFO, 32, 0, "retVal = %s\n", retVal.to_string().c_str());

        //for now push the result to all output queues that need this result -- assume if no route, then receives data
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

        output_->verbose(CALL_INFO, 4, 0, ">> Compute 0x%" PRIx32 " - COMPLETE\n", op_binding_);
        return true;
    }

    //TODO for testing only
    virtual void inputQueueInit() {};
    virtual void outputQueueInit() {};

};// IntProcessingElement

class IntConstProcessingElement : public IntProcessingElement
{
public:
    IntConstProcessingElement(opType op_binding, uint32_t processor_id, LlyrConfig* llyr_config,
                              QueueArgMap* arguments)  :
                              IntProcessingElement(op_binding, processor_id, llyr_config)
    {
        if( op_binding == DIVCONST ) {
            latency_ = llyr_config->int_div_latency_;
        } else {
            latency_ = llyr_config->int_latency_;
        }
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
        for( uint32_t i = 0; i < total_num_inputs; ++i) {
            if( input_queues_->at(i)->argument_ > -1 ) {
                num_inputs = num_inputs + 1;
            }
        }

        // FIXME check to see of there are any routing jobs -- should be able to do this without waiting to fire
        bool routed = doRouting( total_num_inputs );

        //check to see if all of the input queues have data -- this no longer assumes contiguous input args
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

        std::cout << "++++++ Input Queue Size: " << input_queues_->at(0)->data_queue_->size();
        std::cout << ", Num Inputs: " << num_inputs;
        std::cout << ", Num Ready: " << num_ready << std::endl;

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
            for( uint32_t i = 0; i < total_num_inputs; ++i ) {
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

        // first queue should be const, so save for later
        input_queues_->at(0)->data_queue_->push(LlyrData(argList[0].to_ullong()));

        switch( op_binding_ ) {
            case ADDCONST :
                intResult = argList[0].to_ullong() + argList[1].to_ullong();
                break;
            case SUBCONST :
                intResult = argList[0].to_ullong() - argList[1].to_ullong();
                break;
            case MULCONST :
                intResult = argList[0].to_ullong() * argList[1].to_ullong();
                break;
            case DIVCONST :
                intResult = argList[0].to_ullong() / argList[1].to_ullong();
                break;
            case REMCONST :
                intResult = argList[0].to_ullong() % argList[1].to_ullong();
                break;
            default :
                output_->verbose(CALL_INFO, 0, 0, "Error: could not find corresponding op-%" PRIu32 ".\n", op_binding_);
                exit(-1);
        }

        retVal = LlyrData(intResult);

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
            tempQueue->argument_ = 0;  //TODO all args are consts right now, should change to -1
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

};// IntConstProcessingElement

class AdvIntProcessingElement : public IntProcessingElement
{
public:
    AdvIntProcessingElement(opType op_binding, uint32_t processor_id, LlyrConfig* llyr_config,
                            QueueArgMap* arguments)  :
                            IntProcessingElement(op_binding, processor_id, llyr_config)
    {
        latency_ = llyr_config->int_latency_;
        cycles_to_fire_ = latency_;

        // iterate through the arguments and set initial queue values
        for( auto it = arguments->begin(); it != arguments->end(); ++it ) {
            std::cout << "[AdvIntProcessingElement]";
            std::cout << "input_queues_init_ -- ";
            std::cout << " queue: " << it->first;
            std::cout << " arg: "   << it->second;
            std::cout << std::endl;

            auto retVal = input_queues_init_.emplace( it->first, it->second );
            if( retVal.second == false ) {
                ///TODO
            }
        }

        triggered_ = 0;
        initialized_ = 0;
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
        for( uint32_t i = 0; i < total_num_inputs; ++i) {
            if( input_queues_->at(i)->argument_ > -1 ) {
                num_inputs = num_inputs + 1;
            }
        }

        // buffer the initial values for restart
        if( num_inputs > 2 && initialized_ == 0 ) {
            initialized_ = 1;
            init0_ = input_queues_->at(0)->data_queue_->front();
            init1_ = input_queues_->at(1)->data_queue_->front();
        }

        // and on the sync reset for inc
        if( op_binding_ == INC_RST && total_num_inputs > 2 ) {
            std::cout << "MMMOFODSOFSDOFDSDS" << std::endl;
            input_queues_->at(2)->argument_ = -1;
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

        // // if there is an extra non-routed input queue, this is a triggered PE
        // if( num_inputs == 3 && input_queues_->at(2)->data_queue_->size() > 0 ) {
        //     triggered_ = 1;
        //     input_queues_->at(2)->data_queue_->pop();
        //
        //     // reset if necessary
        //     if( initialized_ == 1 ) {
        //         initialized_ = 2;
        //     } else {
        //         input_queues_->at(0)->data_queue_->push(init0_);
        //         input_queues_->at(1)->data_queue_->push(init1_);
        //     }
        // }
        // std::cout << std::flush;
        //
        // // tricksy to force event
        // if( triggered_ == 1 ) {
        //     num_inputs = num_inputs - 1;
        // }

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

        // there is no way to purge the queue so assume that compute is done
        std::cout << "++++++ Input Queue Size: " << input_queues_->at(0)->data_queue_->size();
        std::cout << ", Num Inputs: " << num_inputs;
        std::cout << ", Num Ready: " << num_ready;
        std::cout << ", Triggered: " << triggered_;
        std::cout << ", Init: " << initialized_ << std::endl;

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

        if( op_binding_ == INC ) {
            if( argList[0].to_ullong() <= argList[1].to_ullong() ) {
                intResult = argList[0].to_ullong();
                input_queues_->at(0)->data_queue_->push(LlyrData(intResult + 1));
                input_queues_->at(1)->data_queue_->push(LlyrData(argList[1].to_ullong()));

                retVal = LlyrData(intResult);

                output_->verbose(CALL_INFO, 32, 0, "intResult = %" PRIu64 "\n", intResult);
                output_->verbose(CALL_INFO, 32, 0, "retVal = %s\n", retVal.to_string().c_str());

                // for now push the result to all output queues that need this result
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
            } else if( triggered_ == 1 ){
                triggered_ = 0;

                retVal = LlyrData(intResult);

                output_->verbose(CALL_INFO, 32, 0, "intResult = %" PRIu64 "\n", intResult);
                output_->verbose(CALL_INFO, 32, 0, "retVal = %s\n", retVal.to_string().c_str());

                if( output_->getVerboseLevel() >= 10 ) {
                    output_->verbose(CALL_INFO, 10, 0, "Queue Contents (1)\n");
                    printInputQueue();
                    printOutputQueue();
                }
            }
        } else if( op_binding_ == INC_RST ) {
            if( argList[0].to_ullong() <= argList[1].to_ullong() ) {
                intResult = argList[0].to_ullong();
                input_queues_->at(0)->data_queue_->push(LlyrData(intResult + 1));
                input_queues_->at(1)->data_queue_->push(LlyrData(argList[1].to_ullong()));

                retVal = LlyrData(intResult);

                output_->verbose(CALL_INFO, 32, 0, "intResult = %" PRIu64 "\n", intResult);
                output_->verbose(CALL_INFO, 32, 0, "retVal = %s\n", retVal.to_string().c_str());

                // for now push the result to all output queues that need this result
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
            }

            std::cout << "total_num_inputs=" << total_num_inputs;
            if( total_num_inputs > 2 )
                std::cout << "   queue_size=" << input_queues_->at(2)->data_queue_->size();
            std::cout << std::endl;
            if( total_num_inputs == 3 && input_queues_->at(2)->data_queue_->size() > 0 ) {
                std::cout << "RESET ME PLEASE!!" << std::endl;
                if( argList[0].to_ullong() > argList[1].to_ullong() ) {
                    std::cout << "RESET NOW!!!!!" << std::endl;
                    input_queues_->at(0)->data_queue_->push(LlyrData(init0_));
                    input_queues_->at(1)->data_queue_->push(LlyrData(init1_));
                    input_queues_->at(2)->data_queue_->pop();
                }
            }

        } else if( op_binding_ == ACC ) {
            // need to save the next accumulator value
            LlyrData temp = input_queues_->at(0)->data_queue_->front();
            input_queues_->at(0)->data_queue_->pop();
std::cout << "XXX " << temp.to_ullong() << " + " << argList[0].to_ullong() <<std::endl;
            intResult = temp.to_ullong() + argList[0].to_ullong();
            input_queues_->at(0)->data_queue_->push(LlyrData(intResult));

            retVal = LlyrData(intResult);

            output_->verbose(CALL_INFO, 32, 0, "intResult = %" PRIu64 "\n", intResult);
            output_->verbose(CALL_INFO, 32, 0, "retVal = %s\n", retVal.to_string().c_str());

            // for now push the result to all output queues that need this result
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
        } else {
            output_->verbose(CALL_INFO, 0, 0, "Error: could not find corresponding op-%" PRIu32 ".\n", op_binding_);
            exit(-1);
        }

        return true;
    }

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

        // this is hacky but need to ignore queue-0 on the accumulator
        if( op_binding_ == ACC ) {
            input_queues_->at(0)->argument_ = -1;
        }
    }

private:
    uint16_t initialized_;
    uint16_t triggered_;     // 0-inactive, 1-active, 2-resetting

    LlyrData init0_;
    LlyrData init1_;

    std::map< uint32_t, Arg > input_queues_init_;

};// AdvIntProcessingElement

}//SST
}//Llyr

#endif // _INT_PE_H_H
