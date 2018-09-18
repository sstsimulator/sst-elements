// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
// 
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _LINKBUILDER_H
#define _LINKBUILDER_H

#include <sst/core/element.h>
#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>
#include <sst/core/elementinfo.h>

#include "Job.h"

#include <string>

namespace SST {
    namespace Scheduler {

        enum linkTypes{PARENT, CHILD};


        class linkChanger {
            public:
                virtual void addLink(SST::Link * link, enum linkTypes type) = 0;
                virtual void rmLink(SST::Link * link, enum linkTypes type) = 0;
                virtual void disconnectYourself() = 0;
                virtual std::string getType() = 0;  // should describe the class of component (CPU, mem, NIC, etc)
                virtual std::string getID() = 0;    // should describe the exact component uniquely within each class (CPU1, CPU2, CPU3, NIC1, NIC2, NIC3, etc)
                virtual ~linkChanger() {};
        };


        class linkBuilder : public SST::Component {
            public:

                SST_ELI_REGISTER_COMPONENT(
                    linkBuilder,
                    "scheduler",
                    "linkBuilder",
                    SST_ELI_ELEMENT_VERSION(1,0,0),
                    "Node Graph Link Modifier",
                    COMPONENT_CATEGORY_UNCATEGORIZED
                )

                linkBuilder(SST::ComponentId_t id, SST::Params & params);
                void connectGraph(Job* job);
                void disconnectGraph(Job* job);

            private:
                void initNodePtrRequests(SST::Event* event);
                void handleNewNodePtr(SST::Event* event);

                typedef std::map<std::string, linkChanger*> nodeMap;
                typedef std::map<std::string, nodeMap> nodeTypeMap;

                nodeTypeMap nodes;
                // node pointers are indexed by Type and then by ID

//                SST::Link* selfLink;
                std::vector<SST::Link *> nodeLinks;
        };

    }
}
#endif

