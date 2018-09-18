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

#ifndef SST_SCHEDULER_FAULTINJECTIONCOMPONENT_H__
#define SST_SCHEDULER_FAULTINJECTIONCOMPONENT_H__

#include <fstream>
#include <string>

#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/elementinfo.h>

namespace SST{
	class Event;
	class Link;
	class Params;
	namespace Scheduler{

		class FaultEvent;

		class faultInjectionComponent : public SST::Component{
			public:

                SST_ELI_REGISTER_COMPONENT(
                    faultInjectionComponent,
                    "scheduler",
                    "faultInjectionComponent",
                    SST_ELI_ELEMENT_VERSION(1,0,0),
                    "Generates and injects failures from a flatfile",
                    COMPONENT_CATEGORY_UNCATEGORIZED
                )

                SST_ELI_DOCUMENT_PARAMS(
                    { "faultInjectionFilename",
                        "File to use for fault injections",
                        NULL
                    },
                    { "injectionFrequency",
                        "Frequency with which injections occur",
                        NULL
                    }
                )

                SST_ELI_DOCUMENT_PORTS(
                    {"faultInjector",
                     "Causes nodes to fail",
                     {"faultActivationEvents"}
                    }
                )

				faultInjectionComponent();
				faultInjectionComponent( SST::ComponentId_t id, SST::Params& params );
				~faultInjectionComponent();

				void setup();

				void handleSelfLink( SST::Event * );
				void handleNodeLink( SST::Event *, int );

			private:
				std::map<std::string, std::string> * readFailFile();

				SST::Link * selfLink;
				std::string resumeSimulationToken;
				std::string failFilename;
				time_t fileLastWritten;
				int failFrequency;
				int failPollFreq;

				std::vector<SST::Link *> nodeLinks;
				std::vector<std::string> nodeIDs;
		};
	};
};

#endif
 
