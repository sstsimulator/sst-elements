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


#ifndef _H_EMBER_GENERATOR
#define _H_EMBER_GENERATOR

#include <sst/core/output.h>
#include <sst/core/module.h>
#include <sst/core/params.h>
#include <queue>

#include <sst/elements/hermes/msgapi.h>

#include "emberevent.h"
#include "embermap.h"

namespace SST {
namespace Ember {

#define GEN_DBG( lvl, fmt , args... ) \
 {\
    if( m_output ) \
    m_output->verbosePrefix(  m_outputPrefix.c_str(), CALL_INFO, lvl, 0, \
                    fmt, ##args); \
 }

class EmberGeneratorData {
  public:
    int                 jobId;
    int					motifNum;
    virtual ~EmberGeneratorData() {}  
};

class EmberGenerator : public Module {

  public:
    EmberGenerator( Component* owner, Params& params ) :
        m_name("???"),
        m_output( NULL ),
        m_dataMode( NoBacking )
    {}

	~EmberGenerator(){};
    
    virtual void initAPI( Hermes::Interface* api ) {
        m_api = api; 
    }

    virtual void initData( EmberGeneratorData** data ) {
        assert(0);
    } 

    virtual void initOutput( Output* output ) {
        m_output = output;
    }

    virtual void setOutputPrefix() {
	m_outputPrefix = "";
    }

    virtual void generate( const SST::Output* output, const uint32_t phase,
        std::queue<EmberEvent*>* evQ ) {
        assert(0);
    }

    virtual bool generate( std::queue<EmberEvent*>& evQ ) {
        assert(0);
    }

	virtual void finish( const SST::Output* output, uint64_t time) { 
        assert(0);
    }

    virtual bool primary( ) {
        return true;
    }

  protected:
    virtual void printHistogram( const Output* output, Histo* histo );
    virtual void* memAlloc( size_t );
    virtual void memFree( void* );
	virtual void* memAddr( void * addr ) {
		return  m_dataMode == Backing ? addr : NULL;
	}

    Hermes::Interface*  m_api;

    std::string m_name;
    Output* 	m_output;
    string  	m_outputPrefix;

    enum { NoBacking, Backing, BackingZeroed  } m_dataMode; 
};

}
}

#endif
