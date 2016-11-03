// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2016, Sandia Corporation
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_EMBER_EVENT
#define _H_EMBER_EVENT

#include <sst/core/event.h>
#include <sst/core/statapi/statbase.h>
#include <sst/elements/hermes/msgapi.h>

namespace SST {
namespace Ember {

typedef Hermes::MP::Functor FOO;

#undef FOREACH_ENUM
#define FOREACH_ENUM(NAME) \
    NAME( Issue ) \
    NAME( IssueFunctor ) \
    NAME( Complete ) \

#define GENERATE_ENUM(ENUM) ENUM,
#define GENERATE_STRING(STRING) #STRING,

typedef Statistic<uint32_t> EmberEventTimeStatistic;

class EmberEvent : public SST::Event {

public:

    enum State { 
        FOREACH_ENUM(GENERATE_ENUM)
    } m_state;

	EmberEvent( Output* output, EmberEventTimeStatistic* stat = NULL) :
        m_state(Issue), m_output(output), m_evStat(stat), m_completeDelayNS(0)
	{}
	EmberEvent( ) : 
        m_state(Issue), m_output(NULL), m_evStat(NULL), m_completeDelayNS(0) {}
	~EmberEvent() {} 

	virtual std::string getName() { return "?????"; };

    State state() { return m_state; }
    std::string stateName( State i ) { return m_enumName[i]; }

    virtual void issue( uint64_t time, FOO* = NULL ) {
        if ( m_output ) {
            m_output->verbose(CALL_INFO, 3, 0, "%s\n",getName().c_str());
        }
        m_issueTime = time;
        m_state = Complete;
    }

    virtual bool complete( uint64_t time, int retval = 0 ) {

        if ( m_output ) {
            m_output->verbose(CALL_INFO, 3, 0, "%s\n",getName().c_str());
        }
        
        if ( m_evStat ) {
            m_evStat->addData( time - m_issueTime );
        }  
        return true; 
    }

    virtual uint64_t completeDelayNS() {
        m_output->verbose(CALL_INFO, 2, 0, "delay=%" PRIu64 " ns\n",
                                                m_completeDelayNS);
        return m_completeDelayNS;
    }


  protected:
    static const char*  m_enumName[];
    Output*             m_output;
    EmberEventTimeStatistic*  m_evStat;
    uint64_t            m_completeDelayNS;
    uint64_t            m_issueTime;
    
    NotSerializable(EmberEvent)
};

}
}

#endif
