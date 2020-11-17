#include <sst_config.h>

#include <random>
#include <cstring>

#include "processingElement.h"

namespace SST {
namespace Llyr {

ProcessingElement::ProcessingElement(opType op_binding, uint32_t processor_id, uint32_t queue_depth,
                                     LSQueue* lsqueue, SimpleMem*  mem_interface)  :
                    op_binding_(op_binding), processor_id_(processor_id), queue_depth_(queue_depth),
                    lsqueue_(lsqueue), mem_interface_(mem_interface)
{
    //setup up i/o for messages
    char prefix[256];
    sprintf(prefix, "[t=@t][ProcessingElement-%u]: ", processor_id_);
    output_ = new SST::Output(prefix, 0, 0, Output::STDOUT);

    input_queues_= new std::vector< std::queue< LlyrData >* >;
    output_queues_ = new std::vector< std::queue< LlyrData >* >;

    pending_op_ = 0;
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

    if( op_binding_ == LD )
    {
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
    }
    else
    {
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

uint32_t ProcessingElement::bindInputQueue(uint32_t id)
{
    uint32_t queueId = input_queues_->size();

    output_->verbose(CALL_INFO, 0, 0, ">> Binding Input Queue-%" PRIu32 " on PE-%" PRIu32 " to PE-%" PRIu32 "\n",
                     queueId, processor_id_, id );

    auto retVal = input_queue_map_.emplace( id, queueId );
    if( retVal.second == false ) {
        return 0;
    }

    //if insert succeeded need to create the queue
    std::queue< LlyrData >* tempQueue = new std::queue< LlyrData >;
    input_queues_->push_back(tempQueue);

    return queueId;
}

uint32_t ProcessingElement::bindOutputQueue(uint32_t id)
{
    uint32_t queueId = output_queues_->size();

    output_->verbose(CALL_INFO, 0, 0, ">> Binding Output Queue-%" PRIu32 " on PE-%" PRIu32 " to PE-%" PRIu32 "\n",
                     queueId, processor_id_, id );

    auto retVal = output_queue_map_.emplace( id, queueId );
    if( retVal.second == false ) {
        return 0;
    }

    //if insert succeeded need to create the queue
    std::queue< LlyrData >* tempQueue = new std::queue< LlyrData >;
    output_queues_->push_back(tempQueue);

    return queueId;
}

uint32_t ProcessingElement::getInputQueueSrc(uint32_t id)
{
    auto it = input_queue_map_.begin();
    for( ; it != input_queue_map_.end(); ++it ) {
        if( it->second == id ) {
            return it->first;
        }
    }

    return 0;
}

uint32_t ProcessingElement::getOutputQueueDst(uint32_t id)
{
    auto it = output_queue_map_.begin();
    for( ; it != output_queue_map_.end(); ++it ) {
        if( it->second == id ) {
            return it->first;
        }
    }

    return 0;
}

bool ProcessingElement::doSend(std::vector< Edge* >* adjacencyList)
{
    LlyrData sendVal;
    uint32_t dstPe;
    uint32_t num_outputs  = output_queues_->size();

    for( auto boo = adjacencyList->begin(); boo != adjacencyList->end(); ++boo ) {
        std::cout << (*boo)->getDestination() << " ";
    }
    std::cout << std::endl;

    //skip dummy nodes
    if( op_binding_ == DUMMY )
        return 1;

    output_->verbose(CALL_INFO, 0, 0, ">> Sending\n");

    //
    for( uint32_t i = 0; i < num_outputs; ++i) {
        if( output_queues_->at(i)->size() > 0 ) {
            dstPe = getOutputQueueDst(i);
            sendVal = output_queues_->at(i)->front();
            output_queues_->at(i)->pop();

            for( auto moop = output_queue_map_.begin(); moop != output_queue_map_.end(); ++moop) {
                std::cout << "First " << moop->first;
                std::cout << " Second " << moop->second;
                std::cout << "\n" << std::endl;
            }

            for( auto moop = input_queue_map_.begin(); moop != input_queue_map_.end(); ++moop) {
                std::cout << "First " << moop->first;
                std::cout << " Second " << moop->second;
                std::cout << std::endl;
            }

//             input_queues_->at(getInputQueueId(i));

            output_->verbose(CALL_INFO, 0, 0, ">> Sending from %" PRIu32 " to %" PRIu32 "\n", i, dstPe );
        }
    }

    printInputQueue();
    printOutputQueue();

    return true;
}

bool ProcessingElement::doCompute()
{
    if( op_binding_ == DUMMY )
        return 1;

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
        case ST :
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

bool ProcessingElement::doLoad(uint64_t addr)
{
    output_->verbose(CALL_INFO, 0, 0, "Creating a load from address: %" PRIu64 "\n", addr);

    uint32_t targetPe;
    SimpleMem::Request* req = new SimpleMem::Request(SimpleMem::Request::Read, addr, 8);

    //find out where the load actually needs to go
    auto it = output_queue_map_.begin();
    for( ; it != output_queue_map_.end(); ++it ) {
        if( it->second == 0 ) {
            targetPe = it->first;
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

void ProcessingElement::pushInputQueue(uint32_t id, uint64_t &inVal )
{
    LlyrData newValue = LlyrData(inVal);
    input_queues_->at(id)->push(newValue);
}

void ProcessingElement::printInputQueue()
{
    for( uint32_t i = 0; i < input_queues_->size(); ++i )
    {
        std::cout << "i:" << i << ": " << input_queues_->at(i)->size();
        if( input_queues_->at(i)->size() > 0 )
            std::cout << ":" << input_queues_->at(i)->front().to_ullong() << ":" << input_queues_->at(i)->front() << "\n";
        else
            std::cout << ":x" << ":x" << "\n";
    }
}

void ProcessingElement::printOutputQueue()
{
    for( uint32_t i = 0; i < output_queues_->size(); ++i )
    {
        std::cout << "o:" << i << ": " << output_queues_->at(i)->size();
        if( output_queues_->at(i)->size() )
            std::cout << ":" << output_queues_->at(i)->back().to_ullong() << ":" << output_queues_->at(i)->front() << "\n";
        else
            std::cout << ":x" << ":x" << "\n";
    }
}

} // namespace llyr
} // namespace SST



