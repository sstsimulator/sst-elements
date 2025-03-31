# Example MemHierarchy Configuration

This directory contains example configurations for different memory hierarchies.
Most examples use memHierarchy's "standardCPU" model for the processor. These can be swapped for any CPU model that implements SST::Interfaces::StandardMem.
Likewise, most examples use memHierarchy's "SimpleMem" memory timing model for DRAM timing; this can be swapped for any available backend.

## Example List
**Single core, various number of cache levels**
* example-1core-nocache.py : Single core connected to a single memory; no caches
* example-1core-1level.py  : Single core with one L1 cache and one memory
* example-1core-3level.py  : Single core with a three-level cache hierarchy (L1, L2, L3) and one memory

**Multicore, inclusive cache hierarchies**
* example-4core-shared-llc.py : Four cores with private L1s and a shared L2; one memory
* example-4core-shared-distributed-llc.py : Same as 'example-4core-shared-llc' except that the shared L2 is distributed across 4 slices

