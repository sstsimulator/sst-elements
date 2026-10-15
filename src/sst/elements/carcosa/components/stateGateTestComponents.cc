// Copyright 2009-2026 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.

#include "sst_config.h"
#include "sst/elements/carcosa/components/stateGateTestComponents.h"
#include "sst/elements/carcosa/injectors/portModuleStateGate.h"

#include <string>
#include <vector>

using namespace SST;
using namespace SST::Carcosa;
using namespace SST::MemHierarchy;

namespace {

Params gateParams(const std::string& key, const std::string& mode,
                  const std::string& direction = "Receive") {
    Params params;
    params.insert("state_key", key);
    params.insert("fault_mode", mode);
    params.insert("install_direction", direction);
    params.insert("flip_probability", "1.0");
    params.insert("drop_probability", "0.0");
    params.insert("kernels", "1");
    params.insert("seed", "12345");
    return params;
}

void require(Output& out, bool ok, const std::string& context) {
    if (!ok) out.fatal(CALL_INFO, -1, "StateGateRegressionTest: FAIL %s\n", context.c_str());
}

// Exercise the public interception entry points, including their delivery
// cancellation contract. These are the same entry points SST link hooks use.
bool intercept(PortModuleStateGate& gate, Event*& event, bool send = false) {
    bool cancel = false;
    if (send) gate.eventSent(0, event);
    else gate.interceptHandler(0, event, cancel);
    return cancel;
}

Event* payloadlessEvent(unsigned kind) {
    switch (kind) {
        case 0: return new Event();
        case 1: return new MemEvent("test", 0x103c, 0x1000, Command::Inv);
        case 2: return new MemEvent("test", 0x103c, 0x1000, Command::GetS, 8);
        default: return new MemEvent("test", 0x103c, 0x1000, Command::GetX, 8);
    }
}

void testPayloads(Output& out, const std::string& key) {
    const char* kinds[] = {"generic Event", "zero-size Inv", "payloadless GetS", "payloadless GetX"};
    // A flip must preserve traffic with no actual payload in both directions,
    // including the flip path of drop_flip when its drop probability is zero.
    for (unsigned configuration = 0; configuration < 3; ++configuration) {
        const bool send = configuration == 1;
        const std::string mode = configuration == 2 ? "drop_flip" : "flip";
        Params params = gateParams(key, mode, send ? "Send" : "Receive");
        PortModuleStateGate gate(params);
        for (unsigned kind = 0; kind < 4; ++kind) {
            Event* event = payloadlessEvent(kind);
            Event* original = event;
            const bool cancelled = intercept(gate, event, send);
            const std::string context = mode + (send ? " Send " : " Receive ") + kinds[kind];
            require(out, !cancelled && event == original, context + " was not delivered unchanged");
            if (auto* memory = dynamic_cast<MemEvent*>(event)) {
                // getPayload() would manufacture the very data this checks for.
                require(out, memory->getPayloadSize() == 0, context + " acquired fabricated payload bytes");
                require(out, memory->getSize() == (kind == 1 ? 0u : 8u), context + " changed request size");
                const Command command = kind == 1 ? Command::Inv : kind == 2 ? Command::GetS : Command::GetX;
                require(out, memory->getCmd() == command, context + " changed command");
            }
            delete event;
        }

        std::vector<uint8_t> payload(8, 0xA5);
        Event* event = new MemEvent("test", 0x103c, 0x1000, Command::Write, payload);
        require(out, !intercept(gate, event, send) && event != nullptr, mode + " dropped payload-bearing Write");
        auto* memory = dynamic_cast<MemEvent*>(event);
        require(out, memory && memory->getPayloadSize() == payload.size(), mode + " changed payload length");
        unsigned changed_bits = 0;
        for (uint8_t byte : memory->getPayload()) {
            unsigned difference = byte ^ 0xA5u;
            while (difference) { difference &= difference - 1; ++changed_bits; }
        }
        require(out, changed_bits == 1, mode + " did not flip exactly one existing bit");
        delete event;
    }

    Params params = gateParams(key, "drop_flip");
    params.insert("drop_probability", "1.0");
    PortModuleStateGate gate(params);
    for (unsigned kind = 0; kind < 4; ++kind) {
        Event* event = payloadlessEvent(kind);
        const bool cancelled = intercept(gate, event);
        require(out, cancelled && event == nullptr,
                std::string("drop_flip failed to drop ") + kinds[kind]);
    }
}

void testRegions(Output& out, const std::string& key, PipelineStateBase& state) {
    const Command response_commands[] = {
        Command::GetSResp, Command::GetXResp, Command::FetchResp, Command::FetchXResp
    };
    for (bool use_virtual : {false, true}) {
        const uint64_t virtual_offset = use_virtual ? 0x10000 : 0;
        for (bool use_names : {false, true}) {
            Params params = gateParams(key, "drop");
            params.insert("drop_probability", "1.0");
            params.insert(use_names ? "region_names" : "region_ids", use_names ? "target" : "3");
            PortModuleStateGate gate(params);

            // The original request starts at offset 60. Cached responses carry
            // [0x1000,0x1040); byte-addressed transfers carry [0x103c,0x107c).
            for (unsigned form = 0; form < 3; ++form) {
                const bool cached = form == 0;
                const unsigned command_count = form == 2 ? 1 : 4;
                for (unsigned command_index = 0; command_index < command_count; ++command_index) {
                    const Command command = form == 2 ? Command::Write : response_commands[command_index];
                    for (bool neighbor : {false, true}) {
                        const uint64_t region_base = (neighbor ? 0x1040 : 0x1000) + virtual_offset;
                        state.publishRegion(3, region_base, neighbor ? 64 : 32, "target");
                        std::vector<uint8_t> payload(64, 0xA5);
                        auto* memory = new MemEvent("test", 0x103c, 0x1000, command, payload);
                        if (use_virtual) memory->setVirtualAddress(0x103c + virtual_offset);
                        if (form == 1) memory->setFlag(MemEventBase::F_NONCACHEABLE);
                        Event* event = memory;
                        const bool cancelled = intercept(gate, event);
                        const bool expected_drop = cached != neighbor;
                        const std::string context =
                            std::string(use_virtual ? "virtual " : "physical ") +
                            (use_names ? "region name " : "region id ") +
                            (cached ? "cached response " : form == 1 ? "noncacheable response " : "write ") +
                            (neighbor ? "neighbor" : "earlier line portion");
                        require(out, cancelled == expected_drop && (event == nullptr) == expected_drop,
                                context + " has incorrect region overlap");
                        if (event) {
                            require(out, memory->getPayloadSize() == payload.size() && memory->getPayload() == payload,
                                    context + " modified nonmatching payload");
                            delete event;
                        }
                    }
                }
            }
        }
    }
}

} // namespace

StateGateRegressionTest::StateGateRegressionTest(ComponentId_t id, Params& params) : Component(id) {
    requireLibrary("memHierarchy");
    Output out("", 1, 0, Output::STDOUT);
    const std::string key = getName();
    auto* state = PipelineStateRegistry<PipelineStateBase>::getOrCreate(key);
    state->publishKernel(1, "TEST", 1);
    const std::string suite = params.find<std::string>("suite", "payload");
    if (suite == "payload") testPayloads(out, key);
    else if (suite == "regions") testRegions(out, key, *state);
    else out.fatal(CALL_INFO, -1, "StateGateRegressionTest: unknown suite '%s'.\n", suite.c_str());
    out.output("StateGateRegressionTest: PASS %s\n", suite.c_str());
}
