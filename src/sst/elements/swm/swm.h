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

#ifndef _SWM_COMPONENT_H
#define _SWM_COMPONENT_H

#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>
#include <sst/core/timeLord.h>
#include <sst/core/output.h>
#include <sst/elements/hermes/msgapi.h>

#include "sst/elements/swm/workload.h"
#include "sst/elements/swm/event.h"

using namespace SST::Hermes::MP;
using namespace SST::Hermes;

namespace SST {
namespace Swm {

class SwmComponent : public Component {
  public:
    SST_ELI_REGISTER_COMPONENT(
        SwmComponent,
        "sstSwm",
        "Swm",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "",
        COMPONENT_CATEGORY_UNCATEGORIZED
    )
    SST_ELI_DOCUMENT_PARAMS()
    SST_ELI_DOCUMENT_PORTS()

  public:
    SwmComponent( ComponentId_t id, Params& params );
    ~SwmComponent();

    void init( unsigned int phase ) { m_os->_componentInit(phase); }

	void setup();
    void finish() { m_workload->stop(); }

  private:

    void handleSelfEvent( SST::Event* ev );

    SST::TimeConverter* m_tConv;
	int				m_rank;
    OS*				m_os;
	MP::Interface*	m_msgapi;
    Link*	  		m_selfLink;
	Output			m_output;
	Workload*		m_workload;
    Convert*        m_convert;
    std::string     m_workloadName;
    std::string     m_path;
    int             m_verboseLevel;
    int             m_verboseMask;
    int             m_convertVerboseLevel;
    int             m_convertVerboseMask;
    int             m_workloadVerboseLevel;
    int             m_workloadVerboseMask;
};

}
}

#endif
