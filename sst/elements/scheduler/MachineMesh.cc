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
#include "sst_config.h"
#include "MachineMesh.h"

#include <sst_config.h>

#include <vector>
#include <string>
#include <sstream>

#include "sst/core/serialization.h"

#include "Allocator.h"
#include "Machine.h"
#include "MeshAllocInfo.h"
#include "misc.h"
#include "schedComponent.h"

using namespace SST::Scheduler;

namespace SST {
    namespace Scheduler {
        class MeshLocation;
        class MeshAllocInfo;
    }
}

MachineMesh::MachineMesh(int Xdim, int Ydim, int Zdim, schedComponent* sc, double** D_matrix) : Machine(D_matrix)
{
    schedout.init("", 8, 0, Output::STDOUT);
    xdim = Xdim;
    ydim = Ydim;
    zdim = Zdim;
    numProcs = Xdim * Ydim;
    numProcs *= Zdim;
    isFree.resize(xdim);
    for (int i = 0; i < xdim; i++) {
        isFree[i].resize(ydim);
        for (int j = 0; j < (ydim); j++) {
            isFree[i][j].resize(zdim);
            for (int k = 0; k < zdim; k++) {
                isFree[i][j][k] = true;
            }
        }
    }
    this -> sc = sc;
    reset();
}

std::string MachineMesh::getParamHelp() 
{
    return "[<x dim>,<y dim>,<z dim>]\n\t3D Mesh with specified dimensions";
}

std::string MachineMesh::getSetupInfo(bool comment)
{
    std::string com;
    if (comment) com="# ";
    else com="";
    std::stringstream ret;
    ret << com<<xdim<<"x"<<ydim<<"x"<<zdim<<" Mesh";
    return ret.str();
}

int MachineMesh::getXDim() 
{
    return xdim;
}

int MachineMesh::getYDim() 
{
    return ydim;
}

int MachineMesh::getZDim() 
{
    return zdim;
}

int MachineMesh::getMachSize() 
{
    return xdim*ydim*zdim;
}

void MachineMesh::reset() 
{
    numAvail = xdim * ydim * zdim;
    for (int i = 0; i < xdim; i++) {
        for (int j = 0; j < ydim; j++) {
            for (int k = 0; k < zdim; k++) {
                isFree[i][j][k] = true;
            }
        }
    }
}

//returns list of free processors
std::vector<MeshLocation*>* MachineMesh::freeProcessors() 
{
    std::vector<MeshLocation*>* retVal = new std::vector<MeshLocation*>();
    for (int i = 0; i < xdim; i++) {
        for (int j = 0; j < ydim; j++) {
            for (int k = 0; k < zdim; k++) {
                if (isFree[i][j][k]) {
                    retVal -> push_back(new MeshLocation(i,j,k));
                }
            }
        }
    }
    return retVal;
}

//returns list of used processors
std::vector<MeshLocation*>* MachineMesh::usedProcessors() 
{
    std::vector<MeshLocation*>* retVal = new std::vector<MeshLocation*>();
    for (int i = 0; i < xdim; i++) {
        for (int j = 0; j < ydim; j++) {
            for (int k = 0; k < zdim; k++) {
                if (!isFree[i][j][k]) {
                    retVal -> push_back(new MeshLocation(i,j,k));
                }
            }
        }
    }
    return retVal;
}

//allocate list of processors in allocInfo
void MachineMesh::allocate(AllocInfo* allocInfo) 
{
    std::vector<MeshLocation*>* procs = ((MeshAllocInfo*)allocInfo) -> processors;
    //machinemesh (unlike simplemachine) is not responsible for setting
    //which processors are used in allocInfo as it's been set by the
    //allocator already

    for (unsigned int i = 0; i < procs -> size(); i++) {
        if (!isFree[((*procs)[i]) -> x][((*procs)[i]) -> y][((*procs)[i]) -> z]) {
            schedout.fatal(CALL_INFO, 0, "Attempt to allocate a busy processor: " );
        }
        isFree[((*procs)[i]) -> x][((*procs)[i]) -> y][((*procs)[i]) -> z] = false;
    }
    numAvail  -= procs-> size();
    sc -> startJob(allocInfo);
}

