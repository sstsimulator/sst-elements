// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_EMBER_MPI_EVENT
#define _H_EMBER_MPI_EVENT

#include <sst/core/statapi/statbase.h>
#include "emberevent.h"

using namespace Hermes;
using namespace Hermes::MP;

namespace SST {
namespace Ember {

using TimeStatDataType = uint32_t;
using EventTimeStat = Statistic<TimeStatDataType>;

#define FOREACH_MPI_EVENT(NAME) \
    NAME( Init ) \
    NAME( Finalize ) \
    NAME( Rank ) \
    NAME( Size ) \
    NAME( Send ) \
    NAME( Recv ) \
    NAME( Irecv ) \
    NAME( Isend ) \
    NAME( Wait ) \
    NAME( Waitall ) \
    NAME( Waitany ) \
    NAME( Compute ) \
    NAME( Barrier ) \
    NAME( Alltoallv ) \
    NAME( Alltoall ) \
    NAME( Allreduce ) \
    NAME( Reduce ) \
    NAME( Bcast) \
    NAME( Scatter) \
    NAME( Scatterv) \
    NAME( Gettime ) \
    NAME( Commsplit ) \
    NAME( Commcreate ) \
    NAME( NUM_MPI_EVENTS ) \

#define GENERATE_ENUM(ENUM) ENUM,

enum MPIEvents {
    FOREACH_MPI_EVENT(GENERATE_ENUM)
};

#undef GENERATE_ENUM

extern const char* MPIEventNames[];

class EmberMPIEvent : public EmberEvent {

  public:

    EmberMPIEvent( MP::Interface& api, Output* output,
                  EmberEventTimeStatistic* stat = NULL):
        EmberEvent( output, stat ), m_api( api )
    {
        m_state = IssueFunctor;
    }

  protected:

    MP::Interface&   m_api;

  private:
};

}
}

#endif
