"""Unaligned cached requests return full lines with original virtual metadata.

The corrupt byte is at the line start, before the request address: shifting
the response payload to the request address must fail exact checksum checks.
Watching requests as well also verifies that inspecting an empty read does
not allocate a synthetic payload.
"""
import framePipelineCommon as common

common.build(cached_responses=True, virtual_address_offset=0x1000,
             watcher_responses_only=False, corrupt_frame=1,
             expect_argmax_diff=1, expect_unsafe=1, expect_corrupted=1)
