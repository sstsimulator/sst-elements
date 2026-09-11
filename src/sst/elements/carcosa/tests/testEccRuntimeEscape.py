from eccRuntimeCommon import build
build({"fault_model": "campaign", "ecc_scheme": "secded",
       "campaign_event_budget": 1, "campaign_event_rate": 1,
       "campaign_mode": "cell", "campaign_errors_fixed": 3,
       "test_total_min": 1, "test_total_max": 1,
       "test_escape_min": 1, "test_escape_max": 1},
      {"expect_mutated": 1, "expect_abort": 0, "expect_escapes": 1})
