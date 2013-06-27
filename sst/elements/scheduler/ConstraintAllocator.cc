
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

#include "sst_config.h"
#include "ConstraintAllocator.h"

#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include <boost/tokenizer.hpp>

#include "sst/core/serialization/element.h"

#include "AllocInfo.h"
#include "Job.h"
#include "Machine.h"
#include "misc.h"
#include "SimpleMachine.h"

#define DEBUG false

using namespace SST::Scheduler;

ConstraintAllocator::ConstraintAllocator(SimpleMachine* m, std::string DepsFile, std::string ConstFile) 
{
    machine = m;
    ConstraintsFileName = ConstFile;
    // read Dependencies
    // if file does not exist or is empty, D will be an empty mapping
    // and in effect we default to simple allocator
    std::ifstream DepsStream(DepsFile.c_str(),  std::ifstream::in );
    std::string u,v;
    std::string curline;
    std::stringstream lineStream;
    while (std::getline(DepsStream, curline)) { //for each line in file
        lineStream << curline;
        lineStream >> u; // line is u followed by elements of D[u]
        if( DEBUG ) std::cout << "------------------Dependencies of " << u << std::endl;
        while (lineStream >> v) {
            D[u].insert(v);
            if( DEBUG ) std::cout << v << " ";
        }
        if( DEBUG ) std::cout << std::endl;
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
void ConstraintAllocator::GetConstraints()
{
}

std::string ConstraintAllocator::getParamHelp()
{
    return "";
}

std::string ConstraintAllocator::getSetupInfo(bool comment)
{
    std::string com;
    if (comment) {
        com = "# ";
    } else { 
        com = "";
    }
    return com + "Constraint Allocator";
}

AllocInfo* ConstraintAllocator::allocate(Job* job){
    AllocInfo * allocation = NULL;
    
    boost::char_separator<char> space_separator( " " );

    std::string u;
    std::ifstream ConstraintsStream(ConstraintsFileName.c_str(), std::ifstream::in );
    std::string curline;
    std::stringstream lineStream;
    std::vector<std::string> CurrentCluster;
    //for now just read first line

    do{
        if( allocation ){
            delete allocation;
                // we had a failed allocation hanging around from last time
        }
        
        CurrentCluster.clear();

        getline(ConstraintsStream, curline);
        boost::tokenizer< boost::char_separator<char> > tok( curline, space_separator );
        for( boost::tokenizer< boost::char_separator<char> >::iterator iter = tok.begin(); iter != tok.end(); ++iter ){
            CurrentCluster.push_back( *iter );
        }

        allocation = allocate( job, CurrentCluster );
        if( -1 != allocation->nodeIndices[0] ){
            // the allocation succeeded, we're done!
            break;
        }
    }while( !ConstraintsStream.eof() and ConstraintsStream.is_open() );
        // yes, we check it the file is open *after* we read it.
        // The original code relied on the fact that getline gives you an empty string if you read a file that isn't there.

    return allocation;
}

AllocInfo* ConstraintAllocator::allocate(Job* job, std::vector<std::string> Cluster){
    //allocates job if possible
    //returns information on the allocation or null if it wasn't possible

    if (!canAllocate(job)) return NULL;

    AllocInfo* retVal = new AllocInfo(job);
    unsigned int numProcs = job -> getProcsNeeded();
    std::vector<int>* available = ((SimpleMachine*)machine) -> freeProcessors(); 
    // the allocation we will return
    std::vector<int> Alloc;
    std::string u;
    std::map <std::string, std::vector<int> > Only; // Only[u] = avail nodes that depend on u
    // but on no other node of Cluster

    int curIdx;
    std::string curNode;
    // true iff a cluster-separating allocation exists
    bool sepAllocExists = false;

    // Try to find an allocation that depends upon *exactly* one node of Cluster
    // if this is impossible, remove last (i.e. least important) node of Cluster
    // and try again until cluster is down to just two nodes


    unsigned int i,j;
    int depcount;
    std::string suspect; // this will be set to a node u such that a \in D[u]
    std::vector<int> FreeNodes; // not dependent on any node in Cluster


    // We assume that suspect list is ordered by importance.
    // if we cannot get separating allocation for Cluster = {u_1 ... u_k} then try to get
    // separating allocation for {u_1 ... u_{k-1}} and so on.
    // TODO: is there a more efficient way to do this?
    // there is a lot of redundant computation here
    while ((Cluster.size() > 1) && !sepAllocExists) {

        // classify available nodes
        // for i in Cluster
        // find sets D[u_i] \minus \union_{j \in Cluster and j \neq i} D[u_j]
        // along with 'free set'
        Only.clear();
        FreeNodes.clear();
        for (i = 0; i < available -> size(); i++) {
            curIdx  = (*available)[i];
            curNode = ((SimpleMachine*)machine) -> getNodeID(curIdx);
            depcount = 0; //how many D[u] contain current available compute node
            for (j = 0; j < Cluster.size(); j++) {
                if (D[Cluster[j]].count(curNode)) {
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
        for (j = 0; j < Cluster.size(); j++){
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

    //TODO: split two clusters simultaneously
    //////////////////////////


    // TODO:There are other choices here; 
    // can use one node from D[u] \minus \union_{v \neq u} D[v]
    // then use as many free nodes as possible, or might try to use
    // both sets eaually, or in porportion to their relative sizes.
    // this is lower priority; current heuristic of 'use up Only[u] first'
    // is reasonable
    if (sepAllocExists) { // allocate to depend on u only
        if( DEBUG ) std::cout << "Found Allocation Separating " << u << " for job "  << job -> getJobNum() << std::endl;
        i = 0;
        while((i<Only[u].size()) && (Alloc.size() < numProcs))
            Alloc.push_back(Only[u][i++]);
        i=0;
        while((i<FreeNodes.size()) && (Alloc.size() < numProcs))
            Alloc.push_back(FreeNodes[i++]);
        for(i=0; i<numProcs; i++) 
            retVal -> nodeIndices[i] = Alloc[i];
    }
    else {
        //std::cout << "Failed to find separating allocation for job " << job.getJobNum() << std:endl;
        // return 'empty' allocation to SimpleMachine;
        // this will produce default allocation
        retVal -> nodeIndices[0] = -1;
    }

    available -> clear();
    delete available;
    return retVal;
}
