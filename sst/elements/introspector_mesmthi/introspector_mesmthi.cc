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

#include "introspector_mesmthi.h"
#include "sst/core/element.h"

#include <string>
#include <sstream>




bool Introspector_mesmthi::triggeredUpdate()
{
	std::ostringstream setname;
	for (std::list<IntrospectedComponent*>::iterator i = MyCompList.begin();
	     i != MyCompList.end(); ++i) {	
		std::cout << "Pull data from component ID " << (*i)->getId() << " and MC reads = " << std::dec << getData<uint64_t>(*i, "MCreads") << std::endl;
		std::cout << "Pull data from component ID " << (*i)->getId() << " and MC writes = " << std::dec << getData<uint64_t>(*i, "MCwrites") << std::endl;
		
		for (int idx = 0; idx < 16; idx++){
      		    setname << "Tile[" << idx << "]DIRreads";
      		    std::cout << "Pull data of component ID " << (*i)->getId() << " and tile[" << idx << "]DIRreads = " << std::dec << getData<uint64_t>(*i, setname.str() ) << std::endl;
		    totalDIRreads += getData<uint64_t>(*i, setname.str());
		    setname << "Tile[" << idx << "]DIRwrites";
      		    std::cout << "Pull data of component ID " << (*i)->getId() << " and tile[" << idx << "]DIRwrite = " << std::dec << getData<uint64_t>(*i, setname.str() ) << std::endl;
		    totalDIRwrites += getData<uint64_t>(*i, setname.str());

		    setname << "Tile[" << idx << "]L1Ireads";
      		    std::cout << "Pull data of component ID " << (*i)->getId() << " and tile[" << idx << "]L1Ireads = " << std::dec << getData<uint64_t>(*i, setname.str() ) << std::endl;
		    totalL1Ireads += getData<uint64_t>(*i, setname.str());
		    setname << "Tile[" << idx << "]L1Iwrites";
      		    std::cout << "Pull data of component ID " << (*i)->getId() << " and tile[" << idx << "]L1Iwrites = " << std::dec << getData<uint64_t>(*i, setname.str() ) << std::endl;
		    totalL1Iwrites += getData<uint64_t>(*i, setname.str());
		    setname << "Tile[" << idx << "]L1Ireadhits";
      		    std::cout << "Pull data of component ID " << (*i)->getId() << " and tile[" << idx << "]L1Ireadhits = " << std::dec << getData<uint64_t>(*i, setname.str() ) << std::endl;
		    totalL1Ireadhits += getData<uint64_t>(*i, setname.str());
		    setname << "Tile[" << idx << "]L1Iwritehits";
      		    std::cout << "Pull data of component ID " << (*i)->getId() << " and tile[" << idx << "]L1Iwritehits = " << std::dec << getData<uint64_t>(*i, setname.str() ) << std::endl;
		    totalL1Iwritehits += getData<uint64_t>(*i, setname.str());

		    setname << "Tile[" << idx << "]L1Dreads";
      		    std::cout << "Pull data of component ID " << (*i)->getId() << " and tile[" << idx << "]L1Dreads = " << std::dec << getData<uint64_t>(*i, setname.str() ) << std::endl;
		    totalL1Dreads += getData<uint64_t>(*i, setname.str());
		    setname << "Tile[" << idx << "]L1Dwrites";
      		    std::cout << "Pull data of component ID " << (*i)->getId() << " and tile[" << idx << "]L1Dwrites = " << std::dec << getData<uint64_t>(*i, setname.str() ) << std::endl;
		    totalL1Dwrites += getData<uint64_t>(*i, setname.str());
		    setname << "Tile[" << idx << "]L1Dreadhits";
      		    std::cout << "Pull data of component ID " << (*i)->getId() << " and tile[" << idx << "]L1Dreadhits = " << std::dec << getData<uint64_t>(*i, setname.str() ) << std::endl;
		    totalL1Dreadhits += getData<uint64_t>(*i, setname.str());
		    setname << "Tile[" << idx << "]L1Dwritehits";
      		    std::cout << "Pull data of component ID " << (*i)->getId() << " and tile[" << idx << "]L1Dwritehits = " << std::dec << getData<uint64_t>(*i, setname.str() ) << std::endl;
		    totalL1Dwritehits += getData<uint64_t>(*i, setname.str());


		    setname << "Tile[" << idx << "]L2reads";
      		    std::cout << "Pull data of component ID " << (*i)->getId() << " and tile[" << idx << "]L2reads = " << std::dec << getData<uint64_t>(*i, setname.str() ) << std::endl;
		    totalL2reads += getData<uint64_t>(*i, setname.str());
		    setname << "Tile[" << idx << "]L2writes";
      		    std::cout << "Pull data of component ID " << (*i)->getId() << " and tile[" << idx << "]L2writes = " << std::dec << getData<uint64_t>(*i, setname.str() ) << std::endl;
		    totalL2writes += getData<uint64_t>(*i, setname.str());
		    setname << "Tile[" << idx << "]L2readhits";
      		    std::cout << "Pull data of component ID " << (*i)->getId() << " and tile[" << idx << "]L2readhits = " << std::dec << getData<uint64_t>(*i, setname.str() ) << std::endl;
		    totalL2readhits += getData<uint64_t>(*i, setname.str());
		    setname << "Tile[" << idx << "]L2writehits";
      		    std::cout << "Pull data of component ID " << (*i)->getId() << " and tile[" << idx << "]L2writehits = " << std::dec << getData<uint64_t>(*i, setname.str() ) << std::endl;
		    totalL2writehits += getData<uint64_t>(*i, setname.str());


		    setname << "Tile[" << idx << "]Instructions";
      		    std::cout << "Pull data of component ID " << (*i)->getId() << " and tile[" << idx << "]Instructions = " << std::dec << getData<uint64_t>(*i, setname.str() ) << std::endl;
		    totalInstructions += getData<uint64_t>(*i, setname.str());
		    setname << "Tile[" << idx << "]BusTransfers";
      		    std::cout << "Pull data of component ID " << (*i)->getId() << " and tile[" << idx << "]BusTransfers = " << std::dec << getData<uint64_t>(*i, setname.str() ) << std::endl;
		    totalBusTransfers += getData<uint64_t>(*i, setname.str());
		    setname << "Tile[" << idx << "]NetworkTransactions";
      		    std::cout << "Pull data of component ID " << (*i)->getId() << " and tile[" << idx << "]NetworkTransactions = " << std::dec << getData<uint64_t>(*i, setname.str() ) << std::endl;
		    totalNetworkTransactions += getData<uint64_t>(*i, setname.str());
		    setname << "Tile[" << idx << "]TotalPackets";
      		    std::cout << "Pull data of component ID " << (*i)->getId() << " and tile[" << idx << "]TotalPackets = " << std::dec << getData<uint64_t>(*i, setname.str() ) << std::endl;
		    totalPackets += getData<uint64_t>(*i, setname.str());

  		}
		
 	}
	
	std::cout << "introspector_mesmthi: TOTAL DIRreads = " << std::dec << totalDIRreads << std::endl; 
	std::cout << "introspector_mesmthi: TOTAL DIRwrites = " << std::dec << totalDIRwrites << std::endl; 
	
	std::cout << "introspector_mesmthi: TOTAL L1Ireads = " << std::dec << totalL1Ireads << std::endl; 
	std::cout << "introspector_mesmthi: TOTAL L1Iwrites = " << std::dec << totalL1Iwrites << std::endl; 
	std::cout << "introspector_mesmthi: TOTAL L1Ireadhits = " << std::dec << totalL1Ireadhits << std::endl; 
	std::cout << "introspector_mesmthi: TOTAL L1Iwritehits = " << totalL1Iwritehits << std::endl; 

	std::cout << "introspector_mesmthi: TOTAL L1Dreads = " << std::dec << totalL1Dreads << std::endl; 
	std::cout << "introspector_mesmthi: TOTAL L1Dwrites = " << std::dec << totalL1Dwrites << std::endl; 
	std::cout << "introspector_mesmthi: TOTAL L1Dreadhits = " << std::dec << totalL1Dreadhits << std::endl; 
	std::cout << "introspector_mesmthi: TOTAL L1Dwritehits = " << std::dec << totalL1Dwritehits << std::endl; 

	std::cout << "introspector_mesmthi: TOTAL L2reads = " << std::dec << totalL2reads << std::endl; 
	std::cout << "introspector_mesmthi: TOTAL L2writes = " << std::dec << totalL2writes << std::endl; 
	std::cout << "introspector_mesmthi: TOTAL L2readhits = " << std::dec << totalL2readhits << std::endl; 
	std::cout << "introspector_mesmthi: TOTAL L2writehits = " << std::dec << totalL2writehits << std::endl; 

	std::cout << "introspector_mesmthi: TOTAL Instructions = " << std::dec << totalInstructions << std::endl; 
	std::cout << "introspector_mesmthi: TOTAL BusTransfers = " << std::dec << totalBusTransfers << std::endl; 
	std::cout << "introspector_mesmthi: TOTAL NetworkTransactions = " << std::dec << totalNetworkTransactions << std::endl; 
	std::cout << "introspector_mesmthi: TOTAL Packets = " << std::dec << totalPackets << std::endl; 

	
	totalInstructions = totalPackets = totalBusTransfers = totalNetworkTransactions = 0;
	totalDIRreads = totalDIRwrites = 0;
	totalL1Ireads = totalL1Iwrites = totalL1Ireadhits = totalL1Iwritehits = 0;
	totalL1Dreads = totalL1Dwrites = totalL1Dreadhits = totalL1Dwritehits = 0;
	totalL2reads = totalL2writes = totalL2readhits = totalL2writehits = 0;
}




static Introspector*
create_introspector_mesmthi(SST::Component::Params_t &params)
{
    return new Introspector_mesmthi(params);
}

static const ElementInfoIntrospector introspectors[] = {
    { "introspector_mesmthi",
      "mesmthi Introspector",
      NULL,
      create_introspector_mesmthi,
    },
    { NULL, NULL, NULL, NULL }
};

extern "C" {
    ElementLibraryInfo introspector_mesmthi_eli =  {
        "introspector_mesmthi",
        "mesmthi Introspector",
        NULL,
        NULL,
        introspectors
    };
}


BOOST_CLASS_EXPORT(Introspector_mesmthi)

