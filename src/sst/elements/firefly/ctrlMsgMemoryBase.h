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

#ifndef COMPONENTS_FIREFLY_CTRL_MSG_MEMORY_BASE_H
#define COMPONENTS_FIREFLY_CTRL_MSG_MEMORY_BASE_H

namespace SST {
namespace Firefly {
namespace CtrlMsg {

class MemoryBase : public SubComponent {
  public:
    typedef std::function<void()> Callback;
    typedef uint64_t MemAddr;

    MemoryBase( ComponentId_t id ) : SubComponent( id ) {}
    virtual ~MemoryBase() {}
    virtual void copy( Callback, MemAddr to, MemAddr from, size_t ) = 0;
    virtual void write( Callback, MemAddr to, size_t ) = 0;
    virtual void read( Callback, MemAddr to, size_t ) = 0;
    virtual void pin( Callback, MemAddr, size_t ) = 0;
    virtual void unpin( Callback, MemAddr, size_t ) = 0;
    virtual void walk( Callback, int count ) = 0;
};

}
}
}

#endif
