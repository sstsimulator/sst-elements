"""Self-contained MMIO control-transport smoke test.

A scripted StandardMem requestor (carcosa.ExampleMmioDriver) drives carcosa.Hali
in MMIO-peripheral mode, which dispatches the control accesses to
ExampleControlAgent via the transport-neutral handleControlAccess hook. The
agent parks the value read (Deferred) until the driver arms a value, then
completes it via completePendingRead. Exercises Hali's MMIO transport plus the
Deferred path end to end -- no VLA, no external host, no QEMU.

Expect output lines: 'step 0', 'step 1', then
  'PASS: deferred read returned 0xabcd via completePendingRead'
and 'Simulation is complete'.
"""
import sst

mmio_base = 0xBEEF0000
arm_value = 0xABCD

sst.setProgramOption("stop-at", "1us")  # backstop; driver ends the sim itself

driver = sst.Component("driver", "carcosa.ExampleMmioDriver")
driver.addParams({
    "clock":     "1GHz",
    "mmio_base": str(mmio_base),
    "arm_value": str(arm_value),
})
driver_iface = driver.setSubComponent("mem_iface", "memHierarchy.standardInterface")

hali = sst.Component("hali", "carcosa.Hali")
hali.addParams({
    "clock":     "1GHz",
    "mmio_base": str(mmio_base),
    "verbose":   "false",
})
hali.setSubComponent("interceptionAgent", "carcosa.ExampleControlAgent")
hali_iface = hali.setSubComponent("mmio_iface", "memHierarchy.standardInterface")

link = sst.Link("driver_to_hali")
link.connect((driver_iface, "lowlink", "1ns"), (hali_iface, "lowlink", "1ns"))
link.setNoCut()
