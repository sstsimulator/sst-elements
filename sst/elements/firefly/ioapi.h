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

#ifndef COMPONENTS_FIREFLY_IO_H
#define COMPONENTS_FIREFLY_IO_H

#include <sst/core/module.h>
#include <sst/core/component.h>

#include <stdint.h>
#include <stddef.h>
#include <vector>

#include "sst/elements/hermes/functor.h"

namespace SST {
namespace Firefly {
namespace IO { 

typedef uint32_t NodeId;
static const NodeId AnyId = -1;

struct IoVec {
    void*  ptr;
    size_t len;
};

class Entry {
  public:
    typedef VoidArg_FunctorBase< Entry* > Functor;
    Entry() : callback(NULL) {}
    virtual ~Entry() { if ( callback ) delete callback; }

    Functor* callback; 
};

typedef Arg_FunctorBase< NodeId >     Functor2;

class Interface : public SST::Module {

  public:

    Interface() { }
    virtual ~Interface() { }
     virtual void _componentInit(unsigned int phase ) {}

    virtual NodeId getNodeId() {};
    virtual void setDataReadyFunc(Functor2*) { };
    virtual size_t peek(NodeId& src) { }
    virtual bool isReady(NodeId dest) { } 
    virtual bool sendv(NodeId dest, std::vector<IoVec>&, Entry::Functor*) { }
    virtual bool recvv(NodeId src, std::vector<IoVec>&, Entry::Functor*) { }
};

}
}
}

#endif
