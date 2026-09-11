"""
Exercise the generic PortModuleStateGate against SimplePipelineProducer.

SimplePipelineProducer writes its FSM state into PipelineStateRegistry under
`state_key`, stashing the current stage id in PipelineStateBase.currentKernel
and advancing PipelineStateBase.pipelineCycle once per full pass:

  stage 0 -> FETCH
  stage 1 -> DECODE
  stage 2 -> EXECUTE
  stage 3 -> COMMIT

PortModuleStateGate on the sink's "in" port is configured with
`kernels="1,3"` + `fault_mode="drop"` + `drop_probability=1.0`, so any event
it intercepts while the producer's registered currentKernel is DECODE or
COMMIT is dropped. This is the same behavior SimpleStageGate demonstrates,
but routed through the generic predicate machinery.

NOTE on timing: because the producer->sink link has a 1us delay, the gate
sees the *next* registry snapshot at the moment each event arrives (producer
has already advanced one stage). The histogram you'll see below therefore
reflects "gated on stage-at-arrival", not "gated on stage-at-send":

  tick t:   producer sends stage X;  registry says currentKernel=X
  tick t+1: event arrives at gate;   registry says currentKernel=(X+1) mod 4

With drop_stages={DECODE=1, COMMIT=3}, the gate drops events whose arrival-
time stage is in {1,3} -- i.e. the events that were *emitted* during FETCH
and EXECUTE. Events emitted during DECODE/COMMIT arrive when the producer
is at EXECUTE/FETCH, which are not in the drop set, and go through.
Expected histogram: FETCH=0 DECODE=N EXECUTE=0 COMMIT=N-1 (last COMMIT never
arrives because the producer exits on reaching total_cycles).

Run:
  sst testPortModuleStateGate.py
"""

import sst

# See testSimplePipeline.py for rationale: side-effect import so SST's
# factory dlopens libmemHierarchy.so before libCarcosa.so. The import
# itself raises ModuleNotFoundError (memHierarchy has no Python module),
# but by then the shared object is already resident in the process.
try:
    import sst.memHierarchy  # noqa: F401
except ModuleNotFoundError:
    pass

sst.setProgramOption("timebase", "1ps")
sst.setProgramOption("stop-at", "0 ns")

STATE_KEY    = "gate_test"
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

# Generic state-gated drop injector.
#   - state_key:          same key the producer publishes into.
#   - kernels="1,3":      match when registered currentKernel is DECODE or COMMIT.
#   - fault_mode=drop:    cancel delivery on match.
#   - drop_probability=1: every matched event is dropped (deterministic).
sink.addPortModule("in", "carcosa.PortModuleStateGate", {
    "state_key":         STATE_KEY,
    "fault_mode":        "drop",
    "drop_probability":  "1.0",
    "kernels":           "1,3",
    "verbose":           "1",
    "debug":             "1",
    "debug_level":       "2",
})

link = sst.Link("producer_to_sink")
link.connect((producer, "out", "1us"),
             (sink,     "in",  "1us"))
