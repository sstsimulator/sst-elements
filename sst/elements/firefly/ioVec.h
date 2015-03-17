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

namespace SST {
namespace Firefly {

struct IoVec {
    IoVec() {}
    IoVec( void* _ptr, size_t _size ) : 
        ptr( _ptr ), len( _size ) {}
    void*  ptr;
    size_t len;
};
}
}

#endif
