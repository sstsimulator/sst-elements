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

#include "introspector_cpu.h"
#include "sst/core/element.h"


bool Introspector_cpu::pullData( Cycle_t current )
{
    //_INTROSPECTOR_DBG("id=%lu currentCycle=%lu \n", Id(), current );
	IntrospectedComponent *c;

        printf("introspector_cpu pulls data @ cycle %ld\n", (long int)current ); //current here specifies it's the #th call
	for( Database_t::iterator iter = DatabaseInt.begin();
                            iter != DatabaseInt.end(); ++iter )
    	{
	    c = iter->first;
            std::cout << "Pull data of component ID " << c->getId() << " with dataID = " << iter->second << " and data value = " << c->getIntData(iter->second) << std::endl;
	    intData = c->getIntData(iter->second);
	}
	for( Database_t::iterator iter = DatabaseDouble.begin();
                            iter != DatabaseDouble.end(); ++iter )
    	{
            c = iter->first;
            std::cout << "Pull data of component ID " << c->getId() << " with dataID = " << iter->second << " and data value = " << c->getDoubleData(iter->second) << std::endl;
	    
	}

	return false;
}


bool Introspector_cpu::mpiCollectInt( Cycle_t current )
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
void Introspector_cpu::mpiOneTimeCollect( Event* e)
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
create_introspector_cpu(SST::Component::Params_t &params)
{
    return new Introspector_cpu(params);
}

static const ElementInfoIntrospector introspectors[] = {
    { "introspector_cpu",
      "CPU Introspector",
      NULL,
      create_introspector_cpu,
    },
    { NULL, NULL, NULL, NULL }
};

extern "C" {
    ElementLibraryInfo introspector_cpu_eli =  {
        "introspector_cpu",
        "CPU Introspector",
        NULL,
        NULL,
        introspectors
    };
}

/*template<class Archive>
void 
Introspector_cpu::serialize(Archive & ar, const unsigned int version )
    {
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
        ar & BOOST_SERIALIZATION_NVP(params);
        ar & BOOST_SERIALIZATION_NVP(frequency);
	ar & BOOST_SERIALIZATION_NVP(model);
    }

SST_BOOST_SERIALIZATION_INSTANTIATE(SST::Introspector_cpu::serialize)*/
BOOST_CLASS_EXPORT(Introspector_cpu)

