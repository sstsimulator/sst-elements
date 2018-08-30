
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

#include "sst_config.h"
#include "ConstraintAllocator.h"

#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>

#include "AllocInfo.h"
#include "Job.h"
#include "Machine.h"
#include "output.h"
#include "SimpleMachine.h"
#include "schedComponent.h"

#include <sst/core/stringize.h>

#define DEBUG false

using namespace SST::Scheduler;

ConstraintAllocator::ConstraintAllocator(SimpleMachine* m, std::string DepsFile, std::string ConstFile, schedComponent* sc) 
    : Allocator(*m)
{
    schedout.init("", 8, 0, Output::STDOUT);
    ConstraintsFileName = ConstFile;
    this->sc = sc;
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
        schedout.debug(CALL_INFO, 7, 0, "------------------Dependencies of %s", u.c_str());
        while (lineStream >> v) {
            D[u].insert(v);
            if( DEBUG ) std::cout << v << " ";
            schedout.debug(CALL_INFO, 7, 0, "%s ", v.c_str());
        }
        if( DEBUG ) std::cout << std::endl;
        lineStream.clear(); //so we can write to it again
    }

    long int seed = 42;

    allocPRNGstate = (unsigned short *) malloc( 3 * sizeof( short ) );

    allocPRNGstate[ 0 ] = 0x330E;
    allocPRNGstate[ 1 ] = seed & 0xFFFF;
    allocPRNGstate[ 2 ] = seed >> 16;
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

std::string ConstraintAllocator::getSetupInfo(bool comment) const
{
    std::string com;
    if (comment) {
        com = "# ";
    } else { 
        com = "";
    }
    return com + "Constraint Allocator";
}

//allocates job if possible
//returns info on the allocation or NULL if it wasn't possible
AllocInfo* ConstraintAllocator::allocate(Job* job){
	AllocInfo * allocation = NULL;

	static int count = 0;

	std::vector<int> * freeNodes = machine.getFreeNodes();

	if( (unsigned) ceil((double) job->getProcsNeeded() / machine.coresPerNode) <= freeNodes->size() ){
		if( constraints_changed() ){
			read_constraints();
		}

		++ count;

		std::list<ConstrainedAllocation *> possible_allocations;

		ConstrainedAllocation * top_allocation = NULL;

                int i = 1;
		for( std::list<std::vector<std::string> * >::iterator constraint_iter = constraints.begin();
		     constraint_iter != constraints.end(); ++ constraint_iter ){
                            if (DEBUG) std::cout << "Attempting constraint " << i 
                                        << " for jobid " << *(job->getID()) << std::endl;
			top_allocation = allocate_constrained( job, *constraint_iter ); 
                        if (top_allocation != NULL) {
                            if (DEBUG) std::cout << " SUCCESS in satisfying constraint " << i 
                                        << " for jobid " << *(job->getID()) << std::endl;
                            break; // stop searching as soon as a constraint is satisfied
                        }
                        i++;
		}

		if( top_allocation != NULL ){
			allocation = generate_AllocInfo( top_allocation );
		}else{
                        if (DEBUG) std::cout << " FAILED to satisfy any constraint" 
                                        << " for jobid " << *(job->getID()) << std::endl;
			allocation = generate_RandomAllocInfo( job );
		}

		while( ! possible_allocations.empty() ){
			delete possible_allocations.back();
			possible_allocations.pop_back();
		}

	}

	delete freeNodes;

	return allocation;
}


AllocInfo * ConstraintAllocator::generate_RandomAllocInfo( Job * job ){
	AllocInfo * alloc = new AllocInfo( job, machine );
	std::vector<int> * free_comp_nodes = machine.getFreeNodes();
    
    int numNodes = ceil((double) job->getProcsNeeded() / machine.coresPerNode);

	for( int node_counter = 0; node_counter < numNodes; node_counter ++ ){
#define LINEAR_FROM_TOP true
#ifdef LINEAR_FROM_TOP
                // mimic the simple allocator by selecting nodes linearly, from the top
                std::vector<int>::iterator node_iter = free_comp_nodes->end();
                node_iter--;
#else
                // otherwise, randomize the node selection (which reduces uncertainty in faultiness estimates)
		std::vector<int>::iterator node_iter = free_comp_nodes->begin();
		std::advance( node_iter, (nrand48( allocPRNGstate ) % free_comp_nodes->size()) );
#endif
		alloc->nodeIndices[ node_counter ] = *node_iter;
		free_comp_nodes->erase( node_iter );
	}
	
	delete free_comp_nodes;

	return alloc;
}


