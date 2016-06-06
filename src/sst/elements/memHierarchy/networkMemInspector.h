// -*- mode: c++ -*-
// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2016, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.
//

#ifndef NETWORKMEMINSECTOR_H_
#define NETWORKMEMINSECTOR_H_

#include <sst/core/output.h>
#include <sst/core/interfaces/simpleNetwork.h>
#include "memEvent.h"

using namespace SST;
using namespace SST::Interfaces;

namespace SST { namespace MemHierarchy {

        class networkMemInspector : public SimpleNetwork::NetworkInspector {
        public:
            networkMemInspector(Component *parent);
            
            virtual ~networkMemInspector() {}
            
            virtual void inspectNetworkData(SimpleNetwork::Request* req);
            
            virtual void initialize(std::string id);
            
            Output dbg;
            // statistics
            Statistic<uint64_t>*  memCmdStat[LAST_CMD];
        };

    }}

#endif /* NETWORKMEMINSECTOR_H_ */
