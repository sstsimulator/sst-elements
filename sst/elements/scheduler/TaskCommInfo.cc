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

#include "Job.h"

#include "TaskCommInfo.h"

using namespace SST::Scheduler;

TaskCommInfo::TaskCommInfo( Job* job, int ** inCommMatrix, int xdim, int ydim, int zdim)
{
	job->taskCommInfo = this;
	this->commMatrix = inCommMatrix;
	this->xdim = xdim;
	this->ydim = ydim;
	this->zdim = zdim;
	size = job -> getProcsNeeded();
}

TaskCommInfo::~TaskCommInfo()
{
    if(commMatrix != NULL){
        for(int i = 0; i < size; ++i) {
            delete [] commMatrix[i];
        }
        delete [] commMatrix;
    }
}
