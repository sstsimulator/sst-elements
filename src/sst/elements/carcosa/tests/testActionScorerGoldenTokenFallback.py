from framePipelineCommon import build_scorer_case

# A zero golden token is unpublished, just like a zero frame token. Compare
# checksums for both the clean frame and the corrupted frame.
build_scorer_case(
    frame_tokens=[77, 88], golden_tokens=[0, 0], corrupt_frame=1,
    expect_frames_dropped=0, expect_frames_argmax_diff=1,
    expect_frames_action_diff=0, expect_frames_unsafe=1,
    expect_frames_o1=1, expect_frames_o2=0,
    expect_frames_o3=1, expect_frames_o4=0)
