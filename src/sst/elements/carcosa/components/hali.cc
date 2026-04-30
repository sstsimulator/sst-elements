// Copyright 2009-2024 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2024, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"
#include "sst/elements/memHierarchy/memEventBase.h"
#include "sst/elements/memHierarchy/memEvent.h"
#include "sst/elements/memHierarchy/memLink.h"
#include "sst/elements/memHierarchy/memTypes.h"
#include "sst/elements/carcosa/components/carcosaMemCtrl.h"
#include "sst/elements/carcosa/components/hali.h"
#include "sst/elements/carcosa/components/haliEvent.h"
#include "sst/elements/carcosa/components/cpuEvent.h"
#include "sst/elements/carcosa/components/faultInjEvent.h"
#include "sst/elements/carcosa/components/interceptionAgentAPI.h"
#include "sst/elements/carcosa/components/sensorEvent.h"
#include <limits>
#include <sstream>
#include <typeinfo>
#include <climits>
#include <cstring>

using namespace SST;
using namespace SST::MemHierarchy;
using namespace SST::Interfaces;
using namespace SST::Carcosa;

/*****************************************************************************
 * Lifecycle Phase #1: Construction
 *****************************************************************************/
Hali::Hali(ComponentId_t id, Params& params) : Component(id) {
    requireLibrary("memHierarchy");

    out_ = new Output("", 1, 0, Output::STDOUT);
    out_->output("Phase: Construction, %s\n\n", getName().c_str());

    verbose_ = params.find<bool>("verbose", false);

    // Ring links: only used for ring-discovery topology.
    if (isPortConnected("left")) {
        leftHaliLink_ = configureLink("left", new Event::Handler<Hali, &Hali::handleHaliEvent>(this));
        if (verbose_) out_->output("%s: left port connected\n", getName().c_str());
    } else {
        leftHaliLink_ = nullptr;
    }
    if (isPortConnected("right")) {
        rightHaliLink_ = configureLink("right", new Event::Handler<Hali, &Hali::handleHaliEvent>(this));
        if (verbose_) out_->output("%s: right port connected\n", getName().c_str());
    } else {
        rightHaliLink_ = nullptr;
    }

    // cpu link is unused in MMIO / Vanadis data-path mode.
    if (isPortConnected("cpu")) {
        cpuLink_ = configureLink("cpu", new Event::Handler<Hali, &Hali::handleCpuEvent>(this));
        if (verbose_) out_->output("%s: cpu port connected\n", getName().c_str());
    } else {
        cpuLink_ = nullptr;
    }

    if (isPortConnected("memCtrl")) {
        memCtrlLink_ = configureLink("memCtrl", new Event::Handler<Hali, &Hali::handleMemCtrlEvent>(this));
        if (verbose_) out_->output("%s: memCtrl port connected\n", getName().c_str());
    } else {
        memCtrlLink_ = nullptr;
    }

    if (isPortConnected("sensor")) {
        sensorLink_ = configureLink("sensor", new Event::Handler<Hali, &Hali::handleSensorEvent>(this));
        if (verbose_) out_->output("%s: sensor port connected\n", getName().c_str());
    } else {
        sensorLink_ = nullptr;
    }

    // MemHierarchy integration links.
    if (isPortConnected("highlink")) {
        highlink_ = configureLink("highlink", new Event::Handler<Hali, &Hali::highlinkMemEvent>(this));
        if (verbose_) out_->output("%s: highlink port connected\n", getName().c_str());
    } else {
        highlink_ = nullptr;
        if (verbose_) out_->output("%s: highlink port NOT connected\n", getName().c_str());
    }

    if (isPortConnected("lowlink")) {
        lowlink_ = configureLink("lowlink", new Event::Handler<Hali, &Hali::lowlinkMemEvent>(this));
        if (verbose_) out_->output("%s: lowlink port connected\n", getName().c_str());
    } else {
        lowlink_ = nullptr;
        if (verbose_) out_->output("%s: lowlink port NOT connected\n", getName().c_str());
    }

    Params faultInjParams;
    faultInjParams.insert("pmRegistryId", params.find<std::string>("pmRegistryId", "default"));
    faultInjParams.insert("debugManagerLogic", params.find<bool>("debugManagerLogic", false) ? "1" : "0");
    faultInjManager_ = loadAnonymousSubComponent<FaultInjManagerAPI>(
        "carcosa.FaultInjManager", "faultInjManager", 0,
        ComponentInfo::SHARE_NONE, faultInjParams);

    // Parse intercept_ranges: semicolon-separated "base,size" pairs (base hex, size dec).
    interceptionAgent_ = loadUserSubComponent<InterceptionAgentAPI>("interceptionAgent", ComponentInfo::SHARE_NONE);
    std::string rangesStr = params.find<std::string>("intercept_ranges", "");
    if (!rangesStr.empty()) {
        std::istringstream iss(rangesStr);
        std::string pair;
        while (std::getline(iss, pair, ';')) {
            size_t comma = pair.find(',');
            if (comma != std::string::npos) {
                uint64_t base = 0;
                uint64_t size = 0;
                std::istringstream(pair.substr(0, comma)) >> std::hex >> base;
                std::istringstream(pair.substr(comma + 1)) >> std::dec >> size;
                if (size > 0) {
                    interceptRanges_.push_back({base, base + size});
                }
            }
        }
    }
    if (interceptionAgent_) {
        if (leftHaliLink_) interceptionAgent_->setRingLink(leftHaliLink_);
        if (highlink_) interceptionAgent_->setHighlink(highlink_);
        if (!interceptRanges_.empty()) {
            interceptionAgent_->setInterceptBase(interceptRanges_[0].first);
        }
    }

    if (!interceptionAgent_) {
        registerAsPrimaryComponent();
        primaryComponentDoNotEndSim();
    }

    eventsReceived_ = 0;
    eventsForwarded_ = 0;
    eventsSent_ = 0;
    cpuEventCount_ = 0;
}

