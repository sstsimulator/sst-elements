// Copyright 2013-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef COMPONENTS_FIREFLY_HADES_H
#define COMPONENTS_FIREFLY_HADES_H

#include <sst/core/output.h>
#include <sst/core/params.h>

#include <sst/core/shared/sharedArray.h>

#include "sst/elements/hermes/hermes.h"
#include "sst/elements/thornhill/memoryHeapLink.h"
#include "group.h"
#include "info.h"
#include "ctrlMsg.h"

namespace SST {
namespace Firefly {

class FunctionSM;
class VirtNic;


class Hades : public OS
{
  public:

    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
        Hades,
        "firefly",
        "hades",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "",
       	SST::Hermes::OS
    )

    SST_ELI_DOCUMENT_PARAMS(
        {"verboseLevel","Sets the output level","0"},
        {"verboseMask","Sets the output mask","0"},
        {"numNodes","Sets the number of nodes","0"},
        {"nicModule", "Sets the NIC module", "firefly.VirtNic"},
        {"nodePerf", "Sets the node performance module ", "1"},
        {"netMapSize","Sets the network map Size of the endpoint", ""},
        {"netId","Sets the network id of the endpoint", ""},
        {"netMapId","Sets the network mapping id of the endpoint", ""},
        {"netMapName","Sets the network map Name of the endpoint", ""},
        {"coreId","Sets the core ID","0"},

		/* PARAMS
			ctrlmsg.*
			functionSM.*
			nicParams.*
			nodePerf.*
			detailedCompute.*
			memoryHeapLink.*
		*/
    )

    Hades(ComponentId_t id, Params& params);
    ~Hades();
    virtual void _componentInit(unsigned int phase );
    virtual void _componentSetup();
    virtual void finish();

    int getRank();
    int getNodeNum();

    int sizeofDataType( MP::PayloadDataType type ) {
        return m_info.sizeofDataType(type);
    }

    NodePerf* getNodePerf() {
        return m_nodePerf;
    }

    Thornhill::DetailedCompute* getDetailedCompute() {
        return m_detailedCompute;
    }

    Thornhill::MemoryHeapLink* getMemHeapLink() {
        return m_memHeapLink;
    }

    VirtNic*            getNic() { return m_virtNic; }
    Info*               getInfo() { return &m_info; }
    FunctionSM&         getFunctionSM() { return *m_functionSM; }
    ProtocolAPI&        getMsgStack() { return *m_proto; }
    int                 getNumNodes() { return m_numNodes; }

  private:

    VirtNic*            m_virtNic;
    Info                m_info;
    Output              m_dbg;

  private:
    FunctionSM*                          m_functionSM;
    ProtocolAPI*                         m_proto;
    Thornhill::DetailedCompute*          m_detailedCompute;
    Thornhill::MemoryHeapLink*           m_memHeapLink;
    NodePerf*                            m_nodePerf;
    // SharedRegion*                        m_sreg;
    Shared::SharedArray<int>             m_sreg;
    int                                  m_netMapSize;
    std::string                          m_netMapName;
    int                                  m_numNodes;
};

} // namesapce Firefly
} // namespace SST

#endif
