"""Cycle-only golden fallback: the golden CSV's kernel_at_close (9) never
matches the recorded frames (close_kernel_id=1), so the exact
(cycle, kernel) lookup misses for every frame and both the watcher and the
scorer must fall back to the unambiguous cycle-only entry. With
golden_required=true, a broken fallback fatals (unmatched frames); the
corrupt frame additionally checks the fallback comparison still flags
divergence.

Run: sst testFramePipelineFallback.py
"""
import framePipelineCommon as common

common.build(corrupt_frame=1, golden_kernel_id=9, expect_argmax_diff=1,
             expect_unsafe=1, expect_corrupted=1)
