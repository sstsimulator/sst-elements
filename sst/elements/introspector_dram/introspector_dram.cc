// Copyright 2009-2010 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2010, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>
#include "sst/core/serialization/element.h"

#include <boost/mpi.hpp>

#include "introspector_dram.h"
#include "sst/core/element.h"


bool Introspector_dram::pullData( Cycle_t current )
{
    //_INTROSPECTOR_DBG("id=%lu currentCycle=%lu \n", Id(), current );
	IntrospectedComponent *c;

        printf("introspector_dram pulls data @ cycle %ld\n", (long int)current ); //current here specifies it's the #th call
	for( Database_t::iterator iter = DatabaseInt.begin();
                            iter != DatabaseInt.end(); ++iter )
    	{
	        c = iter->second;
		for (int i = 0; i < 2; ++i) { //2 is the NUM_RANKS in DRAMSim
                    std::cout << "Pull data of component ID " << c->getId() << " with dataID = " << iter->first << "; index = " << i << " and data value = " << c->getIntData(iter->first, i) << std::endl;
	            intData = c->getIntData(iter->first, i);
		}
	}
	for( Database_t::iterator iter = DatabaseDouble.begin();
                            iter != DatabaseDouble.end(); ++iter )
    	{
            c = iter->second;
            std::cout << "Pull data of component ID " << c->getId() << " with dataID = " << iter->first << " and data value = " << c->getDoubleData(iter->first) << std::endl;
	    
	}

	return false;
}


bool Introspector_dram::mpiCollectInt( Cycle_t current )
{
	boost::mpi::communicator world;

	
	collectInt(REDUCE, intData, MINIMUM);
	collectInt(REDUCE, intData, MAXIMUM);
	//collectInt(ALLREDUCE, intData, SUM);
        collectInt(BROADCAST, intData, NOT_APPLICABLE, 1); // rank 1 will broadcast the value 
	collectInt(GATHER, intData, NOT_APPLICABLE);
	//collectInt(ALLGATHER, intData, NOT_APPLICABLE);

	if (world.rank() == 0){
	    std::cout << " The minimum value of data is " << minvalue << std::endl;
	    std::cout << " The maximum value of data is " << maxvalue << std::endl;

	    std::cout << "Gather data into vector: ";
   	    for(unsigned int ii=0; ii < arrayvalue.size(); ii++)
            {
                 std::cout << arrayvalue[ii] << " ";
            }
	    std::cout << std::endl;
	}
	//std::cout << " The sum of data is " << value << std::endl;
	std::cout << " The value of the broadcast data is " << value << std::endl;
	/*std::cout << "All_gather data into vector: ";
   	for(int ii=0; ii < arrayvalue.size(); ii++)
        {
             std::cout << arrayvalue[ii] << " ";
        }
	std::cout << std::endl;*/

	return (false);
}

//An example MPI collect functor that is put into the queue.
//Introspector-writer implements their own MPI functor and pass that
//to Introspector::oneTimeCollect().
void Introspector_dram::mpiOneTimeCollect( Event* e)
{
//option 1: utilize the built-in basic collective communication calls
/*	boost::mpi::communicator world;

	collectInt(REDUCE, intData, MINIMUM);

	if (world.rank() == 0){
	    std::cout << "One Time Collect:: The minimum value of data is " << minvalue << std::endl;
	}
*/

//option2: implement it on your own 
	boost::mpi::communicator world;

	
	if (world.rank() == 0){
	    reduce( world, intData, maxvalue, boost::mpi::maximum<int>(), 0); 
    	    std::cout << "One Time Collect: The maximum value is " << maxvalue << std::endl;
  	} else {
    	    reduce(world, intData, boost::mpi::maximum<int>(), 0);
  	} 
	return;
}


static Introspector*
create_introspector_dram(SST::Component::Params_t &params)
{
    return new Introspector_dram(params);
}

static const ElementInfoIntrospector introspectors[] = {
    { "introspector_dram",
      "DRAM Introspector",
      NULL,
      create_introspector_dram,
    },
    { NULL, NULL, NULL, NULL }
};

extern "C" {
    ElementLibraryInfo introspector_dram_eli =  {
        "introspector_dram",
        "DRAM Introspector",
        NULL,
        NULL,
        introspectors
    };
}


BOOST_CLASS_EXPORT(Introspector_dram)

