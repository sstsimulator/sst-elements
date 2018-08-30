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

#include "RCBTaskMapper.h"

#include <cmath>
#include <cfloat>

#include "AllocInfo.h"
#include "Job.h"
#include "StencilMachine.h"
#include "SimpleTaskMapper.h"
#include "TaskCommInfo.h"
#include "TaskMapInfo.h"
#include "output.h"

using namespace SST::Scheduler;
using namespace std;

RCBTaskMapper::RCBTaskMapper(const StencilMachine & inMach) : TaskMapper(inMach), mMachine(inMach)
{
    if(inMach.numDims() != 3){
        schedout.fatal(CALL_INFO, 1, "RCB task mapper requires a 3D mesh/torus.");
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
    job = allocInfo->job;
    tci = job->taskCommInfo;

    //check job compatibility - use simple task mapper if not compatible
    if(tci->getCommType() != TaskCommInfo::MESH &&
       tci->getCommType() != TaskCommInfo::COORDINATE) {
        SimpleTaskMapper simpleMapper = SimpleTaskMapper(mMachine);
        return simpleMapper.mapTasks(allocInfo);
        //schedout.fatal(CALL_INFO, 1, "Job %ld attempted to use RCB task mapper without coordinate input\n", job->getJobNum());
    }

    TaskMapInfo* tmi = new TaskMapInfo(allocInfo, mMachine);
    int jobSize = job->getProcsNeeded();
    
    //Optimization: SimpleTaskMapper if <= 2 tasks are provided
    if(jobSize <= 2 || allocInfo->getNodesNeeded() == 1){
        SimpleTaskMapper simpleMapper = SimpleTaskMapper(mMachine);
        return simpleMapper.mapTasks(allocInfo);
    }

    //dummy rotator for initialization
    Rotator dummyRotator = Rotator(*this, mMachine);

    //get node locations
    vector<MeshLocation>* nodes = new vector<MeshLocation>();
    for(int i = 0; i < allocInfo->getNodesNeeded(); i++){
        //put the same node location for each core
        for(int j = 0; j < mMachine.coresPerNode; j++){
            MeshLocation loc = MeshLocation(allocInfo->nodeIndices[i], mMachine);
            nodes->push_back(loc);
        }
    }
    Grouper<MeshLocation> nodeGroup = Grouper<MeshLocation>(nodes, &dummyRotator);

    vector<int>* tasks = new vector<int>();
    for(int i = 0; i < jobSize; i++){
        tasks->push_back(i);
    }
    Grouper<int> taskGroup = Grouper<int>(tasks, &dummyRotator);

    //apply rotation
    Rotator rotator = Rotator(&nodeGroup, &taskGroup, *this, mMachine);

    //map
    mapTaskHelper(&nodeGroup, &taskGroup, tmi);

    return tmi;
}

void RCBTaskMapper::mapTaskHelper(Grouper<MeshLocation>* inLocs, Grouper<int>* inTasks, TaskMapInfo* tmi)
{
    if(inTasks->elements->size() == 1){
        //map node to task
        tmi->insert(inTasks->elements->at(0), inLocs->elements->at(0).toInt(mMachine));
    } else {
        Grouper<MeshLocation>** firstLocs = new Grouper<MeshLocation>*();
        Grouper<MeshLocation>** secondLocs = new Grouper<MeshLocation>*();
        Grouper<int>** firstTasks = new Grouper<int>*();
        Grouper<int>** secondTasks = new Grouper<int>*();

        int locLongest[3];
        int jobLongest[3];
        int dimToCut;
        inTasks->sortDims(jobLongest);
        inLocs->sortDims(locLongest);
        //prioritize the geometry with larger max/min dimension ratio
        double locRatio = inLocs->getDim(locLongest[0]) / inLocs->getDim(locLongest[2]);
        double jobRatio = inTasks->getDim(jobLongest[0]) / inTasks->getDim(jobLongest[2]);
        if( jobRatio >= locRatio ){
            dimToCut = jobLongest[0];
        } else {
            dimToCut = locLongest[0];
        }

        inLocs->divideBy(dimToCut, firstLocs, secondLocs);
        inTasks->divideBy(dimToCut, firstTasks, secondTasks);

        mapTaskHelper(*firstLocs, *firstTasks, tmi);
        mapTaskHelper(*secondLocs, *secondTasks, tmi);

        delete *firstLocs;
        delete *secondLocs;
        delete *firstTasks;
        delete *secondTasks;
        delete firstLocs;
        delete secondLocs;
        delete firstTasks;
        delete secondTasks;
    }
}

template <class T>
RCBTaskMapper::Grouper<T>::Grouper(std::vector<T>* elements, Rotator *rotator)
{
    this->elements = elements;
    this->rotator = rotator;
    dims = rotator->getStrDims<T>(*elements);
}

template <class T>
RCBTaskMapper::Grouper<T>::~Grouper()
{
    delete elements;
}

template <class T>
void RCBTaskMapper::Grouper<T>::sortDims(int sortedDims[3]) const
{
    sortedDims[0] = 0;
    sortedDims[1] = 1;
    sortedDims[2] = 2;
    if(dims.val[sortedDims[0]] < dims.val[sortedDims[1]])
        swap(sortedDims[0], sortedDims[1]);
    if(dims.val[sortedDims[0]] < dims.val[sortedDims[2]])
        swap(sortedDims[0], sortedDims[2]);
    if(dims.val[sortedDims[1]] < dims.val[sortedDims[2]])
        swap(sortedDims[1], sortedDims[2]);
}

template <class T>
void RCBTaskMapper::Grouper<T>::divideBy(int dim, Grouper<T>** first, Grouper<T>** second)
{
    //sort elements using heap sort
    for( int i = elements->size() - 2; i >= 0; --i){
        sort_maxHeapify(*elements, i, dim);
    }
    int x = 0;
    int i = elements->size() - 1;
    while(i > x){
        std::swap(elements->at(i), elements->at(x));
        x++;
        i--;
    }
    //divide into two Grouper objects
    int halfSize = ceil(elements->size() / 2);
    *first = new Grouper<T>(new vector<T>(elements->begin(), elements->begin() + halfSize), rotator);
    *second = new Grouper<T>(new vector<T>(elements->begin() + halfSize, elements->end()), rotator);
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
    Dims firstDims = rotator->getDims(first);
    Dims secondDims = rotator->getDims(second);

    if(dim == 0) { //ties broken by y, then z
        if(firstDims.val[0] != secondDims.val[0]) return (firstDims.val[0] - secondDims.val[0]);
        if(firstDims.val[1] != secondDims.val[1]) return (firstDims.val[1] - secondDims.val[1]);
        return (firstDims.val[2] - secondDims.val[2]);
    } else if(dim == 1) { //ties broken by x, then z
        if(firstDims.val[1] != secondDims.val[1]) return (firstDims.val[1] - secondDims.val[1]);
        if(firstDims.val[0] != secondDims.val[0]) return (firstDims.val[0] - secondDims.val[0]);
        return (firstDims.val[2] - secondDims.val[2]);
    } else { //ties broken by x, then y
        if(firstDims.val[2] != secondDims.val[2]) return (firstDims.val[2] - secondDims.val[2]);
        if(firstDims.val[0] != secondDims.val[0]) return (firstDims.val[0] - secondDims.val[0]);
        return (firstDims.val[1] - secondDims.val[1]);
    }
}

RCBTaskMapper::Rotator::Rotator(const RCBTaskMapper & rcb,
                                const StencilMachine & mach) : rcb(rcb), mach(mach)
{
    indMap = NULL;
    locs = NULL;
}

RCBTaskMapper::Rotator::Rotator(Grouper<MeshLocation> *meshLocs,
                                Grouper<int> *jobLocs,
                                const RCBTaskMapper & rcb,
                                const StencilMachine & mach) : rcb(rcb), mach(mach)
{
    //register yourself
    meshLocs->rotator = this;
    jobLocs->rotator = this;
    //save the dimension info of node locations
    int size = meshLocs->elements->size();
    locs = new int*[3];
    locs[0] = new int[size];
    locs[1] = new int[size];
    locs[2] = new int[size];
    indMap = new int[mach.numNodes];//maps real node indexed to node vector (elements) indexes
    for(int i = 0; i < size; i++){
        MeshLocation curLoc = meshLocs->elements->at(i);
        indMap[curLoc.toInt(mach)] = i; //overwriting the same location is fine
        locs[0][i] = curLoc.dims[0];
        locs[1][i] = curLoc.dims[1];
        locs[2][i] = curLoc.dims[2];
    }

    //apply rotation where needed
    int meshSorted[3];
    int jobSorted[3];
    meshLocs->sortDims(meshSorted);
    jobLocs->sortDims(jobSorted);

    if(jobSorted[0] != meshSorted[0]){
        swap(meshSorted[0],meshSorted[1]);
        swap(locs[meshSorted[0]], locs[meshSorted[1]]);
        swap(meshLocs->dims.val[meshSorted[0]], meshLocs->dims.val[meshSorted[1]]);
    }
    if(jobSorted[0] != meshSorted[0]){
        swap(meshSorted[0],meshSorted[2]);
        swap(locs[meshSorted[0]], locs[meshSorted[2]]);
        swap(meshLocs->dims.val[meshSorted[0]], meshLocs->dims.val[meshSorted[2]]);
    }
    if(jobSorted[1] != meshSorted[1]){
        swap(locs[meshSorted[1]], locs[meshSorted[2]]);
        swap(meshLocs->dims.val[meshSorted[1]], meshLocs->dims.val[meshSorted[2]]);
    }

}

RCBTaskMapper::Rotator::~Rotator()
{
    if(indMap != NULL) {
        delete [] indMap;
        delete [] locs[0];
        delete [] locs[1];
        delete [] locs[2];
        delete [] locs;
    }
}

template <typename T>
RCBTaskMapper::Dims RCBTaskMapper::Rotator::getStrDims(const vector<T> & elements) const
{
    double mins[3];
    double maxs[3];
    for(int j = 0; j < 3; j++){
        mins[j] = DBL_MAX;
        maxs[j] = 0;
    }
    Dims dims;
    for(unsigned int i = 0; i < elements.size(); i++){
        dims = getDims(elements[i]);
        for(int j = 0; j < 3; j++){
            if(dims.val[j] > maxs[j]) maxs[j] = dims.val[j];
            if(dims.val[j] < mins[j]) mins[j] = dims.val[j];
        }
    }
    Dims outDims;
    outDims.val[0] = maxs[0] - mins[0];
    outDims.val[1] = maxs[1] - mins[1];
    outDims.val[2] = maxs[2] - mins[2];
    return outDims;
}

template <typename T>
RCBTaskMapper::Dims RCBTaskMapper::Rotator::getDims(T t) const
{
    Dims outDims;
    schedout.fatal(CALL_INFO, 1, "Template function getDims should have been overloaded\n");
    return outDims;
}

RCBTaskMapper::Dims RCBTaskMapper::Rotator::getDims(int taskID) const
{
    Dims outDims;
    if(rcb.tci->getCommType() == TaskCommInfo::MESH) {
        outDims.val[0] = taskID % rcb.tci->xdim;
        outDims.val[1] = (taskID % (rcb.tci->xdim * rcb.tci->ydim)) / rcb.tci->xdim;
        outDims.val[2] = taskID / (rcb.tci->xdim * rcb.tci->ydim);
    } else if (rcb.tci->getCommType() == TaskCommInfo::COORDINATE) {
        //return coordinates normalized by rotated machine dimensions
        outDims.val[0] = rcb.tci->coordMatrix[taskID][0];
        outDims.val[1] = rcb.tci->coordMatrix[taskID][1];
        outDims.val[2] = rcb.tci->coordMatrix[taskID][2];
    } else {
        schedout.fatal(CALL_INFO, 1, "Unknown communication type");
    }
    return outDims;
}

RCBTaskMapper::Dims RCBTaskMapper::Rotator::getDims(MeshLocation loc) const
{
    Dims outDims;
    if(indMap == NULL){ //support for Grouper initialization
        outDims.val[0] = loc.dims[0];
        outDims.val[1] = loc.dims[1];
        outDims.val[2] = loc.dims[2];
    } else {
        outDims.val[0] = locs[0][indMap[loc.toInt(mach)]];
        outDims.val[1] = locs[1][indMap[loc.toInt(mach)]];
        outDims.val[2] = locs[2][indMap[loc.toInt(mach)]];
    }
    return outDims;
}

//debug functions:
template <typename T>
int RCBTaskMapper::Rotator::getTaskNum(T t) const
{
    schedout.fatal(CALL_INFO, 1, "Template function getNum should have been overloaded\n");
    return -1;
}

int RCBTaskMapper::Rotator::getTaskNum(int taskID) const
{
    return taskID;
}

int RCBTaskMapper::Rotator::getTaskNum(MeshLocation loc) const
{
    return loc.toInt(rcb.mMachine);
}

