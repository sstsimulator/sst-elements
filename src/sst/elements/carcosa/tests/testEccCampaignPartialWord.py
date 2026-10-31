"""Correlated draws spill past a partial final word without losing error bits."""
from eccRuntimeCommon import build

build({"fault_model": "campaign", "ecc_scheme": "secded",
       "campaign_event_budget": 32, "campaign_event_rate": 1,
       "campaign_mode": "word", "campaign_errors_fixed": 64,
       "test_total_min": 32, "test_total_max": 32,
       "test_escape_min": 32, "test_escape_max": 32},
      {"requests": 32, "payload_size": 9, "test_min_changed_bits": 64,
       "expect_mutated": 32, "expect_escapes": 32}, name="campaign")

build({"fault_model": "jedec_mix", "ecc_scheme": "secded",
       "fault_event_rate": 1, "fault_mode_weights": "0:0:0:0:0:1",
       "test_total_min": 32, "test_total_max": 32,
       "test_escape_min": 32, "test_escape_max": 32},
      {"requests": 32, "payload_size": 9, "test_min_changed_bits": 32,
       "expect_mutated": 32, "expect_escapes": 32}, name="jedec")
