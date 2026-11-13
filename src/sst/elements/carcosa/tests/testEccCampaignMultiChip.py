"""Default and fixed multi-chip campaigns must reach three chips in one word."""
from eccRuntimeCommon import build

for name, campaign_params in (
    ("alias", {"campaign_mode": "multi_chip"}),
    ("cell", {"campaign_mode": "cell", "campaign_force_multi_chip": True}),
    ("fixed", {"campaign_mode": "word", "campaign_force_multi_chip": True,
               "campaign_errors_fixed": 3}),
    ("policy_chipkill", {"ecc_scheme": "none", "campaign_mode": "cell",
                         "campaign_force_multi_chip": True,
                         "kernel_policy": "TEST:chipkill:0:0:0:0"}),
):
    build({"fault_model": "campaign", "ecc_scheme": "chipkill",
           "campaign_event_budget": 8, "campaign_event_rate": 1,
           "test_total_min": 8, "test_total_max": 8,
           "test_correctable_max": 0, "test_due_max": 0,
           "test_escape_min": 8, "test_escape_max": 8,
           **campaign_params},
          {"requests": 8, "payload_size": 64, "test_min_changed_bits": 3,
           "expect_mutated": 8, "expect_escapes": 8}, name=name)

# The effective policy, rather than the uniform fallback, determines whether
# the flag changes a draw. SECDED retains two-bit DUE and single-cell correction.
for name, scheme_params in (
    ("secded", {"ecc_scheme": "secded"}),
    ("policy_secded", {"ecc_scheme": "chipkill",
                       "kernel_policy": "TEST:secded:0:0:0:0"}),
):
    build({"fault_model": "campaign", "campaign_force_multi_chip": True,
           "campaign_mode": "word", "campaign_errors_fixed": 2,
           "campaign_event_budget": 8, "campaign_event_rate": 1,
           "test_total_min": 8, "test_total_max": 8,
           "test_due_min": 8, "test_due_max": 8, "test_escape_max": 0,
           **scheme_params},
          {"requests": 8, "payload_size": 64, "test_min_changed_bits": 2,
           "expect_mutated": 8, "expect_escapes": 0}, name=name + "_fixed")
    build({"fault_model": "campaign", "campaign_force_multi_chip": True,
           "campaign_mode": "cell",
           "campaign_event_budget": 8, "campaign_event_rate": 1,
           "test_total_min": 8, "test_total_max": 8,
           "test_correctable_min": 8, "test_correctable_max": 8,
           "test_due_max": 0, "test_escape_max": 0,
           **scheme_params},
          {"requests": 8, "payload_size": 64,
           "expect_mutated": 0, "expect_escapes": 0}, name=name + "_cell")
