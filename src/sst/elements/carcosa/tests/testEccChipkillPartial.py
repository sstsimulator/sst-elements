"""A one-byte Chipkill tail occupies two x4 chips and cannot escape."""
from eccRuntimeCommon import build


# Even a draw containing many bit errors can only touch the two represented
# chips. Require correction and DUE outcomes so a disabled sampler cannot pass.
build({"ecc_scheme": "chipkill", "ber": 0.1,
       "test_total_min": 5000, "test_total_max": 5000,
       "test_correctable_min": 1, "test_due_min": 1,
       "test_escape_min": 0, "test_escape_max": 0},
      {"requests": 5000, "payload_size": 1, "expect_escapes": 0},
      name="partial_chipkill")

# A forced campaign must choose the full word, rather than the one-byte tail,
# when only the full word has enough chips to deliver the requested escape.
build({"fault_model": "campaign", "ecc_scheme": "chipkill",
       "campaign_mode": "cell", "campaign_force_multi_chip": True,
       "campaign_errors_fixed": 3, "campaign_event_budget": 32,
       "campaign_event_rate": 1,
       "test_total_min": 32, "test_total_max": 32,
       "test_escape_min": 32, "test_escape_max": 32},
      {"requests": 32, "payload_size": 17, "test_min_changed_bits": 3,
       "expect_mutated": 32, "expect_escapes": 32},
      name="forced_chipkill_with_tail")
