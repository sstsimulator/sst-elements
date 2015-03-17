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


#ifndef _H_EMBER_MPI_EVENT
#define _H_EMBER_MPI_EVENT

#include "emberevent.h" 

using namespace Hermes;
using namespace Hermes::MP;

namespace SST {
namespace Ember {

class EmberMPIEvent : public EmberEvent {

  public:

    EmberMPIEvent( MP::Interface& api, Output* output, Histo* histo = NULL):
        EmberEvent( output, histo ), m_api( api )
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
