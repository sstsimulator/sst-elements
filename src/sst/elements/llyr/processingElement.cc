#include <sst_config.h>

#include <random>
#include <cstring>

#include "processingElement.h"

namespace SST {
namespace Llyr {

ProcessingElement::ProcessingElement(void)
{
}

ProcessingElement::ProcessingElement(opType op_binding, uint32_t processor_id, uint32_t queue_depth,
                                     SimpleMem*  mem_interface)  :
    processor_id_(processor_id), queue_depth_(queue_depth), mem_interface_(mem_interface)
{
    char prefix[256];
    sprintf(prefix, "[t=@t][ProcessingElement]: ");
    output_ = new SST::Output(prefix, 0, 0, Output::STDOUT);

    input_queues_= new std::vector< std::queue< std::bitset< Bit_Length > >* >;
    output_queues_ = new std::vector< std::queue< std::bitset< Bit_Length > >* >;
    setOpBinding(op_binding);

    pending_op_ = 0;
}

ProcessingElement::ProcessingElement(opType op_binding, uint32_t processor_id, uint32_t queue_depth,
                                     SimpleMem*  mem_interface,
                                     std::queue< std::bitset< Bit_Length > >* init_input_0) :
    processor_id_(processor_id), queue_depth_(queue_depth), mem_interface_(mem_interface)
{
    char prefix[256];
    sprintf(prefix, "[t=@t][ProcessingElement]: ");
    output_ = new SST::Output(prefix, 0, 0, Output::STDOUT);

    input_queues_= new std::vector< std::queue< std::bitset< Bit_Length > >* >;
    output_queues_ = new std::vector< std::queue< std::bitset< Bit_Length > >* >;
    setOpBinding(op_binding);

    input_queues_->push_back(init_input_0);

    pending_op_ = 0;
}

ProcessingElement::ProcessingElement(opType op_binding, uint32_t processor_id, uint32_t queue_depth,
                                     SimpleMem*  mem_interface,
                                     std::queue< std::bitset< Bit_Length > >* init_input_0,
                                     std::queue< std::bitset< Bit_Length > >* init_input_1) :
    processor_id_(processor_id), queue_depth_(queue_depth), mem_interface_(mem_interface)
{
    char prefix[256];
    sprintf(prefix, "[t=@t][ProcessingElement]: ");
    output_ = new SST::Output(prefix, 0, 0, Output::STDOUT);

    input_queues_= new std::vector< std::queue< std::bitset< Bit_Length > >* >;
    output_queues_ = new std::vector< std::queue< std::bitset< Bit_Length > >* >;
    setOpBinding(op_binding);

    input_queues_->push_back(init_input_0);
    input_queues_->push_back(init_input_1);

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
            std::bitset< Bit_Length > temp = std::bitset< Bit_Length >(addr);
            std::cout << "Init(" << processor_id_ << "-" << i << ")::" << "0" << "::" << temp << "::" << temp << std::endl;
            input_queues_->at(i)->push(temp);
        }
    }
    else
    {
//         for( uint32_t i = 0; i < input_queues_->size(); ++i )
//         {
//             int64_t my_int = int_dist( generator );
//             std::bitset< Bit_Length > temp = std::bitset< Bit_Length >(my_int);
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

    std::queue< std::bitset< Bit_Length > >* tempQueue = new std::queue< std::bitset< Bit_Length > >;
    input_queues_->push_back(tempQueue);

    auto retVal = input_queue_map_.emplace( queueId, id );
    if( retVal.second == false )
    {
        return 0;
    }

    return queueId;
}

uint32_t ProcessingElement::bindOutputQueue(uint32_t id)
{
    uint32_t queueId = output_queues_->size();

    output_->verbose(CALL_INFO, 0, 0, ">> Binding Output Queue-%" PRIu32 " on PE-%" PRIu32 " to PE-%" PRIu32 "\n",
                     queueId, processor_id_, id );

    std::queue< std::bitset< Bit_Length > >* tempQueue = new std::queue< std::bitset< Bit_Length > >;
    output_queues_->push_back(tempQueue);

    auto retVal = output_queue_map_.emplace( queueId, id );
    if( retVal.second == false )
    {
        return 0;
    }

    return queueId;
}

void ProcessingElement::setOpBinding(opType binding)
{
    op_binding_ = binding;

    switch( op_binding_ )
    {
        case ANY :
            num_inputs_ = 0;
            num_outputs_ = 0;
            break;
        case LD :
            num_inputs_ = 0;
            num_outputs_ = 0;
            break;
        case ST :
        case ADD :
        case SUB :
        case MUL :
        case DIV :
        case FPADD :
        case FPSUB :
        case FPMUL :
        case FPDIV :
            num_inputs_ = 0;
            num_outputs_ = 0;
            break;
        case OTHER :
            num_inputs_ = 0;
            num_outputs_ = 0;
            break;
        case DUMMY :
            num_inputs_ = 0;
            num_outputs_ = 0;
            break;
    }

//     //Allocate queues for the inputs
//     for( uint32_t numQueues = 0; numQueues < num_inputs_; ++numQueues )
//     {
//         std::queue< std::bitset< Bit_Length > >* tempQueue = new std::queue< std::bitset< Bit_Length > >;
//         input_queues_->push_back(tempQueue);
//     }
//
//     //Allocate queues for the outputs
//     for( uint32_t numQueues = 0; numQueues < num_outputs_; ++numQueues )
//     {
//         std::queue< std::bitset< Bit_Length > >* tempQueue = new std::queue< std::bitset< Bit_Length > >;
//         output_queues_->push_back(tempQueue);
//     }

}

bool ProcessingElement::doCompute()
{
    output_->verbose(CALL_INFO, 0, 0, ">> Compute on %" PRIu32 " with %" PRIu32 "\n", processor_id_, op_binding_ );

    uint64_t intResult = 0x0F;

    std::bitset< Bit_Length > arg0;
    std::bitset< Bit_Length > arg1;
    std::bitset< Bit_Length > retVal;

    printInputQueue();
    printOutputQueue();

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
                retVal = std::bitset< Bit_Length >(intResult);
            }
            else if( op_binding_ == SUB ) {
                intResult = arg0.to_ullong() - arg1.to_ullong();
                retVal = std::bitset< Bit_Length >(intResult);
            }
            else if( op_binding_ == MUL ) {
                intResult = arg0.to_ullong() * arg1.to_ullong();
                retVal = std::bitset< Bit_Length >(intResult);
            }
            else if( op_binding_ == DIV ) {
                intResult = arg0.to_ullong() / arg1.to_ullong();
                retVal = std::bitset< Bit_Length >(intResult);
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
                std::memcpy(std::addressof(fpArg1), bufferA, size);

                const auto temp1 = arg1.to_ullong();
                std::memcpy(bufferB, std::addressof(temp1), size);
                std::memcpy(std::addressof(fpArg1), bufferB, size);

                fpResult = fpArg0 + fpArg1;

                std::memcpy(bufferA, std::addressof(fpResult), size);
                std::memcpy(std::addressof(intResult), bufferA, size);
                retVal = std::bitset< Bit_Length >(intResult);
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
                std::memcpy(std::addressof(fpArg1), bufferA, size);

                const auto temp1 = arg1.to_ullong();
                std::memcpy(bufferB, std::addressof(temp1), size);
                std::memcpy(std::addressof(fpArg1), bufferB, size);

                fpResult = fpArg0 - fpArg1;

                std::memcpy(bufferA, std::addressof(fpResult), size);
                std::memcpy(std::addressof(intResult), bufferA, size);
                retVal = std::bitset< Bit_Length >(intResult);
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
                std::memcpy(std::addressof(fpArg1), bufferA, size);

                const auto temp1 = arg1.to_ullong();
                std::memcpy(bufferB, std::addressof(temp1), size);
                std::memcpy(std::addressof(fpArg1), bufferB, size);

                fpResult = fpArg0 * fpArg1;

                std::memcpy(bufferA, std::addressof(fpResult), size);
                std::memcpy(std::addressof(intResult), bufferA, size);
                retVal = std::bitset< Bit_Length >(intResult);
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
                std::memcpy(std::addressof(fpArg1), bufferA, size);

                const auto temp1 = arg1.to_ullong();
                std::memcpy(bufferB, std::addressof(temp1), size);
                std::memcpy(std::addressof(fpArg1), bufferB, size);

                fpResult = fpArg0 / fpArg1;

                std::memcpy(bufferA, std::addressof(fpResult), size);
                std::memcpy(std::addressof(intResult), bufferA, size);
                retVal = std::bitset< Bit_Length >(intResult);
            }

        } else {
            //if no args available, do nothing
        }
    }

    if( num_outputs_ == 1 ) {
        output_queues_->at(0)->push(retVal);
    }

    if( num_inputs_ > 0 )
        printInputQueue();
    if( num_outputs_ > 0 )
        printOutputQueue();;

    std::cout << "intResult = " << intResult << std::endl;
    std::cout << "retVal = " << retVal << std::endl;

    return true;
}

bool ProcessingElement::doLoad(uint64_t addr)
{

//             void createLoadRequest( uint64_t addr, uint8_t reg ) {
    output_->verbose(CALL_INFO, 1, 0, "Creating a load from address: %" PRIu64 "\n", addr);
//
// 		if( addr >= maxAddr ) {
// 			output->fatal(CALL_INFO, -1, "Address requested: %" PRIu64 " but maximum address is: %" PRIu64 "\n",
// 				addr, maxAddr);
// 		}
//
    SimpleMem::Request* req = new SimpleMem::Request(SimpleMem::Request::Read, addr, 8);
    mem_interface_->sendRequest( req );
//
//                 JunoLoadStoreEntry* entry = new JunoLoadStoreEntry( req->id, reg );
//                 addEntry( entry );
//
//                 mem->sendRequest( req );
//             }

    return 1;

}

std::bitset< Bit_Length > ProcessingElement::popOutput()
{

    return true;
}

void ProcessingElement::printInputQueue()
{
    for( uint32_t i = 0; i < num_inputs_; ++i )
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
    for( uint32_t i = 0; i < num_outputs_; ++i )
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



