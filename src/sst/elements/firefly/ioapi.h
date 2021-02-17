// Copyright 2013-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
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
#include "ioVec.h"

namespace SST {
namespace Firefly {
namespace IO {

typedef uint32_t NodeId;
static const NodeId AnyId = -1;

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

    virtual NodeId getNodeId() { assert(0); };
    virtual NodeId peek() { assert(0); }
    virtual bool sendv(NodeId dest, std::vector<IoVec>&, Entry::Functor*)
                                            { assert(0); }
    virtual bool recvv(NodeId src, std::vector<IoVec>&, Entry::Functor*)
                                            { assert(0); }
    virtual void wait() { assert(0);}
    virtual void setReturnLink( SST::Link* ) { assert(0); }
};

}
}
}

#endif
