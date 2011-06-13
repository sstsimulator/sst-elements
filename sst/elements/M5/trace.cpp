#include "trace.h"

#include <iostream>
#include <sstream>
#include <boost/mpi.hpp>

Trace __trace;

static __attribute__ ((constructor)) void init(void)
{
    boost::mpi::communicator world;
    std::ostringstream tmp;
    tmp << world.rank();

    TRACE_SET_PREFIX( tmp.str() + ":" );
}
