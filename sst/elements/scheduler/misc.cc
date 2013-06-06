// Copyright 2009-2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2013, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

/*
 * Miscellaneous code (outside a class or too small a class for its own file)
 */

#include <stdlib.h>
#include <vector>
#include <string>
#include <iostream>

using namespace std;

#include "sst/core/serialization/element.h"

#include "misc.h"
#include "Allocator.h"
#include "Machine.h"
#include "Job.h"

using namespace SST::Scheduler;

//returns whether j can be allocated
//default strategies are non-contig so "true" if enough free processors
bool Allocator::canAllocate(Job* j) 
{  
    return (machine -> getNumFreeProcessors() >= j -> getProcsNeeded());
}


//returns whether j can be allocated
//default strategies are non-contig so "true" if enough free processors
bool Allocator::canAllocate(Job* j, vector<MeshLocation*>* available) 
{  
    return (available -> size() >= (unsigned int)j -> getProcsNeeded());
}

namespace SST {
    namespace Scheduler {
        void warning(string mesg) {        //report warning (program continues)
            cerr << "WARNING: " << mesg << endl;
        }

        void error(string mesg) {           //report user-caused error
            cerr << "ERROR: " << mesg << endl;
            exit(1);
        }

        void internal_error(string mesg) {  //report invalid program state
            cerr << "INTERNAL ERROR: " << mesg << endl;
            exit(1);
        }
    }
}
