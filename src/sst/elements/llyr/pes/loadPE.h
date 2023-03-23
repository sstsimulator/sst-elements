/*
 * // Copyright 2013-2022 NTESS. Under the terms
 * // of Contract DE-NA0003525 with NTESS, the U.S.
 * // Government retains certain rights in this software.
 * //
 * // Copyright (c) 2013-2022, NTESS
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

#ifndef _LOAD_PE_H
#define _LOAD_PE_H

#include "pes/processingElement.h"

namespace SST {
namespace Llyr {

/**
 * @todo write docs
 */
class LoadProcessingElement : public ProcessingElement
{
public:
    LoadProcessingElement(opType op_binding, uint32_t processor_id, LlyrConfig* llyr_config,
                          uint32_t cycles = 1)  :
                          ProcessingElement(op_binding, processor_id, llyr_config)
    {
        cycles_ = cycles;


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

        return true;
    }

    virtual bool doReceive(LlyrData data)
    {
        output_->verbose(CALL_INFO, 8, 0, ">> Receive 0x%" PRIx64 "\n", uint64_t(data.to_ullong()));

        //for now push the result to all output queues that need this result
        for( uint32_t i = 0; i < output_queues_->size(); ++i ) {
            if( *output_queues_->at(i)->routing_arg_ == "" ) {
                output_queues_->at(i)->data_queue_->push(data);
            }
        }

        return true;
    }

    virtual bool doCompute()
    {
        TraceFunction trace(CALL_INFO_LONG);
        output_->verbose(CALL_INFO, 4, 0, ">> Compute %s\n", getOpString(op_binding_).c_str());

        if( output_->getVerboseLevel() >= 10 ) {
            output_->verbose(CALL_INFO, 10, 0, "Queue Contents (0)\n");
            printInputQueue();
            printOutputQueue();
        }

        std::vector< LlyrData > argList;
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
            output_->verbose(CALL_INFO, 4, 0, "-Inputs %" PRIu32 " Ready %" PRIu32 "\n", num_inputs, num_ready);
            return false;
        } else {
            output_->verbose(CALL_INFO, 4, 0, "+Inputs %" PRIu32 " Ready %" PRIu32 "\n", num_inputs, num_ready);
            for( uint32_t i = 0; i < total_num_inputs; ++i) {
                if( input_queues_->at(i)->argument_ > -1 ) {
                    argList.push_back(input_queues_->at(i)->data_queue_->front());
                    input_queues_->at(i)->forwarded_ = 0;
                    input_queues_->at(i)->data_queue_->pop();
                }
            }
        }

        //create the memory request
        switch( op_binding_ ) {
            case LD :
                doLoad(argList[0].to_ullong());
                break;
            case ALLOCA :
                break;
            default :
                output_->fatal(CALL_INFO, -1, "Error: could not find corresponding op-%" PRIu32 ".\n", op_binding_);
                exit(-1);
        }

        if( output_->getVerboseLevel() >= 10 ) {
            output_->verbose(CALL_INFO, 10, 0, "Queue Contents (1)\n");
            printInputQueue();
            printOutputQueue();
        }

        return true;
    }

    virtual void inputQueueInit()
    {
        output_->verbose(CALL_INFO, 4, 0, ">> Fake Init Input Queue(%" PRIu32 "), Op %" PRIu32 " \n",
                         processor_id_, op_binding_ );
        while( input_queues_->size() < input_queues_init_.size() ) {
//             std::cout << "Num queues (a): " << input_queues_->size() << std::endl;
            LlyrQueue* tempQueue = new LlyrQueue;
            tempQueue->forwarded_ = 0;
            tempQueue->argument_ = 0;
            tempQueue->routing_arg_ = new std::string("");
            tempQueue->data_queue_ = new std::queue< LlyrData >;
            input_queues_->push_back(tempQueue);
//             std::cout << "Num queues (b): " << input_queues_->size() << std::endl;
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
        } else {
            //for now assume that the address queue is on in-0
            uint64_t addr = llyr_config_->starting_addr_ + ( (processor_id_ - 1) * (Bit_Length / 8) );
            if( input_queues_->size() > 0 ) {
                LlyrData temp = LlyrData(addr);
                output_->verbose(CALL_INFO, 8, 0, "Init(%" PRIu32 ")::%" PRIx64 "::%" PRIu64 "\n", 0, addr, temp.to_ulong());
                input_queues_->at(0)->data_queue_->push(temp);

                addr = addr + (Bit_Length / 8);
            }
        }
    }

    virtual void outputQueueInit()
    {
        output_->verbose(CALL_INFO, 4, 0, ">> Fake Init Output Queue(%" PRIu32 "), Op %" PRIu32 " \n",
                         processor_id_, op_binding_ );

        //TODO Need a more elegant way to initialize these queues
        uint32_t queue_id = 0;
        if( output_queues_init_.size() > 0 ) {
            for( auto it = output_queues_init_.begin(); it != output_queues_init_.end(); ++it ) {
                if( it->first == queue_id ) {
                    int64_t init_value = std::stoll(it->second);
                    LlyrData temp = LlyrData(init_value);
                    output_queues_->at(queue_id)->data_queue_->push(temp);
                }
                queue_id = queue_id + 1;
            }
        } else {
            //FIXME going to initialize all of the output queues
            uint64_t addr = ( llyr_config_->starting_addr_ + ( (processor_id_ - 1) * (Bit_Length / 8) ) ) % 2;

            for( uint32_t i = 0; i < output_queues_->size(); ++i ) {
                LlyrData temp = LlyrData(addr);
                output_->verbose(CALL_INFO, 8, 0, "Init(%" PRIu32 ")::%" PRIx64 "::%" PRIu64 "\n", i, addr, temp.to_ulong());
                output_queues_->at(i)->data_queue_->push(temp);
            }
        }
    };

