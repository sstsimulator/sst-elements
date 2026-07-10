#include "sst_config.h"
#include "sst/elements/carcosa/components/regressionTestComponents.h"
#include "sst/elements/carcosa/components/eccModelMath.h"
#include <cmath>
#include <sstream>

using namespace SST;
using namespace SST::Carcosa;
using namespace SST::MemHierarchy;

EccModelTest::EccModelTest(ComponentId_t id, Params&) : Component(id) {
    Output out("", 1, 0, Output::STDOUT);
    auto require = [&](bool ok, const char* what) {
        if (!ok) out.fatal(CALL_INFO, -1, "EccModelTest: FAIL %s\n", what);
    };
    // 100 FIT/Mbit/h * 1024 MiB * 8 Mbit/MiB * 100 ns/event / 3.6e12 ns/h.
    const double want_fit = 100.0 * 1e-9 * 1024.0 * 8.0 * 100.0 / 3.6e12;
    require(std::fabs(EccModelMath::fitEventRate(100, 1024, 100) - want_fit) < 1e-30,
            "FIT conversion (including byte-to-bit x8)");
    require(EccModelMath::wordCount(10, EccScheme::SECDED_64) == 2 &&
            EccModelMath::wordBits(10, EccScheme::SECDED_64, 0) == 64 &&
            EccModelMath::wordBits(10, EccScheme::SECDED_64, 1) == 16,
            "partial SECDED word sizing");
    require(EccModelMath::jedecEventRate(0.25, 0.9, 64) == 0.25,
            "explicit fault_event_rate precedence");
    require(EccModelMath::jedecEventRate(0.0, 0.001, 10) == 0.08,
            "BER payload-bit fallback");
    std::string prior = "TARGET"; uint64_t count = 2;
    require(!EccModelMath::resetCampaignEntry("TARGET", prior, count) && count == 2,
            "same campaign entry preserves count");
    require(EccModelMath::resetCampaignEntry("OTHER", prior, count) && count == 0,
            "off-target transition resets count");
    count = 1;
    require(EccModelMath::resetCampaignEntry("TARGET", prior, count) && count == 0,
            "target re-entry resets count");
    out.output("EccModelTest: PASS deterministic ECC math and campaign reset.\n");
}

HaliTestAgent::HaliTestAgent(ComponentId_t id, Params& params)
    : InterceptionAgentAPI(id, params) {
    out_ = new Output("", 1, 0, Output::STDOUT);
    mode_ = params.find<std::string>("mode", "payloadless_getx");
    if (mode_ == "deferred_complete") {
        self_ = configureSelfLink("complete", "1ns",
            new Event::Handler<HaliTestAgent, &HaliTestAgent::complete>(this));
    }
}

HaliTestAgent::~HaliTestAgent() { delete out_; }

bool HaliTestAgent::handleInterceptedEvent(MemEvent*, Link*) {
    out_->fatal(CALL_INFO, -1, "HaliTestAgent: payload-less GetX reached legacy API.\n");
    return true;
}

ControlResult HaliTestAgent::handleControlAccess(ControlAccess& acc) {
    if (mode_ == "payloadless_getx")
        out_->fatal(CALL_INFO, -1, "HaliTestAgent: payload-less GetX reached neutral API.\n");
    if (mode_ == "payload_getx") {
        if (!acc.isWrite || acc.value != 0x12345678u)
            out_->fatal(CALL_INFO, -1, "HaliTestAgent: payload-bearing GetX decoded incorrectly.\n");
        return ControlResult::Handled;
    }
    if (mode_ == "deferred_complete") {
        if (acc.isWrite) return ControlResult::Ignored;
        self_->send(new Event());
        return ControlResult::Deferred;
    }
    return acc.isWrite ? ControlResult::Ignored : ControlResult::Deferred;
}

void HaliTestAgent::complete(Event* ev) {
    delete ev;
    if (!channel_) out_->fatal(CALL_INFO, -1, "HaliTestAgent: missing ControlChannel.\n");
    channel_->completePendingRead(0xAABBCCDDu);
}

