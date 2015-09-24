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

#ifndef COMPONENTS_FIREFLY_IOVEC_H
#define COMPONENTS_FIREFLY_IOVEC_H

#include <stddef.h>
#include <sst/elements/hermes/msgapi.h>

using namespace Hermes::MP;

namespace SST {
namespace Firefly {

class IoVec {
  public:
    IoVec() : m_len( 0 ) {}
    IoVec( Addr addr, size_t size ) : 
        m_addr( addr ), m_len( size ) {}

    Addr addr() { return m_addr; }
    size_t len() { return m_len; }


    void setAddr( Addr addr ) { m_addr = addr; }
    void setLen(size_t len ) { m_len = len; }
  private:
    Addr m_addr;
    size_t m_len;
};
}
}

#endif
