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

#include "TaskCommInfo.h"

#include "Job.h"

using namespace SST::Scheduler;

TaskCommInfo::TaskCommInfo(Job* job)
{
    init(job);
    taskCommType = TaskCommInfo::ALLTOALL;
    commMatrix = NULL;
    coords = new std::vector<double*>();
    xdim = 0;
    ydim = 0;
    zdim = 0;
}

TaskCommInfo::TaskCommInfo(Job* job, int ** inCommMatrix)
{
    init(job);
    taskCommType = TaskCommInfo::CUSTOM;
    commMatrix = inCommMatrix;
    coords = new std::vector<double*>();
    xdim = 0;
    ydim = 0;
    zdim = 0;
}

TaskCommInfo::TaskCommInfo(Job* job, int xdim, int ydim, int zdim)
{
    init(job);
    taskCommType = TaskCommInfo::MESH;
    commMatrix = buildMeshComm(xdim, ydim, zdim);
    coords = new std::vector<double*>();
    this->xdim = xdim;
    this->ydim = ydim;
    this->zdim = zdim;
}

TaskCommInfo::TaskCommInfo(Job* job, int ** inCommMatrix, std::vector<double*> * inCoords)
{
    init(job);
    taskCommType = TaskCommInfo::COORDINATE;
    commMatrix = inCommMatrix;
    coords = inCoords;
    this->xdim = xdim;
    this->ydim = ydim;
    this->zdim = zdim;
}

void TaskCommInfo::init(Job* job)
{
    job->taskCommInfo = this;
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
    if(!coords->empty()){
        for(unsigned int i = 0; i < coords->size(); ++i) {
            delete [] coords->at(i);
        }
        delete coords;
    }
}

int** TaskCommInfo::buildMeshComm(int xdim, int ydim, int zdim) const
{
    int ** outMatrix;
    //initialize matrix
    int size = xdim * ydim * zdim;
    outMatrix = new int*[size];
    for(int i=0; i < size; i++){
        outMatrix[i] = new int[size];
        for(int j=0; j < size; j++){
            outMatrix[i][j] = 0;
        }
    }
    //index dimensions of matrix elements
    int x_ind[size];
    int y_ind[size];
    int z_ind[size];
    for(int i=0; i<size; i++){
        x_ind[i] = i % xdim;
        y_ind[i] = (i / xdim) % ydim;
        z_ind[i] = i / (xdim*ydim);
    }
    //set neighbor communication
    for(int i=0; i<size; i++){
        //x_dim
        if(x_ind[i] != 0)
            outMatrix[i][i-1] = 1;
        if((x_ind[i] + 1) != xdim)
            outMatrix[i+1][i] = 1;
        //y_dim
        if(y_ind[i] != 0)
            outMatrix[i][i-xdim] = 1;
        if((y_ind[i] + 1) != ydim)
            outMatrix[i+xdim][i] = 1;
        //z_dim
        if(z_ind[i] != 0)
            outMatrix[i][i-(xdim*ydim)] = 1;
        if((z_ind[i] + 1) != zdim)
            outMatrix[i+(xdim*ydim)][i] = 1;
    }
    return outMatrix;
}