HaliTestDriver::HaliTestDriver(ComponentId_t id, Params& params) : Component(id) {
    out_ = new Output("", 1, 0, Output::STDOUT);
    base_ = params.find<uint64_t>("base", 0xBEEF0000);
    mode_ = params.find<std::string>("mode", "payloadless_getx");
    defer_ = mode_ == "double_defer";
    cpu_ = configureLink("cpu_side", new Event::Handler<HaliTestDriver, &HaliTestDriver::cpuEvent>(this));
    mem_ = configureLink("mem_side", new Event::Handler<HaliTestDriver, &HaliTestDriver::memEvent>(this));
    registerClock("1GHz", new Clock::Handler<HaliTestDriver, &HaliTestDriver::tick>(this));
    registerAsPrimaryComponent(); primaryComponentDoNotEndSim();
}

bool HaliTestDriver::tick(Cycle_t) {
    if (sent_) return false;
    sent_ = true;
    if (defer_) {
        cpu_->send(new MemEvent(getName(), base_, base_ & ~63ull, Command::GetS, 4));
        cpu_->send(new MemEvent(getName(), base_ + 4, base_ & ~63ull, Command::GetS, 4));
    } else if (mode_ == "deferred_complete") {
        cpu_->send(new MemEvent(getName(), base_, base_ & ~63ull, Command::GetS, 4));
    } else if (mode_ == "payload_getx") {
        std::vector<uint8_t> payload = {0x78, 0x56, 0x34, 0x12};
        cpu_->send(new MemEvent(getName(), base_, base_ & ~63ull,
                               Command::GetX, payload));
    } else {
        MemEvent* req = new MemEvent(getName(), base_, base_ & ~63ull,
                                     Command::GetX, 64);
        req->getPayload().clear();
        cpu_->send(req);
    }
    return false;
}

void HaliTestDriver::memEvent(Event* ev) {
    auto* req = dynamic_cast<MemEvent*>(ev);
    downstream_seen_ = true;
    if (!req || mode_ != "payloadless_getx" ||
        req->getCmd() != Command::GetX || req->getPayloadSize() != 0)
        out_->fatal(CALL_INFO, -1, "HaliTestDriver: forwarded request was not payload-less GetX.\n");
    MemEvent* resp = req->makeResponse();
    std::vector<uint8_t> payload(64, 0);
    resp->setPayload(payload);
    mem_->send(resp); delete req;
}

void HaliTestDriver::cpuEvent(Event* ev) {
    auto* resp = dynamic_cast<MemEvent*>(ev);
    if (!resp) out_->fatal(CALL_INFO, -1, "HaliTestDriver: non-MemEvent response.\n");
    if (mode_ == "deferred_complete") {
        auto& p = resp->getPayload();
        if (p.size() < 4 || p[0] != 0xDD || p[1] != 0xCC ||
            p[2] != 0xBB || p[3] != 0xAA)
            out_->fatal(CALL_INFO, -1, "HaliTestDriver: deferred read returned wrong value.\n");
    }
    delete resp; passed_ = true; primaryComponentOKToEndSim();
}

void HaliTestDriver::finish() {
    if (!defer_ && !passed_)
        out_->fatal(CALL_INFO, -1, "HaliTestDriver: mode %s did not complete.\n", mode_.c_str());
    if ((mode_ == "payload_getx" || mode_ == "deferred_complete") && downstream_seen_)
        out_->fatal(CALL_INFO, -1, "HaliTestDriver: handled control access leaked downstream.\n");
    if (!defer_) out_->output("HaliTestDriver: PASS mode=%s.\n", mode_.c_str());
    delete out_; out_ = nullptr;
}

