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

//#include <stdlib.h>
#include <vector>
#include <string>
#include <iostream>

#include "Allocator.h"
#include "Machine.h"
#include "misc.h"
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
bool Allocator::canAllocate(Job* j, std::vector<MeshLocation*>* available) 
{  
    return (available -> size() >= (unsigned int)j -> getProcsNeeded());
}

namespace SST {
    namespace Scheduler {
        void warning(std::string mesg) {        //report warning (program continues)
            std::cerr << "WARNING: " << mesg << std::endl;
        }

        void error(std::string mesg) {           //report user-caused error
            std::cerr << "ERROR: " << mesg << std::endl;
            exit(1);
        }

        void internal_error(std::string mesg) {  //report invalid program state
            std::cerr << "INTERNAL ERROR: " << mesg << std::endl;
            exit(1);
        }
    }
}
