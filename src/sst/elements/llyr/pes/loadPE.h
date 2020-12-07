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

#ifndef _LOAD_PE_H
#define _LOAD_PE_H

#include <random>

#include "pes/processingElement.h"

namespace SST {
namespace Llyr {

/**
 * @todo write docs
 */
class LoadProcessingElement : public ProcessingElement
{
public:
    LoadProcessingElement(opType op_binding, uint32_t processor_id, LlyrConfig* llyr_config, uint32_t cycles = 1)  :
                    ProcessingElement(op_binding, processor_id, llyr_config)
    {
        cycles_ = cycles;
    }

    LoadProcessingElement(opType op_binding, uint32_t processor_id, LlyrConfig* llyr_config,
                    std::vector< std::queue< LlyrData >* >* input_queues_init,
                    uint32_t cycles = 1)  :
                    ProcessingElement(op_binding, processor_id, llyr_config)
    {
        cycles_ = cycles;
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
                output_->verbose(CALL_INFO, 0, 0, ">> Sending...%" PRIu32 "-%" PRIu32 " to %" PRIu32 "\n",
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
        output_->verbose(CALL_INFO, 0, 0, ">> Receive 0x%" PRIx64 " on PE %" PRIu32 "\n", uint64_t(data.to_ullong()), processor_id_ );

        //for now push the result to all output queues
        for( uint32_t i = 0; i < output_queues_->size(); ++i) {
            output_queues_->at(i)->push(data);
        }

        return true;
    }

    virtual bool doCompute()
    {
        output_->verbose(CALL_INFO, 0, 0, ">> Compute 0x%" PRIx32 " on PE %" PRIu32 "\n", op_binding_, processor_id_ );

        printInputQueue();
        printOutputQueue();

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
            std::cout << "-Inputs " << num_inputs << " Ready " << num_ready <<std::endl;
            return false;
        } else {
            std::cout << "+Inputs " << num_inputs << " Ready " << num_ready <<std::endl;
            for( uint32_t i = 0; i < num_inputs; ++i) {
                argList.push_back(input_queues_->at(i)->front());
                input_queues_->at(i)->pop();
            }
        }

        //create the memory request
        switch( op_binding_ ) {
            case LD :
                doLoad(argList[0].to_ullong());
                return true;
                break;
            case LD_ST :
                doLoad(argList[0].to_ullong());
                return true;
                break;
            default :
                output_->verbose(CALL_INFO, 0, 0, "Error: could not find corresponding op.\n");
                exit(-1);
        }

        printInputQueue();
        printOutputQueue();

        return true;
    }

    //TODO for testing only
    virtual void fakeInit()
    {
        std::random_device some_rand;
        std::mt19937 generator( some_rand() );
        std::uniform_int_distribution< uint64_t > int_dist(0, 50);
    //     std::uniform_real_distribution< double > int_dist(0.0, 50.0);

        output_->verbose(CALL_INFO, 0, 0, ">> Fake Init(%" PRIu32 "), Op %" PRIu32 " \n",
                        processor_id_, op_binding_ );

        uint64_t addr;
        if(processor_id_ == 1)
            addr = 0x00;
        else if (processor_id_ == 2)
            addr = 0x08;
        else if (processor_id_ == 3)
            addr = 0x10;
        else
            addr = 0x20;
        for( uint32_t i = 0; i < input_queues_->size(); ++i )
        {
            LlyrData temp = LlyrData(addr);
            std::cout << "Init(" << processor_id_ << "-" << i << ")::" << addr << "::" << temp << std::endl;
            input_queues_->at(i)->push(temp);
        }
    }

private:
    bool doLoad(uint64_t addr)
    {
        uint32_t targetPe;
        SimpleMem::Request* req = new SimpleMem::Request(SimpleMem::Request::Read, addr, 8);

        output_->verbose(CALL_INFO, 0, 0, "Creating a load request (%" PRIu32 ") from address: %" PRIu64 "\n", uint32_t(req->id), addr);

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
            output_->verbose(CALL_INFO, 0, 0, "Error: could not find corresponding PE.\n");
            exit(-1);
        }

        LSEntry* tempEntry = new LSEntry( req->id, processor_id_, targetPe );
        lsqueue_->addEntry( tempEntry );

        mem_interface_->sendRequest( req );

        return 1;

    }

};

}//SST
}//Llyr

#endif // _LOAD_PE_H
