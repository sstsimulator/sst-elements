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

#include <sst_config.h>
#include "embergen.h"

using namespace SST::Ember;

void* EmberGenerator::memAlloc( size_t size )
{
    void *ret = NULL;
    switch ( m_dataMode  ) {
      case Backing:
        ret = malloc( size );
        break;
      case BackingZeroed: 
        ret = malloc( size );
        memset( ret, 0, size ); 
        break;
      case NoBacking:
        break;
    } 
    return ret;
}

void EmberGenerator::memFree( void* ptr )
{
    switch ( m_dataMode  ) {
      case Backing:
      case BackingZeroed: 
        free( ptr );
        break;
      case NoBacking:
        break;
    } 
}


