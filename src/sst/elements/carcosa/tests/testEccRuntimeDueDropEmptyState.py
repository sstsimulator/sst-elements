# EXPECT_FAIL: drop_frame requires a state_key even before the first DUE.
from eccRuntimeCommon import build

build({"state_key": "", "due_action": "drop_frame"})
