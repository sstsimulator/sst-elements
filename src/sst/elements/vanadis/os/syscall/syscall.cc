// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>

#include "os/syscall/syscall.h"
#include "os/vnodeos.h"

using namespace SST::Vanadis;

VanadisSyscall::VanadisSyscall( VanadisNodeOSComponent* os, SST::Link* coreLink, OS::ProcessInfo* process, VanadisSyscallEvent* event, std::string name )
        :   m_os(os), m_process(process), m_coreLink(coreLink), m_event(event), m_name(name),
            m_complete(false), m_memHandler(nullptr),  m_pageFaultAddr(0)
{
    m_output = m_os->getOutput();
    m_os->setSyscall( getCoreId(), getThreadId(), this);
}

VanadisSyscall::~VanadisSyscall() {
    VanadisSyscallResponse* resp = new VanadisSyscallResponse(m_returnInfo.code,m_returnInfo.success,m_returnInfo.hasExited);;

    resp->setHWThread( m_event->getThreadID() );

    m_output->verbose(CALL_INFO, 8, 0,"syscall '%s' has finished, send response to core %d hwThead %d, %s, returnCode=%d\n",
        getName().c_str(),
        m_event->getCoreID(),
        m_event->getThreadID(),
        resp->isSuccessful() ? "Success" : "Failed",
        resp->getReturnCode() );

    m_coreLink->send( resp );

    m_os->clearSyscall( getCoreId(), getThreadId() );

    if ( m_returnInfo.hasExited ) {
        delete m_process;
    }

    delete m_event;
}
