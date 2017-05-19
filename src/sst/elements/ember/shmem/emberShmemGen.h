// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2017, Sandia Corporation
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
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


class EmberShmemGenerator : public EmberGenerator {

public:

    enum Events {
        FOREACH_ENUM(GENERATE_ENUM)
    };

    typedef std::queue<EmberEvent*> Queue;

	EmberShmemGenerator( Component* owner, Params& params );
	~EmberShmemGenerator();
    virtual void completed( const SST::Output*, uint64_t time );

private:

    int                 m_printStats;
    static const char*  m_eventName[];
};

static inline Shmem::Interface* cast( Hermes::Interface *in )
{
    return static_cast<Shmem::Interface*>(in);
}

}
}

#endif
