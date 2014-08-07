// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "AllocMapper.h"

#include "output.h"

using namespace SST::Scheduler;

//set aside memory for mappings
std::map<long, std::vector<int>*> AllocMapper::mappings = std::map<long, std::vector<int>*>();

void AllocMapper::addMapping(long jobNum, std::vector<int>* data)
{
    AllocMapper::mappings[jobNum] = data;
}

std::vector<int>* AllocMapper::getMappingOf(long jobNum)
{
    if(mappings.count(jobNum) == 0){
        schedout.fatal(CALL_INFO, 1, "AllocMapper: task mapping for job %ld called before allocation\n", jobNum);
    }
    std::vector<int>* retVal = mappings[jobNum];
    mappings.erase(jobNum);
    return retVal;
}

