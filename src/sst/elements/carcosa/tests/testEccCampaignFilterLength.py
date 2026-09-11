"""A request outside the region prefix must leave the campaign budget intact."""
from eccRuntimeCommon import build

build({"fault_model": "campaign", "ecc_scheme": "none",
       "addr_filter_region": "action_queue", "addr_filter_len": 64,
       "campaign_event_budget": 1, "campaign_event_rate": 1,
       "campaign_mode": "cell", "campaign_errors_fixed": 1,
       "test_escape_min": 1, "test_escape_max": 1},
      {"requests": 2, "payload_size": 64, "region_name": "action_queue",
       "request_addresses": "0x4080,0x4000",
       "expect_mutated_sequence": "0,1",
       "expect_mutated": 1, "expect_abort": 0, "expect_escapes": 1})
