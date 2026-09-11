"""Fault-free frame pipeline: watcher checksums must match the Python-computed
golden exactly (verifies lowlink response observation, the straddling
partial-overlap merge, and that PREFILL traffic is not snapshotted), no frame
may classify corrupted, and the scorer must report zero divergence.

Run: sst testFramePipelineClean.py
"""
import framePipelineCommon as common

common.build()
