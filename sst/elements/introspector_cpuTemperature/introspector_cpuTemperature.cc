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

#include <sst_config.h>
#include "sst/core/serialization.h"

#include "introspector_cpuTemperature.h"
#include "sst/core/element.h"

using namespace SST;
using namespace SST::Introspector_CPUTemperature;

bool Introspector_cpuTemperature::triggeredUpdate()
{
	for (std::list<IntrospectedComponent*>::iterator i = MyCompList.begin();
	     i != MyCompList.end(); ++i) {
		
		std::cout << "Introspector_cpuTemperature pulls data from component ID " << (*i)->getId() << " and temperature = " << getData<double>(*i, "temperature") << std::endl;
		
	    }
  return true;	    
}

/*bool Introspector_cpuTemperature::pullData( Cycle_t current )
{
    //_INTROSPECTOR_DBG("id=%lu currentCycle=%lu \n", Id(), current );
    IntrospectedComponent *c;

        printf("introspector_cpuTemperature pulls data @ cycle %ld\n", 
               (long int) current ); //current here specifies it's the #th call
	for( Database_t::iterator iter = DatabaseInt.begin();
                            iter != DatabaseInt.end(); ++iter )
    	{
	    c = iter->second;
            std::cout << "Pull data of component ID " << c->getId() << " with dataID = " << iter->first << " and data value = " << c->getIntData(iter->first) << std::endl;
	    
	}
	for( Database_t::iterator iter = DatabaseDouble.begin();
                            iter != DatabaseDouble.end(); ++iter )
    	{
            c = iter->second;
            std::cout << "Pull data of component ID " << c->getId() << " with dataID = " << iter->first << " and data value = " << c->getDoubleData(iter->first) << std::endl;	    
	}

	return false;
}*/



static Introspector*
create_introspector_cpuTemperature(SST::Params &params)
{
    return new Introspector_cpuTemperature(params);
}

static const ElementInfoIntrospector introspectors[] = {
    { "introspector_cpuTemperature",
      "CPU Introspector",
      NULL,
      create_introspector_cpuTemperature,
    },
    { NULL, NULL, NULL, NULL }
};

extern "C" {
    ElementLibraryInfo introspector_cpuTemperature_eli =  {
        "introspector_cpuTemperature",
        "CPU Introspector",
        NULL,
        NULL,
        introspectors
    };
}

BOOST_CLASS_EXPORT(Introspector_cpuTemperature)
