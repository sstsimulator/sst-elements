from framePipelineCommon import build_scorer_case

# O1 clean; O2 dropped/correct; O3 token mismatch; O4 escape with matching
# token. Frame 3's checksum is corrupt on purpose: token presence must make
# the matching decoded action authoritative.
build_scorer_case(
    frame_tokens=[101, 102, 999, 104], golden_tokens=[101, 102, 103, 104],
    dropped_frames=[1], escape_frames=[3], corrupt_frame=3,
    expect_frames_dropped=1, expect_frames_argmax_diff=1,
    expect_frames_action_diff=1, expect_frames_unsafe=1,
    expect_frames_o1=1, expect_frames_o2=1,
    expect_frames_o3=1, expect_frames_o4=1)
