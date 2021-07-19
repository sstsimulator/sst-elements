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

#include <sst_config.h>
#include <sst/core/component.h>
#include <sst/core/subcomponent.h>
#include <sst/core/link.h>

#include "sst/elements/miranda/mirandaEvent.h"

#include "singleThread.h"

using namespace SST;
using namespace SST::Miranda;
using namespace SST::Thornhill;

SingleThread::SingleThread( ComponentId_t id,
        Params& params )
        : DetailedCompute( id ), m_link(NULL), m_busy(false)
{
    std::string portName = params.find<std::string>( "portName", "detailed0" );

    if ( isPortConnected( portName.c_str() ) ) {
        m_link = configureLink( portName.c_str(), "0ps",
            new Event::Handler<SingleThread>(
                    this,&SingleThread::eventHandler ) );
    }
    assert(m_link);
}

void SingleThread::eventHandler( SST::Event* ev )
{
    MirandaRspEvent* event = static_cast<MirandaRspEvent*>(ev);

	Entry* entry = static_cast<Entry*>((void*)event->key);
	entry->finiHandler();
	delete entry;
	m_busy = false;
	if ( ! m_pendingQ.empty() ){
		Pending& pending =  m_pendingQ.front();
		start2( pending.work, pending.retHandler, pending.finiHandler );
		m_pendingQ.pop();
	}
}

void SingleThread::start2( const std::deque< std::pair< std::string, SST::Params> >& generators,
                 std::function<int()> retHandler, std::function<int()> finiHandler )
{
	m_busy = true;
	MirandaReqEvent* event = new MirandaReqEvent;

	if ( finiHandler ) {
		retHandler();
		event->key = (uint64_t) new Entry( finiHandler );
	} else {
		event->key = (uint64_t) new Entry( retHandler );
	}

	event->generators = generators;

	m_link->send( 0, event );
}
