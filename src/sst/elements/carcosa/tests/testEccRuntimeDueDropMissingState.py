# EXPECT_FAIL: a DUE must fail if its abort target has never been published.
from eccRuntimeCommon import build

build({"state_key": "unpublished_abort_target",
       "fault_model": "campaign", "ecc_scheme": "secded",
       "campaign_event_budget": 1, "campaign_event_rate": 1,
       "campaign_mode": "cell", "campaign_errors_fixed": 2,
       "due_action": "drop_frame"})
