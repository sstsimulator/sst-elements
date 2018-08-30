
// Copyright 2011-2018 NTESS. Under the terms                          
// of Contract DE-NA0003525 with NTESS, the U.S.             
// Government retains certain rights in this software.                         
//                                                                             
// Copyright (c) 2011-2018, NTESS                                      
// All rights reserved.                                                        
//                                                                             
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
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

        class schedComponent;
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
                ConstraintAllocator(SimpleMachine* m, std::string DepsFile, std::string ConstFile, schedComponent* sc);

                //ConstraintAllocator Make(vector<string*>* params);

                std::string getParamHelp();

                std::string getSetupInfo(bool comment) const;

                AllocInfo* allocate(Job* job);
                ConstrainedAllocation * allocate_constrained(Job* job, std::vector<std::string> * constrained_leaves );

            private:
                //constraints
                //for now, only type of constraint is to separate one node from rest of cluster
                //check file for updates to parameter estimates and set constraints accordingly
                void GetConstraints();

		        AllocInfo * generate_RandomAllocInfo( Job * job );
		        AllocInfo * generate_AllocInfo( ConstrainedAllocation * constrained_alloc );
		        std::set< std::string > * get_constrained_leaves( std::vector<std::string> * constraint );
		        std::set< std::string > * get_constrained_leaves( std::string constraint );
		        bool try_to_remove_constraint_set( unsigned int num_constrained_needed, std::list<std::vector<int> *> * constrained_compute_nodes );
		        bool constraints_changed();
		        void read_constraints();

                //map from internal node u to  set of dependent compute nodes D[u]
                std::map< std::string, std::set<std::string> > D;
                std::string ConstraintsFileName;

		        std::list< std::set< std::string > * > constraint_leaves;
		        std::list< std::vector< std::string > * > constraints;

		        unsigned short * allocPRNGstate;
		        schedComponent* sc;
        };

#endif

    }
}
