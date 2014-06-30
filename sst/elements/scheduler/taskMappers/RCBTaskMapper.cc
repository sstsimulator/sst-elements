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

#include "RCBTaskMapper.h"

#include <iostream> //debug
#include <iterator>

#include "AllocInfo.h"
#include "Job.h"
#include "MeshMachine.h"
#include "TaskCommInfo.h"
#include "TaskMapInfo.h"
#include "output.h"

using namespace SST::Scheduler;
using namespace std;

RCBTaskMapper::RCBTaskMapper(Machine* mach) : TaskMapper(mach)
{
    mMachine = dynamic_cast<MeshMachine*>(machine);
    if(mMachine == NULL){
        schedout.fatal(CALL_INFO, 1, "RCB task mapper requires a mesh machine");
    }
}

std::string RCBTaskMapper::getSetupInfo(bool comment) const
{
    std::string com;
    if (comment) {
        com="# ";
    } else  {
        com="";
    }
    return com + "RCB Task Mapper";
}

TaskMapInfo* RCBTaskMapper::mapTasks(AllocInfo* allocInfo)
{
    TaskMapInfo* tmi = new TaskMapInfo(allocInfo);
    job = allocInfo->job;
    int jobSize = job->getProcsNeeded();
    tci = job->taskCommInfo;

    //get node locations
    vector<MeshLocation>* nodes = new vector<MeshLocation>();;
    for(int i = 0; i < jobSize; i++){
        MeshLocation loc = MeshLocation(allocInfo->nodeIndices[i], *mMachine);
        nodes->push_back(loc);
    }
    Grouper<MeshLocation> nodeGroup = Grouper<MeshLocation>(nodes, *this);

    vector<int>* jobs = new vector<int>();
    for(int i = 0; i < jobSize; i++){
        jobs->push_back(i);
    }
    Grouper<int> jobGroup = Grouper<int>(jobs, *this);

    //map
    mapTaskHelper(&nodeGroup, &jobGroup, tmi);

    //DEBUG
    //for(int i = 0; i<jobSize; i++){
    //    cout << "task:" << i << " <-> node:" << tmi->taskMap->left.at(i) << endl;
    //}
    ///////

    return tmi;
}

void RCBTaskMapper::mapTaskHelper(Grouper<MeshLocation>* inLocs, Grouper<int>* inJobs, TaskMapInfo* tmi)
{
    if(inLocs->elements->size() == 1){
        //map node to task
        tmi->insert(inJobs->elements->at(0), inLocs->elements->at(0).toInt(*mMachine));
    } else {
        Grouper<MeshLocation>** firstLocs = new Grouper<MeshLocation>*();
        Grouper<MeshLocation>** secondLocs = new Grouper<MeshLocation>*();
        Grouper<int>** firstJobs = new Grouper<int>*();
        Grouper<int>** secondJobs = new Grouper<int>*();

        int locLongest;
        int jobLongest;
        int dimToCut;

        if(inJobs->getLongestDim(&jobLongest) >= inLocs->getLongestDim(&locLongest)){
            dimToCut = jobLongest;
        } else {
            dimToCut = locLongest;
        }

        inLocs->divideBy(dimToCut, firstLocs, secondLocs);
        inJobs->divideBy(dimToCut, firstJobs, secondJobs);

        mapTaskHelper(*firstLocs, *firstJobs, tmi);
        mapTaskHelper(*secondLocs, *secondJobs, tmi);

        delete *firstLocs;
        delete *secondLocs;
        delete *firstJobs;
        delete *secondJobs;
        delete firstLocs;
        delete secondLocs;
        delete firstJobs;
        delete secondJobs;
    }
}

template <typename T>
void RCBTaskMapper::getDims(int* x, int* y, int* z, T t) const
{
    schedout.fatal(CALL_INFO, 1, "Template function getDims should have been overloaded\n");
}

void RCBTaskMapper::getDims(int* x, int* y, int* z, int taskID) const
{
    if(tci->taskCommType == TaskCommInfo::MESH ||
       tci->taskCommType == TaskCommInfo::ALLTOALL   ) {
        *x = taskID % tci->xdim;
        *y = (taskID % (tci->xdim * tci->ydim)) / tci->xdim;
        *z = taskID / (tci->xdim * tci->ydim);
    } else if (tci->taskCommType == TaskCommInfo::COORDINATE) {
        *x = tci->coords->at(taskID)[0];
        *y = tci->coords->at(taskID)[1];
        *z = tci->coords->at(taskID)[2];
    } else if(tci->taskCommType == TaskCommInfo::CUSTOM) {
        schedout.fatal(CALL_INFO, 1, "RCB task mapper does not support custom task communication.\n");
    } else {
        schedout.fatal(CALL_INFO, 1, "Unknown communication type for Job %d\n", job->getJobNum());
    }
}

