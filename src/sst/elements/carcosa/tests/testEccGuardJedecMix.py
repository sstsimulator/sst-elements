"""Phase 6 smoke test: EccGuard JEDEC fault-mode mixture.

Wires CarcosaCPU -> Hali (passthrough) -> L1 -> EccGuard -> CarcosaMemCtrl with
fault_model='jedec_mix', a high fault_event_rate, and the default Sridharan
2015 mode weights. Pass = sst exits 0; the '=== EccGuard ... Fault-Mode Draws
===' block is printed and (when run with -v) cells dominate, devices are rare.

A statistical assertion that mode counts match the configured weights within
+/-5% over 1e6 events lives in the analyzer (fault_mode_mix.csv); the harness
test runner only checks exit code, so this script focuses on correct wiring +
parser acceptance.
"""
import sst

# Force memHierarchy library load BEFORE any Carcosa component is constructed.
# On macOS dyld uses a flat-namespace lookup for SimpleMemBackendConvertor's
# RTTI symbol, which is exported by libmemHierarchy.so. If libCarcosa.so loads
# first that lookup fails. Instantiating any memHierarchy component first
# brings memHierarchy.so into dyld's namespace and unblocks the lookup.
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
    "opCount":        500,
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
    "verbose":                 "false",
    "state_key":               "",   # no state needed for the mix test
    "ecc_scheme":              "secded",
    "ber":                     "0.0",
    "correctable_latency_ps":  "5000",
    "due_latency_ps":          "20000",
    "escape_latency_ps":       "0",
    "fault_model":             "jedec_mix",
    "fault_event_rate":        "0.5",
    # Distinct, non-default weights so the test exercises the parser; values
    # are normalized in EccGuard.cc so they need not sum to 1.
    "fault_mode_weights":      "60:15:10:8:5:2",
    "payload_dtype":           "bf16",
    "due_action":              "drop_frame",
    "apply_on_responses_only": "true",
    "seed":                    "7",
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
sst.setProgramOption("stop-at", "100 us")
