"""Regression test for the region-predicate address-overlap fix (#19).

The driver publishes a valid 'decoy' region that no traffic ever touches,
and a PortModuleStateGate on the driver's mem_side (Send) is told to flip
every event with region_names="decoy". Under the fixed semantics the
predicate tests the EVENT's address range for overlap, so the gate never
fires and the run must produce exact fault-free checksums. Under the old
published-region-exists semantics every response would be flipped and the
checksum / corruption expectations here would fail.

Run: sst testStateGateRegionNoOverlap.py
"""
import framePipelineCommon as common

common.build(
    extra_region="decoy:0x80000:64",
    mem_side_gate={
        "state_key": common.STATE_KEY,
        "fault_mode": "flip",
        "flip_probability": "1.0",
        "region_names": "decoy",
        "install_direction": "Send",
    },
)
