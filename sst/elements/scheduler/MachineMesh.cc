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

/*
 * Machine based on a mesh structure
 */

#include <vector>
#include <string>
#include <sstream>
using namespace std;

#include "sst/core/serialization/element.h"

#include "Allocator.h"
#include "Machine.h"
#include "MachineMesh.h"
#include "MeshAllocInfo.h"
#include "misc.h"

using namespace SST::Scheduler;

namespace SST {
namespace Scheduler {
class MeshLocation;
class MeshAllocInfo;
}
}


MachineMesh::MachineMesh(int Xdim, int Ydim, int Zdim, schedComponent* sc) {
  //constructor that takes mesh dimensions

  xdim = Xdim;
  ydim = Ydim;
  zdim = Zdim;
  numProcs = Xdim * Ydim;
  numProcs *= Zdim;
  isFree.resize(xdim);
  for(int i = 0; i < xdim; i++)
  {
    isFree[i].resize(ydim);
    for(int j = 0; j < (ydim); j++)
    {
      isFree[i][j].resize(zdim);
      for(int k = 0; k < zdim; k++)
        isFree[i][j][k] = true;
    }
  }

  this->sc = sc;
  reset();
}

MachineMesh::MachineMesh(MachineMesh* inmesh) {

  //constructor for testing
  //takes array telling which processors are free

  isFree = inmesh->isFree;
  xdim = inmesh->getXDim();
  ydim = inmesh->getYDim();
  zdim = inmesh->getZDim();

  numAvail = 0;
  for(int i=0; i<xdim; i++)
    for(int j=0; j<ydim; j++)
      for(int k=0; k<zdim; k++)
        if(isFree[i][j][k])
          numAvail++;
}

string MachineMesh::getParamHelp() {
  return "[<x dim>,<y dim>,<z dim>]\n\t3D Mesh with specified dimensions";
}

string MachineMesh::getSetupInfo(bool comment){
  string com;
  if(comment) com="# ";
  else com="";
  stringstream ret;
  ret << com<<xdim<<"x"<<ydim<<"x"<<zdim<<" Mesh";
  return ret.str();
}

int MachineMesh::getXDim() {
  return xdim;
}

int MachineMesh::getYDim() {
  return ydim;
}

int MachineMesh::getZDim() {
  return zdim;
}

int MachineMesh::getMachSize() {
  return xdim*ydim*zdim;
}

void MachineMesh::reset() {
  numAvail = xdim * ydim * zdim;
  for(int i=0; i<xdim; i++)
    for(int j=0; j<ydim; j++)
      for(int k=0; k<zdim; k++)
        isFree[i][j][k] = true;
}

vector<MeshLocation*>* MachineMesh::freeProcessors() {
  //returns list of free processors

  vector<MeshLocation*>* retVal = new vector<MeshLocation*>();
  for(int i=0; i<xdim; i++)
    for(int j=0; j<ydim; j++)
      for(int k=0; k<zdim; k++)
        if(isFree[i][j][k])
          retVal->push_back(new MeshLocation(i,j,k));
  return retVal;
}

vector<MeshLocation*>* MachineMesh::usedProcessors() {
  //returns list of used processors
  vector<MeshLocation*>* retVal = new vector<MeshLocation*>();
  for(int i=0; i<xdim; i++)
    for(int j=0; j<ydim; j++)
      for(int k=0; k<zdim; k++)
        if(!isFree[i][j][k])
          retVal->push_back(new MeshLocation(i,j,k));
  return retVal;
}

void MachineMesh::allocate(AllocInfo* allocInfo) {
  //allocate list of processors in allocInfo
  vector<MeshLocation*>* procs = ((MeshAllocInfo*)allocInfo)->processors;
  //machinemesh (unlike simplemachine) is not responsible for setting
  //which processors are used in allocInfo as it's been set by the
  //allocator already

  for(unsigned int i=0; i<procs->size(); i++) {
    if(!isFree[((*procs)[i])->x][((*procs)[i])->y][((*procs)[i])->z])
      error("Attempt to allocate a busy processor: " );
    isFree[((*procs)[i])->x][((*procs)[i])->y][((*procs)[i])->z] = false;
  }
  numAvail -= procs->size();
  sc->startJob(allocInfo);
}

void MachineMesh::deallocate(AllocInfo* allocInfo) {
  //deallocate list of processors in allocInfo

  vector<MeshLocation*>* procs = ((MeshAllocInfo*)allocInfo)->processors;

  for(unsigned int i=0; i<procs->size(); i++) {
    if(isFree[((*procs)[i])->x][((*procs)[i])->y][((*procs)[i])->z])
      error("Attempt to allocate a busy processor: " );
    isFree[((*procs)[i])->x][((*procs)[i])->y][((*procs)[i])->z] = true;
  }
  numAvail += procs->size();
}

long MachineMesh::pairwiseL1Distance(vector<MeshLocation*>* locs) {
  //returns total pairwise L_1 distance between all array members
  return pairwiseL1Distance(locs, locs->size());
}

long MachineMesh::pairwiseL1Distance(vector<MeshLocation*>* locs, int num) {
  //returns total pairwise L_1 distance between 1st num members of array
  long retVal = 0;
  for(int i=0; i<num; i++)
    for(int j=i+1; j<num; j++)
      retVal += ((*locs)[i])->L1DistanceTo((*locs)[j]);
  return retVal;
}
/*
string MachineMesh::toString(){
  //returns human readable view of which processors are free
  //presented in layers by z-coordinate (0 first), with the
  //  (0,0) position of each layer in the bottom left
  //uses "X" and "." to denote free and busy processors respectively

  string retVal = "";
  for(int k=0; k<isFree[0][0].size(); k++) {
    for(int j=isFree[0].length-1; j>=0; j--) {
      for(int i=0; i<isFree.length; i++)
        if(isFree[i][j][k])
          retVal += "X";
        else
          retVal += ".";
      retVal += "\n";
    }

    if(k != isFree[0][0].length-1)  //add blank line between layers
      retVal += "\n";
  }

  return retVal;
}
*/

