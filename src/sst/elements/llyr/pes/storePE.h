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

#ifndef _STORE_PE_H
#define _STORE_PE_H

#include <random>

#include "pes/processingElement.h"

namespace SST {
namespace Llyr {

/**
 * @todo write docs
 */
class StoreProcessingElement : public ProcessingElement
{
public:
    StoreProcessingElement(opType op_binding, uint32_t processor_id, LlyrConfig* llyr_config,
                           uint32_t cycles = 1)  :
                    ProcessingElement(op_binding, processor_id, llyr_config)
    {
        cycles_ = cycles;
        output_queues_ = new std::vector< LlyrQueue* >;

        //stores need two input queues -- address and data
        input_queues_= new std::vector< LlyrQueue* >;
        LlyrQueue* tempQueue = new LlyrQueue;
        tempQueue->data_queue_ = new std::queue< LlyrData >;
        input_queues_->push_back(tempQueue);
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

        //for now push the result to all output queues
        for( uint32_t i = 0; i < output_queues_->size(); ++i) {
            output_queues_->at(i)->data_queue_->push(data);
        }

        return true;
    }

    virtual bool doCompute()
    {
        output_->verbose(CALL_INFO, 4, 0, ">> Compute 0x%" PRIx32 "\n", op_binding_);

        if( output_->getVerboseLevel() >= 64 ) {
            printInputQueue();
            printOutputQueue();
        }

        std::vector< LlyrData > argList;
        uint32_t num_ready = 0;
        uint32_t num_inputs  = input_queues_->size();

        //check to see if all of the input queues have data
        for( uint32_t i = 0; i < num_inputs; ++i) {
            if( input_queues_->at(i)->data_queue_->size() > 0 ) {
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
                argList.push_back(input_queues_->at(i)->data_queue_->front());
                input_queues_->at(i)->data_queue_->pop();
            }
        }

        doStore(argList[0].to_ullong(), argList[1].to_ullong());

        if( output_->getVerboseLevel() >= 10 ) {
            printInputQueue();
            printOutputQueue();
        }

        return true;
    }

    virtual void queueInit()
    {
        output_->verbose(CALL_INFO, 4, 0, ">> Fake Init(%" PRIu32 "), Op %" PRIu32 " \n",
                         processor_id_, op_binding_ );

        //TODO Need a more elegant way to initialize these queues
        if( input_queues_init_.size() > 0 ) {
            std::queue< LlyrData >* tempQueue(&input_queues_init_);
            input_queues_->at(0)->data_queue_ = tempQueue;
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

protected:
    std::queue< LlyrData > input_queues_init_;

    bool doStore(uint64_t addr, LlyrData data)
    {
        uint32_t targetPe = processor_id_;

        const auto newValue = data.to_ullong();

        constexpr auto size = sizeof(uint64_t);
        uint8_t buffer[size] = {};
        std::memcpy(buffer, std::addressof(newValue), size);

        output_->verbose(CALL_INFO, 64, 0, "llyr = %s\n", data.to_string().c_str());
        output_->verbose(CALL_INFO, 64, 0, "conv = %llu\n", newValue);

        if( output_->getVerboseLevel() >= 64 ) {
            std::stringstream dataOut;
            for(uint32_t i = 0; i < size; ++i) {
                dataOut << static_cast< uint16_t >(buffer[i]) << ", ";
            }
            output_->verbose(CALL_INFO, 64, 0, "%s\n", dataOut.str().c_str());
        }

        std::vector< uint8_t > payload(8);
        memcpy( std::addressof(payload[0]), std::addressof(newValue), size );

        if( output_->getVerboseLevel() >= 64 ) {
            std::stringstream dataOut;
            for( auto it = payload.begin(); it != payload.end(); ++it ) {
                dataOut << static_cast< uint16_t >(*it) << ", ";
            }
            output_->verbose(CALL_INFO, 64, 0, "%s\n", dataOut.str().c_str());
        }

        StandardMem::Request* req = new StandardMem::Write(addr, 8, payload);
        output_->verbose(CALL_INFO, 4, 0, "Creating a store request (%" PRIu32 ") to address: %" PRIu64 "\n", uint32_t(req->getID()), addr);

        LSEntry* tempEntry = new LSEntry( req->getID(), processor_id_, targetPe );
        lsqueue_->addEntry( tempEntry );

        mem_interface_->send( req );

        return 1;
    }

private:

};// END StoreProcessingElement

class AdvStoreProcessingElement : public StoreProcessingElement
{
public:
    AdvStoreProcessingElement(opType op_binding, uint32_t processor_id, LlyrConfig* llyr_config,
                          std::string *arguments, uint32_t cycles = 1)  :
                          StoreProcessingElement(op_binding, processor_id, llyr_config)
    {
        cycles_ = cycles;

        if( arguments[0] != "" ) {
            int64_t init_value = std::stoll(arguments[0]);
            input_queues_init_.push(LlyrData(init_value));
        }

        if( arguments[1] != "" ) {
            termination_ = std::stoll(arguments[1]);
        } else {
            termination_ = 0;
        }

        output_queues_ = new std::vector< LlyrQueue* >;

        //stores need two input queues -- address and data
        input_queues_= new std::vector< LlyrQueue* >;
        LlyrQueue* tempQueue = new LlyrQueue;
        tempQueue->data_queue_ = new std::queue< LlyrData >;
        input_queues_->push_back(tempQueue);
    }

    virtual bool doCompute()
    {
        output_->verbose(CALL_INFO, 4, 0, ">> Compute 0x%" PRIx32 "\n", op_binding_);

        if( output_->getVerboseLevel() >= 64 ) {
            printInputQueue();
            printOutputQueue();
        }

        std::vector< LlyrData > argList;
        uint32_t num_ready = 0;
        uint32_t num_inputs  = input_queues_->size();

        //check to see if all of the input queues have data
        for( uint32_t i = 0; i < num_inputs; ++i) {
            if( input_queues_->at(i)->data_queue_->size() > 0 ) {
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
                argList.push_back(input_queues_->at(i)->data_queue_->front());
                input_queues_->at(i)->data_queue_->pop();
            }
        }

        //create the memory request
        if( op_binding_ == STADDR ) {
            doStore(argList[0].to_ullong(), argList[1].to_ullong());
        } else if( op_binding_ == STREAM_ST ) {
            doStore(argList[0].to_ullong(), argList[1].to_ullong());
            if( termination_ > 0 ) {
                termination_ = termination_ - 1;
                input_queues_->at(0)->data_queue_->push(LlyrData(argList[0].to_ullong() + (Bit_Length / 8)));
            }
        } else {
            output_->fatal(CALL_INFO, -1, "Error: could not find corresponding op-%" PRIu32 ".\n", op_binding_);
            exit(-1);
        }

        if( output_->getVerboseLevel() >= 10 ) {
            printInputQueue();
            printOutputQueue();
        }

        return true;
    }

private:
    uint64_t termination_;

};// AdvStoreProcessingElement

}//SST
}//Llyr

#endif // _STORE_PE_H
