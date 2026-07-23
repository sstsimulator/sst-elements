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
#include "sst/elements/carcosa/components/framePipelineDriver.h"
#include <algorithm>
#include <cinttypes>
#include <sstream>

using namespace SST;
using namespace SST::MemHierarchy;
using namespace SST::Carcosa;

FramePipelineDriver::FramePipelineDriver(ComponentId_t id, Params& params)
    : Component(id) {
    out_ = new Output("", 1, 0, Output::STDOUT);
    verbose_         = params.find<bool>("verbose", false);
    state_key_       = params.find<std::string>("state_key", "");
    region_name_     = params.find<std::string>("region_name", "action_queue");
    region_base_     = params.find<uint64_t>("region_base", 8192);
    region_size_     = params.find<uint64_t>("region_size", 64);
    frames_          = params.find<int>("frames", 3);
    corrupt_frame_   = params.find<int>("corrupt_frame", -1);
    close_kernel_id_ = params.find<int>("close_kernel_id", 1);
    expect_corrupted_frames_ = params.find<int64_t>("expect_corrupted_frames", -1);

    auto parseU64Csv = [](const std::string& csv, std::vector<uint64_t>& out) {
        std::stringstream ss(csv); std::string tok;
        while (std::getline(ss, tok, ','))
            if (!tok.empty()) out.push_back(std::stoull(tok, nullptr, 0));
    };
    auto parseIntCsv = [](const std::string& csv, std::vector<int>& out) {
        std::stringstream ss(csv); std::string tok;
        while (std::getline(ss, tok, ','))
            if (!tok.empty()) out.push_back(std::stoi(tok, nullptr, 0));
    };
    parseU64Csv(params.find<std::string>("expect_checksums", ""), expect_checksums_);
    parseU64Csv(params.find<std::string>("frame_tokens", ""), frame_tokens_);
    parseIntCsv(params.find<std::string>("dropped_frames", ""), dropped_frames_);
    parseIntCsv(params.find<std::string>("escape_frames", ""), escape_frames_);

    if (state_key_.empty()) {
        out_->fatal(CALL_INFO, -1,
            "FramePipelineDriver '%s': state_key is required.\n", getName().c_str());
    }
    if (region_base_ < 16) {
        out_->fatal(CALL_INFO, -1,
            "FramePipelineDriver '%s': region_base must be >= 16 (straddling "
            "read starts at region_base - 16).\n", getName().c_str());
    }
    if (region_size_ < 48) {
        out_->fatal(CALL_INFO, -1,
            "FramePipelineDriver '%s': region_size must be >= 48 (the script "
            "covers [0,48) and leaves the rest as an unread hole).\n",
            getName().c_str());
    }

    cpu_side_ = configureLink("cpu_side",
        new Event::Handler<FramePipelineDriver, &FramePipelineDriver::handleCpuSide>(this));
    mem_side_ = configureLink("mem_side",
        new Event::Handler<FramePipelineDriver, &FramePipelineDriver::handleMemSide>(this));
    if (!cpu_side_ || !mem_side_) {
        out_->fatal(CALL_INFO, -1,
            "FramePipelineDriver '%s': both cpu_side and mem_side must be "
            "connected (through the watcher under test).\n", getName().c_str());
    }

    registerClock("1GHz",
        new Clock::Handler<FramePipelineDriver, &FramePipelineDriver::clockTick>(this));
    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();

    // Create the registry entry here rather than in setup(): the watcher's
    // setup() looks the key up and fatals on a miss, and setup() order
    // across components is unspecified. Constructors all run first.
    state_ptr_ = PipelineStateRegistry<PipelineStateBase>::getOrCreate(state_key_);
    state_ptr_->actuationKernelName = "ACTUATE";
    state_ptr_->publishKernel(-1, "IDLE", 0);
    state_ptr_->publishRegion(0, region_base_, region_size_, region_name_);

    std::string extra = params.find<std::string>("extra_region", "");
    if (!extra.empty()) {
        std::stringstream es(extra);
        std::string name, base_s, size_s;
        if (!std::getline(es, name, ':') || !std::getline(es, base_s, ':') ||
            !std::getline(es, size_s, ':')) {
            out_->fatal(CALL_INFO, -1,
                "FramePipelineDriver '%s': extra_region must be "
                "'name:base:size' (got '%s').\n", getName().c_str(), extra.c_str());
        }
        state_ptr_->publishRegion(1, std::stoull(base_s, nullptr, 0),
                                  std::stoull(size_s, nullptr, 0), name);
    }

    buildScript();
}

