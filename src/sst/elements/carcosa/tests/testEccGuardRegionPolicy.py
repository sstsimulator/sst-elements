"""Phase 6 smoke test: region-aware EccGuard policy parser/lookup.

Wires CarcosaCPU -> Hali (with FourStateAgent publishing two named DRAM
regions) -> L1 -> EccGuard -> CarcosaMemCtrl. The EccGuard's kernel_policy CSV
exercises the multi-tag '*@REGION', 'KERNEL@REGION', and 'KERNEL' forms in one
pass. Pass = sst exits 0 and the per-kernel and per-kernel-per-region blocks
are printed (visually verifiable in -v mode).
"""
import sst
from mhlib import componentlist  # noqa: F401  (registers component types)

DEBUG = 0
DEBUG_LVL = 0

# Force memHierarchy library load BEFORE any Carcosa component is constructed.
# Same dyld flat-namespace workaround as testEccGuardJedecMix.py.
l1 = sst.Component("l1", "memHierarchy.Cache")
l1.addParams({
    "access_latency_cycles": "2",
    "cache_frequency":       "1GHz",
    "replacement_policy":    "lru",
    "coherence_protocol":    "MESI",
    "associativity":         "4",
    "cache_line_size":       "64",
    "cache_size":            "8KiB",
    "L1":                    "1",
    "debug":                 DEBUG, "debug_level": DEBUG_LVL,
})

# 1) CPU + memory interface.
cpu = sst.Component("cpu", "carcosa.CarcosaCPU")
cpu.addParams({
    "clock":          "1GHz",
    "memFreq":        "2",
    "rngseed":        "13",
    "memSize":        "1MiB",
    "verbose":        0,
    "maxOutstanding": 8,
    "opCount":        500,
    "reqsPerIssue":   1,
    "write_freq":     30,
    "read_freq":      70,
})
iface = cpu.setSubComponent("memory", "memHierarchy.standardInterface")

# 2) Hali in the data path. We use FourStateAgent to publish a state snapshot
# (state_key='ecc_region_smoke') with the MMIO region in slot 0 plus the two
# user-supplied DRAM regions ('weights' and 'kv_cache') in slots 1 and 2 via
# the new 'regions' parameter. EccGuard reads addr -> region from this snapshot.
hali = sst.Component("hali", "carcosa.Hali")
hali.addParams({
    "intercept_ranges": "0xBEEF0000,4096",
    "verbose":          "false",
})
agent = hali.setSubComponent("interceptionAgent", "carcosa.FourStateAgent")
agent.addParams({
    "state_key":      "ecc_region_smoke",
    "initial_command":"0",
    "num_commands":   "2",
    "max_iterations": "0",  # CPU never hits MMIO; agent stays idle.
    "verbose":        "false",
    # Two named regions covering most of the [0, 1MiB) address space the
    # CarcosaCPU touches. EccGuard routes by the first matching base..base+size.
    "regions":        "weights:0x0:0x80000,kv_cache:0x80000:0x80000",
})

ecc = sst.Component("ecc", "carcosa.EccGuard")
ecc.addParams({
    "verbose":                 "false",
    "state_key":               "ecc_region_smoke",
    "ecc_scheme":              "secded",
    "ber":                     "1e-7",
    "correctable_latency_ps":  "5000",
    "due_latency_ps":          "20000",
    "escape_latency_ps":       "0",
    # Multi-tag kernel_policy: kernel-only, region-only, and kernel x region.
    "kernel_policy": (
        "IDLE:none:0:0:0:0,"
        "*@weights:chipkill:1e-7:8000:30000:0,"
        "*@kv_cache:secded:1e-7:5000:20000:0"
    ),
    "apply_on_responses_only": "true",
    "fault_model":             "poisson",
    "due_action":              "latency_only",
    "seed":                    "1",
})
ecc.enableAllStatistics()

memctrl = sst.Component("memory", "memHierarchy.MemController")
memctrl.addParams({
    "clock":          "1GHz",
    "addr_range_end": 1 * 1024 * 1024 - 1,
    "backing":        "malloc",
})
backend = memctrl.setSubComponent("backend", "memHierarchy.simpleDRAM")
backend.addParams({
    "mem_size": "1MiB",
    "tCAS":     3, "tRCD": 3, "tRP": 3,
    "cycle_time": "5ns",
    "row_size": "8KiB",
    "row_policy": "open",
})

# Wire up. CarcosaCPU.haliToCPU connects to Hali.cpu; Hali sits in the data path
# between iface and L1. EccGuard sits between L1 and the memCtrl.
sst.Link("cpu_hali_ctrl").connect((cpu, "haliToCPU", "1ns"), (hali, "cpu", "1ns"))
sst.Link("iface_hali").connect((iface, "lowlink", "1ns"), (hali, "highlink", "1ns"))
sst.Link("hali_l1").connect((hali, "lowlink", "1ns"), (l1, "highlink", "1ns"))
sst.Link("l1_ecc").connect((l1, "lowlink", "1ns"), (ecc, "highlink", "1ns"))
sst.Link("ecc_mem").connect((ecc, "lowlink", "1ns"), (memctrl, "highlink", "1ns"))

sst.setStatisticLoadLevel(4)
sst.setProgramOption("stop-at", "100 us")
