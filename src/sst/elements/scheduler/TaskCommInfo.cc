// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
// 
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
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
    : xdim(0), ydim(0), zdim(0), centerTask(0)
{
    init(job);
    taskCommType = TaskCommInfo::ALLTOALL;
    commInfo = NULL;
    coordMatrix = NULL;
}

TaskCommInfo::TaskCommInfo(Job* job, std::vector<std::map<int,int> >* inCommInfo, int inCenterTask)
    : xdim(0), ydim(0), zdim(0), centerTask(inCenterTask)
{
    init(job);
    taskCommType = TaskCommInfo::CUSTOM;
    commInfo = inCommInfo;
    coordMatrix = NULL;
}

TaskCommInfo::TaskCommInfo(Job* job, int inx, int iny, int inz, int inCenterTask)
    : xdim(inx), ydim(iny), zdim(inz), centerTask(inCenterTask)
{
    init(job);
    taskCommType = TaskCommInfo::MESH;
    commInfo = NULL;
    coordMatrix = NULL;
}

TaskCommInfo::TaskCommInfo(Job* job, std::vector<std::map<int,int> >* inCommInfo, double** inCoords, int inCenterTask)
    : xdim(0), ydim(0), zdim(0), centerTask(inCenterTask)
{
    init(job);
    taskCommType = TaskCommInfo::COORDINATE;
    commInfo = inCommInfo;
    coordMatrix = inCoords;
}

TaskCommInfo::TaskCommInfo(const TaskCommInfo& tci)
    : xdim(tci.xdim), ydim(tci.ydim), zdim(tci.zdim), centerTask(tci.centerTask)
{
    size = tci.size;
    taskCommType = tci.taskCommType;
    
    if(taskCommType == CUSTOM || taskCommType == COORDINATE){
        commInfo = new std::vector<std::map<int,int> >(size);
        for(unsigned int i = 0; i < size; i++){
            for(std::map<int, int>::iterator it = tci.commInfo->at(i).begin(); it != tci.commInfo->at(i).end(); it++){
                commInfo->at(i)[it->first] = it->second;
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
        schedout.fatal(CALL_INFO, 1, "Unknown Communication type\n");
        outMatrix = NULL;
    }
    return outMatrix;
}

std::vector<std::map<int,int> >* TaskCommInfo::getCommInfo() const
{
    std::vector<std::map<int,int> >* retVec = new std::vector<std::map<int,int> >(size);

    switch(taskCommType){
    case ALLTOALL:
        for(unsigned int taskIt = 0; taskIt < size; taskIt++){
            for(unsigned int otherIt = 0; otherIt < size; otherIt++){
                if(otherIt != taskIt){
                    retVec->at(taskIt)[otherIt] = 1;
                }
            }
        }
        break;
    case MESH:
        for(unsigned int taskIt = 0; taskIt < size; taskIt++){
            for(unsigned int otherIt = 0; otherIt < size; otherIt++){
                int weight = getCommWeight(taskIt, otherIt);
                if(weight != 0){
                    retVec->at(taskIt)[otherIt] = weight;
                }
            }
        }
        break;
    case CUSTOM:
    case COORDINATE:
        delete retVec;
        retVec = new std::vector<std::map<int,int> >(*commInfo);
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
    switch(taskCommType){
    case ALLTOALL:
        dist = 1;
        break;
    case MESH:
    {
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
        break;
    }
    case CUSTOM:
    case COORDINATE:
        if(commInfo->at(task0).count(task1) != 0){
            dist = commInfo->at(task0)[task1];
        }
        break;
    default:
        schedout.fatal(CALL_INFO, 1, "Unknown Communication type");
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
        for(std::map<int, int>::iterator it = commInfo->at(taskIt).begin(); it != commInfo->at(taskIt).end(); it++){
            outMatrix[taskIt][it->first] = it->second;
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

