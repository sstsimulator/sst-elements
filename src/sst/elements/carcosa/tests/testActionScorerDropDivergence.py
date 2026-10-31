from framePipelineCommon import build_scorer_case

# Divergence takes O3 precedence even when the same frame is also dropped.
build_scorer_case(
    frame_tokens=[999], golden_tokens=[100], dropped_frames=[0],
    expect_frames_dropped=1, expect_frames_argmax_diff=0,
    expect_frames_action_diff=1, expect_frames_unsafe=1,
    expect_frames_o1=0, expect_frames_o2=0,
    expect_frames_o3=1, expect_frames_o4=0)
