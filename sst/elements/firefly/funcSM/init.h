// Copyright 2013-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2015, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef COMPONENTS_FIREFLY_FUNCSM_INIT_H
#define COMPONENTS_FIREFLY_FUNCSM_INIT_H

#include "funcSM/api.h"

namespace SST {
namespace Firefly {

class InitFuncSM :  public FunctionSMInterface 
{
  public:
    InitFuncSM( SST::Params& params ) : FunctionSMInterface( params ) {}

    virtual void handleStartEvent( SST::Event *e, Retval& retval ) {

        m_dbg.verbose(CALL_INFO,1,0,"\n");

        retval.setExit(0);

        delete e;
    }
};

}
}

#endif
