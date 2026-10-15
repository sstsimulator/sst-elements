"""Inactive campaign parameters must not turn JEDEC device faults into escapes."""
from eccRuntimeCommon import build

for name, campaign_params in (
    ("flag", {"campaign_force_multi_chip": True}),
    ("alias", {"campaign_mode": "multi_chip", "campaign_errors_fixed": 2}),
):
    build({"fault_model": "jedec_mix", "ecc_scheme": "chipkill",
           "fault_event_rate": 1, "fault_mode_weights": "0:0:0:0:0:1",
           "test_total_min": 8, "test_total_max": 8,
           "test_correctable_min": 8, "test_correctable_max": 8,
           "test_due_max": 0, "test_escape_max": 0,
           **campaign_params},
          {"requests": 8, "payload_size": 64,
           "expect_mutated": 0, "expect_escapes": 0}, name=name)
