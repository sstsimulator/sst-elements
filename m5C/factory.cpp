// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"

#include "debug.h"
#include "factory.h"
//#include "sst/core/sdl.h"

#include <dll/gem5dll.hh>

using namespace std;

namespace SST {
namespace M5 {

class M5;

class SimObject;

extern "C" {
	SimObject* create_Bus( M5* /*Component**/, string name, Params& params );
	SimObject* create_Bridge( M5* /*Component**/, string name, Params& params );
	SimObject* create_BaseCache( M5* /*Component**/, string name, Params& params );
	SimObject* create_PhysicalMemory( M5* /*Component**/, string name, Params& params );
	SimObject* create_Syscall( M5* /*Component**/, string name, Params& params );
	SimObject* create_O3Cpu( M5* /*Component**/, string name, Params& params );
#ifdef HAVE_DRAMSIM
	SimObject* create_DRAMSimWrap( M5* /*Component**/, string name, Params& params );
#endif
}

Factory::Factory( M5* comp ) :
    m_comp( comp )
{
}

Factory::~Factory()
{
}

Gem5Object_t* Factory::createObject( std::string name, 
                std::string type, SST::Params& params )
{
    /* If the first character in the type is a +, then the object to
       be created is created directly from M5 without any wrapper code
       fronting it.  If the first character in the type is not a +,
       then the object to be created is a wrapper object created in
       this component.  The type signatures are slightly different
       between the two, but the basic creation process is the same.

       TODO: Since we're now explicit about which subcomponents can be
       used (rather than the dlsym approach previously used), there's
       really no reason for the plus/no-plus approach and could just
       have a single list. */
    if ( type[0] != '+' ) {
        return createWrappedObject( name, type, params );
    } else {
        return createDirectObject( name, type.substr(1), params );
    }
}

/* Create an M5 object by invoking the constructor for it's wrapper interface */
Gem5Object_t* Factory::createWrappedObject( std::string name, 
                std::string type, SST::Params& params )
{
    DBGX(2,"type `%s`\n", type.c_str());

    Gem5Object_t* obj = new Gem5Object_t;
    assert(obj);

    if (type == "Bus") {
	obj->memObject = create_Bus( m_comp, name, params );
    } else if (type == "Bridge") {
	obj->memObject = create_Bridge( m_comp, name, params );
    } else if (type == "BaseCache") {
	obj->memObject = create_BaseCache( m_comp, name, params );
    } else if (type == "PhysicalMemory") {
	obj->memObject = create_PhysicalMemory( m_comp, name, params );
    } else if (type == "Syscall") {
	obj->memObject = create_Syscall( m_comp, name, params );
    } else if (type == "O3Cpu") {
	obj->memObject = create_O3Cpu( m_comp, name, params);
    } else if (type == "DRAMSimWrap") {
#ifdef HAVE_DRAMSIM
	obj->memObject = create_DRAMSimWrap( m_comp, name, params);
#else
	std::cerr << "m5C attempted to create a DRAMSim subcomponent in a build without DRAMSim support"
		<< std::endl;
	exit(1);
#endif
    } else {
	std::cerr << "m5C attempted to create a wrapped subcomponent of unknown type: "
                  << type << std::endl;
	exit(1);
    }

    /* check we actually did create what we needed to */
    if ( ! obj ) {
        std::cerr << "m5C failed to create wrapped subcomponent of type " << type << std::endl;
        exit(1);
    }

    return obj;
}

/* Create an M5 object directly from the M5 library */
Gem5Object_t* Factory::createDirectObject( const std::string name, 
                std::string type, SST::Params& params )
{
    size_t num_params = params.size();
    xxx_t *m5_params = (xxx_t*) malloc(sizeof(xxx_t) * (num_params + 1));

    SST::Params::iterator iter = params.begin(), end = params.end();
    for (int i = 0 ; iter != end ; ++iter, i++) {
        m5_params[i].key   = strdup(Params::getParamName(iter->first).c_str());
        m5_params[i].value = strdup(iter->second.c_str());
        if (NULL == m5_params[i].key || NULL == m5_params[i].value) abort();
    }
    m5_params[num_params].key   = NULL;
    m5_params[num_params].value = NULL;

    Gem5Object_t* obj = NULL;

    if(type == "DerivO3CPU") {
    	obj = CreateDerivO3CPU( name.c_str(), m5_params );
    } else if(type == "Bus") {
    	obj = CreateBus( name.c_str(), m5_params );
    } else if(type == "Bridge") {
    	obj = CreateBridge( name.c_str(), m5_params );
    } else if(type == "BaseCache") {
    	obj = CreateBaseCache( name.c_str(), m5_params );
    } else if(type == "PhysicalMemory") {
    	obj = CreatePhysicalMemory( name.c_str(), m5_params );
    } else {
	std::cerr << "m5C attempted to create a direct subcomponent of unknown type: "
                  << type << std::endl;
	exit(1);
    }

    /* check we actually did create what we needed to */
    if ( ! obj ) {
        std::cerr << "m5C failed to create direct subcomponent of type " << type << std::endl;
        exit(1);
    }

    for (size_t i = 0 ; i < num_params ; i++) {
        free( m5_params[i].key );
        free( m5_params[i].value );
    }

    free( m5_params );
    return obj;
}

}
}

