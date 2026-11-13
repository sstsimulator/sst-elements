from eccRuntimeCommon import build
build({"fault_model": "campaign", "ecc_scheme": "secded",
       "campaign_target_kernel": "TARGET", "campaign_event_budget": 2,
       "campaign_event_rate": 1, "campaign_max_events_per_kernel_entry": 1,
       "campaign_mode": "cell", "campaign_errors_fixed": 3,
       "test_total_min": 4, "test_total_max": 4,
       "test_clean_min": 2, "test_clean_max": 2,
       "test_escape_min": 2, "test_escape_max": 2},
      {"requests": 4, "kernel_sequence": "TARGET,TARGET,OTHER,TARGET",
       "expect_mutated": 2, "expect_abort": 0, "expect_escapes": 2})
