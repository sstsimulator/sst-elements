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

#define MAX(X,Y) ((X) > (Y) ? (X) : (Y))
#define ABS(X) ((X) >= 0 ? (X) : (-(X)))

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

int MeshMachine::getMachSize() const
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
std::vector<MeshLocation*>* MeshMachine::freeProcessors() const
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
std::vector<MeshLocation*>* MeshMachine::usedProcessors() const
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

long MeshMachine::pairwiseL1Distance(std::vector<MeshLocation*>* locs) const
{
    //returns total pairwise L_1 distance between all array members
    return pairwiseL1Distance(locs, locs -> size());
}

long MeshMachine::pairwiseL1Distance(std::vector<MeshLocation*>* locs, int num) const
{
    //returns total pairwise L_1 distance between 1st num members of array
    long retVal = 0;

    for (int i = 0; i < num; i++) {
        for (int j = i + 1; j < num; j++) {
            retVal += ((*locs)[i])->L1DistanceTo(*((*locs)[j]));
        }
    }

  return retVal;
}

double MeshMachine::getCoolingPower() const
{
    int Putil=2000;
    int Pidle=1000;

    double Tred=30;  

    MeshLocation* tempLoc = NULL;
    int busynodes = 0;
    double max_inlet = 0;
    double sum_inlet = 0;

    //max inlet temp and number of busy nodes
    for (int i = 0; i < getNumProcs(); i++) {
        tempLoc = new MeshLocation(i, *this);
        if( !isFree[tempLoc->x][tempLoc->y][tempLoc->z] ){
            busynodes++;
        }
        if(D_matrix != NULL){
            sum_inlet = 0;
            for (int j = 0; j < getNumProcs(); j++)
            {
                sum_inlet += D_matrix[i][j] * (Pidle + Putil * (!isFree[tempLoc->x][tempLoc->y][tempLoc->z]));
            }
            if(sum_inlet > max_inlet){
                max_inlet = sum_inlet;
            }
        }
        delete tempLoc;
    }

    // Total power of data center
    double Pcompute = busynodes * Putil + getNumProcs() * Pidle;

    // Supply temperature
    double Tsup;
    if(D_matrix != NULL){
        Tsup = Tred - max_inlet;
    } else {
        Tsup = Tred;
    }

    // Coefficient of performance
    double COP = 0.0068 * Tsup * Tsup + 0.0008 * Tsup + 0.458;

    // Cooling power in kW
    double Pcooling = 0.001 * Pcompute * (1 / COP);

    return  Pcooling;
}

MeshLocation::MeshLocation(int X, int Y, int Z) 
{
    schedout.init("", 8, 0, Output::STDOUT);
    x = X;
    y = Y;
    z = Z;
}

MeshLocation::MeshLocation(int inpos, const MeshMachine & m) 
{
    //return x + m -> getXDim() * y + m -> getXDim() * m -> getYDim() * z; 

    schedout.init("", 8, 0, Output::STDOUT);
    z = inpos / (m.getXDim() * m.getYDim());
    inpos -= z * m.getXDim() * m.getYDim();
    y = inpos / m.getXDim();
    inpos -= y * m.getXDim();
    x = inpos;
}


MeshLocation::MeshLocation(const MeshLocation & in)
{
    schedout.init("", 8, 0, Output::STDOUT);
    //copy constructor
    x = in.x;
    y = in.y;
    z = in.z;
}

int MeshLocation::L1DistanceTo(const MeshLocation & other) const
{
    return ABS(x - other.x) + ABS(y - other.y) + ABS(z - other.z);
}

int MeshLocation::LInfDistanceTo(const MeshLocation & other) const
{
    return MAX(ABS(x - other.x), MAX(ABS(y - other.y), ABS(z - other.z)));
}

bool MeshLocation::operator()(MeshLocation* loc1, MeshLocation* loc2)
{
    if (loc1 -> x == loc2 -> x){
        if (loc1 -> y == loc2 -> y) {
            return loc1 -> z < loc2 -> z;
        }
        return loc1 -> y < loc2 -> y;
    }
    return loc1 -> x < loc2 -> x;
}

bool MeshLocation::equals(const MeshLocation & other) const 
{
    return x == other.x && y == other.y && z == other.z;
}

void MeshLocation::print() {
    //printf("(%d,%d,%d)\n",x,y,z);
    schedout.output("(%d,%d,%d)\n", x, y, z);
}


int MeshLocation::toInt(const MeshMachine & m){
    return x + m.getXDim() * y + m.getXDim() * m.getYDim() * z; 
}

std::string MeshLocation::toString(){
    std::stringstream ret;
    ret << "(" << x <<  ", " << y  << ", " << z << ")";
    return ret.str();
}

int MeshLocation::hashCode() {
    return x + 31 * y + 961 * z;
}