EccRuntimeTestDriver::EccRuntimeTestDriver(ComponentId_t id, Params& params)
    : Component(id) {
    out_ = new Output("", 1, 0, Output::STDOUT);
    state_key_ = params.find<std::string>("state_key", "ecc_runtime_test");
    requests_ = params.find<int>("requests", 1);
    payload_size_ = params.find<int>("payload_size", 8);
    expect_mutated_ = params.find<int>("expect_mutated", -1);
    expect_abort_ = params.find<int>("expect_abort", -1);
    expect_escapes_ = params.find<int64_t>("expect_escapes", -1);
    std::stringstream ss(params.find<std::string>("kernel_sequence", ""));
    std::string tok;
    while (std::getline(ss, tok, ',')) kernels_.push_back(tok);
    state_ = PipelineStateRegistry<PipelineStateBase>::getOrCreate(state_key_);
    state_->currentKernelName = "TEST";
    cpu_ = configureLink("cpu_side", new Event::Handler<EccRuntimeTestDriver,
                         &EccRuntimeTestDriver::cpuEvent>(this));
    mem_ = configureLink("mem_side", new Event::Handler<EccRuntimeTestDriver,
                         &EccRuntimeTestDriver::memEvent>(this));
    registerClock("1GHz", new Clock::Handler<EccRuntimeTestDriver,
                  &EccRuntimeTestDriver::tick>(this));
    registerAsPrimaryComponent(); primaryComponentDoNotEndSim();
}

bool EccRuntimeTestDriver::tick(Cycle_t) {
    if (!started_) { started_ = true; issue(); }
    return false;
}

void EccRuntimeTestDriver::issue() {
    if (issued_ >= requests_) return;
    if (issued_ < static_cast<int>(kernels_.size()))
        state_->currentKernelName = kernels_[issued_];
    const uint64_t addr = 0x4000 + static_cast<uint64_t>(issued_ % 64) * 64;
    cpu_->send(new MemEvent(getName(), addr, addr & ~63ull,
                           Command::GetS, payload_size_));
    ++issued_;
}

void EccRuntimeTestDriver::memEvent(Event* ev) {
    auto* req = dynamic_cast<MemEvent*>(ev);
    if (!req) out_->fatal(CALL_INFO, -1, "EccRuntimeTestDriver: non-MemEvent request.\n");
    MemEvent* resp = req->makeResponse();
    std::vector<uint8_t> payload(payload_size_, 0xA5);
    resp->setPayload(payload);
    mem_->send(resp); delete req;
}

void EccRuntimeTestDriver::cpuEvent(Event* ev) {
    auto* resp = dynamic_cast<MemEvent*>(ev);
    if (!resp) out_->fatal(CALL_INFO, -1, "EccRuntimeTestDriver: non-MemEvent response.\n");
    bool changed = false;
    for (uint8_t b : resp->getPayload()) if (b != 0xA5) { changed = true; break; }
    if (changed) ++mutated_;
    delete resp;
    ++completed_;
    if (completed_ == requests_) primaryComponentOKToEndSim();
    else issue();
}

void EccRuntimeTestDriver::finish() {
    bool ok = completed_ == requests_;
    if (expect_mutated_ >= 0 && mutated_ != expect_mutated_) ok = false;
    if (expect_abort_ >= 0 && state_->frameAbortRequested != (expect_abort_ != 0)) ok = false;
    if (expect_escapes_ >= 0 && state_->eccCumulativeEscapes !=
        static_cast<uint64_t>(expect_escapes_)) ok = false;
    if (!ok) {
        out_->fatal(CALL_INFO, -1,
            "EccRuntimeTestDriver: FAIL completed=%d/%d mutated=%d abort=%d escapes=%" PRIu64 "\n",
            completed_, requests_, mutated_, state_->frameAbortRequested ? 1 : 0,
            state_->eccCumulativeEscapes);
    }
    out_->output("EccRuntimeTestDriver: PASS completed=%d mutated=%d abort=%d escapes=%" PRIu64 "\n",
                 completed_, mutated_, state_->frameAbortRequested ? 1 : 0,
                 state_->eccCumulativeEscapes);
    delete out_; out_ = nullptr;
}
