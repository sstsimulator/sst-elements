#!/usr/bin/env python3
"""Run real CarcosaCPU traffic through Hali, an L1, and simpleMem.

Usage: python3 testCarcosaTrafficStandalone.py --sst /path/to/sst
       [--lib-path /path/to/elements/libraries]
"""
import argparse
import csv
from pathlib import Path
import re
import subprocess
import tempfile

OPERATIONS = 4096
MEMORY_BYTES = 16384
CONFIG = '''import sst
# Construct memHierarchy first so Carcosa's shared event types are registered.
cache = sst.Component("cache", "memHierarchy.Cache")
cache.addParams({{"cache_frequency": "1GHz", "access_latency_cycles": 1,
    "cache_size": "2KiB", "cache_line_size": 64, "associativity": 2,
    "coherence_protocol": "MSI", "replacement_policy": "lru", "L1": 1}})
hali = sst.Component("hali", "carcosa.Hali")
cpu = sst.Component("cpu", "carcosa.CarcosaCPU")
cpu.addParams({{"clock": "1GHz", "memFreq": 1, "memSize": "16KiB",
    "opCount": {operations}, "rngseed": 101, "verbose": 2,
    "maxOutstanding": 16, "reqsPerIssue": 4,
    "noncacheableRangeStart": {noncacheable_start}, "noncacheableRangeEnd": {noncacheable_end}}})
cpu.addParams({frequencies!r})
iface = cpu.setSubComponent("memory", "memHierarchy.standardInterface")
memory = sst.Component("memory", "memHierarchy.MemController")
memory.addParams({{"clock": "1GHz", "addr_range_end": 16383}})
backend = memory.setSubComponent("backend", "memHierarchy.simpleMem")
backend.addParams({{"access_time": "10ns", "mem_size": "16KiB"}})
sst.Link("cpu_control").connect((cpu, "haliToCPU", "100ps"), (hali, "cpu", "100ps"))
sst.Link("cpu_hali").connect((iface, "lowlink", "100ps"), (hali, "highlink", "100ps"))
sst.Link("hali_cache").connect((hali, "lowlink", "100ps"), (cache, "highlink", "100ps"))
sst.Link("cache_memory").connect((cache, "lowlink", "100ps"), (memory, "highlink", "100ps"))
sst.setStatisticLoadLevel(1)
sst.setStatisticOutput("sst.statOutputCSV", {{"filepath": "stats.csv"}})
cpu.enableStatistics({statistics!r},
    {{"type": "sst.AccumulatorStatistic", "rate": "0ns"}})
sst.setProgramOption("stop-at", "500us")
'''


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def run_simulation(args, root, name, frequencies, operations, noncacheable_range, statistics):
    directory = root / name
    directory.mkdir()
    config = directory / "traffic.py"
    config.write_text(CONFIG.format(operations=operations, frequencies=frequencies,
                                   noncacheable_start=noncacheable_range[0],
                                   noncacheable_end=noncacheable_range[1], statistics=statistics))
    command = [args.sst]
    if args.lib_path:
        command.append("--lib-path=" + args.lib_path)
    command.append(str(config))
    result = subprocess.run(command, cwd=directory, capture_output=True, text=True, timeout=60)
    output = result.stdout + result.stderr
    (directory / "sst.log").write_text(output)
    require(result.returncode == 0, name + ": SST failed\n" + output[-6000:])
    require("Test Completed Successfully" in output, name + ": CPU did not finish all requests")
    counts = {}
    with (directory / "stats.csv").open(newline="") as stream:
        for row in csv.DictReader(stream, skipinitialspace=True):
            row = {key.strip(): value.strip() for key, value in row.items()}
            if row["ComponentName"] == "cpu":
                statistic = row["StatisticName"]
                require(statistic not in counts, name + ": duplicate final statistic " + statistic)
                counts[statistic] = int(row["Sum.u64"])
    for statistic in statistics:
        require(statistic in counts, name + ": missing statistic " + statistic)
    return counts, output


