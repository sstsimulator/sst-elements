#ifndef SST_ELEMENTS_CARCOSA_REGRESSION_TEST_COMPONENTS_H
#define SST_ELEMENTS_CARCOSA_REGRESSION_TEST_COMPONENTS_H

#include "sst/elements/carcosa/components/interceptionAgentAPI.h"
#include "sst/elements/carcosa/components/pipelineStateRegistry.h"
#include <sst/core/component.h>
#include <sst/core/output.h>
#include <string>
#include <vector>

namespace SST { namespace Carcosa {

class EccModelTest : public SST::Component {
public:
    SST_ELI_REGISTER_COMPONENT(EccModelTest, "carcosa", "EccModelTest",
        SST_ELI_ELEMENT_VERSION(1,0,0), "Self-asserting ECC model math test.",
        COMPONENT_CATEGORY_UNCATEGORIZED)
    EccModelTest(ComponentId_t id, Params& params);
};

class HaliTestAgent : public InterceptionAgentAPI {
public:
    SST_ELI_REGISTER_SUBCOMPONENT(HaliTestAgent, "carcosa", "HaliTestAgent",
        SST_ELI_ELEMENT_VERSION(1,0,0), "Self-asserting Hali control test agent.",
        SST::Carcosa::InterceptionAgentAPI)
    HaliTestAgent(ComponentId_t id, Params& params);
    HaliTestAgent() : InterceptionAgentAPI() {}
    ~HaliTestAgent() override;
    bool handleInterceptedEvent(SST::MemHierarchy::MemEvent*, SST::Link*) override;
    ControlResult handleControlAccess(ControlAccess&) override;
    void setControlChannel(ControlChannel* ch) override { channel_ = ch; }
private:
    void complete(Event* ev);
    SST::Output* out_ = nullptr;
    std::string mode_;
    ControlChannel* channel_ = nullptr;
    Link* self_ = nullptr;
};

class HaliTestDriver : public SST::Component {
public:
    SST_ELI_REGISTER_COMPONENT(HaliTestDriver, "carcosa", "HaliTestDriver",
        SST_ELI_ELEMENT_VERSION(1,0,0), "Self-asserting Hali data-plane driver.",
        COMPONENT_CATEGORY_UNCATEGORIZED)
    SST_ELI_DOCUMENT_PARAMS({"mode", "payloadless_getx or double_defer", "payloadless_getx"},
                            {"base", "Intercept range base.", "3203334144"})
    SST_ELI_DOCUMENT_PORTS(
        {"cpu_side", "Connect to Hali highlink.", {"memHierarchy.MemEventBase"}},
        {"mem_side", "Connect to Hali lowlink.", {"memHierarchy.MemEventBase"}})
    HaliTestDriver(ComponentId_t id, Params& params);
    void finish() override;
private:
    bool tick(Cycle_t);
    void cpuEvent(Event*);
    void memEvent(Event*);
    Output* out_ = nullptr;
    Link *cpu_ = nullptr, *mem_ = nullptr;
    uint64_t base_ = 0;
    std::string mode_;
    bool defer_ = false, sent_ = false, passed_ = false, downstream_seen_ = false;
};

class EccRuntimeTestDriver : public SST::Component {
public:
    SST_ELI_REGISTER_COMPONENT(EccRuntimeTestDriver, "carcosa", "EccRuntimeTestDriver",
        SST_ELI_ELEMENT_VERSION(1,0,0), "Self-asserting EccGuard runtime driver.",
        COMPONENT_CATEGORY_UNCATEGORIZED)
    SST_ELI_DOCUMENT_PARAMS(
        {"state_key", "Pipeline registry key.", "ecc_runtime_test"},
        {"requests", "Number of serialized reads.", "1"},
        {"payload_size", "Response payload bytes.", "8"},
        {"kernel_sequence", "Optional CSV kernel name per request.", ""},
        {"expect_mutated", "Expected mutated responses (-1 disables).", "-1"},
        {"expect_abort", "Expected frameAbortRequested (-1 disables).", "-1"},
        {"expect_escapes", "Expected cumulative escapes (-1 disables).", "-1"})
    SST_ELI_DOCUMENT_PORTS(
        {"cpu_side", "Connect to EccGuard highlink.", {"memHierarchy.MemEventBase"}},
        {"mem_side", "Connect to EccGuard lowlink.", {"memHierarchy.MemEventBase"}})
    EccRuntimeTestDriver(ComponentId_t id, Params& params);
    void finish() override;
private:
    bool tick(Cycle_t);
    void cpuEvent(Event*);
    void memEvent(Event*);
    void issue();
    Output* out_ = nullptr;
    Link *cpu_ = nullptr, *mem_ = nullptr;
    PipelineStateBase* state_ = nullptr;
    std::string state_key_;
    std::vector<std::string> kernels_;
    int requests_ = 1, payload_size_ = 8, issued_ = 0, completed_ = 0;
    int mutated_ = 0, expect_mutated_ = -1, expect_abort_ = -1;
    int64_t expect_escapes_ = -1;
    bool started_ = false;
};

}} // namespace SST::Carcosa
#endif
