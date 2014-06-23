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

#include <sst_config.h>

#include <cmath>
#include <vector>
#include <string>
#include <sstream>
#include <utility>

#include "sst_config.h"
#include "sst/core/serialization.h"

#include "Allocator.h"
#include "Job.h"
#include "Machine.h"
#include "MeshMachine.h"
#include "MeshAllocInfo.h"
#include "output.h"

using namespace SST::Scheduler;

namespace SST {
    namespace Scheduler {
        class MeshLocation;
        class MeshAllocInfo;
    }
}

MeshMachine::MeshMachine(int Xdim, int Ydim, int Zdim, double** D_matrix) : Machine((Xdim*Ydim*Zdim), D_matrix)
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
    reset();
}

std::string MeshMachine::getParamHelp() 
{
    return "[<x dim>,<y dim>,<z dim>]\n\t3D Mesh with specified dimensions";
}

std::string MeshMachine::getSetupInfo(bool comment)
{
    std::string com;
    if (comment) com="# ";
    else com="";
    std::stringstream ret;
    ret << com<<xdim<<"x"<<ydim<<"x"<<zdim<<" Mesh";
    return ret.str();
}

int MeshMachine::getXDim() 
{
    return xdim;
}

int MeshMachine::getYDim() 
{
    return ydim;
}

int MeshMachine::getZDim() 
{
    return zdim;
}

int MeshMachine::getMachSize() 
{
    return xdim*ydim*zdim;
}

void MeshMachine::reset()
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
std::vector<MeshLocation*>* MeshMachine::freeProcessors() 
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
std::vector<MeshLocation*>* MeshMachine::usedProcessors() 
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
void MeshMachine::allocate(AllocInfo* allocInfo)
{
    std::vector<MeshLocation*>* procs = ((MeshAllocInfo*)allocInfo) -> processors;
    //MeshMachine (unlike simplemachine) is not responsible for setting
    //which processors are used in allocInfo as it's been set by the
    //allocator already

    for (unsigned int i = 0; i < procs -> size(); i++) {
        if (!isFree[((*procs)[i]) -> x][((*procs)[i]) -> y][((*procs)[i]) -> z]) {
            schedout.fatal(CALL_INFO, 0, "Attempt to allocate a busy processor: " );
        }
        isFree[((*procs)[i]) -> x][((*procs)[i]) -> y][((*procs)[i]) -> z] = false;
    }
    numAvail  -= procs-> size();
}

void MeshMachine::deallocate(AllocInfo* allocInfo) {
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

long MeshMachine::pairwiseL1Distance(std::vector<MeshLocation*>* locs) {
    //returns total pairwise L_1 distance between all array members
    return pairwiseL1Distance(locs, locs -> size());
}

long MeshMachine::pairwiseL1Distance(std::vector<MeshLocation*>* locs, int num) {
    //returns total pairwise L_1 distance between 1st num members of array
    long retVal = 0;

    for (int i = 0; i < num; i++) {
        for (int j = i + 1; j < num; j++) {
            retVal += ((*locs)[i]) -> L1DistanceTo((*locs)[j]);
        }
    }

  return retVal;
}

double MeshMachine::getCoolingPower() 
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

long MeshMachine::baselineL1Distance(Job* job)
{
    int numProcs = job -> getProcsNeeded();
    
    //baseline communication scheme: minimum-volume rectangular prism that fits into the machine
    
    //TODO: currently dummy:
    return 1;
    
    int xSize, ySize, zSize;
    xSize = (int)ceil( (float)cbrt((float)numProcs) ); //if we fit job in a cube
    ySize = xSize;
    zSize = xSize;
    //restrict dimensions
    if(xSize > xdim) {
        xSize = xdim;
        ySize = (int)ceil( (float)std::sqrt( ((float)numProcs) / xdim ) );
        zSize = ySize;
        if( ySize > ydim ) {
            ySize = ydim;
            zSize = (int)ceil( ((float)numProcs) / (xdim * ydim) );
        } else if ( zSize > zdim ) {
            zSize = zdim;
            ySize = (int)ceil( ((float)numProcs) / (xdim * zdim) );
        }
    } else if(ySize > ydim) {
        ySize = ydim;
        xSize = (int)ceil( (float)std::sqrt( ((float)numProcs) / ydim ) );
        zSize = xSize;
        if( xSize > xdim ) {
            xSize = xdim;
            zSize = (int)ceil( ((float)numProcs) / (xdim * ydim) );
        } else if ( zSize > zdim ) {
            zSize = zdim;
            xSize = (int)ceil( ((float)numProcs) / (ydim * zdim) );
        }
    } else if(zSize > zdim) {
        zSize = zdim;
        ySize = (int)ceil( (float)std::sqrt( ((float)numProcs) / zdim ) );
        xSize = ySize;
        if( ySize > ydim ){
            ySize = ydim;
            xSize = (int)ceil( ((float)numProcs) / (zdim * ydim) );
        } else if ( xSize > xdim ) {
            xSize = xdim;
            ySize = (int)ceil( ((float)numProcs) / (xdim * zdim) );
        }
    }
    
    //order dimensions from shortest to longest
	int state; //keeps order mapping
	if(xSize <= ySize && ySize <= zSize) {
		state = 0;
	} else if(ySize <= xSize && xSize <= zSize) {
		state = 1;
		std::swap(xSize, ySize);
	} else if(zSize <= ySize && ySize <= xSize) {
		state = 2;
		std::swap(xSize, zSize);
	} else if(xSize <= zSize && zSize <= ySize) {
		state = 3;
		std::swap(zSize, ySize);
	} else if(ySize <= zSize && zSize <= xSize) {
		state = 4;
		std::swap(xSize, ySize);
		std::swap(ySize, zSize);
	} else if(zSize <= xSize && xSize <= ySize) {
		state = 5;
		std::swap(xSize, ySize);
		std::swap(xSize, zSize);
	}
   
    //Fill given space, use shortest dim first
    int nodeCount = 0;
    bool done = false;
    std::vector<MeshLocation> nodes;
    for(int k = 0; k < zSize && !done; k++){
        for(int j = 0; j < ySize && !done; j++){
            for(int i = 0; i < xSize && !done; i++){
                //use state not to mix dimension of the actual machine
                switch(state) {
                case 0: nodes.push_back(MeshLocation(i,j,k)); break;
                case 1: nodes.push_back(MeshLocation(j,i,k)); break;
                case 2: nodes.push_back(MeshLocation(k,j,i)); break;
                case 3: nodes.push_back(MeshLocation(i,k,j)); break;
                case 4: nodes.push_back(MeshLocation(k,i,j)); break;
                case 5: nodes.push_back(MeshLocation(j,k,i)); break;
                default: schedout.fatal(CALL_INFO, 0, "Unexpected error.");
                }
                nodeCount++;
                if(nodeCount == numProcs){
                    done = true;
                }
            }
        }
    }
    
    //calculate total distance
    long distance = 0;
    for(int i = 0; i < (nodeCount - 1); i++){
        for(int j = (i + 1); j < nodeCount; j++){
            distance += nodes[i].L1DistanceTo(&(nodes[j]));
        }
    }
    
    return distance;
}




/*
   std::string MeshMachine::tostd::string(){
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

