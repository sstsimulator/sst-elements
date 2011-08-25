/*
 * =====================================================================================
//       Filename:  genericBuffer.cc
 * =====================================================================================
 */

#ifndef  _GENERICBUFFER_CC_INC
#define  _GENERICBUFFER_CC_INC

#include	"genericBuffer.h"

GenericBuffer::GenericBuffer ()// : Router_params()
{
    buffers.resize(r_param.vcs);
}  /* -----  end of method GenericBuffer::GenericBuffer  (constructor)  ----- */

GenericBuffer::~GenericBuffer ()
{
}		/* -----  end of method GenericBuffer::~GenericBuffer  ----- */


void
GenericBuffer::push ( Flit* f, uint16_t push_channel )
{
    assert( buffers[push_channel].size() > 0 
            && buffers[push_channel].size() <=r_param.buffer_size 
            && "Pushed flit into buffer. Buffer overflow!");
    buffers[push_channel].push_back(f);
    return ;
}		/* -----  end of method GenericBuffer::push  ----- */

Flit*
GenericBuffer::pull ( uint16_t pull_channel )
{
    assert ( pull_channel < buffers.size() 
             && buffers[pull_channel].size() != 0 
             && " Incorrect pull channel! pull channel>no of channels");

    Flit* f = buffers[pull_channel].front();
    buffers[pull_channel].pop_front();
    return f;
}		/* -----  end of method GenericBuffer::pull  ----- */

/* !
 * * === FUNCTION
 * ======================================================================
 * * Name: peek
 * * Description: Usage is to get a pointer to the head of the buffer
 * * on the specified channel without actually pulling it out from the buffer.
 * =====================================================================================
 * */
Flit*
GenericBuffer::peek ( uint16_t peek_channel)
{
    assert ( peek_channel < buffers.size() 
             && buffers[peek_channel].size() != 0 
             && " Incorrect peek channel! peek channel>no of channels");

    Flit* f = buffers[peek_channel].front();
    return f;
}		/* -----  end of method GenericBuffer::peek  ----- */

uint16_t
GenericBuffer::get_occupancy ( uint16_t channel ) const
{
    return buffers[channel].size();
}		/* -----  end of method GenericBuffer::get_occupancy  ----- */

/* !
 * * === FUNCTION
 * ======================================================================
 * * Name: is_buffer_full 
 * * Description: Function to check if the number of flits in the buffer is greater than the
 * * buffer depth.
 * =====================================================================================
 * */
bool
GenericBuffer::is_buffer_full ( uint16_t channel ) const
{
    return buffers[channel].size() > r_param.buffer_size;
}		/* -----  end of method GenericBuffer::is_channel_full  ----- */

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  is_buffer_empty
 *  Description: Function to check if there is a flit in the buffer on the
 *  specified channel 
 * =====================================================================================
 */
bool
GenericBuffer::is_buffer_empty ( uint16_t channel ) const
{
    return buffers[channel].empty();
}		/* -----  end of method GenericBuffer::is_empty  ----- */

#endif   /* ----- #ifndef _GENERICBUFFER_CC_INC  ----- */
