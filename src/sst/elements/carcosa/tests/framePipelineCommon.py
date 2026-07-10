"""Shared topology + oracle for the testFramePipeline*.py configs.

Builds carcosa.FramePipelineDriver <-> carcosa.CriticalActionWatcher wiring
(driver.cpu_side <-> watcher.highlink, watcher.lowlink <-> driver.mem_side)
plus a carcosa.ActionScorer, and computes the expected per-frame watcher
checksums in pure Python — an oracle independent of the C++ FNV/merge code
under test. Golden CSVs are written to a tempfile at config time.

Per frame f the driver produces (region base B, size 64):
  PREFILL : read [B, B+64)        -- must NOT be merged by the watcher
  ACTUATE : read [B+16, B+48)     -- fully in-region (corruptible)
            read [B-16, B+16)     -- straddles the region's lower edge
  stamp   : FrameRecord pushed with the watcher checksum
  POST    : read [B, B+4)         -- transition event, watcher finalizes

Expected snapshot: [0,16) straddle bytes, [16,48) in-region read bytes,
[48,64) never read during ACTUATE and must stay zero (this is what catches
a watcher that wrongly merges the PREFILL read).

The byte pattern mirrors FramePipelineDriver::patternByte — keep in sync.
"""

import os
import tempfile

import sst

# Force libmemHierarchy.so to load before libcarcosa.so (macOS flat-namespace
# RTTI lookup); the import fails but the dlopen side effect is what matters.
try:
    import sst.memHierarchy  # noqa: F401
except ModuleNotFoundError:
    pass

REGION_BASE = 0x2000
REGION_SIZE = 64
STATE_KEY = "frame_pipeline_test"

_FNV_OFFSET = 14695981039346656037
_FNV_PRIME = 1099511628211
_MASK64 = (1 << 64) - 1

# Read seeds, matching FramePipelineDriver::buildScript.
_SEED_ACTUATE_IN = 0x10
_SEED_STRADDLE = 0x50


def fnv1a64(data):
    h = _FNV_OFFSET
    for b in data:
        h ^= b
        h = (h * _FNV_PRIME) & _MASK64
    return h


def pattern_byte(seed, frame, addr):
    return (seed + 29 * frame + (addr & 0xFF)) & 0xFF


def expected_checksum(frame, corrupt=False):
    snap = bytearray(REGION_SIZE)
    for i in range(16):
        snap[i] = pattern_byte(_SEED_STRADDLE, frame, REGION_BASE + i)
    for j in range(32):
        snap[16 + j] = pattern_byte(_SEED_ACTUATE_IN, frame, REGION_BASE + 16 + j)
    if corrupt:
        snap[16] ^= 0x80  # first byte of the in-region ACTUATE read
    return fnv1a64(bytes(snap))


def _write_golden(rows):
    fd, path = tempfile.mkstemp(prefix="framePipelineGolden.", suffix=".csv")
    with os.fdopen(fd, "w") as f:
        has_tokens = bool(rows and len(rows[0]) == 4)
        f.write("pipeline_cycle,kernel_at_close,action_checksum%s\n" %
                (",action_token" if has_tokens else ""))
        for row in rows:
            f.write(",".join(str(v) for v in row) + "\n")
    return path


