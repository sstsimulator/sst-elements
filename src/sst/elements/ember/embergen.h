// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
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
#include "embergettimeev.h"
#include "libs/emberLib.h"

#define ENGINE_MASK (1<<0)
#define MOTIF_MASK (1<<1)
#define MOTIF_START_STOP_MASK (1<<2)
// #define EVENT_MASK (1<<2)  defined in emberevent.h"

namespace SST {
namespace Ember {

class EmberEngine;

class EmberGenerator : public SubComponent {

  public:

    typedef std::queue<EmberEvent*> Queue;

	SST_ELI_REGISTER_SUBCOMPONENT_API(SST::Ember::EmberGenerator)

    SST_ELI_DOCUMENT_PARAMS(
        { "primary", "is this a primary motif", "0"},
        { "_motifNum", "used internally", "-1"},
        { "_jobId", "used internally", "-1"},
        { "_enginePtr", "used internally", "-1"},
		{ "distribModule", "Sets the distribution SST module for compute modeling, default is a constant distribution of mean 1", "1.0"},
	)

    EmberGenerator( ComponentId_t id, Params& params ) : SubComponent(id) { assert(0); }
    EmberGenerator( ComponentId_t id, Params& params, std::string name ="" );

	void setEngine( EmberEngine* );

	~EmberGenerator(){ };

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

    virtual bool primary( ) { return m_primary; }

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

    void setVerbosePrefix( int rank = -1 ) {
        m_verbosePrefix.str(std::string());
        m_verbosePrefix << "@t:" << getJobId() << ":" << rank <<
                    ":EmberEngine:" << getMotifName() << ":@p:@l: ";
    }

    std::string getMotifName() { return m_motifName; }
    int getJobId()    { return m_jobId; }
    int getMotifNum() { return m_motifNum; }

    Hermes::NodePerf& nodePerf() { return *m_nodePerf; }

    virtual void* memAlloc( size_t );
    virtual void memFree( void* );
	virtual void memSetBacked() { m_dataMode = Backing; }
    bool haveDetailed() { return m_detailedCompute; }

    Thornhill::DetailedCompute*   m_detailedCompute;
    Thornhill::MemoryHeapLink*    m_memHeapLink;

	inline void enQ_getTime( Queue&, uint64_t* time );
    inline void enQ_memAlloc( Queue&, Hermes::MemAddr* addr, size_t length  );
    inline void enQ_compute( Queue&, uint64_t nanoSecondDelay );
    inline void enQ_compute( Queue& q, std::function<uint64_t()> func );
    inline void enQ_detailedCompute( Queue& q, std::string, Params&, std::function<int()> func );

  private:
    EmberEngine*            m_ee;
    Output* 	        	m_output;
    enum { NoBacking, Backing, BackingZeroed  } m_dataMode;
    std::string				m_motifName;
    std::ostringstream      m_verbosePrefix;
    Hermes::NodePerf*       m_nodePerf;
    int                     m_jobId;
    int                     m_motifNum;
    bool                    m_primary;
    EmberComputeDistribution*           m_computeDistrib;
    uint64_t m_curVirtAddr;
};

void EmberGenerator::enQ_getTime( Queue& q, uint64_t* time ) {
	q.push( new EmberGetTimeEvent( &getOutput(), time ) );
}

void EmberGenerator::enQ_compute( Queue& q, uint64_t delay )
{
    q.push( new EmberComputeEvent( &getOutput(), delay, m_computeDistrib ) );
}

void EmberGenerator::enQ_compute( Queue& q, std::function<uint64_t()> func )
{
    q.push( new EmberComputeEvent( &getOutput(), func, m_computeDistrib ) );
}

void EmberGenerator::enQ_detailedCompute( Queue& q, std::string name,
        Params& params, std::function<int()> fini = NULL )
{
    assert( m_detailedCompute );
    q.push( new EmberDetailedComputeEvent( &getOutput(), *m_detailedCompute, name, params, fini ) );
}

void EmberGenerator::enQ_memAlloc( Queue& q, Hermes::MemAddr* addr, size_t length )
{
    if ( m_memHeapLink ) {
        addr->setBacking( memAlloc(length) );
        q.push( new EmberMemAllocEvent( *m_memHeapLink, &getOutput(), addr, length  ) );
    } else {
        if ( length % 16 ) {
            length += 16;
            length &= ~(16-1);
        }
        *addr = Hermes::MemAddr( m_curVirtAddr, memAlloc( length ) );
        m_curVirtAddr += length;
        q.push( new EmberComputeEvent( &getOutput(), 0, m_computeDistrib ) );
    }
}

}
}

#endif