void RCBTaskMapper::getDims(int* x, int* y, int* z, MeshLocation loc) const
{
    *x = loc.x;
    *y = loc.y;
    *z = loc.z;
}

//debug functions:
template <typename T>
int RCBTaskMapper::getTaskNum(T t) const
{
    schedout.fatal(CALL_INFO, 1, "Template function getNum should have been overloaded\n");
    return -1;
}

int RCBTaskMapper::getTaskNum(int taskID) const
{
    return taskID;
}

int RCBTaskMapper::getLocNum(MeshLocation loc) const
{
    return loc.toInt(*mMachine);
}

template <class T>
RCBTaskMapper::Grouper<T>::Grouper(std::vector<T>* elements,
                                     const RCBTaskMapper & rcb) : rcb(rcb)
{
    this->elements = elements;
    //get dimensions
    int xmin = elements->size();
    int ymin = elements->size();
    int zmin = elements->size();
    int xmax = 0;
    int ymax = 0;
    int zmax = 0;
    int x, y, z;
    for(unsigned int i = 0; i < elements->size(); i++){
        rcb.getDims(&x, &y, &z, elements->at(i));
        if(x > xmax) xmax = x;
        if(x < xmin) xmin = x;
        if(y > ymax) ymax = y;
        if(y < ymin) ymin = y;
        if(z > zmax) zmax = z;
        if(z < zmin) zmin = z;
    }
    dims[0] = xmax - xmin + 1;
    dims[1] = ymax - ymin + 1;
    dims[2] = zmax - zmin + 1;
}

template <class T>
RCBTaskMapper::Grouper<T>::~Grouper()
{
    delete elements;
}

template <class T>
int RCBTaskMapper::Grouper<T>::getLongestDim(int* result) const
{
    if(dims[0] >= dims[1] && dims[0] >= dims[2]){
        *result = 0;
        return dims[0];
    } else if (dims[1] >= dims[0] && dims[1] >= dims[2]){
        *result = 1;
        return dims[1];
    } else {
        *result = 2;
        return dims[2];
    }
}

template <class T>
void RCBTaskMapper::Grouper<T>::divideBy(int dim, Grouper<T>** first, Grouper<T>** second)
{
    //sort elements using heap sort
    sort_buildMaxHeap(*elements, dim);
    int x = 0;
    int i = elements->size() - 1;
    while(i > x){
        std::swap(elements->at(i), elements->at(x));
        x++;
        i--;
    }
    //divide
    int halfSize = elements->size() / 2;
    *first = new Grouper<T>(new vector<T>(elements->begin(), elements->begin() + halfSize), rcb);
    *second = new Grouper<T>(new vector<T>(elements->begin() + halfSize, elements->end()), rcb);
}

template <class T>
void RCBTaskMapper::Grouper<T>::sort_buildMaxHeap(vector<T> & v, int dim)
{
    for( int i = v.size() - 2; i >= 0; --i){
        sort_maxHeapify(v, i, dim);
    }
}

template <class T>
void RCBTaskMapper::Grouper<T>::sort_maxHeapify(std::vector<T> & v, int i, int dim)
{
    unsigned int left = i + 1;
    unsigned int right = i + 2;
    int largest;

    if( left < v.size() && sort_compByDim(v[left], v[i], dim) > 0) {
        largest = left;
    } else {
        largest = i;
    }

    if( right < v.size() && sort_compByDim(v[right], v[largest], dim) > 0) {
        largest = right;
    }

    if( largest != i) {
        std::swap(v[i], v[largest]);
        sort_maxHeapify(v, largest, dim);
    }
}

template <class T>
int RCBTaskMapper::Grouper<T>::sort_compByDim(T first, T second, int dim)
{
    int x1, y1, z1;
    int x2, y2, z2;

    rcb.getDims(&x1, &y1, &z1, first);
    rcb.getDims(&x2, &y2, &z2, second);

    if(dim == 0) { //ties broken by y, then z
        if(x1 != x2) return (x1 - x2);
        if(y1 != y2) return (y1 - y2);
        return (z1 - z2);
    } else if(dim == 1) { //ties broken by x, then z
        if(y1 != y2) return (y1 - y2);
        if(x1 != x2) return (x1 - x2);
        return (z1 - z2);
    } else { //ties broken by x, then y
        if(z1 != z2) return (z1 - z2);
        if(x1 != x2) return (x1 - x2);
        return (y1 - y2);
    }

    return false;
}

