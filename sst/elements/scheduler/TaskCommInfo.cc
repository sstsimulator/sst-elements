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

TaskCommInfo::TaskCommInfo( Job* job, int ** commMatrix)
{
	this -> job = job;
	job -> taskCommInfo = this;
	if(commMatrix == NULL){
		//init commMatrix
		int size = job -> getProcsNeeded();
		commMatrix = new int*[size];
		for(int i=0; i<size; i++){
			commMatrix[i] = new int[size];
		}
		//fill with all-to-all
		for(int i=0; i<size; i++){
			for(int j=i+1; j<size; j++){
				commMatrix[j][i] = 1;
			}
		}
	} else {
		this -> commMatrix = commMatrix;
	}
}

TaskCommInfo::~TaskCommInfo()
{
    for(int i = 0; i < job -> getProcsNeeded(); ++i) {
        delete [] commMatrix[i];
    }
    delete [] commMatrix;
}
