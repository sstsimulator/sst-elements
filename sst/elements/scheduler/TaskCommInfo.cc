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

#include <stdlib.h>

#include "Job.h"
#include "output.h"

using namespace SST::Scheduler;

TaskCommInfo::TaskCommInfo(Job* job)
{
    init(job);
    taskCommType = TaskCommInfo::ALLTOALL;
    commInfo = NULL;
    coordMatrix = NULL;
    xdim = 0;
    ydim = 0;
    zdim = 0;
}

TaskCommInfo::TaskCommInfo(Job* job, std::vector<std::vector<std::vector<int>*> >* inCommInfo)
{
    init(job);
    taskCommType = TaskCommInfo::CUSTOM;
    commInfo = inCommInfo;
    coordMatrix = NULL;
    xdim = 0;
    ydim = 0;
    zdim = 0;
}

TaskCommInfo::TaskCommInfo(Job* job, int xdim, int ydim, int zdim)
{
    init(job);
    taskCommType = TaskCommInfo::MESH;
    commInfo = NULL;
    coordMatrix = NULL;
    this->xdim = xdim;
    this->ydim = ydim;
    this->zdim = zdim;
}

TaskCommInfo::TaskCommInfo(Job* job, std::vector<std::vector<std::vector<int>*> >* inCommInfo, double** inCoords)
{
    init(job);
    taskCommType = TaskCommInfo::COORDINATE;
    commInfo = inCommInfo;
    coordMatrix = inCoords;
    this->xdim = 0;
    this->ydim = 0;
    this->zdim = 0;
}

TaskCommInfo::TaskCommInfo(const TaskCommInfo& tci)
{
    size = tci.size;
    taskCommType = tci.taskCommType;
    xdim = tci.xdim;
    ydim = tci.ydim;
    zdim = tci.zdim;
    
    if(taskCommType == CUSTOM || taskCommType == COORDINATE){
        commInfo = new std::vector<std::vector<std::vector<int>*> >(2);
        commInfo->at(0).resize(size);
        commInfo->at(1).resize(size);
        for(unsigned int i = 0; i < size; i++){
            commInfo->at(0)[i] = new std::vector<int>(tci.commInfo->at(0)[i]->size());
            commInfo->at(1)[i] = new std::vector<int>(tci.commInfo->at(0)[i]->size());
            for(unsigned int j = 0; j < tci.commInfo->at(0)[i]->size(); j++){
                commInfo->at(0)[i]->push_back(tci.commInfo->at(0)[i]->at(j));
                commInfo->at(1)[i]->push_back(tci.commInfo->at(1)[i]->at(j));
            }
        }
    } else {
        commInfo = NULL;
    }
    
    if(taskCommType == COORDINATE){
        double ** coordMatrix = new double*[size];
        for(unsigned int i = 0; i < size; i++){
            coordMatrix[i] = new double[3];
            for(int j = 0; j < 3; j++){
                coordMatrix[i][j] = tci.coordMatrix[i][j];
            }
        }
    } else {
        coordMatrix = NULL;
    }
}

void TaskCommInfo::init(Job* job)
{
    job->taskCommInfo = this;
    size = job -> getProcsNeeded();
}

TaskCommInfo::~TaskCommInfo()
{
    if(commInfo != NULL){
        for(unsigned int i = 0; i < size; i++){
            delete commInfo->at(0)[i];
            delete commInfo->at(1)[i];
        }
        delete commInfo;
    }
    if(coordMatrix != NULL){
        for(unsigned int i = 0; i < size; ++i) {
            delete [] coordMatrix[i];
        }
        delete [] coordMatrix;
    }
}

int** TaskCommInfo::getCommMatrix() const
{
    int **outMatrix;
    switch(taskCommType){
    case ALLTOALL:
        outMatrix = buildAllToAllMatrix(size);
        break;
    case MESH:
        outMatrix = buildMeshMatrix();
        break;
    case CUSTOM:
    case COORDINATE:
        outMatrix = buildCustomMatrix();
        break;
    default:
        schedout.fatal(CALL_INFO, 1, "Unknown Communication type");
        outMatrix = NULL;
    }
    return outMatrix;
}