/*****************************************************************************
 * Lifecycle Phase #2: Init
 *****************************************************************************/
void Hali::init(unsigned phase) {
    if (verbose_) {
        out_->output("Phase: Init(%u), %s\n", phase, getName().c_str());
        out_->output("    %s: highlink_=%p, lowlink_=%p\n", getName().c_str(),
                     (void*)highlink_, (void*)lowlink_);
    }

    // Forward init events between highlink/lowlink; preserve source for coherence routing.
    if (highlink_ && lowlink_) {
        SST::Event* ev;
        while ((ev = highlink_->recvUntimedData()) != nullptr) {
            if (verbose_) out_->output("    %s: forwarding event from highlink to lowlink\n", getName().c_str());
            lowlink_->sendUntimedData(ev);
        }
        while ((ev = lowlink_->recvUntimedData()) != nullptr) {
            if (verbose_) out_->output("    %s: forwarding event from lowlink to highlink\n", getName().c_str());
            highlink_->sendUntimedData(ev);
        }
    }

    // Ring discovery requires both ring links; skip if topology isn't a ring.
    if (!leftHaliLink_ || !rightHaliLink_) {
        return;
    }

    if (phase == 0) {
        HaliEvent* event = new HaliEvent(getName());
        leftHaliLink_->sendUntimedData(event);
    }

    while (SST::Event* ev = rightHaliLink_->recvUntimedData()) {
        HaliEvent* event = dynamic_cast<HaliEvent*>(ev);
        if (event) {
            if (verbose_) {
                out_->output("    %" PRIu64 " %s received %s\n",
                             getCurrentSimCycle(), getName().c_str(), event->toString().c_str());
            }

            if (event->getStr() == getName()) {
                delete event;
            } else {
                neighbors_.insert(event->getStr());
                leftHaliLink_->sendUntimedData(event);
            }
        } else {
            out_->fatal(CALL_INFO, -1, "Error in %s: Unexpected event type during init()\n", getName().c_str());
        }
    }
}

/*****************************************************************************
 * Lifecycle Phase #3: Setup
 *****************************************************************************/
void Hali::setup() {
    out_->output("Phase: Setup, %s\n", getName().c_str());

    if (faultInjManager_) {
        faultInjManager_->processMessagesFromPMs();
    }

    if (interceptionAgent_) {
        interceptionAgent_->agentSetup();
        out_->output("Phase: Run, %s\n", getName().c_str());
        return;
    }

    eventsToSend_ = neighbors_.size();
    out_->output("    %s will send 1 event to each other component.\n", getName().c_str());

    if (neighbors_.empty()) {
        out_->output("    %s: No neighbors found.\n", getName().c_str());
        primaryComponentOKToEndSim();
        return;
    }
    if (eventsToSend_ == 0) {
        out_->output("    %s: No events to send.\n", getName().c_str());
        primaryComponentOKToEndSim();
        return;
    }

    iter_ = neighbors_.upper_bound(getName());
    if (iter_ == neighbors_.end()) iter_ = neighbors_.begin();

    if (leftHaliLink_) {
        leftHaliLink_->send(new HaliEvent(*iter_));
        eventsSent_++;
        iter_++;
        if (iter_ == neighbors_.end()) iter_ = neighbors_.begin();
    }

    out_->output("Phase: Run, %s\n", getName().c_str());
}

/*****************************************************************************
 * Lifecycle Phase #4: Run - Event Handlers
 *****************************************************************************/

