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

#ifndef _H_EMBER_SHMEM_GENERATOR
#define _H_EMBER_SHMEM_GENERATOR

#include "embergen.h"

#include <sst/elements/hermes/shmemapi.h>

using namespace Hermes;

namespace SST {
namespace Ember {

#undef FOREACH_ENUM
#define FOREACH_ENUM(NAME) \
    NAME( FOO ) \

#define GENERATE_ENUM(ENUM) ENUM,
#define GENERATE_STRING(STRING) #STRING,


class EmberShmemGeneratorData : public EmberGeneratorData {
public:
	EmberShmemGeneratorData() {} 
};

class EmberShmemGenerator : public EmberGenerator {

public:

    enum Events {
        FOREACH_ENUM(GENERATE_ENUM)
    };

    typedef std::queue<EmberEvent*> Queue;

	EmberShmemGenerator( Component* owner, Params& params );
	~EmberShmemGenerator();
    virtual void completed( const SST::Output*, uint64_t time );

protected:

    void initData( EmberGeneratorData** data ) {
        assert( *data );
        m_data = static_cast<EmberShmemGeneratorData*>(*data);
    }

	void setData( EmberShmemGeneratorData* data ) {
		m_data = data;
	}
	EmberShmemGeneratorData* getData() {
		return m_data;
	}

    void initOutput( Output* output ) {
#if 0
        std::ostringstream prefix;
        prefix << "@t:" << m_data->jobId << ":" << (signed) m_data->rank 
                << ":EmberEngine:MPI:" << m_name << ":@p:@l: ";
        m_outputPrefix = prefix.str();
#endif
    }

private:

    int                 m_printStats;
    static const char*  m_eventName[];

    EmberShmemGeneratorData* 	m_data;
};

static inline Shmem::Interface* cast( Hermes::Interface *in )
{
    return static_cast<Shmem::Interface*>(in);
}

}
}

#endif
