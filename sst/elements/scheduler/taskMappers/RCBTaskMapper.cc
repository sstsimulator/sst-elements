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
#include <math.h>

#include "AllocInfo.h"
#include "Job.h"
#include "MeshMachine.h"
#include "SimpleTaskMapper.h"
#include "TaskCommInfo.h"
#include "TaskMapInfo.h"
#include "output.h"

using namespace SST::Scheduler;
using namespace std;

RCBTaskMapper::RCBTaskMapper(Machine* mach) : TaskMapper(mach)
{
    mMachine = dynamic_cast<MeshMachine*>(machine);
    if(mMachine == NULL){
        std::cout<<"error\n";//schedout.fatal(CALL_INFO, 1, "RCB task mapper requires a mesh machine");
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

    //Call SimpleTaskMapper if the job does not have proper communication info
    if(tci->taskCommType != TaskCommInfo::MESH &&
       tci->taskCommType != TaskCommInfo::COORDINATE) {
        SimpleTaskMapper simpleMapper = SimpleTaskMapper(machine);
        return simpleMapper.mapTasks(allocInfo);
    }

    //dummy rotator for initialization
    Rotator dummyRotator = Rotator(*this, *mMachine);

    //get node locations
    vector<MeshLocation>* nodes = new vector<MeshLocation>();
    for(int i = 0; i < jobSize; i++){
        MeshLocation loc = MeshLocation(allocInfo->nodeIndices[i], *mMachine);
        nodes->push_back(loc);
    }
    Grouper<MeshLocation> nodeGroup = Grouper<MeshLocation>(nodes, &dummyRotator);

    vector<int>* jobs = new vector<int>();
    for(int i = 0; i < jobSize; i++){
        jobs->push_back(i);
    }
    Grouper<int> jobGroup = Grouper<int>(jobs, &dummyRotator);

    //apply rotation
    Rotator rotator = Rotator(&nodeGroup, &jobGroup, *this, *mMachine);

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

template <class T>
RCBTaskMapper::Grouper<T>::Grouper(std::vector<T>* elements, Rotator *rotator)
{
    this->elements = elements;
    this->rotator = rotator;
    //get dimensions
    int xmin = elements->size();
    int ymin = elements->size();
    int zmin = elements->size();
    int xmax = 0;
    int ymax = 0;
    int zmax = 0;
    int x, y, z;
    for(unsigned int i = 0; i < elements->size(); i++){
        rotator->getDims(&x, &y, &z, elements->at(i));
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
    *first = new Grouper<T>(new vector<T>(elements->begin(), elements->begin() + halfSize), rotator);
    *second = new Grouper<T>(new vector<T>(elements->begin() + halfSize, elements->end()), rotator);
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

    rotator->getDims(&x1, &y1, &z1, first);
    rotator->getDims(&x2, &y2, &z2, second);

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

RCBTaskMapper::Rotator::Rotator(const RCBTaskMapper & rcb,
                                const MeshMachine & mach) : rcb(rcb), mach(mach)
{
    indMap = NULL;
    xlocs = NULL;
    ylocs = NULL;
    zlocs = NULL;
    numTasksNorm = sqrt(rcb.job->getProcsNeeded());
}

RCBTaskMapper::Rotator::Rotator(Grouper<MeshLocation> *meshLocs,
                                Grouper<int> *jobLocs,
                                const RCBTaskMapper & rcb,
                                const MeshMachine & mach) : rcb(rcb), mach(mach)
{
    //register yourself
    meshLocs->rotator = this;
    jobLocs->rotator = this;
    //save the dimensions
    int size = rcb.job->getProcsNeeded();
    xlocs = new int[size];
    ylocs = new int[size];
    zlocs = new int[size];
    indMap = new int[mach.getNumProcs()];
    for(int i = 0; i < size; i++){
        indMap[meshLocs->elements->at(i).toInt(mach)] = i;
        xlocs[i] = meshLocs->elements->at(i).x;
        ylocs[i] = meshLocs->elements->at(i).y;
        zlocs[i] = meshLocs->elements->at(i).z;
    }
    numTasksNorm = sqrt(rcb.job->getProcsNeeded());
    //apply rotation where needed
    if(meshLocs->dims[0] < meshLocs->dims[1] && jobLocs->dims[0] >= jobLocs->dims[1]){
        swap(xlocs, ylocs);
        swap(meshLocs->dims[0], meshLocs->dims[1]);
    }
    if(meshLocs->dims[0] < meshLocs->dims[2] && jobLocs->dims[0] >= jobLocs->dims[2]){
        swap(xlocs, zlocs);
        swap(meshLocs->dims[0], meshLocs->dims[2]);
    }
    if(meshLocs->dims[0] < meshLocs->dims[1] && jobLocs->dims[0] >= jobLocs->dims[1]){
        swap(xlocs, ylocs);
        swap(meshLocs->dims[0], meshLocs->dims[1]);
    }
}

RCBTaskMapper::Rotator::~Rotator()
{
    if(indMap != NULL) {
        delete [] indMap;
        delete [] xlocs;
        delete [] ylocs;
        delete [] zlocs;
    }
}

template <typename T>
void RCBTaskMapper::Rotator::getDims(int* x, int* y, int* z, T t) const
{
    std::cout<<"error\n";//schedout.fatal(CALL_INFO, 1, "Template function getDims should have been overloaded\n");
}

void RCBTaskMapper::Rotator::getDims(int* x, int* y, int* z, int taskID) const
{
    if(rcb.tci->taskCommType == TaskCommInfo::MESH) {
        *x = taskID % rcb.tci->xdim;
        *y = (taskID % (rcb.tci->xdim * rcb.tci->ydim)) / rcb.tci->xdim;
        *z = taskID / (rcb.tci->xdim * rcb.tci->ydim);
    } else if (rcb.tci->taskCommType == TaskCommInfo::COORDINATE) {
        //return coordinates normalized by rotated machine dimensions
        *x = rcb.tci->coords->at(taskID)[0] * numTasksNorm;
        *y = rcb.tci->coords->at(taskID)[1] * numTasksNorm;
        *z = rcb.tci->coords->at(taskID)[2] * numTasksNorm;
    } else {
        std::cout<<"error\n";//schedout.fatal(CALL_INFO, 1, "Unknown communication type for Job %d\n", job->getJobNum());
    }
}

void RCBTaskMapper::Rotator::getDims(int* x, int* y, int* z, MeshLocation loc) const
{
    if(indMap == NULL){ //support for Grouper initialization
        *x = loc.x;
        *y = loc.y;
        *z = loc.z;
    } else {
        *x = xlocs[indMap[loc.toInt(mach)]];
        *y = ylocs[indMap[loc.toInt(mach)]];
        *z = zlocs[indMap[loc.toInt(mach)]];
    }
}

//debug functions:
template <typename T>
int RCBTaskMapper::Rotator::getTaskNum(T t) const
{
    std::cout<<"error\n";//schedout.fatal(CALL_INFO, 1, "Template function getNum should have been overloaded\n");
    return -1;
}

int RCBTaskMapper::Rotator::getTaskNum(int taskID) const
{
    return taskID;
}

int RCBTaskMapper::Rotator::getLocNum(MeshLocation loc) const
{
    return loc.toInt(*(rcb.mMachine));
}

