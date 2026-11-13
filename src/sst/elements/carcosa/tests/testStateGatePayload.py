"""Verify flip eligibility and delivery for generic and memory events."""

import sst

try:
    import sst.memHierarchy  # noqa: F401
except ModuleNotFoundError:
    pass

test = sst.Component("state_gate_payload", "carcosa.StateGateRegressionTest")
test.addParam("suite", "payload")
