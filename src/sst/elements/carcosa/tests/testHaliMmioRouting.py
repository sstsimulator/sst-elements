"""Discover Hali's MMIO region through a cache and a bus shared with RAM.

The value read parks until the adjacent arm register is written. Both must
reach Hali as noncacheable accesses; a missing MMIO advertisement deadlocks
behind the cache's outstanding line fill. The driver asserts completion.
"""
import sst

sst.setProgramOption("stop-at", "2us")
base = 4096
driver = sst.Component("driver", "carcosa.ExampleMmioDriver")
driver.addParams({"clock": "1GHz", "mmio_base": base, "arm_value": 43981})
iface = driver.setSubComponent("mem_iface", "memHierarchy.standardInterface")

cache = sst.Component("cache", "memHierarchy.Cache")
cache.addParams({
    "cache_frequency": "1GHz", "access_latency_cycles": 1,
    "cache_size": "2KiB", "cache_line_size": 64, "associativity": 2,
    "coherence_protocol": "MSI", "replacement_policy": "lru", "L1": 1,
})
bus = sst.Component("bus", "memHierarchy.Bus")
bus.addParams({"bus_frequency": "1GHz"})

memory = sst.Component("memory", "memHierarchy.MemController")
memory.addParams({"clock": "1GHz", "addr_range_end": base - 1})
backend = memory.setSubComponent("backend", "memHierarchy.simpleMem")
backend.addParams({"access_time": "10ns", "mem_size": "4KiB"})

hali = sst.Component("hali", "carcosa.Hali")
hali.addParams({"mmio_base": base, "mmio_size": 8})
hali.setSubComponent("interceptionAgent", "carcosa.ExampleControlAgent")
mmio = hali.setSubComponent("mmio_iface", "memHierarchy.standardInterface")

sst.Link("driver_cache").connect((iface, "lowlink", "1ns"), (cache, "highlink", "1ns"))
sst.Link("cache_bus").connect((cache, "lowlink", "1ns"), (bus, "highlink0", "1ns"))
sst.Link("bus_memory").connect((bus, "lowlink0", "1ns"), (memory, "highlink", "1ns"))
sst.Link("bus_hali").connect((bus, "lowlink1", "1ns"), (mmio, "lowlink", "1ns"))
