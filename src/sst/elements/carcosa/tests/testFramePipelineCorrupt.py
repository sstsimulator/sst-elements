"""One bit flipped in frame 1's in-region ACTUATE payload: the watcher must
classify exactly that frame corrupted against its golden log, and the scorer
must flag exactly one argmax divergence / unsafe frame.

Run: sst testFramePipelineCorrupt.py
"""
import framePipelineCommon as common

common.build(corrupt_frame=1, expect_argmax_diff=1, expect_unsafe=1,
             expect_corrupted=1)
