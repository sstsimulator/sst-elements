"""Fast smoke test for carcosa.EccGuard: CarcosaCPU -> Hali -> L1 -> EccGuard -> memCtrl.

Knobs come from the environment (ECC_SCHEME, ECC_BER, ECC_*_LATENCY_PS,
ECC_KERNEL_POLICY) so the same config doubles as a quick spot-check driver.
Pass = sst exits 0 and the EccGuard outcome tables print.

Traffic comes from carcosa.CarcosaCPU rather than miranda: the local
build-sst-carcosa.sh tree compiles only the VLA subset of sst-elements
(carcosa, memHierarchy, vanadis, merlin, mmu), so miranda is not available.
"""
import os
import sst

ecc_scheme = os.getenv("ECC_SCHEME", "none")
ecc_ber    = os.getenv("ECC_BER", "0.0")
ecc_correctable_ps = os.getenv("ECC_CORRECTABLE_LATENCY_PS", "0")
ecc_due_ps         = os.getenv("ECC_DUE_LATENCY_PS", "0")
ecc_escape_ps      = os.getenv("ECC_ESCAPE_LATENCY_PS", "0")
ecc_kernel_policy  = os.getenv("ECC_KERNEL_POLICY", "")

sst.setStatisticLoadLevel(6)

# Force memHierarchy library load BEFORE any Carcosa component is constructed.
# On macOS dyld uses a flat-namespace lookup for SimpleMemBackendConvertor's
# RTTI symbol, which is exported by libmemHierarchy.so. If libCarcosa.so loads
# first that lookup fails. Instantiating any memHierarchy component first
# brings memHierarchy.so into dyld's namespace and unblocks the lookup.
l1 = sst.Component("l1cache", "memHierarchy.Cache")
l1.addParams({
    "access_latency_cycles": "2",
    "cache_frequency": "2.4 GHz",
    "replacement_policy": "lru",
    "coherence_protocol": "MESI",
    "associativity": "4",
    "cache_line_size": "64",
    "L1": "1",
    "cache_size": "32KB",
})

cpu = sst.Component("cpu", "carcosa.CarcosaCPU")
cpu.addParams({
    "clock":          "2.4GHz",
    "memFreq":        "2",
    "rngseed":        "29",
    "memSize":        "1MiB",
    "verbose":        0,
    "maxOutstanding": 8,
    "opCount":        1000,
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

memctrl = sst.Component("memory", "memHierarchy.MemController")
memctrl.addParams({
    "clock": "1GHz",
    "addr_range_end": 1 * 1024 * 1024 - 1,
    "backing": "malloc",
})
backend = memctrl.setSubComponent("backend", "memHierarchy.simpleMem")
backend.addParams({
    "access_time": "100 ns",
    "mem_size": "1MiB",
})

ecc = sst.Component("ecc_guard", "carcosa.EccGuard")
ecc.addParams({
    "verbose":                 "true",
    "state_key":               "",
    "ecc_scheme":              ecc_scheme,
    "ber":                     ecc_ber,
    "correctable_latency_ps":  ecc_correctable_ps,
    "due_latency_ps":          ecc_due_ps,
    "escape_latency_ps":       ecc_escape_ps,
    "kernel_policy":           ecc_kernel_policy,
    "apply_on_responses_only": "true",
    "seed":                    "1",
})
ecc.enableAllStatistics()

sst.Link("cpu_hali_ctrl").connect((cpu, "haliToCPU", "1ns"), (hali, "cpu", "1ns"))
sst.Link("iface_hali").connect((iface, "lowlink", "1ns"), (hali, "highlink", "1ns"))
sst.Link("hali_l1").connect((hali, "lowlink", "1ns"), (l1, "highlink", "1ns"))
sst.Link("l1_ecc").connect((l1, "lowlink", "50ps"), (ecc, "highlink", "50ps"))
sst.Link("ecc_mem").connect((ecc, "lowlink", "50ps"), (memctrl, "highlink", "50ps"))

# No stop-at: the CPU votes to end the sim once opCount drains, so a stalled
# run hangs visibly (CI timeout) instead of hitting a deadline that still
# prints plausible-looking outcome tables and exits 0.
