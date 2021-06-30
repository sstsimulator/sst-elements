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

#ifndef _FP_PE_H
#define _FP_PE_H

#include <limits>
#include <string>

#include "pes/processingElement.h"

namespace SST {
namespace Llyr {

/**
 * @todo write docs
 */
class FPProcessingElement : public ProcessingElement
{
public:
    FPProcessingElement(opType op_binding, uint32_t processor_id, LlyrConfig* llyr_config,
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
        std::vector< double > convList;
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

        //need to convert from the raw bits to floating point
        for(auto it = argList.begin() ; it != argList.end(); ++it ) {
            double fpResult = bits_to_double(*it);
            convList.push_back(fpResult);
        }

        double fpResult;
        switch( op_binding_ ) {
            case FPADD :
                fpResult = convList[0] + convList[1];
                std::cout << std::setprecision(std::numeric_limits<decltype(fpResult)>::max_digits10) << fpResult << " = ";
                std::cout << std::setprecision(std::numeric_limits<decltype(fpResult)>::max_digits10) << convList[0] << " + " << convList[1];
                std::cout << std::endl;
                break;
            case FPSUB :
                fpResult = convList[0] - convList[1];
                std::cout << std::setprecision(std::numeric_limits<decltype(fpResult)>::max_digits10) << fpResult << " = ";
                std::cout << std::setprecision(std::numeric_limits<decltype(fpResult)>::max_digits10) << convList[0] << " - " << convList[1];
                std::cout << std::endl;
                break;
            case FPMUL :
                fpResult = convList[0] * convList[1];
                std::cout << std::setprecision(std::numeric_limits<decltype(fpResult)>::max_digits10) << fpResult << " = ";
                std::cout << std::setprecision(std::numeric_limits<decltype(fpResult)>::max_digits10) << convList[0] << " * " << convList[1];
                std::cout << std::endl;
                break;
            case FPDIV :
                fpResult = convList[0] / convList[1];
                std::cout << std::setprecision(std::numeric_limits<decltype(fpResult)>::max_digits10) << fpResult << " = ";
                std::cout << std::setprecision(std::numeric_limits<decltype(fpResult)>::max_digits10) << convList[0] << " / " << convList[1];
                std::cout << std::endl;
                break;
            default :
                output_->verbose(CALL_INFO, 0, 0, "Error: could not find corresponding op-%" PRIu32 ".\n", op_binding_);
                exit(-1);
        }

        //convert the fp value back to raw bits for storage
        retVal = LlyrData(fp_to_bits(&fpResult));

        std::cout << "fpResult = " << fpResult << std::endl;
        std::cout << "intResult = " << intResult << std::endl;
        std::cout << "retVal = " << retVal << std::endl;

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
    //helper to convert from bitset to float
    float bits_to_float( std::bitset<Bit_Length> valIn )
    {
        const auto newValue = valIn.to_ullong();
        constexpr auto size = sizeof(float);
        uint8_t fpBuffer[size] = {};
        std::memcpy(fpBuffer, std::addressof(newValue), size);

        float fpResult;
        std::memcpy(std::addressof(fpResult), fpBuffer, size);

        return fpResult;
    }

    //helper to convert from bitset to double
    double bits_to_double( std::bitset<Bit_Length> valIn )
    {
        const auto newValue = valIn.to_ullong();
        constexpr auto size = sizeof(double);
        uint8_t fpBuffer[size] = {};
        std::memcpy(fpBuffer, std::addressof(newValue), size);

        double fpResult;
        std::memcpy(std::addressof(fpResult), fpBuffer, size);

        return fpResult;
    }

    //helper to convert from fp to bitset
    template <typename T>
    std::bitset<Bit_Length> fp_to_bits( T* fpIn )
    {
        uint64_t intResult = 0;
        constexpr auto size = sizeof(T);
        uint8_t bufferA[size] = {};

        std::memcpy(bufferA, std::addressof(*fpIn), size);
        std::memcpy(std::addressof(intResult), bufferA, size);
        std::bitset<Bit_Length> myBits = std::bitset<Bit_Length>(intResult);

        return myBits;
    }

    //helper for debugging -- convert bitset to string
    std::string bits_to_string( std::bitset<Bit_Length> valIn )
    {
        return valIn.to_string<char,std::string::traits_type,std::string::allocator_type>();
    }



};

}//SST
}//Llyr

#endif // _FP_PE_H
