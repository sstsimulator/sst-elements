// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_EMBER_SHMEM_EVENT
#define _H_EMBER_SHMEM_EVENT

#include "emberevent.h"

using namespace Hermes;

namespace SST {
namespace Ember {

typedef Statistic<uint32_t> EmberEventTimeStatistic;

class EmberShmemEvent : public EmberEvent {

  public:

    EmberShmemEvent( Shmem::Interface& api, Output* output,
            EmberEventTimeStatistic* stat = NULL ):
        EmberEvent( output, stat ), m_api( api )
    {
        m_state = IssueCallback;
    }

  protected:

    Shmem::Interface&   m_api;

  private:
};

}
}

#endif