FramePipelineDriver::~FramePipelineDriver() {
    delete out_;
}

void FramePipelineDriver::buildScript() {
    const uint64_t base = region_base_;
    for (int f = 0; f < frames_; ++f) {
        // PREFILL: full-region read the watcher must ignore. If it wrongly
        // merged, the [48,64) hole would carry 0xE0-pattern bytes instead of
        // zeros and every expected checksum below would mismatch.
        script_.push_back({Op::Kind::Publish, 0, "PREFILL", f});
        script_.push_back({Op::Kind::Read, -1, "", f, base, 64, 0xE0, false});
        // ACTUATE: in-region read [base+16, base+48) then a straddling read
        // [base-16, base+16) whose upper half overlaps the region start.
        script_.push_back({Op::Kind::Publish, 1, "ACTUATE", f});
        script_.push_back({Op::Kind::Read, -1, "", f, base + 16, 32, 0x10,
                           f == corrupt_frame_});
        script_.push_back({Op::Kind::Read, -1, "", f, base - 16, 32, 0x50, false});
        // Frame-closing status write happens before the kernel transition in
        // the real pipeline; mirror that ordering here.
        script_.push_back({Op::Kind::Stamp});
        script_.push_back({Op::Kind::Publish, 2, "POST", f});
        // The watcher only notices kernel transitions on observed events;
        // this read makes it finalize (and classify) the ACTUATE frame.
        script_.push_back({Op::Kind::Read, -1, "", f, base, 4, 0x00, false});
    }
}

bool FramePipelineDriver::clockTick(Cycle_t) {
    if (awaiting_ || done_) return done_;
    while (pc_ < script_.size()) {
        const Op& op = script_[pc_++];
        switch (op.kind) {
        case Op::Kind::Publish:
            state_ptr_->publishKernel(op.kernelId, op.kernelName, op.cycle);
            cur_frame_                    = op.cycle;
            if (verbose_) {
                out_->output("FramePipelineDriver '%s': publish kernel=%d "
                             "('%s') cycle=%d\n", getName().c_str(),
                             op.kernelId, op.kernelName.c_str(), op.cycle);
            }
            break;
        case Op::Kind::Stamp:
            stampFrame();
            break;
        case Op::Kind::Read: {
            cur_seed_    = op.seed;
            cur_corrupt_ = op.corrupt;
            MemEvent* req = new MemEvent(getName(), op.addr, op.addr & ~63ull,
                                         Command::GetS, op.size);
            cpu_side_->send(req);
            awaiting_ = true;
            return false;
        }
        }
    }
    done_ = true;
    primaryComponentOKToEndSim();
    return true;
}

