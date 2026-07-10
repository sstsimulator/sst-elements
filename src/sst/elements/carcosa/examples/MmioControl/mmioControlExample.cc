// Copyright 2009-2026 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2026, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory of the distribution.
//
// This file is part of the SST software package. For license information,
// see the LICENSE file in the top level directory of the distribution.

#include "sst_config.h"
#include "sst/elements/carcosa/examples/MmioControl/mmioControlExample.h"
#include <cinttypes>
#include <cstring>

using namespace SST;
using namespace SST::Interfaces;
using namespace SST::Carcosa;

ExampleMmioDriver::ExampleMmioDriver(ComponentId_t id, Params& params)
    : Component(id)
{
    out_ = new Output("ExampleMmioDriver: ", 1, 0, Output::STDOUT);
    requireLibrary("memHierarchy");

    mmioBase_ = params.find<uint64_t>("mmio_base", 0xBEEF0000);
    armValue_ = params.find<uint32_t>("arm_value", 0xABCD);

    std::string clock = params.find<std::string>("clock", "1GHz");
    TimeConverter tc = getTimeConverter(clock);

    respHandler_ = new RespHandler(this, out_);
    iface_ = loadUserSubComponent<StandardMem>(
        "mem_iface", ComponentInfo::SHARE_NONE, tc,
        new StandardMem::Handler<ExampleMmioDriver, &ExampleMmioDriver::handleResponse>(this));
    if (!iface_)
        out_->fatal(CALL_INFO, -1, "ExampleMmioDriver: no 'mem_iface' loaded\n");

    registerClock(tc, new Clock::Handler<ExampleMmioDriver, &ExampleMmioDriver::tick>(this));
    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();
}

ExampleMmioDriver::~ExampleMmioDriver()
{
    delete respHandler_;
    delete out_;
}

void ExampleMmioDriver::init(unsigned phase) { iface_->init(phase); }
void ExampleMmioDriver::setup()              { iface_->setup(); }
void ExampleMmioDriver::finish()             {}

void ExampleMmioDriver::sendRead(uint64_t offset)
{
    iface_->send(new StandardMem::Read(mmioBase_ + offset, 4));
}

void ExampleMmioDriver::sendWrite(uint64_t offset, uint32_t value)
{
    std::vector<uint8_t> data(4);
    std::memcpy(data.data(), &value, 4);
    iface_->send(new StandardMem::Write(mmioBase_ + offset, 4, data, false));
}

bool ExampleMmioDriver::tick(Cycle_t)
{
    switch (step_) {
    case 0:
        out_->output("step 0: read value@0x%" PRIx64
                     " (peripheral should park it until armed)\n",
                     mmioBase_ + ExampleControlAgent::kValueOffset);
        sendRead(ExampleControlAgent::kValueOffset);
        step_ = 1;
        return false;
    case 1:
        out_->output("step 1: arm 0x%x via write@0x%" PRIx64
                     " (should complete the parked read)\n",
                     armValue_, mmioBase_ + ExampleControlAgent::kArmOffset);
        sendWrite(ExampleControlAgent::kArmOffset, armValue_);
        step_ = 2;
        return false;
    case 2:
        if (gotReadResp_) {
            if (readValue_ == armValue_)
                out_->output("PASS: deferred read returned 0x%x via completePendingRead\n",
                             readValue_);
            else
                out_->output("FAIL: deferred read returned 0x%x (expected 0x%x)\n",
                             readValue_, armValue_);
            primaryComponentOKToEndSim();
            return true;
        }
        if (++waitCycles_ > 1000) {
            out_->output("FAIL: parked read never completed after %d cycles\n", waitCycles_);
            primaryComponentOKToEndSim();
            return true;
        }
        return false;
    default:
        primaryComponentOKToEndSim();
        return true;
    }
}

void ExampleMmioDriver::handleResponse(StandardMem::Request* req)
{
    req->handle(respHandler_);
}

void ExampleMmioDriver::RespHandler::handle(StandardMem::ReadResp* resp)
{
    uint32_t v = 0;
    if (resp->data.size() >= sizeof(uint32_t))
        std::memcpy(&v, resp->data.data(), sizeof(uint32_t));
    drv_->readValue_   = v;
    drv_->gotReadResp_ = true;
    delete resp;
}

void ExampleMmioDriver::RespHandler::handle(StandardMem::WriteResp* resp)
{
    delete resp;
}
