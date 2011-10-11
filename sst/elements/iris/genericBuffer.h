/*
 * =====================================================================================
// Copyright 2010 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2010, Sandia Corporation
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
        ~GenericBuffer(); 

        void push( irisNPkt*);
        irisNPkt* pull(uint16_t);
        irisNPkt* peek(uint16_t);
        uint16_t get_occupancy( uint16_t ) const;
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

#endif   /* ----- #ifndef _GENERICBUFFER_H_INC  ----- */
