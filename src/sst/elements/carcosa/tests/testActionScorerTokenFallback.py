from framePipelineCommon import build_scorer_case

# Token 0 means absent. Despite a token in the golden row, scoring must fall
# back to the mismatching checksum and classify the frame O3/unsafe.
build_scorer_case(
    frame_tokens=[0], golden_tokens=[77], corrupt_frame=0,
    expect_frames_dropped=0, expect_frames_argmax_diff=1,
    expect_frames_action_diff=0, expect_frames_unsafe=1,
    expect_frames_o1=0, expect_frames_o2=0,
    expect_frames_o3=1, expect_frames_o4=0)
