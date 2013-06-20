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
        
#include "sst_config.h"
#include "AllocInfo.h"

#include <string>

#include "Job.h"

using namespace SST::Scheduler;

AllocInfo::AllocInfo(Job* job) 
{
  this -> job = job;
  nodeIndices = new int[job -> getProcsNeeded()];
  nodeIndices[0] = -1; // ConstraintAllocator puts allocation here
}

AllocInfo::~AllocInfo() 
{
  delete [] nodeIndices;
}

std::string AllocInfo::getProcList() 
{
  return "";
}


