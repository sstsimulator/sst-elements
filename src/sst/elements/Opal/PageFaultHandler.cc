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

/* Author: Vamsee Reddy Kommareddy
 * E-mail: vamseereddy8@knights.ucf.edu
 */

#include <sst_config.h>

#include "PageFaultHandler.h"
#include "Opal_Event.h"

#include <sst/core/link.h>


using namespace SST::OpalComponent;

PageFaultHandler::PageFaultHandler(ComponentId_t id, Params& params) :
            SambaComponent::PageFaultHandler(id, params) {

    // Find links
    std::string linkprefix = "opal_link_";
    std::string linkname = linkprefix + "0";
    int numPorts = 0;

    std::string latency = params.find<std::string>("opal_latency", "32ps");

    while (isPortConnected(linkname)) {
        SST::Link* link = configureLink(linkname, latency, new Event::Handler<PageFaultHandler>(this, &PageFaultHandler::handleEvent));
        opalLink.push_back(link);
        numPorts++;
        linkname = linkprefix + std::to_string(numPorts);
    }
}


PageFaultHandler::~PageFaultHandler() {
}


void PageFaultHandler::handleEvent(SST::Event *event) {
    OpalEvent * ev = static_cast<OpalComponent::OpalEvent*>(event);
    output->verbose(CALL_INFO, 4, 0, "Core %" PRIu32 " handling opal page fault event\n", ev->getCoreId());

    PageFaultHandlerPacket pkt;

    switch(ev->getType()) {
        case SST::OpalComponent::EventType::RESPONSE:
        	pkt.action = PageFaultHandlerAction::RESPONSE;
            break;
        default:
            output->fatal(CALL_INFO, -4, "Opal event interrupt to core: %" PRIu32 " was not valid.\n", ev->getCoreId());
    }

    pkt.vAddress = ev->getAddress();
    pkt.pAddress = ev->getPaddress();
    pkt.size = 4096;
    (*(pageFaultHandlerInterface[ev->getCoreId()]))(pkt);

    delete ev;
}

void PageFaultHandler::allocatePage(const uint32_t thread, const uint32_t level, const uint64_t virtualAddress, const uint64_t size) {
    OpalEvent * tse = new OpalEvent(OpalComponent::EventType::REQUEST);
    tse->setResp(virtualAddress, 0, size);
    tse->setFaultLevel(level);
    opalLink[thread]->send(tse);

}