AllocInfo * ConstraintAllocator::generate_AllocInfo( ConstrainedAllocation * constrained_alloc ){
	AllocInfo * alloc = new AllocInfo( constrained_alloc->job, machine );

	int node_counter = 0;

	for( std::set<int>::iterator unconstrained_node_iter = constrained_alloc->unconstrained_nodes.begin();
	     unconstrained_node_iter != constrained_alloc->unconstrained_nodes.end(); ++ unconstrained_node_iter ){
		alloc->nodeIndices[ node_counter ] = *unconstrained_node_iter;
		++ node_counter;
	}

	for( std::set<int>::iterator constrained_node_iter = constrained_alloc->constrained_nodes.begin();
	     constrained_node_iter != constrained_alloc->constrained_nodes.end(); ++ constrained_node_iter ){
		alloc->nodeIndices[ node_counter ] = *constrained_node_iter;
		++ node_counter;
	}

	return alloc;
}


bool ConstraintAllocator::constraints_changed(){
	return true;
}


void ConstraintAllocator::read_constraints(){
    SST::char_delimiter space_separator( " " );
	std::ifstream ConstraintsStream(ConstraintsFileName.c_str(), std::ifstream::in );

	while( !constraint_leaves.empty() ){
		delete constraint_leaves.back();
		constraint_leaves.pop_back();
	}

	while( !constraints.empty() ){
		delete constraints.back();
		constraints.pop_back();
	}

	while(!ConstraintsStream.eof() and ConstraintsStream.is_open()){
		std::string curline;
		std::vector<std::string> * CurrentCluster = new std::vector<std::string>();

		getline(ConstraintsStream, curline);
        SST::Tokenizer<> tok( curline, space_separator );
		for (SST::Tokenizer<>::iterator iter = tok.begin(); iter != tok.end(); ++iter) {
			CurrentCluster->push_back(*iter);
		}

		this->constraints.push_back( CurrentCluster );
		this->constraint_leaves.push_back( get_constrained_leaves( CurrentCluster ) );
	}

	ConstraintsStream.close();
}


std::set< std::string > * ConstraintAllocator::get_constrained_leaves( std::vector<std::string> * constraint ){
	std::set< std::string > * leaves = new std::set<std::string>;

	for( std::vector<std::string>::iterator constraint_iter = constraint->begin();
	     constraint_iter != constraint->end(); ++ constraint_iter ){
		std::set<std::string> constraint_children = D[ *constraint_iter ];
		for( std::set<std::string>::iterator constraint_child_iter = constraint_children.begin();
		     constraint_child_iter != constraint_children.end(); ++ constraint_child_iter ){
			if( 1 == D[ *constraint_child_iter ].size() ){
				leaves->insert( *constraint_child_iter );
			}
		}
	}

	return leaves;
}


std::set< std::string > * ConstraintAllocator::get_constrained_leaves( std::string constraint ){
	std::set< std::string > * leaves = new std::set<std::string>;

	std::set<std::string> constraint_children = D[ constraint ];
	for( std::set<std::string>::iterator constraint_child_iter = constraint_children.begin();
	     constraint_child_iter != constraint_children.end(); ++ constraint_child_iter ){
		if( 1 == D[ *constraint_child_iter ].size() ){
			leaves->insert( *constraint_child_iter );
		}
	}

	return leaves;
}


