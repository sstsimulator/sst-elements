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

#ifndef _factory_h
#define _factory_h

#include <sst/core/sdl.h>

#include <dlfcn.h>
#include <dll/gem5dll.hh>

#include <debug.h>

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

class Factory {
  public:
    Factory( M5* );
    ~Factory( );
    Gem5Object_t* createObject( std::string name, std::string type, 
            SST::Params&  );
    
  private:     
    Gem5Object_t* createObject1( std::string name, std::string type, 
            SST::Params&  );
    Gem5Object_t* createObject2( std::string name, std::string type, 
            SST::Params&  );
    M5* m_comp;
};

#define FAST "fast"
#define OPT  "opt" 
#define PROF "prof" 
#define DEBUG "debug"

inline Factory::Factory( M5* comp ) :
    m_comp( comp )
{
}

inline Factory::~Factory()
{
}

inline Gem5Object_t* Factory::createObject( std::string name, 
                std::string type, SST::Params& params )
{
    if ( type[0] != '+' ) {
        return createObject1( name, type, params );
    } else {
        return createObject2( name, type.substr(1), params );
    }
}

inline Gem5Object_t* Factory::createObject1( std::string name, 
                std::string type, SST::Params& params )
{
    std::string tmp = "create_";
    tmp += type;
    DBGX(2,"type `%s`\n", tmp.c_str());

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
	std::cerr << "m5C attempted to create a DRAMSim component in a build without DRAMSim support"
		<< std::endl;
	exit(-1);
#endif
    } else {
	std::cerr << "m5C attempted to create component: " << type <<
		", the creation of this component is not supported"
		<< std::endl;
	exit(-1);
    }

    return obj;
}

inline char * make_copy( const std::string & str ) 
{
    char * tmp = (char*) malloc( str.size() + 1 );
    assert(tmp);
    return strcpy( tmp, str.c_str() );
}

inline Gem5Object_t* Factory::createObject2( const std::string name,
                std::string type, SST::Params& params )
{

    xxx_t *xxx = (xxx_t*) malloc( (params.size() + 1) * sizeof( *xxx ) );

    SST::Params::iterator iter = params.begin();
    for ( int i=0; iter != params.end(); ++iter, i++ ){

        xxx[i].key = make_copy( (*iter).first );
        xxx[i].value = make_copy( (*iter).second );
    }
    xxx[params.size()].key = NULL;
    xxx[params.size()].value = NULL;

    Gem5Object_t *obj = NULL;
    if ( type == "DerivO3CPU" )
        obj = CreateDerivO3CPU(name.c_str(), xxx);
    else if ( type == "Bus" )
        obj = CreateBus(name.c_str(), xxx);
    else if ( type == "Bridge" )
        obj = CreateBridge(name.c_str(), xxx);
    else if ( type == "BaseCache" )
        obj = CreateBaseCache(name.c_str(), xxx);
    else if ( type == "PhysicalMemory" )
        obj = CreatePhysicalMemory(name.c_str(), xxx);


    for ( int i=0; i < params.size(); i++ ){
        free( xxx[i].key );
        free( xxx[i].value );
    }
    free( xxx );


    if ( !obj ) {
        printf("Factory::Factory() failed to create %s\n", type.c_str() );
        exit(-1);
    }

    return obj;
}

}
}

#endif
