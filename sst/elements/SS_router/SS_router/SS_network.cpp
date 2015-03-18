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


// Copyright 2007 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2007, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>
#include "sst/core/serialization.h"

#include "SS_network.h"

#include <sst/core/debug.h>
#include <sst/core/params.h>

using namespace SST::SS_router;

Network::Network( SST::Params params )
{
    _xDimSize = params.find_integer("xDimSize");
    if ( _xDimSize == -1 ) {
        _abort(Network,"couldn't find xDimSize\n" );
    } 

    _yDimSize = params.find_integer("yDimSize");
    if ( _yDimSize == -1 ) {
        _abort(Network,"couldn't find yDimSize\n" );
    } 

    _zDimSize = params.find_integer("zDimSize");
    if ( _zDimSize == -1 ) {
        _abort(Network,"couldn't find zDimSize\n" );
    } 

    _size = _xDimSize*_yDimSize*_zDimSize;
#if 0
    _DBG(0,"size=%d xdim=%d ydim=%d xdim=%d\n",
           _size,_xDimSize,_yDimSize,_zDimSize);
#endif
}
BOOST_CLASS_EXPORT(RtrEvent)
