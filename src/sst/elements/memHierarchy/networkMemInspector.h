// -*- mode: c++ -*-
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
//

#ifndef NETWORKMEMINSECTOR_H_
#define NETWORKMEMINSECTOR_H_

#include <sst/core/output.h>
#include <sst/core/interfaces/simpleNetwork.h>

#include "sst/elements/memHierarchy/memTypes.h"
#include "sst/elements/memHierarchy/memEvent.h"

using namespace SST;
using namespace SST::Interfaces;

namespace SST { namespace MemHierarchy {

class networkMemInspector : public SimpleNetwork::NetworkInspector {
public:
/* Element Library Info */
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
        networkMemInspector,
        "memHierarchy",
        "networkMemoryInspector",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Used to classify memory traffic going through a network router",
        SST::Interfaces::SimpleNetwork::NetworkInspector
    )

    SST_ELI_DOCUMENT_STATISTICS( networkMemoryInspector_statistics ) // Defined in memTypes.h via x macro

/* Begin class definition */
    networkMemInspector(ComponentId_t, Params &params, const std::string& sub_id);

    virtual ~networkMemInspector() {}

    virtual void inspectNetworkData(SimpleNetwork::Request* req);

    Output dbg;
    // statistics
    Statistic<uint64_t>*  memCmdStat[(int)Command::LAST_CMD];
};

}}

#endif /* NETWORKMEMINSECTOR_H_ */