void Hali::handleMemCtrlEvent(SST::Event* ev) {
    CpuEvent* cpuev = dynamic_cast<CpuEvent*>(ev);
    if (cpuev) {
        if (memCtrlLink_) {
            if (verbose_) out_->output("%s: forwarding CpuEvent to MemCtrl\n", getName().c_str());
            memCtrlLink_->send(cpuev);
        } else {
            delete cpuev;
        }
    } else {
        delete ev;
        out_->fatal(CALL_INFO, -1, "Error in %s: Unexpected event type in handleMemCtrlEvent\n", getName().c_str());
    }
}

void Hali::handleCpuEvent(SST::Event* ev) {
    if (!ev) return;

    CpuEvent* cpuEvent = nullptr;
    FaultInjEvent* faultEvent = nullptr;

    if (typeid(*ev) == typeid(CpuEvent)) {
        cpuEvent = dynamic_cast<CpuEvent*>(ev);
    } else if (typeid(*ev) == typeid(FaultInjEvent)) {
        faultEvent = dynamic_cast<FaultInjEvent*>(ev);
    }

    if (cpuEvent) {
        cpuEventCount_++;

        // Add pending PM request on every other CPU event
        if (cpuEventCount_ % 2 == 0 && faultInjManager_) {
            if (cpuEventCount_ % 4 == 0) {
                faultInjManager_->addHighLinkRequest("injection_rate 0.25");
                if (verbose_) {
                    out_->output("%s: Added injection_rate PM request (cpuEventCount=%u)\n",
                                 getName().c_str(), cpuEventCount_);
                }
            } else {
                faultInjManager_->addHighLinkRequest("test_pm_command");
                if (verbose_) {
                    out_->output("%s: Added test_pm_command PM request (cpuEventCount=%u)\n",
                                 getName().c_str(), cpuEventCount_);
                }
            }
        }

        if (verbose_) {
            out_->output("Hali CPUEVENT %" PRIu64 " %s received %s\n",
                         getCurrentSimCycle(), getName().c_str(), cpuEvent->toString().c_str());
        }
        delete cpuEvent;
    } else if (faultEvent) {
        if (verbose_) {
            out_->output("[cycle %" PRIu64 "] %s: FaultInjEvent '%s' rate=%.2f\n",
                         getCurrentSimCycle(), getName().c_str(),
                         faultEvent->getFname().c_str(), faultEvent->getRate());
        }

        // fname may already contain params (e.g. "set_range 0.1"); only append rate if non-zero.
        if (faultInjManager_) {
            std::string pmCommand = faultEvent->getFname();
            if (faultEvent->getRate() != 0.0) {
                pmCommand += " " + std::to_string(faultEvent->getRate());
            }
            faultInjManager_->addHighLinkRequest(pmCommand);
            if (verbose_) {
                out_->output("%s: Queued PM command: %s\n", getName().c_str(), pmCommand.c_str());
            }
        }
        delete faultEvent;
    }
}

void Hali::highlinkMemEvent(SST::Event* ev) {
    MemEvent* mevent = dynamic_cast<MemEvent*>(ev);
    if (mevent && interceptionAgent_ && isInterceptedAddress(mevent->getAddr())) {
        if (interceptionAgent_->handleInterceptedEvent(mevent, highlink_)) {
            return;
        }
    }
    if (lowlink_) {
        if (mevent && faultInjManager_) {
            MemEvent* processedEvent = faultInjManager_->processHighLinkMessage(mevent);
            if (processedEvent != mevent) {
                if (verbose_) out_->output("%s: Processed MemEvent with PM data\n", getName().c_str());
                delete mevent;
            }
            lowlink_->send(processedEvent);
        } else {
            lowlink_->send(ev);
        }
    } else {
        delete ev;
    }
}

void Hali::lowlinkMemEvent(SST::Event* ev) {
    if (highlink_) {
        highlink_->send(ev);
    } else {
        delete ev;
    }
}

// Signals end-of-sim on the final sensor event.
void Hali::handleSensorEvent(SST::Event* ev) {
    SensorEvent* event = dynamic_cast<SensorEvent*>(ev);

    if (event) {
        if (verbose_) {
            out_->output("    %" PRIu64 " %s received %s\n",
                         getCurrentSimCycle(), getName().c_str(), event->toString().c_str());
        }

        bool last = event->isLast();
        delete event;

        if (last) {
            primaryComponentOKToEndSim();
        }
    } else {
        out_->fatal(CALL_INFO, -1, "Error in %s: Unexpected event type in handleSensorEvent\n", getName().c_str());
    }
}

