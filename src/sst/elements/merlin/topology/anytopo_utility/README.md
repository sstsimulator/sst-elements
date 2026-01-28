# AnyTopo Utility

This directory contains utility scripts for generating input graphs for the `anytopo` topology in SST Merlin. Popular High-Performance Computing (HPC) network topologies are implemented as Python classes.

**Note:** This does not need to be compiled with SST. It is a standalone Python codebase.

## Overview

The AnyTopo utility provides implementations of several state-of-the-art HPC network topologies, enabling users to generate network graphs for SST simulations. Each topology is characterized by the number of routers (R) and the inter-router graph degree (d). (Note that in the context of HPC, `network switch` means the same as `router` in SST.Merlin)

## Implemented Topologies

### 1. **Dally Dragonfly** (`DallyDragonfly.py`)
Implementation of the Dragonfly topology as described in:
> Kim, John, et al. "Technology-driven, highly-scalable dragonfly topology." ACM SIGARCH Computer Architecture News 36.3 (2008): 77-88.

**Key Parameters:**
- `a` = Number of routers per group
- `g` = Number of groups
- `h` = Number of inter-group links per router
- `p` = Number of terminals per router (not modeled in graph)
- Router radix: `k = p + a + h - 1`
- Constraints: `a = 2p = 2h`, `g = ah + 1`

**Usage:**
```python
from DallyDragonfly import DallyDragonflyTopo

# Create a Dally Dragonfly topology with R routers and graph degree d
topo = DallyDragonflyTopo(R, d)
```

### 2. **Slim Fly** (`Slimfly.py`)
Implementation of the Slim Fly topology as described in:
> Besta, Maciej, and Torsten Hoefler. "Slim fly: A cost effective low-diameter network topology." SC'14: proceedings of the international conference for high performance computing, networking, storage and analysis. IEEE, 2014.

**Key Parameters:**
- Based on MMS (Maximally-Minimal Set) graphs
- Number of vertices: `n = 2q²` where q is a prime power
- Degree: `d = (3q + δ)/2` where δ ∈ {-1, 0, 1}

**Usage:**
```python
from Slimfly import SlimflyTopo

# Create a Slim Fly topology with num_vertices routers and specified degree
topo = SlimflyTopo(num_vertices, degree)
```

**Requirements:**
- Requires `galois` library: `pip install galois`

### 3. **PolarFly** (`Polarfly.py`)
Implementation of the PolarFly topology as described in:
> Lakhotia, Kartik, et al. "PolarFly: a cost-effective and flexible low-diameter topology." SC22: International Conference for High Performance Computing, Networking, Storage and Analysis. IEEE, 2022.

**Key Parameters:**
- `q` = Prime power
- Number of routers: `n = q² + q + 1`
- Degree: `d = q + 1`
- Based on projective geometry PG(2,q)

**Usage:**
```python
from Polarfly import PolarflyTopo

# Option 1: Specify q directly
topo = PolarflyTopo(q, r2, r1)  # r2 and r1 must be 0 (extended versions not implemented)

# Option 2: Specify number of routers and degree
topo = PolarflyTopo(num_routers, degree)
```

**Note:** Only non-extended PolarFly is currently implemented.

### 4. **Jellyfish** (`Jellyfish.py`)
Implementation of the Jellyfish (Random Regular Graph) topology as described in:
> Singla, Ankit, et al. "Jellyfish: Networking data centers randomly." 9th USENIX Symposium on Networked Systems Design and Implementation (NSDI 12). 2012.

**Usage:**
```python
from Jellyfish import JellyfishTopo

# Option 1: Generate random regular graph with default seed (0)
topo = JellyfishTopo(num_vertices, degree)

# Option 2: Specify custom seed for reproducibility
topo = JellyfishTopo(num_vertices, degree, seed)

# Option 3: Import from edge list
edge_list = [(0, 1), (1, 2), ...]
topo = JellyfishTopo(edge_list)
```

## Base Class: `HPC_topo` (`HPC_topos.py`)

All topology classes inherit from the abstract base class `HPC_topo`, which provides:

**Common Methods:**
- `get_nx_graph()` - Returns the NetworkX graph representation
- `get_topo_name()` - Returns the topology name
- `calculate_all_shortest_paths_routing_table()` - Calculates all-shortest-paths routing using NetworkX

**Routing Table Format:**
The routing table is a nested dictionary structure:
```python
routing_table[source_router][dest_router] = [(weight, path), ...]
```
- `weight`: Normalized weight for this path (sum of all weights between a pair of source and destination router = 1.0)
- `path`: List of router IDs representing the route

**Pre-calculated Configurations:**
The `HPC_topos.py` file includes pre-calculated valid configurations for:
- Slim Fly: `sf_configs`
- Dally Dragonfly: `ddf_configs`
- PolarFly (all): `pf_configs`
- PolarFly (regular only): `pf_regular_configs`

## Dependencies

Install dependencies:
```bash
pip install networkx numpy
pip install galois  # For Slim Fly
```