void MachineMesh::deallocate(AllocInfo* allocInfo) {
    //deallocate list of processors in allocInfo

    std::vector<MeshLocation*>* procs = ((MeshAllocInfo*)allocInfo) -> processors;

    for (unsigned int i = 0; i < procs -> size(); i++) {
        if (isFree[((*procs)[i]) -> x][((*procs)[i]) -> y][((*procs)[i]) -> z]) {
            schedout.fatal(CALL_INFO, 0, "Attempt to allocate a busy processor: " );
        }
        isFree[((*procs)[i]) -> x][((*procs)[i]) -> y][((*procs)[i]) -> z] = true;
    }
    numAvail += procs -> size();
}

long MachineMesh::pairwiseL1Distance(std::vector<MeshLocation*>* locs) {
    //returns total pairwise L_1 distance between all array members
    return pairwiseL1Distance(locs, locs -> size());
}

long MachineMesh::pairwiseL1Distance(std::vector<MeshLocation*>* locs, int num) {
    //returns total pairwise L_1 distance between 1st num members of array
    long retVal = 0;

    for (int i = 0; i < num; i++) {
        for (int j = i + 1; j < num; j++) {
            retVal += ((*locs)[i]) -> L1DistanceTo((*locs)[j]);
        }
    }

  return retVal;
}

double MachineMesh::getCoolingPower() 
{
    int Putil=2000;
    int Pidle=1000;
    int busynodes=0;
    int i, j, k;

    double Tsup=15;
    double Tred=30;
    double sum_inlet=0;
    double max_inlet=0;

    double COP;
    double Pcompute;
    double Pcooling;
    double Scaling_Factor;

    std::vector<bool> allocation_index;
    std::vector<double> inlet_temperature;

    for (k = 0; k < (int)isFree[0][0].size(); k++) {
        for (i = (int)isFree.size() - 1; i >= 0; i--) {
            for (j = (int)isFree[0].size() - 1; j >= 0; j--) {
                if (isFree[i][j][k]) {
                    allocation_index.push_back(0);
                } else {
                    allocation_index.push_back(1);
                }
            }
        }
    }


    for (j = 0; j < (int)allocation_index.size(); j++)
    {
        for (i = 0; i < (int)allocation_index.size(); i++)
        {
            sum_inlet += D_matrix[j][i] * (Pidle + Putil * allocation_index[i]);
        }
        inlet_temperature.push_back(Tsup+sum_inlet);
        sum_inlet = 0;
    }

    for (i = 0; i < (int)allocation_index.size(); i++)
    {
        if (max_inlet < inlet_temperature[i]) {
            max_inlet = inlet_temperature[i];
        }
        if (allocation_index[i]) {
            busynodes++;	
        }
    }

    // Total power of data center
    Pcompute = busynodes * Putil + allocation_index.size() * Pidle;

    // Supply temperature
    Tsup = Tsup + Tred-max_inlet;

    // Coefficient of performance
    COP = 0.0068 * Tsup * Tsup + 0.0008 * Tsup + 0.458;

    // Cooling power in kW
    Scaling_Factor = 214.649 / 120;
    Pcooling = 0.001 * Pcompute * (1 / COP) / Scaling_Factor;

    return  Pcooling;
}




/*
   std::string MachineMesh::tostd::string(){
//returns human readable view of which processors are free
//presented in layers by z-coordinate (0 first), with the
//  (0,0) position of each layer in the bottom left
//uses "X" and "." to denote free and busy processors respectively

std::string retVal = "";
for(int k=0; k<isFree[0][0].size(); k++) {
for(int j=isFree[0].length-1; j>=0; j--) {
for(int i=0; i<isFree.length; i++)
if(isFree[i][j][k]) retVal += "X";
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

