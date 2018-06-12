// Copyright 2013-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _H_HERMES_MISC_INTERFACE
#define _H_HERMES_MISC_INTERFACE

#include <assert.h>

#include "hermes.h"

namespace SST {
namespace Hermes {
namespace Misc {

class Interface : public Hermes::Interface {
    public:

    Interface( Component* parent ) : Hermes::Interface(parent) {}

    virtual void getNodeNum( int*, Callback) { assert(0); }
    virtual void getNumNodes( int*, Callback) { assert(0); }
};

}
}
}

#endif
