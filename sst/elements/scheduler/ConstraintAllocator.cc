
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

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdlib.h>
#include <time.h>

#include "sst/core/serialization/element.h"

#include "ConstraintAllocator.h"
#include "Machine.h"
#include "SimpleMachine.h"
#include "AllocInfo.h"
#include "Job.h"
#include "misc.h"

using namespace std;
using namespace SST::Scheduler;


ConstraintAllocator::ConstraintAllocator(SimpleMachine* m, string DepsFile, string ConstFile) {
    machine = m;
    ConstraintsFileName = ConstFile;
  // read Dependencies
  // if file does not exist or is empty, D will be an empty mapping
  // and in effect we default to simple allocator
  ifstream DepsStream(DepsFile.c_str(),  ifstream::in );
  string u,v;
  string curline;
  stringstream lineStream;
  while(getline(DepsStream, curline)){ //for each line in file
      lineStream << curline;
      lineStream >> u; // line is u followed by elements of D[u]
      cout << "------------------Dependencies of " << u << endl;
      while (lineStream >> v){
          D[u].insert(v);
          cout << v << " ";
      }
      cout << endl;
      lineStream.clear(); //so we can write to it again
  }

 }

//external process (python) will read analysis output and create a file
//which contains a cluster of nodes on each line
//if we cannot separate the first cluster we try the next et ceteral
//if file does not exist or is empty, Du and Dv will be empty sets
//an in effect we default to simple allocator
//of course this is inefficent, should check if file has changed
//instead of re-reading every time
void ConstraintAllocator::GetConstraints(){
    string u;
    ifstream ConstraintsStream(ConstraintsFileName.c_str(), ifstream::in );
    string curline;
    stringstream lineStream;
    //for now just read first line
    Cluster.clear();
    getline(ConstraintsStream, curline);
    lineStream << curline;
    while(lineStream >> u)
       Cluster.push_back(u);
}

string ConstraintAllocator::getParamHelp(){
  return "";
}

string ConstraintAllocator::getSetupInfo(bool comment){
  string com;
  if(comment) com="# ";
  else com="";
  return com+"Constraint Allocator";
}

AllocInfo* ConstraintAllocator::allocate(Job* job) {
  //allocates job if possible
  //returns information on the allocation or null if it wasn't possible

  if(!canAllocate(job))
    return NULL;

  AllocInfo* retVal = new AllocInfo(job);
  unsigned int numProcs = job->getProcsNeeded();
  vector<int>* available = ((SimpleMachine*)machine)->freeProcessors(); 
  // the allocation we will return
  vector<int> Alloc;
  std::string u;
  map< std::string, vector<int> > Only; // Only[u] = avail nodes that depend on u
                                        // but on no other node of Cluster

  int curIdx;
  std::string curNode;
  // true iff a cluster-separating allocation exists
  bool sepAllocExists = false;
 
  // build the vector this->Cluster
  GetConstraints();
 
// Try to find an allocation that depends upon *exactly* one node of Cluster
// if this is impossible, remove last (i.e. least important) node of Cluster
// and try again until cluster is down to just two nodes

    
  unsigned int i,j;
  int depcount;
  std::string suspect; // this will be set to a node u such that a \in D[u]
  vector<int> FreeNodes; // not dependent on any node in Cluster

// We assume that suspect list is ordered by importance.
// if we cannot get separating allocation for Cluster = {u_1 ... u_k} then try to get
// separating allocation for {u_1 ... u_{k-1}} and so on.
// TODO: is there a more efficient way to do this?
// there is a lot of redundant computation here
while((Cluster.size() > 1) && !sepAllocExists){

    // classify available nodes
    // for i in Cluster
    // find sets D[u_i] \minus \union_{j \in Cluster and j \neq i} D[u_j]
    // along with 'free set'
    Only.clear();
    FreeNodes.clear();
    for(i=0; i<available->size(); i++){
        curIdx  = (*available)[i];
        curNode = ((SimpleMachine*)machine)->getNodeID(curIdx);
        depcount = 0; //how many D[u] contain current available compute node
        for (j=0; j<Cluster.size(); j++){
            if (D[Cluster[j]].count(curNode)){
                depcount++;
                suspect = Cluster[j];
            }
        }
        if (depcount == 0) // curNode is a 'free' node 
            FreeNodes.push_back(curIdx);
        if (depcount == 1) // curNode depends on exactly one node in cluster
            Only[suspect].push_back(curIdx); // add curNode to appropriate set       
    }

    // find a separating allocation if it exists
    for (j=0; j<Cluster.size(); j++){
        if ((Only[Cluster[j]].size() > 0) && (Only[Cluster[j]].size() + FreeNodes.size() > numProcs)){
            //generate allocation separating u = Cluster[j] from all other nodes
            u = Cluster[j];
            sepAllocExists = true;
            break; // Cluster is ordered so that we prefer to separate the first one we find
        }
    }
    if (!sepAllocExists) //Relax constraints by dropping a node from Cluster
        Cluster.pop_back();

}


//TODO:what if no such allocation exists? Could go on to the
//next cluster; but that might be a lot of work to handle nodes whose fault rates
//are too small to matter. Instead:
//TODO: split two clusters simultaneously
//////////////////////////

 
  // TODO:There are other choices here; 
  // can use one node from D[u] \minus \union_{v \neq u} D[v]
  // then use as many free nodes as possible, or might try to use
  // both sets eaually, or in porportion to their relative sizes.
  // this is lower priority; current heuristic of 'use up Only[u] first'
  // is reasonable
  if (sepAllocExists) { // allocate to depend on u only
    cout << "Found Allocation Separating " << u << " for job "  << job->getJobNum() << std::endl;
    i=0;
    while((i<Only[u].size()) && (Alloc.size() < numProcs))
        Alloc.push_back(Only[u][i++]);
    i=0;
    while((i<FreeNodes.size()) && (Alloc.size() < numProcs))
        Alloc.push_back(FreeNodes[i++]);
    for(i=0; i<numProcs; i++) 
        retVal->nodeIndices[i] = Alloc[i];
  }
  else {
      //cout << "Failed to find separating allocation for job " << job.getJobNum() << std:endl;
      // return 'empty' allocation to SimpleMachine;
      // this will produce default allocation
      retVal->nodeIndices[0] = -1;
  }

  available->clear();
  delete available;
  return retVal;
}
