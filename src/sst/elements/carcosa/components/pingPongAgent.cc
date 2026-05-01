// Copyright 2009-2026 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2026, NTESS
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
#include "sst/elements/carcosa/components/pingPongAgent.h"
#include "sst/elements/carcosa/components/haliEvent.h"
#include "sst/elements/memHierarchy/memEvent.h"
#include "sst/elements/memHierarchy/memTypes.h"
#include <cstring>
#include <climits>

using namespace SST;
using namespace SST::MemHierarchy;
using namespace SST::Carcosa;

PingPongAgent::PingPongAgent(ComponentId_t id, Params& params)
    : InterceptionAgentAPI(id, params)
{
    out_ = new Output("", 1, 0, Output::STDOUT);
    initialCommand_ = params.find<int>("initial_command", 0);
    maxIterations_ = params.find<int>("max_iterations", 6);
    verbose_ = params.find<bool>("verbose", false);
}

PingPongAgent::~PingPongAgent()
{
    delete out_;
}

bool PingPongAgent::handleInterceptedEvent(MemEvent* ev, Link* highlink)
{
    uint64_t offset = ev->getAddr() - controlAddrBase_;

    if (offset == 0x0000 && ev->getCmd() == Command::GetS) {
        if (nextCommand_ >= -1 && nextCommand_ != INT_MIN) {
            sendCommandResponse(ev, nextCommand_);
            nextCommand_ = INT_MIN;
        } else {
            pendingCommandRead_ = ev;
        }
        return true;
    }
    if (offset == 0x0004 && (ev->getCmd() == Command::Write || ev->getCmd() == Command::GetX)) {
        sendWriteAck(ev);
        localDone_ = true;
        currentIteration_++;
        if (leftHaliLink_) {
            leftHaliLink_->send(new HaliEvent("done", static_cast<unsigned>(currentIteration_)));
        }
        if (verbose_) {
            out_->output("PingPongAgent: sent done iteration %u\n", currentIteration_);
        }
        checkBothDone();
        return true;
    }
    delete ev;
    return true;
}

void PingPongAgent::notifyPartnerDone(unsigned iteration)
{
    (void)iteration;
    partnerDone_ = true;
    if (verbose_) {
        out_->output("PingPongAgent: partner done\n");
    }
    checkBothDone();
}

void PingPongAgent::agentSetup()
{
    nextCommand_ = initialCommand_;
    if (verbose_) {
        out_->output("PingPongAgent: setup initial_command=%d, max_iterations=%d\n",
                    initialCommand_, maxIterations_);
    }
    if (pendingCommandRead_) {
        sendCommandResponse(pendingCommandRead_, nextCommand_);
        pendingCommandRead_ = nullptr;
        nextCommand_ = INT_MIN;
    }
}

void PingPongAgent::setRingLink(Link* leftLink)
{
    leftHaliLink_ = leftLink;
}

void PingPongAgent::setInterceptBase(uint64_t base)
{
    controlAddrBase_ = base;
}

void PingPongAgent::setHighlink(Link* highlink)
{
    highlink_ = highlink;
}

void PingPongAgent::checkBothDone()
{
    if (!localDone_ || !partnerDone_) return;

    localDone_ = false;
    partnerDone_ = false;

    if (currentIteration_ >= maxIterations_) {
        nextCommand_ = -1;
    } else {
        nextCommand_ = currentIteration_ % 2;
    }

    if (pendingCommandRead_) {
        sendCommandResponse(pendingCommandRead_, nextCommand_);
        pendingCommandRead_ = nullptr;
        nextCommand_ = INT_MIN;
    }
}

void PingPongAgent::sendCommandResponse(MemEvent* request, int value)
{
    MemEvent* resp = request->makeResponse();
    std::vector<uint8_t> data(4);
    std::memcpy(data.data(), &value, sizeof(int));
    resp->setPayload(data);
    if (highlink_) highlink_->send(resp);
    delete request;
}

void PingPongAgent::sendWriteAck(MemEvent* ev)
{
    MemEvent* resp = ev->makeResponse();
    if (highlink_) highlink_->send(resp);
    delete ev;
}