void Hali::handleHaliEvent(SST::Event* ev) {
    HaliEvent* event = dynamic_cast<HaliEvent*>(ev);

    if (event) {
        if (interceptionAgent_ && event->getStr() == "done") {
            if (verbose_) {
                out_->output("    %" PRIu64 " %s received done (iteration %u)\n",
                             getCurrentSimCycle(), getName().c_str(), event->getNum());
            }
            interceptionAgent_->notifyPartnerDone(event->getNum());
            delete event;
            return;
        }

        if (verbose_) {
            out_->output("    %" PRIu64 " %s received %s\n",
                         getCurrentSimCycle(), getName().c_str(), event->toString().c_str());
        }

        if (event->getStr() == getName()) {
            eventsReceived_++;
            delete event;

            if (eventsReceived_ == eventsToSend_) {
                primaryComponentOKToEndSim();
            }

            if (eventsSent_ != eventsToSend_ && leftHaliLink_) {
                leftHaliLink_->send(new HaliEvent(*iter_));
                eventsSent_++;
                iter_++;
                if (iter_ == neighbors_.end()) iter_ = neighbors_.begin();
            }
        } else {
            eventsForwarded_++;
            if (leftHaliLink_) {
                leftHaliLink_->send(event);
            } else {
                delete event;
            }
        }
    } else {
        out_->fatal(CALL_INFO, -1, "Error in %s: Unexpected event type in handleHaliEvent\n", getName().c_str());
    }
}

bool Hali::isInterceptedAddress(uint64_t addr) const {
    for (const auto& range : interceptRanges_) {
        if (addr >= range.first && addr < range.second) {
            return true;
        }
    }
    return false;
}

/*****************************************************************************
 * Lifecycle Phase #5: Complete
 *****************************************************************************/
void Hali::complete(unsigned phase) {
    out_->output("Phase: Complete(%u), %s\n", phase, getName().c_str());

    if (highlink_ && lowlink_) {
        SST::Event* ev;
        while ((ev = highlink_->recvUntimedData()) != nullptr) {
            lowlink_->sendUntimedData(ev);
        }
        while ((ev = lowlink_->recvUntimedData()) != nullptr) {
            highlink_->sendUntimedData(ev);
        }
    }

    if (phase == 0) {
        std::string goodbye = "Goodbye from " + getName();
        std::string farewell = "Farewell from " + getName();
        if (leftHaliLink_) leftHaliLink_->sendUntimedData(new HaliEvent(goodbye));
        if (rightHaliLink_) rightHaliLink_->sendUntimedData(new HaliEvent(farewell));
    }

    if (leftHaliLink_) {
        while (SST::Event* ev = leftHaliLink_->recvUntimedData()) {
            HaliEvent* event = dynamic_cast<HaliEvent*>(ev);
            if (event) {
                if (verbose_) {
                    out_->output("    %" PRIu64 " %s received %s\n",
                                 getCurrentSimCycle(), getName().c_str(), event->toString().c_str());
                }
                leftHaliMsg_ = event->getStr();
                delete event;
            } else {
                out_->fatal(CALL_INFO, -1, "Error in %s: Unexpected event type during complete()\n", getName().c_str());
            }
        }
    }

    if (rightHaliLink_) {
        while (SST::Event* ev = rightHaliLink_->recvUntimedData()) {
            HaliEvent* event = dynamic_cast<HaliEvent*>(ev);
            if (event) {
                if (verbose_) {
                    out_->output("    %" PRIu64 " %s received %s\n",
                                 getCurrentSimCycle(), getName().c_str(), event->toString().c_str());
                }
                rightHaliMsg_ = event->getStr();
                delete event;
            } else {
                out_->fatal(CALL_INFO, -1, "Error in %s: Unexpected event type during complete()\n", getName().c_str());
            }
        }
    }
}

/*****************************************************************************
 * Lifecycle Phase #6: Finish
 *****************************************************************************/
void Hali::finish() {
    out_->output("Phase: Finish, %s\n", getName().c_str());
    out_->output("    %s: sent %u messages, received %u, forwarded %u.\n",
                 getName().c_str(), eventsSent_, eventsReceived_, eventsForwarded_);
}

/*****************************************************************************
 * Lifecycle Phase #7: Destruction
 *****************************************************************************/
Hali::~Hali() {
    out_->output("Phase: Destruction\n");
    delete out_;
}

/*****************************************************************************
 * Signal Handlers
 *****************************************************************************/
void Hali::emergencyShutdown() {
    out_->output("Emergency shutdown: %s, sent %u messages.\n", getName().c_str(), eventsSent_);
}

void Hali::printStatus(Output& sim_out) {
    sim_out.output("%s: sent %u, received %u, forwarded %u.\n",
                   getName().c_str(), eventsSent_, eventsReceived_, eventsForwarded_);
}
