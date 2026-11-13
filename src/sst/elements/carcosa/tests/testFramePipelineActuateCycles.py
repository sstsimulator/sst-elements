from framePipelineCommon import build

# With no intervening PREFILL/POST traffic, consecutive observed kernels are
# both ACTUATE. Finalize the corrupt first frame before merging the next one.
build(frames=3, prefill_reads=False, post_reads=False, corrupt_frame=0,
      expect_argmax_diff=1, expect_unsafe=1, expect_corrupted=1)
