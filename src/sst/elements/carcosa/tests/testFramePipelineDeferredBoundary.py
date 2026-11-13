from framePipelineCommon import build

# No POST traffic: the next cycle's PREFILL is the first observed event
# after ACTUATE. The watcher must use the previous cycle's golden checksum.
build(frames=3, post_reads=False, expect_corrupted=0)
