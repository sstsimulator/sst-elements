#include <sst_config.h>

#include <random>
#include <cstring>

#include "processingElement.h"

namespace SST {
namespace Llyr {

ProcessingElement::ProcessingElement(void)
{
}

ProcessingElement::ProcessingElement(opType op_binding, uint32_t processor_id, uint32_t queue_depth)  :
    processor_id_(processor_id), queue_depth_(queue_depth)
{
    char prefix[256];
    sprintf(prefix, "[t=@t][ProcessingElement]: ");
    output = new SST::Output(prefix, 0, 0, Output::STDOUT);

    input_queues_= new std::vector< std::queue< std::bitset< Bit_Length > >* >;
    output_queues_ = new std::vector< std::queue< std::bitset< Bit_Length > >* >;
    setOpBinding(op_binding);

    if( input_queues_->size() > 0 )
        fakeInit();
}

ProcessingElement::~ProcessingElement()
{
}

bool ProcessingElement::fakeInit()
{
    std::random_device some_rand;
    std::mt19937 generator( some_rand() );
    std::gamma_distribution<> gamma_dist(0, 100);

    for( uint32_t i = 0; i < input_queues_->size(); ++i )
    {
        int64_t my_int = gamma_dist( generator );
        std::bitset< Bit_Length > temp = std::bitset< Bit_Length >(my_int);
        input_queues_->at(i)->push(temp);
    }

    return true;
}

void ProcessingElement::setOpBinding(opType binding)
{
    op_binding_ = binding;

    switch( op_binding_ )
    {
        case DUMMY :
            num_inputs_ = 0;
            num_outputs_ = 0;
            break;
        case ANY :
            num_inputs_ = 2;
            num_outputs_ = 1;
            break;
        case LD :
        case ST :
        case ADD :
        case SUB :
        case MUL :
        case DIV :
        case FPADD :
        case FPSUB :
        case FPMUL :
        case FPDIV :
            num_inputs_ = 2;
            num_outputs_ = 1;
            break;
        case OTHER :
            num_inputs_ = 2;
            num_outputs_ = 1;
            break;
    }

    //Allocate queues for the inputs
    for( uint32_t numQueues = 0; numQueues < num_inputs_; ++numQueues )
    {
        std::queue< std::bitset< Bit_Length > >* tempQueue = new std::queue< std::bitset< Bit_Length > >;
        input_queues_->push_back(tempQueue);
    }

    //Allocate queues for the outputs
    for( uint32_t numQueues = 0; numQueues < num_outputs_; ++numQueues )
    {
        std::queue< std::bitset< Bit_Length > >* tempQueue = new std::queue< std::bitset< Bit_Length > >;
        output_queues_->push_back(tempQueue);
    }

}

bool ProcessingElement::doCompute()
{
    output->verbose(CALL_INFO, 0, 0, ">> Compute on %" PRIu32 " with %" PRIu32 "\n", processor_id_, op_binding_ );

    if( num_inputs_ == 1 )
    {
        if( input_queues_->at(0)->size() > 0 )
        {
        }
    }
    else if( num_inputs_ == 2 )
    {
        if( input_queues_->at(0)->size() > 0 && input_queues_->at(1)->size() > 0 )
        {
        }
    }


//     for( uint32_t i = 0; i < num_inputs_; ++i )
//     {
//         std::cout << i << ":  " << input_queues_->at(i)->size() << "\n";
//     }



    return true;
}

std::bitset< Bit_Length > ProcessingElement::popOutput()
{

    return true;
}


} // namespace llyr
} // namespace SST