def run_case(args, root, name, frequencies):
    counts, output = run_simulation(args, root, name, frequencies, OPERATIONS,
                                    (8192, MEMORY_BYTES),
                                    ["reads", "writes", "readNoncache", "writeNoncache"])
    require(counts["reads"] + counts["writes"] == OPERATIONS, name + ": wrong operation count")
    # The upper half is noncacheable, so its counters prove the CPU uses more
    # than the old hard-coded 200-byte range even in builds without debug logs.
    require(counts["readNoncache"] + counts["writeNoncache"] > 0,
            name + ": no requests reached the upper half of configured memory")
    addresses = [int(value, 16) for value in re.findall(
        r"Issued (?:Noncacheable )?(?:Read|Write) for address 0x([0-9a-fA-F]+)", output)]
    if addresses:
        require(all(address % 4 == 0 and address + 4 <= MEMORY_BYTES for address in addresses),
                name + ": unaligned or out-of-range address")
        require(max(addresses) >= MEMORY_BYTES // 2, name + ": logged addresses stayed in the old range")
    return counts, max(addresses) if addresses else None


def run_cacheable_case(args, root, name, frequency, statistic, noncacheable_range):
    operations = 256
    counts, output = run_simulation(args, root, name,
                                    {"read_freq": 0, "write_freq": 0, frequency: 100},
                                    operations, noncacheable_range, [statistic])
    is_llsc = frequency == "llsc_freq"
    expected = operations // 2 if is_llsc else operations
    require(counts[statistic] == expected, name + ": wrong operation count")
    # These helpers log through verbose(), independently of debug build flags.
    addresses = [int(value, 16) for value in re.findall(
        r"Issued (?:FlushAddrInv|FlushAddr|LoadLink|StoreConditional) for address 0x([0-9a-fA-F]+)",
        output)]
    require(len(addresses) == operations, name + ": missing issued addresses")
    size = 4 if is_llsc else 64
    start, end = noncacheable_range
    require(all(address % size == 0 and address + size <= MEMORY_BYTES for address in addresses),
            name + ": unaligned or out-of-range address")
    require(all(address + size <= start or address >= end for address in addresses),
            name + ": request overlaps noncacheable memory")
    if start:
        require(min(addresses) < start, name + ": no requests below noncacheable memory")
    if end < MEMORY_BYTES:
        # Also catches remapping only samples below 'end', which leaves the
        # top of memory unreachable when the noncacheable interval is narrow.
        require(max(addresses) >= max(end, MEMORY_BYTES - (end - start)),
                name + ": no requests reached the top of cacheable memory")
    print("PASS %s: %s=%d max_address=0x%x" % (name, statistic, counts[statistic], max(addresses)))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sst", default="sst", help="SST executable")
    parser.add_argument("--lib-path", help="SST element library search path (colon-separated if needed)")
    args = parser.parse_args()
    # Resolve an explicitly supplied executable before changing working directory.
    if "/" in args.sst:
        args.sst = str(Path(args.sst).resolve())
    if args.lib_path:
        args.lib_path = ":".join(str(Path(path).expanduser().resolve())
                                 for path in args.lib_path.split(":") if path)
    cases = {"read-only": {"read_freq": 100, "write_freq": 0},
             "write-only": {"read_freq": 0, "write_freq": 100},
             "defaults": {}, "explicit-75-25": {"read_freq": 75, "write_freq": 25}}
    with tempfile.TemporaryDirectory(prefix="carcosa-traffic-") as temporary:
        results = {name: run_case(args, Path(temporary), name, frequencies)
                   for name, frequencies in cases.items()}
        ranges = {"middle-wide": (4096, 12288), "middle-narrow": (4096, 6144),
                  "lower-half": (0, 8192), "upper-half": (8192, MEMORY_BYTES)}
        for range_name, interval in ranges.items():
            for frequency, statistic in (("flush_freq", "flushes"),
                                         ("flushinv_freq", "flushinvs"), ("llsc_freq", "llsc")):
                run_cacheable_case(args, Path(temporary), range_name + "-" + frequency,
                                   frequency, statistic, interval)
    require(results["read-only"][0]["reads"] == OPERATIONS, "read-only configuration issued writes")
    require(results["write-only"][0]["writes"] == OPERATIONS, "write-only configuration issued reads")
    defaults = results["defaults"][0]
    require(defaults == results["explicit-75-25"][0], "defaults differ from documented 75/25 frequencies")
    require(0.70 < defaults["reads"] / OPERATIONS < 0.80, "75/25 configuration produced the wrong mix")
    for name, (counts, maximum) in results.items():
        address = "debug addresses unavailable" if maximum is None else "max_address=0x%x" % maximum
        print("PASS %s: reads=%d writes=%d upper_half=%d %s" % (
            name, counts["reads"], counts["writes"],
            counts["readNoncache"] + counts["writeNoncache"], address))


if __name__ == "__main__":
    main()