void FramePipelineDriver::stampFrame() {
    PipelineStateBase::FrameRecord fr;
    fr.pipelineCycle     = cur_frame_;
    fr.kernelAtClose     = close_kernel_id_;
    fr.kernelAtCloseName = "ACTUATE";
    fr.dropped = std::find(dropped_frames_.begin(), dropped_frames_.end(),
                           cur_frame_) != dropped_frames_.end();
    if (static_cast<size_t>(cur_frame_) < frame_tokens_.size())
        fr.actionToken = frame_tokens_[cur_frame_];
    if (std::find(escape_frames_.begin(), escape_frames_.end(), cur_frame_)
        != escape_frames_.end()) {
        state_ptr_->addEccCounts(1, 0);
    }
    fr.cumulativeEscapes = state_ptr_->eccCumulativeEscapes;
    fr.cumulativeFlips   = state_ptr_->eccCumulativeFlips;
    // Same consumption contract as VlaPipelineDriver: prefer the watcher's
    // fold when it observed critical bytes this frame. The watcher retires
    // the valid flag itself at frame close.
    if (state_ptr_->watcherActionChecksumValid) {
        fr.actionChecksum = state_ptr_->watcherActionChecksum;
    }
    fr.simTimePs = getCurrentSimTimeNano() * 1000;
    state_ptr_->appendFrame(fr);
    if (verbose_) {
        out_->output("FramePipelineDriver '%s': stamped frame cycle=%d "
                     "checksum=%" PRIu64 "\n", getName().c_str(),
                     cur_frame_, fr.actionChecksum);
    }
}

void FramePipelineDriver::handleMemSide(Event* ev) {
    MemEvent* req = dynamic_cast<MemEvent*>(ev);
    if (!req) {
        out_->fatal(CALL_INFO, -1,
            "FramePipelineDriver '%s': non-MemEvent on mem_side.\n",
            getName().c_str());
    }
    MemEvent* resp = req->makeResponse();
    std::vector<uint8_t> payload(req->getSize());
    for (size_t i = 0; i < payload.size(); ++i) {
        payload[i] = patternByte(cur_seed_, cur_frame_, req->getAddr() + i);
    }
    if (cur_corrupt_ && !payload.empty()) payload[0] ^= 0x80;
    resp->setPayload(payload);
    mem_side_->send(resp);
    delete req;
}

void FramePipelineDriver::handleCpuSide(Event* ev) {
    awaiting_ = false;
    delete ev;
}

void FramePipelineDriver::finish() {
    if (!state_ptr_) return;
    bool ok = true;

    if (state_ptr_->frames.size() != static_cast<size_t>(frames_)) {
        out_->output("FramePipelineDriver '%s': FAIL expected %d frames, "
                     "recorded %zu.\n", getName().c_str(), frames_,
                     state_ptr_->frames.size());
        ok = false;
    }
    if (!expect_checksums_.empty()) {
        if (expect_checksums_.size() != state_ptr_->frames.size()) {
            out_->output("FramePipelineDriver '%s': FAIL expect_checksums has "
                         "%zu entries for %zu frames.\n", getName().c_str(),
                         expect_checksums_.size(), state_ptr_->frames.size());
            ok = false;
        } else {
            for (size_t f = 0; f < expect_checksums_.size(); ++f) {
                uint64_t got  = state_ptr_->frames[f].actionChecksum;
                uint64_t want = expect_checksums_[f];
                if (got != want) {
                    out_->output("FramePipelineDriver '%s': FAIL frame %zu "
                                 "checksum %" PRIu64 " != expected %" PRIu64 ".\n",
                                 getName().c_str(), f, got, want);
                    ok = false;
                }
            }
        }
    }
    if (expect_corrupted_frames_ >= 0 &&
        state_ptr_->framesCriticalRegionCorrupted !=
            static_cast<uint64_t>(expect_corrupted_frames_)) {
        out_->output("FramePipelineDriver '%s': FAIL framesCriticalRegionCorrupted="
                     "%" PRIu64 " != expected %" PRId64 ".\n", getName().c_str(),
                     state_ptr_->framesCriticalRegionCorrupted,
                     expect_corrupted_frames_);
        ok = false;
    }

    if (!ok) {
        out_->fatal(CALL_INFO, -1,
            "FramePipelineDriver '%s': expectations not met (see FAIL lines).\n",
            getName().c_str());
    }
    out_->output("FramePipelineDriver '%s': PASS %zu frames verified.\n",
                 getName().c_str(), state_ptr_->frames.size());
}