// returns an allocation satisifying the given constraint, or NULL if it can not be satisifed 
// satisfied means: at least one constrained node used and one constraint node avoided
ConstrainedAllocation * ConstraintAllocator::allocate_constrained(Job* job, std::vector<std::string> * nodes_on_constraint_line ){
	std::vector<int> * all_available_compute_nodes = machine.getFreeNodes();
	std::vector<int> * unconstrained_compute_nodes = machine.getFreeNodes();
	std::list<std::vector<int> *> * constrained_compute_nodes = new std::list<std::vector<int> *>(); 

	std::sort( all_available_compute_nodes->begin(), all_available_compute_nodes->end() );
	std::sort( unconstrained_compute_nodes->begin(), unconstrained_compute_nodes->end() );

        // identify available compute nodes as unconstrained, or by which constraint node(s) they depend on
	for( std::vector<std::string>::iterator constraint_node = nodes_on_constraint_line->begin();
	     constraint_node != nodes_on_constraint_line->end(); ++ constraint_node ){
		std::set<std::string> * dependent_compute_node_IDs = this->get_constrained_leaves( *constraint_node );
		std::vector<int> * dependent_compute_nodes = new std::vector<int>();
		
		for( std::vector<int>::iterator comp_node_iter = all_available_compute_nodes->begin();
		     comp_node_iter != all_available_compute_nodes->end(); ++ comp_node_iter ){
			if( dependent_compute_node_IDs->find( sc->getNodeID( *comp_node_iter ) ) !=
			    dependent_compute_node_IDs->end() ){
				dependent_compute_nodes->push_back( *comp_node_iter );
				
			}
		}

		std::sort( dependent_compute_nodes->begin(), dependent_compute_nodes->end() );
		constrained_compute_nodes->push_back( dependent_compute_nodes );
		
		std::vector<int> * new_unconstrained_compute_nodes = new std::vector<int>( unconstrained_compute_nodes->size() );
		std::vector<int>::iterator unconstrained_iter = std::set_difference(
			unconstrained_compute_nodes->begin(),
			unconstrained_compute_nodes->end(),
			dependent_compute_nodes->begin(),
			dependent_compute_nodes->end(),
			new_unconstrained_compute_nodes->begin() );
		new_unconstrained_compute_nodes->resize( unconstrained_iter - new_unconstrained_compute_nodes->begin() );
		unconstrained_compute_nodes = new_unconstrained_compute_nodes;
		std::sort( unconstrained_compute_nodes->begin(), unconstrained_compute_nodes->end() );
	}
	
	unsigned numNodes = ceil((double) job->getProcsNeeded() / machine.coresPerNode);
	
	int num_constrained_needed = 0;
	if( unconstrained_compute_nodes->size() >= numNodes ){
		num_constrained_needed = 1;
	}else{
		num_constrained_needed = numNodes - unconstrained_compute_nodes->size();
	}

        // try to remove at least one constraint node (including those with no available compute nodes)
	if( !try_to_remove_constraint_set( num_constrained_needed, constrained_compute_nodes ) ){
		/* cleanup */
		return NULL;
	}

        // at least one constraint node is removed, try to remove more
	while( try_to_remove_constraint_set( num_constrained_needed, constrained_compute_nodes ) ){}

	ConstrainedAllocation * new_allocation = new ConstrainedAllocation();
	new_allocation->job = job;

        // ok, allocate as many nodes from the remainining constrained sets as possible
	for( std::list<std::vector<int> *>::iterator constraint_node = constrained_compute_nodes->begin();
	     constraint_node != constrained_compute_nodes->end(); ++constraint_node ){
		for( std::vector<int>::reverse_iterator constrained_node = (*constraint_node)->rbegin();
		     (constrained_node != (*constraint_node)->rend()) && ((new_allocation->constrained_nodes.size() + new_allocation->unconstrained_nodes.size()) < numNodes); ++constrained_node ){
			if (DEBUG) std::cout << " Adding constrained node: " << *constrained_node << std::endl;
			new_allocation->constrained_nodes.insert( *constrained_node );
		}
	}


        // and fill the balance with unconstrained nodes
	for( std::vector<int>::reverse_iterator unconstrained_node = unconstrained_compute_nodes->rbegin();
	     unconstrained_node != unconstrained_compute_nodes->rend() && ((new_allocation->constrained_nodes.size() + new_allocation->unconstrained_nodes.size()) < numNodes); ++unconstrained_node ){
		if (DEBUG) std::cout << " Adding unconstrained node: " << *unconstrained_node << std::endl;
		new_allocation->unconstrained_nodes.insert( *unconstrained_node );
	}


	/* cleanup */

	return new_allocation;
}


