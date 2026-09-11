"""
Same Vanadis + memHierarchy + Hali + FourStateAgent stack as
testFourStateRegistry.py, but with a PortModuleStateGate installed on each
Hali's lowlink port. The gate reads the FourStateAgent-published snapshot in
PipelineStateRegistry<PipelineStateBase> for the matching state_key and
flips a single bit in MemEvent payloads that transit the lowlink while the
agent's currentKernel is in the configured set.

Wiring note: CPU -> Hali(highlink <-> lowlink) -> dTLB -> L1D. The gate is
installed on Hali's lowlink (the L1D-facing side) on Receive direction by
default. Receive direction means the gate intercepts responses coming back
from the cache toward the CPU; Send would corrupt requests heading out.

The gate is configured to:
  - fault_mode=flip           : corrupt one payload byte rather than drop
                                (drop would hang the CPU waiting on the load).
  - flip_probability=0.05     : ~5% of matching events get flipped. High
                                enough to be visibly observable in the
                                checksums K0..K3 print (compare stdout-100
                                from gated vs. ungated runs) and low
                                enough that the run still completes.
  - kernels="2"               : engage only while currentKernel == 2.
                                FourStateAgent publishes the CPU command
                                index currently in flight, so kernels="2"
                                exactly targets CPU handler K2 (the
                                read-modify-write kernel in fourstate.c)
                                and leaves K0/K1/K3 traffic untouched.
                                Diffing stdout against an ungated run
                                should show divergent "K2 ... v=..." lines
                                and identical K0/K1/K3 lines.
  - region_names is NOT set here; we want all cache traffic during K2, not
    just MMIO. Add "region_names": "mmio_control" to restrict faults to
    MMIO writes only, or "region_ids": "<id>" for numeric region IDs.

Build fourstate.c (same prereq as the ungated test):
  riscv64-unknown-linux-gnu-gcc -static -I.. -o fourstate fourstate.c

Run:
  sst testFourStateRegistryGated.py
"""

# Import the baseline module but suppress its auto-build. We override
# GATE_PARAMS_PER_CORE before invoking build_topology() explicitly.
import importlib
_base = importlib.import_module("testFourStateRegistry")

# Must match STATE_KEYS from the baseline (one gate entry per core).
_base.GATE_PARAMS_PER_CORE = {
    "core0": {
        "state_key":        "core0",
        "fault_mode":       "flip",
        "flip_probability": "0.05",
        "kernels":          "2",
        "verbose":          "1",
        "debug":            "1",
        "debug_level":      "1",
        "install_direction": "Receive",
    },
    "core1": {
        "state_key":        "core1",
        "fault_mode":       "flip",
        "flip_probability": "0.05",
        "kernels":          "2",
        "verbose":          "1",
        "debug":            "1",
        "debug_level":      "1",
        "install_direction": "Receive",
    },
}

_base.build_topology()
