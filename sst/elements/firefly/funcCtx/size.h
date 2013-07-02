// Copyright 2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef COMPONENTS_FIREFLY_FUNCCTX_SIZE_H
#define COMPONENTS_FIREFLY_FUNCCTX_SIZE_H

#include "functionCtx.h"

namespace SST {
namespace Firefly {

class SizeCtx : public FunctionCtx 
{ 
  public:
    SizeCtx(Hermes::Communicator group,
            int*                size,
            Hermes::Functor*    retFunc,
            FunctionType        type,
            Hades*              obj);
    bool run();
  private:
    int*                  m_size;
    Hermes::Communicator  m_group;
 };

}
}
#endif
