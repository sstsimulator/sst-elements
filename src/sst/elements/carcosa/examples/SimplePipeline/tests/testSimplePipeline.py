"""
Minimal SST config for SimplePipeline example.

Demonstrates PipelineStateRegistry rendezvous:
  - SimplePipelineProducer advances a 4-stage FSM (FETCH, DECODE, EXECUTE,
    COMMIT) and publishes the current stage + pipeline cycle into the registry
    under `state_key`.
  - SimpleStageGate (a PortModule installed on the sink's "in" port) reads the
    same key and cancels delivery of events whose stage id is in drop_stages.
  - SimplePipelineSink counts per-stage deliveries and prints the histogram in
    finish(), so you can observe that the gated stages show zero arrivals.

Run:
  sst testSimplePipeline.py

Expected behavior with drop_stages="1,3" (DECODE and COMMIT):
  - FETCH and EXECUTE counts equal total_cycles
  - DECODE and COMMIT counts are zero
"""

import sst

# libCarcosa.so has link-time references to SST::MemHierarchy symbols
# (CarcosaMemCtrl pulls in SimpleMemBackendConvertor, etc.). On macOS's
# two-level/flat dlopen semantics those symbols must already be in the
# process image when libCarcosa.so is loaded, or dlopen fails with
# "symbol not found in flat namespace". Importing sst.memHierarchy runs
# SST's Python import hook (pymodel.cc::mlFindModule), which calls
# Factory::hasLibrary -> findLibrary and dlopens libmemHierarchy.so now,
# before Carcosa is touched. Every other cross-element test in the tree
# gets this for free because they instantiate memHierarchy.* components
# first; this example doesn't use memHierarchy, so we force it explicitly.
# Note: we don't actually use any Python-side memHierarchy symbols; we just
# need SST's import hook to dlopen libmemHierarchy.so. The import itself will
# raise ModuleNotFoundError because memHierarchy exposes no Python module,
# but Factory::findLibrary has already been called by that point, so the
# shared object is loaded into the process and its symbols are available
# when libCarcosa.so is loaded next.
try:
    import sst.memHierarchy  # noqa: F401
except ModuleNotFoundError:
    pass

sst.setProgramOption("timebase", "1ps")
sst.setProgramOption("stop-at", "0 ns")

STATE_KEY    = "pipe0"
TOTAL_CYCLES = 4

producer = sst.Component("producer", "carcosa.SimplePipelineProducer")
producer.addParams({
    "state_key":    STATE_KEY,
    "total_cycles": TOTAL_CYCLES,
    "clock":        "1MHz",
    "verbose":      "true",
})

sink = sst.Component("sink", "carcosa.SimplePipelineSink")
sink.addParams({
    "verbose": "true",
})

# PortModule on the sink's receive side: reads the same registry key that
# `producer` publishes under, and cancels DECODE (1) and COMMIT (3) events.
sink.addPortModule("in", "carcosa.SimpleStageGate", {
    "state_key":   STATE_KEY,
    "drop_stages": "1,3",
    "verbose":     "true",
})

link = sst.Link("producer_to_sink")
link.connect((producer, "out", "1us"),
             (sink,     "in",  "1us"))
