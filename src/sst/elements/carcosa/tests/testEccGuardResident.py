"""EccGuard resident fault-map smoke test (fault_model='resident').

Wires CarcosaCPU -> Hali (passthrough) -> L1 -> EccGuard -> CarcosaMemCtrl with
a persistent, address-keyed fault map: faults are born at t=0 and by a Poisson
process in sim time, live at physical locations with mode-shaped footprints
(cell/word/row/column/bank/device confined to one x4 chip), corrupt reads
deterministically, and are cleared only by patrol scrub.

Pass = sst exits 0 and the '=== EccGuard ... Resident Fault Map Summary ==='
block is printed. Paired-comparison property (same seed => identical fault set
under a different ecc_scheme) is checked by rerunning with
ECC_TEST_SCHEME=chipkill and diffing the Fault-Mode Draws block.
"""
import os
import sst

# Force memHierarchy library load BEFORE any Carcosa component is constructed
# (macOS dyld flat-namespace workaround; see testEccGuardJedecMix.py).
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
})

cpu = sst.Component("cpu", "carcosa.CarcosaCPU")
cpu.addParams({
    "clock":          "1GHz",
    "memFreq":        "2",
    "rngseed":        "29",
    "memSize":        "1MiB",
    "verbose":        0,
    "maxOutstanding": 8,
    "opCount":        20000,
    "reqsPerIssue":   1,
    "write_freq":     30,
    "read_freq":      70,
})
iface = cpu.setSubComponent("memory", "memHierarchy.standardInterface")

hali = sst.Component("hali", "carcosa.Hali")
hali.addParams({
    "intercept_ranges": "0xBEEF0000,4096",
    "verbose":          "false",
})

ecc = sst.Component("ecc", "carcosa.EccGuard")
ecc.addParams({
    "verbose":                   "false",
    "state_key":                 "",
    "ecc_scheme":                os.environ.get("ECC_TEST_SCHEME", "secded"),
    "ber":                       "0.0",
    "correctable_latency_ps":    "5000",
    "due_latency_ps":            "20000",
    "escape_latency_ps":         "0",
    "fault_model":               "resident",
    "resident_addr_start":       "0",
    "resident_addr_len":         str(1024 * 1024),
    "resident_faults_at_start":  os.environ.get("ECC_TEST_FAULTS", "12"),
    "resident_fault_rate_per_ms": os.environ.get("ECC_TEST_RATE", "100"),
    "resident_scrub_interval_us": "50",
    "resident_permanent_fraction": "0.3",
    "resident_mode":             os.environ.get("ECC_TEST_MODE", "mix"),
    "payload_dtype":             "bf16",
    "due_action":                "latency_only",
    "apply_on_responses_only":   "true",
    "seed":                      "7",
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

sst.Link("cpu_hali_ctrl").connect((cpu, "haliToCPU", "1ns"), (hali, "cpu", "1ns"))
sst.Link("iface_hali").connect((iface, "lowlink", "1ns"), (hali, "highlink", "1ns"))
sst.Link("hali_l1").connect((hali, "lowlink", "1ns"), (l1, "highlink", "1ns"))
sst.Link("l1_ecc").connect((l1, "lowlink", "1ns"), (ecc, "highlink", "1ns"))
sst.Link("ecc_mem").connect((ecc, "lowlink", "1ns"), (memctrl, "highlink", "1ns"))

sst.setStatisticLoadLevel(4)
sst.setProgramOption("stop-at", "2000 us")