bool ConstraintAllocator::try_to_remove_constraint_set( unsigned int num_constrained_needed, std::list<std::vector<int> *> * constrained_compute_nodes ){
	std::vector<int> * all_nodes = new std::vector<int>();

        if (constrained_compute_nodes->size() == 1 ) { // must use at least one constrained compute node
            return false;
        }

        int zero_size_sets = false;
	for( std::list<std::vector<int> *>::iterator constraint_node = constrained_compute_nodes->begin();
	     constraint_node != constrained_compute_nodes->end();){
             if ((*constraint_node)->size() == 0) {  // remove all zero-size sets
                zero_size_sets = true;
                if (DEBUG) std::cout << " Removing zero-size set" << std::endl;
                constraint_node = constrained_compute_nodes->erase(constraint_node);
             }
             else {
		std::vector<int> * tmp_all_nodes = new std::vector<int>( all_nodes->size() + (*constraint_node)->size() );
		std::vector<int>::iterator iter = std::set_union(
			all_nodes->begin(),
			all_nodes->end(),
			(*constraint_node)->begin(),
			(*constraint_node)->end(),
			tmp_all_nodes->begin() );
		tmp_all_nodes->resize( iter - tmp_all_nodes->begin() );
		all_nodes = tmp_all_nodes;
	        std::sort( all_nodes->begin(), all_nodes->end() );
                ++constraint_node;
             }
	}

        if (all_nodes->size() == 0) { // must use at least one constrained compute node
            return false;
        }
        else if (zero_size_sets) { // we successfully removed at least one constraint node
            return true;
        }

        int i=1;
	for( std::list<std::vector<int> *>::iterator constraint_node = constrained_compute_nodes->begin();
	     constraint_node != constrained_compute_nodes->end(); ++ constraint_node ){

		if( (all_nodes->size() - (*constraint_node)->size()) >= num_constrained_needed ){
		    if (DEBUG) std::cout << " Removing node " << i << ": allsize-thissize >= needed (" << 
                                        all_nodes->size() << "-" << (*constraint_node)->size() << " >= " <<  
                                        num_constrained_needed << ")" << std::endl;
			std::vector<int> * removed_constraint = *constraint_node;
			constrained_compute_nodes->erase( constraint_node ); // remove this constraint node
                        // and its compute nodes from the other sets
			for( std::list<std::vector<int> *>::iterator inner_constraint_node = constrained_compute_nodes->begin();
			     inner_constraint_node != constrained_compute_nodes->end(); ++ inner_constraint_node ){
				std::vector<int> * tmp_constraint = new std::vector<int>( (*inner_constraint_node)->size() );
				std::vector<int>::iterator iter = std::set_difference(
					(*inner_constraint_node)->begin(),
					(*inner_constraint_node)->end(),
					removed_constraint->begin(),
					removed_constraint->end(),
					tmp_constraint->begin() );
				tmp_constraint->resize( iter - tmp_constraint->begin() );
				std::sort( tmp_constraint->begin(), tmp_constraint->end() );
				(*inner_constraint_node)->assign( tmp_constraint->begin(), tmp_constraint->end() );
			}
			++constraint_node;
                        i++;
			/* cleanup */
			return true;
		}
	}
	/* cleanup */
	return false;
}


std::list<std::vector<int> *> * deep_copy_set_list( std::list<std::vector<int> *> * list ){
	std::list<std::vector<int> *> * new_list = new std::list<std::vector<int> *>();
	for( std::list<std::vector<int> *>::iterator list_iter = list->begin();
	     list_iter != list->end(); ++ list_iter ){
		std::vector<int> * new_set = new std::vector<int>();
		new_set->assign( (*list_iter)->begin(), (*list_iter)->end() );
		new_list->push_back( new_set );
	}

	return new_list;
}

