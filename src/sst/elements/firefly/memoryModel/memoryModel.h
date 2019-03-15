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

#ifndef COMPONENTS_FIREFLY_MEMORY_MODEL_H
#define COMPONENTS_FIREFLY_MEMORY_MODEL_H

class MemoryModel : public SubComponent {

public:

   typedef std::function<void()> Callback;

#include "memOp.h"

    MemoryModel( Component* comp ) : SubComponent(comp) {}

    virtual void printStatus( Output& out, int id ) { }
	virtual void schedHostCallback( int core, std::vector< MemOp >* ops, Callback callback ) = 0;
	virtual void schedNicCallback( int unit, int pid, std::vector< MemOp >* ops, Callback callback ) = 0;
};

#endif