def build(frames=3, corrupt_frame=-1, close_kernel_id=1, golden_kernel_id=None,
          drop_golden_frames=(), expect_argmax_diff=0, expect_unsafe=0,
          expect_corrupted=0, verbose=False, extra_region=None,
          mem_side_gate=None, check_exact_checksums=True):
    """Wire up driver/watcher/scorer for one scenario.

    golden_kernel_id: kernel_at_close written into the golden CSV. Defaults
    to close_kernel_id; set differently to force the exact (cycle, kernel)
    lookup to miss so the cycle-only fallback path must fire in both the
    watcher and the scorer.
    drop_golden_frames: cycles omitted from the golden CSV (with
    golden_required=true the watcher must fatal -> mark the config
    # EXPECT_FAIL).
    extra_region: 'name:base:size' second region published by the driver
    (a decoy for the gate tests).
    mem_side_gate: params dict for a carcosa.PortModuleStateGate installed
    on the driver's mem_side port (Send direction corrupts responses before
    the watcher sees them).
    check_exact_checksums: pass expect_checksums to the driver. Disable for
    gate-flip runs where the corrupted values are RNG-dependent and only
    the classification counts are deterministic.
    """
    gk = close_kernel_id if golden_kernel_id is None else golden_kernel_id
    golden_rows = [(f, gk, expected_checksum(f))
                   for f in range(frames) if f not in drop_golden_frames]
    golden_path = _write_golden(golden_rows)

    # What the run should actually record (corruption is deterministic).
    actual = [expected_checksum(f, corrupt=(f == corrupt_frame))
              for f in range(frames)]

    driver = sst.Component("driver", "carcosa.FramePipelineDriver")
    driver.addParams({
        "state_key": STATE_KEY,
        "region_name": "action_queue",
        "region_base": REGION_BASE,
        "region_size": REGION_SIZE,
        "frames": frames,
        "corrupt_frame": corrupt_frame,
        "close_kernel_id": close_kernel_id,
        "expect_corrupted_frames": expect_corrupted,
        "verbose": "true" if verbose else "false",
    })
    if check_exact_checksums:
        driver.addParams({"expect_checksums": ",".join(str(c) for c in actual)})
    if extra_region:
        driver.addParams({"extra_region": extra_region})
    if mem_side_gate:
        driver.addPortModule("mem_side", "carcosa.PortModuleStateGate",
                             mem_side_gate)

    watcher = sst.Component("watcher", "carcosa.CriticalActionWatcher")
    watcher.addParams({
        "state_key": STATE_KEY,
        "critical_region": "action_queue",
        "critical_len": REGION_SIZE,
        "golden_log": golden_path,
        "golden_required": "true",
        "verbose": "true" if verbose else "false",
    })

    scorer = sst.Component("scorer", "carcosa.ActionScorer")
    scorer.addParams({
        "state_key": STATE_KEY,
        "golden_log": golden_path,
        "golden_required": "true",
        "expect_frames_total": frames,
        "expect_frames_dropped": 0,
        "expect_frames_argmax_diff": expect_argmax_diff,
        "expect_frames_unsafe": expect_unsafe,
    })

    sst.Link("driver_watcher_high").connect(
        (driver, "cpu_side", "1ns"), (watcher, "highlink", "1ns"))
    sst.Link("watcher_driver_low").connect(
        (watcher, "lowlink", "1ns"), (driver, "mem_side", "1ns"))

    return driver, watcher, scorer


def build_scorer_case(frame_tokens, golden_tokens, dropped_frames=(),
                      escape_frames=(), corrupt_frame=-1, **expected):
    """Focused ActionScorer taxonomy/token fixture."""
    frames = len(frame_tokens)
    golden_path = _write_golden([
        (f, 1, expected_checksum(f), golden_tokens[f]) for f in range(frames)
    ])
    actual = [expected_checksum(f, corrupt=(f == corrupt_frame))
              for f in range(frames)]

    driver = sst.Component("driver", "carcosa.FramePipelineDriver")
    driver.addParams({
        "state_key": STATE_KEY, "frames": frames, "region_name": "action_queue",
        "region_base": REGION_BASE, "region_size": REGION_SIZE,
        "corrupt_frame": corrupt_frame, "close_kernel_id": 1,
        "expect_checksums": ",".join(str(v) for v in actual),
        "frame_tokens": ",".join(str(v) for v in frame_tokens),
        "dropped_frames": ",".join(str(v) for v in dropped_frames),
        "escape_frames": ",".join(str(v) for v in escape_frames),
    })
    watcher = sst.Component("watcher", "carcosa.CriticalActionWatcher")
    watcher.addParams({"state_key": STATE_KEY, "critical_region": "action_queue",
                       "critical_len": REGION_SIZE, "golden_log": golden_path,
                       "golden_required": "true"})
    scorer = sst.Component("scorer", "carcosa.ActionScorer")
    params = {"state_key": STATE_KEY, "golden_log": golden_path,
              "golden_required": "true", "expect_frames_total": frames}
    params.update(expected)
    scorer.addParams(params)
    sst.Link("driver_watcher_high").connect(
        (driver, "cpu_side", "1ns"), (watcher, "highlink", "1ns"))
    sst.Link("watcher_driver_low").connect(
        (watcher, "lowlink", "1ns"), (driver, "mem_side", "1ns"))
    return driver, watcher, scorer
