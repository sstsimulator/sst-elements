// Copyright 2013-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2018, NTESS
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

#include <sst/core/elementinfo.h>
#include <sst/core/sharedRegion.h>

#include "sst/elements/hermes/hermes.h"
#include "sst/elements/thornhill/memoryHeapLink.h"
#include "group.h"
#include "info.h"
#include "protocolAPI.h"

namespace SST {
namespace Firefly {

class FunctionSM;
class VirtNic;


class Hades : public OS 
{
  public:
    SST_ELI_REGISTER_SUBCOMPONENT(
        Hades,
        "firefly",
        "hades",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "",
        ""
    )

    SST_ELI_DOCUMENT_PARAMS(
        {"mapType","Sets the type of data structure to use for mapping ranks to NICs", ""},
        {"netId","Sets the network id of the endpoint", ""},
        {"netMapId","Sets the network mapping id of the endpoint", ""},
        {"netMapSize","Sets the network map Size of the endpoint", ""},
        {"netMapName","Sets the network map Name of the endpoint", ""},
        {"nicModule", "Sets the NIC module", "firefly.VirtNic"},
        {"verboseLevel", "Sets the output verbosity of the component", "1"},
        {"verboseMask", "Sets the output mask of the component", "1"},
        {"debug", "Sets the messaging API of the end point", "0"},
        {"defaultVerbose","Sets the default function verbose level","0"},
        {"defaultDebug","Sets the default function debug level","0"},
        {"flops", "Sets the FLOP rate of the endpoint ", "1"},
        {"bandwidth", "Sets the bandwidth of the endpoint ", "1"},
        {"nodePerf", "Sets the node performance module ", "1"},
    )

    Hades(Component*, Params&);
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
    SharedRegion*                        m_sreg;
    int                                  m_netMapSize;
    std::string                          m_netMapName;
    int                                  m_numNodes;
};

} // namesapce Firefly 
} // namespace SST

#endif
