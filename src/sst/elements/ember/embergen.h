// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_EMBER_GENERATOR
#define _H_EMBER_GENERATOR

#include <queue>

#include <sst/core/output.h>
#include <sst/core/module.h>
#include <sst/core/subcomponent.h>
#include <sst/core/params.h>

#include <sst/elements/hermes/msgapi.h>
#include "sst/elements/thornhill/detailedCompute.h"
#include "sst/elements/thornhill/memoryHeapLink.h"

#include "emberevent.h"
#include "embermap.h"
#include "embermemoryev.h"
#include "emberconstdistrib.h"
#include "embercomputeev.h"
#include "emberdetailedcomputeev.h"
#include "libs/emberLib.h"

namespace SST {
namespace Ember {

class EmberGenerator : public SubComponent {

  public:

    typedef std::queue<EmberEvent*> Queue;

    EmberGenerator( Component* owner, Params& params, std::string name ="" );

	~EmberGenerator(){
		delete m_computeDistrib;
	};
    
    virtual void generate( const SST::Output* output, const uint32_t phase,
        std::queue<EmberEvent*>* evQ ) {
        assert(0);
    }

    virtual bool generate( std::queue<EmberEvent*>& evQ ) { 
        assert(0); 
    }

    virtual void completed( const SST::Output* output, uint64_t time) {
        assert(0);
    }

    virtual bool primary( ) { return true; }

    virtual std::string getComputeModelName() {
        return "";
    }
    EmberLib* getLib(std::string name );


    Output& getOutput() { return *m_output; }
    void verbose(uint32_t line, const char* file, const char* func,
                 uint32_t output_level, uint32_t output_bits,
                 const char* format, ...)    const
        __attribute__ ((format (printf, 7, 8)));

    void output(const char* format, ...) const
         __attribute__ ((format (printf, 2, 3)));

    void fatal(uint32_t line, const char* file, const char* func,
               uint32_t exit_code,
               const char* format, ...)    const
                  __attribute__ ((format (printf, 6, 7))) ;

    std::string getMotifName() { return m_motifName; }
    void setRank( int rank ) { m_api->setRank( rank ); }
    void setSize( int size ) { m_api->setSize( size ); }
    int rank() { 
        if ( m_api ) {
            return m_api->getRank(); 
        } else {
            return -1;
        }
    }
    int size() { 
        if ( m_api ) {
            return m_api->getSize(); 
        } else { 
            return -1;
        }
    }
    int getJobId()    { return m_jobId; }
    int getMotifNum() { return m_motifNum; }

    Hermes::NodePerf& nodePerf() { return *m_nodePerf; }

    virtual void* memAlloc( size_t );
    virtual void memFree( void* );
    virtual void* memAddr( void * addr ) {
        return  m_dataMode == Backing ? addr : NULL;
    }
    bool haveDetailed() { return m_detailedCompute; }

    Hermes::Interface*  	m_api;
    Thornhill::DetailedCompute*   m_detailedCompute;
    Thornhill::MemoryHeapLink*    m_memHeapLink;

    inline void enQ_memAlloc( Queue&, Hermes::MemAddr* addr, size_t length  );
    inline void enQ_compute( Queue&, uint64_t nanoSecondDelay );
    inline void enQ_compute( Queue& q, std::function<uint64_t()> func );
    inline void enQ_detailedCompute( Queue& q, std::string, Params& );

    enum { NoBacking, Backing, BackingZeroed  } m_dataMode; 

  private:
    Output* 	        	m_output;
    std::string				m_motifName;
    std::ostringstream      m_verbosePrefix;
    Hermes::NodePerf*       m_nodePerf;
    int                     m_jobId;
    int                     m_motifNum;
    EmberComputeDistribution*           m_computeDistrib;
};

void EmberGenerator::enQ_compute( Queue& q, uint64_t delay )
{
    q.push( new EmberComputeEvent( &getOutput(), delay, m_computeDistrib ) );
}

void EmberGenerator::enQ_compute( Queue& q, std::function<uint64_t()> func )
{
    q.push( new EmberComputeEvent( &getOutput(), func, m_computeDistrib ) );
}

void EmberGenerator::enQ_detailedCompute( Queue& q, std::string name,
        Params& params )
{
    assert( m_detailedCompute );
    q.push( new EmberDetailedComputeEvent( &getOutput(), *m_detailedCompute, name, params ) );
}
void EmberGenerator::enQ_memAlloc( Queue& q, Hermes::MemAddr* addr, size_t length )
{
    assert( m_memHeapLink );
    addr->setBacking( memAlloc(length) );
    q.push( new EmberMemAllocEvent( *m_memHeapLink, &getOutput(), addr, length  ) );
}

}
}

#endif
