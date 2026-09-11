"""Golden CSV lacking frame 2's row with golden_required=true: the watcher
must fatal when it finalizes the uncovered frame instead of silently
skipping classification.

# EXPECT_FAIL — the run must exit nonzero.

Run: sst testFramePipelineMissingGolden.py
"""
import framePipelineCommon as common

common.build(drop_golden_frames=(2,))
