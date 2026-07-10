// Copyright 2009-2026 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2026, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"
#include "sst/elements/carcosa/components/fourStateAgent.h"
#include "sst/elements/carcosa/components/haliEvent.h"
#include "sst/elements/carcosa/components/vlaRegions.h"
#include "sst/elements/memHierarchy/memEvent.h"
#include "sst/elements/memHierarchy/memTypes.h"

#include <climits>
#include <cstring>

using namespace SST;
using namespace SST::MemHierarchy;
using namespace SST::Carcosa;

FourStateAgent::FourStateAgent(ComponentId_t id, Params& params)
    : InterceptionAgentAPI(id, params)
{
    out_ = new Output("", 1, 0, Output::STDOUT);

    stateKey_       = params.find<std::string>("state_key", "");
    regionSize_     = params.find<uint64_t>("region_size", 4096);
    regionsCsv_     = params.find<std::string>("regions", "");
    initialCommand_ = params.find<int>("initial_command", 0);
    numCommands_    = params.find<int>("num_commands", 4);
    maxIterations_  = params.find<int>("max_iterations", 12);
    verbose_        = params.find<bool>("verbose", false);
    params.find_array<std::string>("kernel_names", kernelNames_);

    if (numCommands_ < 1) {
        out_->fatal(CALL_INFO, -1,
                    "FourStateAgent: 'num_commands' must be >= 1 (got %d).\n", numCommands_);
    }

    if (stateKey_.empty()) {
        out_->fatal(CALL_INFO, -1,
                    "FourStateAgent: 'state_key' is required (pick something unique per core, "
                    "e.g. 'core0').\n");
    }
}

FourStateAgent::~FourStateAgent()
{
    delete out_;
}

void FourStateAgent::agentSetup()
{
    nextCommand_ = initialCommand_;

    // Establish the registry entry for this core. After this point any
    // PortModule (e.g. PortModuleStateGate) looking up stateKey_ will see
    // a live snapshot instead of nullptr.
    PipelineStateBase* s = PipelineStateRegistry<PipelineStateBase>::getOrCreate(stateKey_);
    s->currentKernel     = IDLE;
    s->currentKernelName = kernelNameFor(IDLE);
    s->pipelineCycle     = 0;
    publishedKernel_     = IDLE;

    // Publish the MMIO control region as a named region so that
    // `region_names="mmio_control"` predicates can match.
    s->ensureRegionSlot(0);
    s->regions[0].base  = controlAddrBase_;
    s->regions[0].size  = regionSize_;
    s->regions[0].valid = regionSize_ > 0;
    s->regions[0].id    = 0;
    s->regions[0].name  = "mmio_control";

    int n_user = publishUserRegions(stateKey_, regionsCsv_, out_, "FourStateAgent");
    if (verbose_ && n_user > 0) {
        out_->output("FourStateAgent[%s]: published %d user region(s)\n",
                     stateKey_.c_str(), n_user);
    }

    if (verbose_) {
        out_->output("FourStateAgent[%s]: setup initial_command=%d num_commands=%d "
                     "max_iterations=%d mmio_base=0x%" PRIx64 " size=%" PRIu64 "\n",
                     stateKey_.c_str(), initialCommand_, numCommands_, maxIterations_,
                     controlAddrBase_, regionSize_);
    }

    if (pendingCommandRead_) {
        sendCommandResponse(pendingCommandRead_, nextCommand_);
        pendingCommandRead_ = nullptr;
        nextCommand_ = INT_MIN;
    }
}

void FourStateAgent::publishState(int kernel)
{
    PipelineStateBase* s = PipelineStateRegistry<PipelineStateBase>::getMutable(stateKey_);
    if (!s) {
        // agentSetup() should have created the entry; defensively re-create
        // so a stray lookup from a PortModule never sees nullptr mid-run.
        s = PipelineStateRegistry<PipelineStateBase>::getOrCreate(stateKey_);
    }
    s->currentKernel     = kernel;
    s->currentKernelName = kernelNameFor(kernel);
    s->pipelineCycle     = (numCommands_ > 0) ? (currentIteration_ / numCommands_) : 0;
    publishedKernel_     = kernel;

    if (verbose_) {
        out_->output("FourStateAgent[%s]: publish currentKernel=%d ('%s') pipelineCycle=%d\n",
                     stateKey_.c_str(), s->currentKernel, s->currentKernelName.c_str(),
                     s->pipelineCycle);
    }
}

