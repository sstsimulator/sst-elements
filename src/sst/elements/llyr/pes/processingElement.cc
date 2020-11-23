#include <sst_config.h>

#include <random>
#include <cstring>

#include "processingElement.h"

namespace SST {
namespace Llyr {

ProcessingElement::ProcessingElement(opType op_binding, uint32_t processor_id, uint32_t queue_depth,
                                     LSQueue* lsqueue, SimpleMem*  mem_interface)  :
                    op_binding_(op_binding), processor_id_(processor_id), queue_depth_(queue_depth),
                    lsqueue_(lsqueue), mem_interface_(mem_interface), pending_op_(0)
{
    //setup up i/o for messages
    char prefix[256];
    sprintf(prefix, "[t=@t][ProcessingElement-%u]: ", processor_id_);
    output_ = new SST::Output(prefix, 0, 0, Output::STDOUT);

    input_queues_= new std::vector< std::queue< LlyrData >* >;
    output_queues_ = new std::vector< std::queue< LlyrData >* >;
}

ProcessingElement::~ProcessingElement()
{
}

bool ProcessingElement::fakeInit()
{
    std::random_device some_rand;
    std::mt19937 generator( some_rand() );
    std::uniform_int_distribution< uint64_t > int_dist(0, 50);
//     std::uniform_real_distribution< double > int_dist(0.0, 50.0);

     output_->verbose(CALL_INFO, 0, 0, ">> Fake Init(%" PRIu32 "), Op %" PRIu32 " \n",
                     processor_id_, op_binding_ );

    if( op_binding_ == LD ) {
        uint64_t addr;
        if(processor_id_ % 2 != 0)
            addr = 0x00;
        else
            addr = 0x40;
        for( uint32_t i = 0; i < input_queues_->size(); ++i )
        {
            LlyrData temp = LlyrData(addr);
            std::cout << "Init(" << processor_id_ << "-" << i << ")::" << "0" << "::" << temp << "::" << temp << std::endl;
            input_queues_->at(i)->push(temp);
        }
    } else if( op_binding_ == ST ) {
        std::queue< LlyrData >* tempQueue = new std::queue< LlyrData >;
        input_queues_->push_back(tempQueue);

        LlyrData temp = LlyrData(0xF0);
        std::cout << "Init(" << processor_id_ << "-" << input_queues_->size() << ")::" << "0" << "::" << temp << "::" << temp << std::endl;
        input_queues_->back()->push(temp);
    } else {
//         for( uint32_t i = 0; i < input_queues_->size(); ++i )
//         {
//             int64_t my_int = int_dist( generator );
//             LlyrData temp = LlyrData(my_int);
//             std::cout << "Init(" << processor_id_ << "-" << i << ")::" << my_int << "::" << temp << "::" << temp << std::endl;
//             input_queues_->at(i)->push(temp);
//         }
    }

    return true;
}

uint32_t ProcessingElement::bindInputQueue(ProcessingElement* src)
{
    uint32_t queueId = input_queues_->size();

    output_->verbose(CALL_INFO, 0, 0, ">> Binding Input Queue-%" PRIu32 " on PE-%" PRIu32 " to PE-%" PRIu32 "\n",
                     queueId, processor_id_, src->getProcessorId() );

    auto retVal = input_queue_map_.emplace( queueId, src );
    if( retVal.second == false ) {
        return 0;
    }

    //if insert succeeded need to create the queue
    std::queue< LlyrData >* tempQueue = new std::queue< LlyrData >;
    input_queues_->push_back(tempQueue);

    return queueId;
}

uint32_t ProcessingElement::bindOutputQueue(ProcessingElement* dst)
{
    uint32_t queueId = output_queues_->size();

    output_->verbose(CALL_INFO, 0, 0, ">> Binding Output Queue-%" PRIu32 " on PE-%" PRIu32 " to PE-%" PRIu32 "\n",
                     queueId, processor_id_, dst->getProcessorId() );

    auto retVal = output_queue_map_.emplace( queueId, dst );
    if( retVal.second == false ) {
        return 0;
    }

    //if insert succeeded need to create the queue
    std::queue< LlyrData >* tempQueue = new std::queue< LlyrData >;
    output_queues_->push_back(tempQueue);

    return queueId;
}

int32_t ProcessingElement::getInputQueueId(uint32_t id) const
{
    auto it = input_queue_map_.begin();
    for( ; it != input_queue_map_.end(); ++it ) {
        if( it->second->getProcessorId() == id ) {
            return it->first;
        }
    }

    return -1;
}

int32_t ProcessingElement::getOutputQueueId(uint32_t id) const
{
    auto it = output_queue_map_.begin();
    for( ; it != output_queue_map_.end(); ++it ) {
        if( it->second->getProcessorId() == id ) {
            return it->first;
        }
    }

    return -1;
}

bool ProcessingElement::doSend()
{
    uint32_t queueId;
    LlyrData sendVal;
    ProcessingElement* dstPe;

    //skip dummy nodes
    if( op_binding_ == DUMMY ) {
        return 1;
    }

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

    printInputQueue();
    printOutputQueue();

    return true;
}

bool ProcessingElement::doLoad(uint64_t addr)
{
    uint32_t targetPe;
    SimpleMem::Request* req = new SimpleMem::Request(SimpleMem::Request::Read, addr, 8);

    output_->verbose(CALL_INFO, 0, 0, "Creating a load request (%" PRIu32 " from address: %" PRIu64 "\n", uint32_t(req->id), addr);

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

bool ProcessingElement::doStore(LlyrData data, uint64_t addr)
{
    uint32_t targetPe = processor_id_;
    SimpleMem::Request* req = new SimpleMem::Request(SimpleMem::Request::Write, addr, 8);

    output_->verbose(CALL_INFO, 0, 0, "Creating a store request (%" PRIu32 " from address: %" PRIu64 "\n", uint32_t(req->id), addr);

    const auto newValue = data.to_ullong();

    constexpr auto size = sizeof(uint64_t);
    uint8_t buffer[size] = {};
    std::memcpy(buffer, std::addressof(newValue), size);

    std::cout << "llyr:  " << data << "\n";
    std::cout << "conv:  " << newValue << "\n";
    for(uint32_t i = 0; i < size; ++i) {
        std::cout << static_cast<unsigned int>(buffer[i]) << ", ";
    }
    std::cout << std::endl;

    std::vector< uint8_t > payload(8);
    memcpy( std::addressof(payload[0]), std::addressof(newValue), size );
    for( auto it = payload.begin(); it != payload.end(); ++it ) {
        std::cout << static_cast<unsigned int>(*it) << ", ";
    }
    std::cout << std::endl;
    req->setPayload( payload );

    LSEntry* tempEntry = new LSEntry( req->id, processor_id_, targetPe );
    lsqueue_->addEntry( tempEntry );

    mem_interface_->sendRequest( req );

    return 1;

}

bool ProcessingElement::doCompute()
{
    if( op_binding_ == DUMMY ) {
        return 1;
    }

    output_->verbose(CALL_INFO, 0, 0, ">> Compute 0x%" PRIx32 " on PE %" PRIu32 "\n", op_binding_, processor_id_ );

    uint64_t intResult = 0x0F;

    std::vector< LlyrData > argList;
    LlyrData retVal;

    printInputQueue();
    printOutputQueue();

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

    switch( op_binding_ ) {
        case LD :
            doLoad(argList[0].to_ullong());
            return true;
            break;
        case LD_ST :
            doLoad(argList[0].to_ullong());
            return true;
            break;
        case ST :
            doStore(argList[0].to_ullong(), argList[1].to_ullong());
            return true;
            break;
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
        case FPADD :
        case FPSUB :
        case FPMUL :
        case FPDIV :
            break;
        case OTHER :
            break;
        case ANY :
        case DUMMY :
            break;
    }

    retVal = LlyrData(intResult);



/*
    //TODO better way to test for variable number of input queues instead of hard coding
    if( num_inputs_ == 1 ) {
        //check to see if all operands are available
        if( input_queues_->at(0)->size() > 0 ) {
            arg0 = input_queues_->at(0)->front();
            input_queues_->at(0)->pop();

            if( op_binding_ == LD ) {
                doLoad(arg0.to_ullong());
            }

        } else {
            //if no args available, do nothing
        }
    }
    else if( num_inputs_ == 2 ) {
        //check to see if all operands are available
        if( input_queues_->at(0)->size() > 0 && input_queues_->at(1)->size() > 0 ) {
            arg0 = input_queues_->at(0)->front();
            input_queues_->at(0)->pop();

            arg1 = input_queues_->at(1)->front();
            input_queues_->at(1)->pop();

            if( op_binding_ == ADD ) {
                intResult = arg0.to_ullong() + arg1.to_ullong();
                retVal = LlyrData(intResult);
            }
            else if( op_binding_ == SUB ) {
                intResult = arg0.to_ullong() - arg1.to_ullong();
                retVal = LlyrData(intResult);
            }
            else if( op_binding_ == MUL ) {
                intResult = arg0.to_ullong() * arg1.to_ullong();
                retVal = LlyrData(intResult);
            }
            else if( op_binding_ == DIV ) {
                intResult = arg0.to_ullong() / arg1.to_ullong();
                retVal = LlyrData(intResult);
            }
            else if( op_binding_ == FPADD ) {
                double fpResult;
                double fpArg0 = 0;
                double fpArg1 = 0;

                constexpr auto size = sizeof(double);
                uint8_t bufferA[size] = {};
                uint8_t bufferB[size] = {};

                const auto temp0 = arg0.to_ullong();
                std::memcpy(bufferA, std::addressof(temp0), size);
                std::memcpy(std::addressof(fpArg0), bufferA, size);

                const auto temp1 = arg1.to_ullong();
                std::memcpy(bufferB, std::addressof(temp1), size);
                std::memcpy(std::addressof(fpArg1), bufferB, size);

                fpResult = fpArg0 + fpArg1;

                std::memcpy(bufferA, std::addressof(fpResult), size);
                std::memcpy(std::addressof(intResult), bufferA, size);
                retVal = LlyrData(intResult);
            }
            else if( op_binding_ == FPSUB ) {
                double fpResult;
                double fpArg0 = 0;
                double fpArg1 = 0;

                constexpr auto size = sizeof(double);
                uint8_t bufferA[size] = {};
                uint8_t bufferB[size] = {};

                const auto temp0 = arg0.to_ullong();
                std::memcpy(bufferA, std::addressof(temp0), size);
                std::memcpy(std::addressof(fpArg0), bufferA, size);

                const auto temp1 = arg1.to_ullong();
                std::memcpy(bufferB, std::addressof(temp1), size);
                std::memcpy(std::addressof(fpArg1), bufferB, size);

                fpResult = fpArg0 - fpArg1;

                std::memcpy(bufferA, std::addressof(fpResult), size);
                std::memcpy(std::addressof(intResult), bufferA, size);
                retVal = LlyrData(intResult);
            }
            else if( op_binding_ == FPMUL ) {
                double fpResult;
                double fpArg0 = 0;
                double fpArg1 = 0;

                constexpr auto size = sizeof(double);
                uint8_t bufferA[size] = {};
                uint8_t bufferB[size] = {};

                const auto temp0 = arg0.to_ullong();
                std::memcpy(bufferA, std::addressof(temp0), size);
                std::memcpy(std::addressof(fpArg0), bufferA, size);

                const auto temp1 = arg1.to_ullong();
                std::memcpy(bufferB, std::addressof(temp1), size);
                std::memcpy(std::addressof(fpArg1), bufferB, size);

                fpResult = fpArg0 * fpArg1;

                std::memcpy(bufferA, std::addressof(fpResult), size);
                std::memcpy(std::addressof(intResult), bufferA, size);
                retVal = LlyrData(intResult);
            }
            else if( op_binding_ == FPDIV ) {
                double fpResult;
                double fpArg0 = 0;
                double fpArg1 = 0;

                constexpr auto size = sizeof(double);
                uint8_t bufferA[size] = {};
                uint8_t bufferB[size] = {};

                const auto temp0 = arg0.to_ullong();
                std::memcpy(bufferA, std::addressof(temp0), size);
                std::memcpy(std::addressof(fpArg0), bufferA, size);

                const auto temp1 = arg1.to_ullong();
                std::memcpy(bufferB, std::addressof(temp1), size);
                std::memcpy(std::addressof(fpArg1), bufferB, size);

                fpResult = fpArg0 / fpArg1;

                std::memcpy(bufferA, std::addressof(fpResult), size);
                std::memcpy(std::addressof(intResult), bufferA, size);
                retVal = LlyrData(intResult);
            }

        } else {
            //if no args available, do nothing
        }
    }

    if( num_outputs_ == 1 ) {
        output_queues_->at(0)->push(retVal);
    }
*/

    std::cout << "intResult = " << intResult << std::endl;
    std::cout << "retVal = " << retVal << std::endl;

    //for now push the result to all output queues
    for( uint32_t i = 0; i < output_queues_->size(); ++i) {
        output_queues_->at(i)->push(retVal);
    }

    printInputQueue();
    printOutputQueue();

    return true;
}

void ProcessingElement::pushInputQueue(uint32_t id, uint64_t &inVal )
{
    LlyrData newValue = LlyrData(inVal);
    input_queues_->at(id)->push(newValue);
}

void ProcessingElement::pushInputQueue(uint32_t id, LlyrData &inVal )
{
    input_queues_->at(id)->push(inVal);
}

void ProcessingElement::printInputQueue()
{
    for( uint32_t i = 0; i < input_queues_->size(); ++i ) {
        std::cout << "i:" << i << ": " << input_queues_->at(i)->size();
        if( input_queues_->at(i)->size() > 0 ) {
            std::cout << ":" << input_queues_->at(i)->front().to_ullong() << ":" << input_queues_->at(i)->front() << "\n";
        } else {
            std::cout << ":x" << ":x" << "\n";
        }
    }
}

void ProcessingElement::printOutputQueue()
{
    for( uint32_t i = 0; i < output_queues_->size(); ++i ) {
        std::cout << "o:" << i << ": " << output_queues_->at(i)->size();
        if( output_queues_->at(i)->size() > 0 ) {
            std::cout << ":" << output_queues_->at(i)->back().to_ullong() << ":" << output_queues_->at(i)->front() << "\n";
        } else {
            std::cout << ":x" << ":x" << "\n";
        }
    }
}

} // namespace llyr
} // namespace SST



