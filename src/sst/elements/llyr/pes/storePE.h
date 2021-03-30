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
        output_queues_ = new std::vector< std::queue< LlyrData >* >;

        //stores need two input queues -- address and data
        input_queues_= new std::vector< std::queue< LlyrData >* >;
        std::queue< LlyrData >* tempQueue = new std::queue< LlyrData >;
        input_queues_->push_back(tempQueue);
    }

    StoreProcessingElement(opType op_binding, uint32_t processor_id, LlyrConfig* llyr_config,
                           std::vector< std::queue< LlyrData >* >* input_queues_init, uint32_t cycles = 1)  :
                    ProcessingElement(op_binding, processor_id, llyr_config)
    {
        cycles_ = cycles;
        output_queues_ = new std::vector< std::queue< LlyrData >* >;
        input_queues_= new std::vector< std::queue< LlyrData >* >(*input_queues_init);
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

        return true;
    }

    virtual bool doReceive(LlyrData data)
    {
        output_->verbose(CALL_INFO, 8, 0, ">> Receive 0x%" PRIx64 "\n", uint64_t(data.to_ullong()));

        //for now push the result to all output queues
        for( uint32_t i = 0; i < output_queues_->size(); ++i) {
            output_queues_->at(i)->push(data);
        }

        return true;
    }

    virtual bool doCompute()
    {
        output_->verbose(CALL_INFO, 4, 0, ">> Compute 0x%" PRIx32 "\n", op_binding_);

        if( output_->getVerboseLevel() >= 10 ) {
            printInputQueue();
            printOutputQueue();
        }

        std::vector< LlyrData > argList;
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

        doStore(argList[0].to_ullong(), argList[1].to_ullong());

        if( output_->getVerboseLevel() >= 10 ) {
            printInputQueue();
            printOutputQueue();
        }

        return true;
    }

    virtual void fakeInit()
    {
         output_->verbose(CALL_INFO, 4, 0, ">> Fake Init(%" PRIu32 "), Op %" PRIu32 " \n",
                        processor_id_, op_binding_ );

        //for now assume that the address queue is on in-0
        uint64_t addr = (processor_id_ - 1) * (Bit_Length / 8);
        if( input_queues_->size() > 0 ) {
            LlyrData temp = LlyrData(addr);
            output_->verbose(CALL_INFO, 8, 0, "Init(%" PRIu32 ")::%" PRIx64 "::%" PRIu64 "\n", 0, addr, temp.to_ulong());
            input_queues_->at(0)->push(temp);

            addr = addr + (Bit_Length / 8);
        }
    }

private:
    bool doStore(uint64_t addr, LlyrData data)
    {
        uint32_t targetPe = processor_id_;
        SimpleMem::Request* req = new SimpleMem::Request(SimpleMem::Request::Write, addr, 8);

        output_->verbose(CALL_INFO, 4, 0, "Creating a store request (%" PRIu32 ") to address: %" PRIu64 "\n", uint32_t(req->id), addr);

        const auto newValue = data.to_ullong();

        constexpr auto size = sizeof(uint64_t);
        uint8_t buffer[size] = {};
        std::memcpy(buffer, std::addressof(newValue), size);

        std::cout << "llyr:  " << data << "\n";
        std::cout << "conv:  " << newValue << "\n";
        for(uint32_t i = 0; i < size; ++i) {
            std::cout << static_cast<uint16_t>(buffer[i]) << ", ";
        }
        std::cout << std::endl;

        std::vector< uint8_t > payload(8);
        memcpy( std::addressof(payload[0]), std::addressof(newValue), size );
        for( auto it = payload.begin(); it != payload.end(); ++it ) {
            std::cout << static_cast<uint16_t>(*it) << ", ";
        }
        std::cout << std::endl;
        req->setPayload( payload );

        LSEntry* tempEntry = new LSEntry( req->id, processor_id_, targetPe );
        lsqueue_->addEntry( tempEntry );

        mem_interface_->sendRequest( req );

        return 1;
    }

};

}//SST
}//Llyr

#endif // _STORE_PE_H
