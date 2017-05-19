// Copyright 2013-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2017, Sandia Corporation
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
    Hades(Component*, Params&);
    ~Hades();
    virtual void _componentInit(unsigned int phase );
    virtual void _componentSetup();

    int getNid();
    int getNumNids();

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
    Info*                getInfo() { return &m_info; }

  private:

    VirtNic*            m_virtNic;
    Info                m_info;

  public:
    Output              m_dbg;

  private:
    Thornhill::DetailedCompute*          m_detailedCompute;
    Thornhill::MemoryHeapLink*           m_memHeapLink;
    NodePerf*                            m_nodePerf;
    SharedRegion*                        m_sreg;
    int                                  m_netMapSize;
    std::string                          m_netMapName;
};

} // namesapce Firefly 
} // namespace SST

#endif
