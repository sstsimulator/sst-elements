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

#ifndef  _GENERICBUFFER_H_INC
#define  _GENERICBUFFER_H_INC

#include        <sst/core/sst_types.h>
#include	"router_params.h"
#include        "SST_interface.h"

namespace SST {
namespace Iris {
/*
 * =====================================================================================
 *        Class:  GenericBuffer
 *  Description:  Implements a generic buffer with virtual channels. Used for
 *  the input and output buffers of the router.
 * =====================================================================================
 */
extern Router_params* r_param; 
class GenericBuffer 
{
    public:
        GenericBuffer ();
	 GenericBuffer (int vcs);
        ~GenericBuffer(); 

        void push( irisNPkt*);
        irisNPkt* pull(uint16_t);
        irisNPkt* peek(uint16_t);
        inline uint16_t get_occupancy ( uint16_t channel ) const
        {
            return buffers[channel].size();
        }

	inline uint16_t get_pktLength ( uint16_t channel ) const
        {
	  assert (buffers[channel].front()!=NULL && "Error packet is NULL\n");
	  return buffers[channel].front()->sizeInFlits;
        }

        bool is_buffer_full( uint16_t ) const;
        bool is_buffer_empty( uint16_t ) const;

        /* 
         * TODO: Implement helper functions
         std::string toString() const;
         * */

    protected:

    private:
        std::vector < std::deque<irisNPkt*> > buffers;

}; /* -----  end of class GenericBuffer  ----- */

}
}
#endif   /* ----- #ifndef _GENERICBUFFER_H_INC  ----- */
