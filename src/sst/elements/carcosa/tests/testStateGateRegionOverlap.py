"""Positive half of the region-predicate address-overlap fix (#19).

A PortModuleStateGate on the driver's mem_side (Send) flips every ACTUATE
event overlapping the action_queue region (kernels="1" AND region_names).
The fully-in-region ACTUATE read gets one payload bit flipped every frame,
so all 3 frames must classify corrupted at the watcher and diverged at the
scorer. Exact checksums are RNG-dependent and not checked; the
classification counts are deterministic because ANY in-payload bit flip of
the in-region read changes the snapshot. PREFILL/POST responses also
overlap the region but are outside the gated kernel and outside the
watcher's actuation window, so they change nothing.

Run: sst testStateGateRegionOverlap.py
"""
import framePipelineCommon as common

common.build(
    expect_argmax_diff=3,
    expect_unsafe=3,
    expect_corrupted=3,
    check_exact_checksums=False,
    mem_side_gate={
        "state_key": common.STATE_KEY,
        "fault_mode": "flip",
        "flip_probability": "1.0",
        "kernels": "1",
        "region_names": "action_queue",
        "install_direction": "Send",
        "seed": "7",
    },
)
