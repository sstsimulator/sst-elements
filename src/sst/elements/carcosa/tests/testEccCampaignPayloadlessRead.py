"""A read's absent payload must not spend its response's campaign budget."""
from eccRuntimeCommon import build

for responses_only in (True, False):
    build({"fault_model": "campaign", "ecc_scheme": "none",
           "apply_on_responses_only": responses_only,
           "addr_filter_region": "action_queue",
           "campaign_event_budget": 1, "campaign_event_rate": 1,
           "campaign_mode": "cell", "campaign_errors_fixed": 1,
           "test_total_min": 1 if responses_only else 2,
           "test_total_max": 1 if responses_only else 2,
           "test_escape_min": 1, "test_escape_max": 1},
          {"region_name": "action_queue", "payload_size": 64,
           "expect_mutated": 1, "expect_abort": 0, "expect_escapes": 1},
          name="responses" if responses_only else "all_events")
