/*
 * =====================================================================================
// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.
 * =====================================================================================
 */

/*
 * =====================================================================================
//       Filename:  genericBuffer.cc
 * =====================================================================================
 */

#ifndef  _GENERICBUFFER_CC_INC
#define  _GENERICBUFFER_CC_INC

#include <sst_config.h>
#include	"genericBuffer.h"

using namespace SST::Iris;

GenericBuffer::GenericBuffer ()
{
    buffers.resize(r_param->vcs);
}  /* -----  end of method GenericBuffer::GenericBuffer  (constructor)  ----- */

GenericBuffer::GenericBuffer (int vcs)
{
    buffers.resize(vcs);
}  /* -----  end of method GenericBuffer::GenericBuffer  (constructor)  ----- */

GenericBuffer::~GenericBuffer ()
{
}		/* -----  end of method GenericBuffer::~GenericBuffer  ----- */


void
GenericBuffer::push ( irisNPkt* f)
{
    uint16_t push_channel = f->vc;
    buffers[push_channel].push_back(f);
    assert( push_channel< buffers.size() && " incorrect vc GenericBuffer::push\n");
    assert( buffers.at(push_channel).size() <= r_param->buffer_size && "Pushed flit into buffer. Buffer overflow!");
    return ;
}		/* -----  end of method GenericBuffer::push  ----- */

irisNPkt*
GenericBuffer::pull ( uint16_t pull_channel )
{
    assert( buffers.size() > 0 
            && pull_channel < buffers.size() 
            && " Incorrect pull channel! pull channel>no of channels");

    irisNPkt* f = buffers[pull_channel].front();
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
irisNPkt*
GenericBuffer::peek ( uint16_t peek_channel)
{
    assert ( peek_channel < buffers.size() 
             && buffers[peek_channel].size() != 0 
             && " Incorrect peek channel! peek channel>no of channels");

    irisNPkt* f = buffers[peek_channel].front();
    return f;
}		/* -----  end of method GenericBuffer::peek  ----- */

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
    return buffers[channel].size() > r_param->buffer_size;
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
