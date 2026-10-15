"""Verify cached-line and byte-addressed region predicates."""

import sst

try:
    import sst.memHierarchy  # noqa: F401
except ModuleNotFoundError:
    pass

test = sst.Component("state_gate_regions", "carcosa.StateGateRegressionTest")
test.addParam("suite", "regions")
