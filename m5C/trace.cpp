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

#include "trace.h"

#include <iostream>
#include <sstream>
#include <boost/mpi.hpp>

namespace SST {
namespace M5 {

_Trace __trace;

static __attribute__ ((constructor)) void init(void)
{
#if 0   /* THIS DOES NOT WORK IN A STATIC BUILD. */
    boost::mpi::communicator world;
    std::ostringstream tmp;
    tmp << world.rank();

//    TRACE_SET_PREFIX( tmp.str() + ":" );
#endif
}


}
}
