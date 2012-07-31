// Copyright 2011 Sandia Corporation. Under the terms                          
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.             
// Government retains certain rights in this software.                         
//                                                                             
// Copyright (c) 2011, Sandia Corporation                                      
// All rights reserved.                                                        
//                                                                             
// This file is part of the SST software package. For license                  
// information, see the LICENSE file in the top level directory of the         
// distribution.                                                               

/*
 * Abstract base class for machines based on a mesh structure
 */

#ifndef __MESHMACHINE_H__
#define __MESHMACHINE_H__

#include <string>
#include <vector>
using namespace std;

//#include "schedComponent.h"
#include "Machine.h"
//#include "sst/core/serialization/element.h"

class MeshLocation;


class MachineMesh : public Machine {

  private:
    int xdim;              //size of mesh in each dimension
    int ydim;
    int zdim;  
    schedComponent* sc;

    vector<vector<vector<bool> > > isFree;  //whether each processor is free
  public:

    MachineMesh(int Xdim, int Ydim, int Zdim, schedComponent* sc);

    MachineMesh(MachineMesh* inmesh);

    //static Mesh Make(vector<String> params);

    static string getParamHelp();

    string getSetupInfo(bool comment);

    int getXDim();

    int getYDim();

    int getZDim();

    int getMachSize();

    void reset();

    vector<MeshLocation*>* freeProcessors();

    vector<MeshLocation*>* usedProcessors();

    void allocate(AllocInfo* allocInfo);

    void deallocate(AllocInfo* allocInfo);

    long pairwiseL1Distance(vector<MeshLocation*>* locs);

    long pairwiseL1Distance(vector<MeshLocation*>* locs, int num);

    //string toString();
};

#endif