std::vector<std::vector<int> >* TaskCommInfo::getCommOfTask(unsigned int taskNo) const
{
    std::vector<std::vector<int> >* retVec = new std::vector<std::vector<int> >(2);
    unsigned int cnt;
    switch(taskCommType){
    case ALLTOALL:
        retVec->at(0).resize(size - 1);
        retVec->at(1).resize(size - 1);
        cnt = 0;
        for(unsigned int i = 0; i < size; i++){
            if(i != taskNo){
                retVec->at(0)[cnt] = i;
                retVec->at(1)[cnt] = 1;
                cnt++;
            }
        }
        break;
    case MESH:
        int taskDims[3];
        getTaskDims(taskNo, taskDims);
        //x-neighbors
        if(taskDims[0] != 0){
            retVec->at(0).push_back(taskNo - 1);
            retVec->at(1).push_back(1);
        }
        if(taskDims[0] != xdim - 1){
            retVec->at(0).push_back(taskNo + 1);
            retVec->at(1).push_back(1);
        }
        //y-neighbors
        if(taskDims[1] != 0){
            retVec->at(0).push_back(taskNo - xdim);
            retVec->at(1).push_back(1);
        }
        if(taskDims[1] != ydim - 1){
            retVec->at(0).push_back(taskNo + xdim);
            retVec->at(1).push_back(1);
        }
        //z-neighbors
        if(taskDims[2] != 0){
            retVec->at(0).push_back(taskNo - (xdim * ydim));
            retVec->at(1).push_back(1);
        }
        if(taskDims[1] != zdim - 1){
            retVec->at(0).push_back(taskNo + (xdim * ydim));
            retVec->at(1).push_back(1);
        }
        break;
    case CUSTOM:
    case COORDINATE:
        retVec->at(0).resize(commInfo->at(0)[taskNo]->size());
        retVec->at(1).resize(commInfo->at(0)[taskNo]->size());
        for(unsigned int i = 0; i < commInfo->at(0)[taskNo]->size(); i++){
            retVec->at(0)[i] = commInfo->at(0)[taskNo]->at(i);
            retVec->at(1)[i] = commInfo->at(1)[taskNo]->at(i);
        }
        break;
    default:
        schedout.fatal(CALL_INFO, 1, "Unknown Communication type");
        retVec = NULL;
    }

    return retVec;
}

int TaskCommInfo::getCommWeight(int task0, int task1) const
{
    int dist = 0;
    if(taskCommType == TaskCommInfo::MESH){
        int task0Dims[3];
        getTaskDims(task0, task0Dims);
        int task1Dims[3];
        getTaskDims(task1, task1Dims);
        int tempDist = 0;
        for(int i = 0; i < 3; i++){
            tempDist += abs(task0Dims[i] - task1Dims[i]);
        }
        if(tempDist == 1){
            dist = 1;
        }
    } else if (taskCommType == TaskCommInfo::ALLTOALL) {
        dist = 1;
    } else {
        for(unsigned int i = 0; i < commInfo->at(0)[task0]->size(); i++){
            if(commInfo->at(0)[task0]->at(i) == task1){
                dist = commInfo->at(1)[task0]->at(i);
                break;
            }
        }
    }
    return dist;
}

int** TaskCommInfo::buildAllToAllMatrix(int size) const
{
    int** outMatrix = new int*[size];
    for(int i = 0; i < size; i++){
        outMatrix[i] = new int[size];
        for(int j = 0; j < size; j++){
            outMatrix[i][j] = 1;
        }
    }
    return outMatrix;
}

int** TaskCommInfo::buildMeshMatrix() const
{
    int ** outMatrix;
    //initialize matrix
    int size = xdim * ydim * zdim;
    outMatrix = new int*[size];
    for(int i = 0; i < size; i++){
        outMatrix[i] = new int[size];
        for(int j = 0; j < size; j++){
            outMatrix[i][j] = 0;
        }
    }
    //index dimensions of matrix elements
    int x_ind[size];
    int y_ind[size];
    int z_ind[size];
    for(int i = 0; i < size; i++){
        int dims[3];
        getTaskDims(i, dims);
        x_ind[i] = dims[0];
        y_ind[i] = dims[1];
        z_ind[i] = dims[2];
    }
    //set neighbor communication
    for(int i = 0; i < size; i++){
        //x_dim
        if(x_ind[i] != 0){
            outMatrix[i][i-1] = 1;
            outMatrix[i-1][i] = 1;
        }
        if((x_ind[i] + 1) != xdim){
            outMatrix[i+1][i] = 1;
            outMatrix[i][i+1] = 1;
        }
        //y_dim
        if(y_ind[i] != 0){
            outMatrix[i][i-xdim] = 1;
            outMatrix[i-xdim][i] = 1;
        }
        if((y_ind[i] + 1) != ydim){
            outMatrix[i+xdim][i] = 1;
            outMatrix[i][i+xdim] = 1;
        }
        //z_dim
        if(z_ind[i] != 0){
            outMatrix[i][i-(xdim*ydim)] = 1;
            outMatrix[i-(xdim*ydim)][i] = 1;
        }
        if((z_ind[i] + 1) != zdim){
            outMatrix[i+(xdim*ydim)][i] = 1;
            outMatrix[i][i+(xdim*ydim)] = 1;
        }
    }
    return outMatrix;
}

int** TaskCommInfo::buildCustomMatrix() const
{
    //init matrix
    int** outMatrix = new int*[size];
    for(unsigned int i = 0; i < size; i++){
        outMatrix[i] = new int[size];
        for(unsigned int j = 0; j < size; j++){
            outMatrix[i][j] = 0;
        }
    }

    //write data
    for(unsigned int taskIt = 0; taskIt < commInfo->at(0).size(); ++taskIt) {
        for(unsigned int adjIt = 0; adjIt < commInfo->at(0)[taskIt]->size(); adjIt++){
            outMatrix[taskIt][commInfo->at(0)[taskIt]->at(adjIt)] = commInfo->at(1)[taskIt]->at(adjIt);
        }
    }

    return outMatrix;
}

void TaskCommInfo::getTaskDims(int taskNo, int outDims[3]) const
{
    outDims[0] = taskNo % xdim;
    outDims[1] = (taskNo / xdim) % ydim;
    outDims[2] = taskNo / (xdim*ydim);
}