std::string FourStateAgent::kernelNameFor(int kernel) const
{
    if (kernel == IDLE) return "IDLE";
    if (kernel >= 0 && kernel < static_cast<int>(kernelNames_.size())
        && !kernelNames_[kernel].empty()) {
        return kernelNames_[kernel];
    }
    return "K" + std::to_string(kernel);
}

bool FourStateAgent::handleInterceptedEvent(MemEvent* ev, Link* highlink)
{
    uint64_t offset = ev->getAddr() - controlAddrBase_;

    // Command read: the CPU is blocked waiting for the next kernel index.
    // We do not publish here; publishState(nextCommand_) happens inside
    // sendCommandResponse(), at the instant we commit to a specific kernel.
    if (offset == 0x0000 && ev->getCmd() == Command::GetS) {
        if (nextCommand_ >= -1 && nextCommand_ != INT_MIN) {
            sendCommandResponse(ev, nextCommand_);
            nextCommand_ = INT_MIN;
        } else {
            pendingCommandRead_ = ev;
        }
        return true;
    }

    // Status write: the CPU has finished running the kernel it was
    // dispatched. The CPU will not issue any more loads/stores from the
    // handler past this point, so publish IDLE to close the gate window.
    if (offset == 0x0004 && (ev->getCmd() == Command::Write || ev->getCmd() == Command::GetX)) {
        sendWriteAck(ev);
        publishState(IDLE);
        localDone_ = true;
        currentIteration_++;
        if (leftHaliLink_) {
            leftHaliLink_->send(new HaliEvent("done", static_cast<unsigned>(currentIteration_)));
        }
        if (verbose_) {
            out_->output("FourStateAgent[%s]: sent done iteration %u\n",
                         stateKey_.c_str(), currentIteration_);
        }
        checkBothDone();
        return true;
    }

    // Intercept range covered an address we don't route through the FSM;
    // drop it and report handled so Hali doesn't forward it elsewhere.
    delete ev;
    return true;
}

void FourStateAgent::notifyPartnerDone(unsigned iteration)
{
    (void)iteration;
    partnerDone_ = true;
    if (verbose_) {
        out_->output("FourStateAgent[%s]: partner done (iteration=%u)\n",
                     stateKey_.c_str(), iteration);
    }
    checkBothDone();
}

void FourStateAgent::checkBothDone()
{
    if (!localDone_ || !partnerDone_) return;

    localDone_   = false;
    partnerDone_ = false;

    // Advance the "next command" by cycling through [0, numCommands_).
    if (currentIteration_ >= maxIterations_) {
        nextCommand_ = -1;
    } else {
        nextCommand_ = currentIteration_ % numCommands_;
    }

    if (pendingCommandRead_) {
        sendCommandResponse(pendingCommandRead_, nextCommand_);
        pendingCommandRead_ = nullptr;
        nextCommand_ = INT_MIN;
    }
}

void FourStateAgent::setRingLink(Link* leftLink) { leftHaliLink_ = leftLink; }

void FourStateAgent::setInterceptBase(uint64_t base) {
    controlAddrBase_ = base;
    // agentSetup() hasn't necessarily run yet, but if the registry entry
    // already exists (e.g. a gate looked it up), keep the region info
    // consistent with the base Hali handed us.
    if (!stateKey_.empty()) {
        PipelineStateBase* s = PipelineStateRegistry<PipelineStateBase>::getMutable(stateKey_);
        if (s) {
            s->ensureRegionSlot(0);
            s->regions[0].base  = controlAddrBase_;
            s->regions[0].size  = regionSize_;
            s->regions[0].valid = regionSize_ > 0;
            s->regions[0].id    = 0;
            s->regions[0].name  = "mmio_control";
        }
    }
}

void FourStateAgent::setHighlink(Link* highlink) { highlink_ = highlink; }

void FourStateAgent::sendCommandResponse(MemEvent* request, int value)
{
    // Publish BEFORE sending the response so PortModules on the lowlink see
    // currentKernel on the first post-unblock load/store. value < 0 => IDLE.
    publishState(value >= 0 ? value : IDLE);

    MemEvent* resp = request->makeResponse();
    std::vector<uint8_t> data(4);
    std::memcpy(data.data(), &value, sizeof(int));
    resp->setPayload(data);
    if (highlink_) highlink_->send(resp);
    delete request;
}

void FourStateAgent::sendWriteAck(MemEvent* ev)
{
    MemEvent* resp = ev->makeResponse();
    if (highlink_) highlink_->send(resp);
    delete ev;
}
