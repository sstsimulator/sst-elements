
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
 Attempt to allocate compute nodes subject to constraints
 Presumably constraints designed to minimize uncertainty
 i.e. to maximize the amount of information that job failures
 will give about system parameters
 */

#ifndef SST_SCHEDULER_CONSTRAINTALLOCATOR_H__
#define SST_SCHEDULER_CONSTRAINTALLOCATOR_H__

#include <string>
#include <vector>
#include <list>
#include <map>
#include <set>
#include "Allocator.h"

namespace SST {
    namespace Scheduler {

        class SimpleMachine;
        class Job;


	class ConstrainedAllocation{
		public:
			Job * job;
			std::set<int> constrained_nodes;
			std::set<int> unconstrained_nodes;
	};



        class ConstraintAllocator : public Allocator {

            public:
                ConstraintAllocator(SimpleMachine* m, std::string DepsFile, std::string ConstFile);

                //ConstraintAllocator Make(vector<string*>* params);

                std::string getParamHelp();

                std::string getSetupInfo(bool comment);

                AllocInfo* allocate(Job* job);
                ConstrainedAllocation * allocate_constrained(Job* job, std::set<std::string> * constrained_leaves );

            private:
                //constraints
                //for now, only type of constraint is to separate one node from rest of cluster
                //check file for updates to parameter estimates and set constraints accordingly
                void GetConstraints();

		AllocInfo * generate_AllocInfo( ConstrainedAllocation * constrained_alloc );
		ConstrainedAllocation * get_top_allocation( std::list<ConstrainedAllocation *> possible_allocations );
		std::set< std::string > * get_constrained_leaves( std::vector<std::string> constraint );
		void read_constraints();

                //map from internal node u to  set of dependent compute nodes D[u]
                std::map< std::string, std::set<std::string> > D;
                std::string ConstraintsFileName;

		std::list< std::set< std::string > * > constraint_leaves;
        };

#endif

    }
}