protected:
    QueueArgMap input_queues_init_;
    QueueArgMap output_queues_init_;

    bool doLoad(uint64_t addr)
    {
        uint32_t targetPe = 0;
        StandardMem::Request* req = new StandardMem::Read(addr, 8);

        output_->verbose(CALL_INFO, 4, 0, "Creating a load request (%" PRIu32 ") from address: %" PRIu64 "\n", uint32_t(req->getID()), addr);

        //find out where the load actually needs to go
        auto it = output_queue_map_.begin();
        for( ; it != output_queue_map_.end(); ++it ) {
            if( it->first == 0 ) {
                targetPe = it->second->getProcessorId();
                break;
            }
        }

        //exit the simulation if there is not a corresponding destination
        if( it == output_queue_map_.end() ) {
            output_->fatal(CALL_INFO, -1, "Error: could not find corresponding PE.\n");
            exit(-1);
        }

        LSEntry* tempEntry = new LSEntry( req->getID(), processor_id_, targetPe );
        lsqueue_->addEntry( tempEntry );

        mem_interface_->send( req );

        return 1;
    }

private:


};//END LoadProcessingElement

class AdvLoadProcessingElement : public LoadProcessingElement
{
public:
    AdvLoadProcessingElement(opType op_binding, uint32_t processor_id, LlyrConfig* llyr_config,
                          QueueArgMap* arguments, uint32_t cycles = 1)  :
                          LoadProcessingElement(op_binding, processor_id, llyr_config)
    {
        cycles_ = cycles;

        // iterate through the arguments and set initial queue values
        for( auto it = arguments->begin(); it != arguments->end(); ++it ) {

            std::cout << "[AdvLoadProcessingElement]";
            std::cout << "input_queues_init_ -- ";
            std::cout << " queue: " << it->first;
            std::cout << " arg: "   << it->second;
            std::cout << std::endl;

            auto retVal = input_queues_init_.emplace( it->first, it->second );
            if( retVal.second == false ) {
                ///TODO
            }
        }
    }

    virtual bool doCompute()
    {
        TraceFunction trace(CALL_INFO_LONG);
        output_->verbose(CALL_INFO, 4, 0, ">> Compute %s\n", getOpString(op_binding_).c_str());

        if( output_->getVerboseLevel() >= 10 ) {
            output_->verbose(CALL_INFO, 10, 0, "Queue Contents (0)\n");
            printInputQueue();
            printOutputQueue();
        }

        std::vector< LlyrData > argList;
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

        //if there are values waiting on any of the inputs (queue-0/-1 are not valid for stream_ld), this PE could still fire
        for( uint32_t i = 2; i < total_num_inputs; ++i ) {
            if( input_queues_->at(i)->data_queue_->size() > 0 ) {
                pending_op_ = 1;
            } else {
                pending_op_ = 0;
            }
        }

        //if all inputs are available pull from queue and add to arg list
        if( num_inputs == 0 || num_ready < num_inputs ) {
            output_->verbose(CALL_INFO, 4, 0, "-Inputs %" PRIu32 " Ready %" PRIu32 "\n", num_inputs, num_ready);
            return false;
        } else {
            output_->verbose(CALL_INFO, 4, 0, "+Inputs %" PRIu32 " Ready %" PRIu32 "\n", num_inputs, num_ready);
            for( uint32_t i = 0; i < total_num_inputs; ++i) {
                if( input_queues_->at(i)->argument_ > -1 ) {
                    argList.push_back(input_queues_->at(i)->data_queue_->front());
                    input_queues_->at(i)->forwarded_ = 0;
                    input_queues_->at(i)->data_queue_->pop();
                }
            }
        }

        //create the memory request
        if( op_binding_ == LDADDR ) {
            doLoad(argList[0].to_ullong());
        } else if( op_binding_ == STREAM_LD ) {
            if( argList[1].to_ullong() > 0 ) {
                input_queues_->at(0)->data_queue_->push(LlyrData(argList[0].to_ullong() + (Bit_Length / 8) ));
                input_queues_->at(1)->data_queue_->push(LlyrData(argList[1].to_ullong() - 1));
                doLoad(argList[0].to_ullong());
            }
        } else {
            output_->fatal(CALL_INFO, -1, "Error: could not find corresponding op-%" PRIu32 ".\n", op_binding_);
            exit(-1);
        }

        if( output_->getVerboseLevel() >= 10 ) {
            output_->verbose(CALL_INFO, 10, 0, "Queue Contents (1)\n");
            printInputQueue();
            printOutputQueue();
        }

        return true;
    }

private:


};// AdvLoadProcessingElement

}//SST
}//Llyr

#endif // _LOAD_PE_H
