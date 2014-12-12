// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2014, Sandia Corporation
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

void EmberGenerator::printHistogram(const Output* output, Histo* histo )
{
    output->output("Histogram Min: %" PRIu32 "\n", histo->getBinStart());
    output->output("Histogram Max: %" PRIu32 "\n", histo->getBinEnd());
    output->output("Histogram Bin: %" PRIu32 "\n", histo->getBinWidth());
    for(uint32_t i = histo->getBinStart(); i <= histo->getBinEnd();
                                            i += histo->getBinWidth()) {

        if( histo->getBinCountByBinStart(i) > 0 ) {
            output->output(" [%" PRIu32 ", %" PRIu32 "]   %" PRIu32 "\n",
                i, (i + histo->getBinWidth()), histo->getBinCountByBinStart(i));
        }
    }
}

