# Ember Python Configurations

The Python file in Ember specifies:
- How to set up the SST simulation
- Allows multiple configurations to be specified and run
- Loads Motifs into the simulation to be performed.

# Running Ember

Ember uses a python interpreter built into the SST compiler.
`sst python-file.py` executes the python script.

# Key Components

The python file specifies the format of the SST simulation. The python file specifies components of our simulation such as network Topology, Routers, NetworkInterface and the computation to be performed in the form of Motifs.

First, we import the necessary components of merlin and set the Platform to firefly defaults.

    import sst
    from sst.merlin.base import *
    from sst.merlin.endpoint import *
    from sst.merlin.interface import *
    from sst.merlin.topology import *

    from sst.ember import *

    if __name__ == "__main__":

        PlatformDefinition.setCurrentPlatform("firefly-defaults")

Next, we create a topology that our experiment will run on.

`topo = topoDragonFly()`
Creates a dragonfly topology.
We can then specify parameters about the topology using a dot-operator. e.g.,
`topo.num_groups = 4`

# Topologies

The python file allows the user to specify comparable hardware configurations.
Four different topologies can be specified with different routing algorithms.
Here we give a comprehensive list of the topologies and how they can be initialized.

1.  HyperX `topoHyperX`
    *   `shape`
        Specifies the shape of the mesh. i.e. topo.shape=4x4 or topo.shape=4x4x4. Any number of dimensions is supported.
    *   `width`
        The number of links between routers in each dimension is specified in the same manner as shape.
    *   `local_ports`
        Number of endpoints attached to each router
    *   `algorithm`
        Routing algorithm to use.
        *   `DOAL`
        *   `DOR`
        *   `DOR-ND`
        *   `MIN-A`
        *   `VDAL`
        *   `valiant`
2.  Torus `topoTorus`
    *   Shape
        Specifies the mesh grid size, i.e., topo.shape=4x4 or topo.shape=4x4x4. Any number of dimensions is supported.
    *   Width
        Number of links between routers in each dimension, specified in the same way as shape.
    *   local\_ports
        Number of endpoints attached to each router
3. Fat Tree `topoFatTree`
    *   `shape`
        The shape of the fat tree. Specified as pair of downlinks and uplinks per level separated by a colon. i.e., topo.shape="4,2:3,5" specifies the first level has 4 down and 2 up, and the following level has 3 down and 5 up.
        If only one number is given, it is interpreted as the number of downlinks and up is set to 0.
    *   `routing_alg` Routing algorithm to use
        * `deterministic` (default)
        * `adaptive`
    *   `adaptive threshold.`
        The threshold is used to determine if a packet will adaptively route.
4. Dragonfly `topoDragonFly`

    *   `hosts_per_router`
        Number of hosts connected to each router
    *   `routers_per_group`
        The number of links used to connect to routers in the same group.
    *   `intergroup_per_router`
        The number of links per router connected to other groups.
    *   `intergroup_links`
        The number of links between each pair of groups.
    *   `num_groups`
        Number of groups in a network
    *   `intergroup_links`
    *   `algorithm`
        Specifies how each virtual network routes messages.
        Specified for each router.vn. i.e., if router.vn=2, than topo.algorithm=\["minimal", "adaptive-local"], the first router uses minimal, and the second will use adaptive-local.
        * `minimal` (default)\
        * `adaptive_local`
        * `ugal`
        * `min-a`
        * `valiant`
        Valiant only operates if there are more than two num\_groups. Otherwise, there is no point in using valiant routing.
    *   `adaptive_threshold`
        Threshold when making adaptive routing decisions.
    *   `global_link_map`
        An array specifying connectivity of global links in each dragonfly group
    *   `global_route_mode`
        Mode for interpreting global link map
        *   `absolute` (default)
        *   `relative`

# Creating a Router

Once the router has been created, the topology needs to be linked to the router. Additionally, the link\_latency can be set in the topology at this point.

    topo.router = router
    topo.link_latency = "20ns"

Parameters for a high radix router or  `hr_routers`:
* `id`
The ID of the router
* `num_ports`
Number of ports the router has
* `topology`
Name of the topology subcomponent loaded to control routing
* `xbar_arb`
Arbitration unit used for the crossbar. Default is `merlin.xbar_arb_lru`.
* `merlin.xbar_arb_lru`
Uses Least recently used arbitration for `hr_router.`
* `merlin.xbar_arb_age`
Age-based arbitration unit for `hr_router`
* `merlin.xbar_arb_lru`
Least recently used arbitration unit with infinite crossbar for `hr_router`
* `merlin.xbar_arb_rand`
Random arbitration unit for `hr_router`
*  `merlin.xbar_arb_rr`
Round-robin arbitration unit for `hr_router`
* `link_bw`
Bandwidth of the links specified in either b/s or B/s (can include SI prefix)
* `flit_size`
Flit size specified in either b or B
* `xbar_bw`
Bandwidth of the crossbar specified in either b/s or B/s
* `input_latency`
Latency of packets entering switch into input buffers. Specified in s.
* `output_latency`
Latency of packets exiting switch from output buffers. Specified in s.
* `network_inspectors`
Comma-separated list of network inspectors to put on output ports
* `oql_track_port`
Set to true track output queue length for an entire port. False tracks per VC (default).

# Creating Network Interface

Create the network interface:

    networkif = ReorderLinkControl()

The ReorderLinkControl() creates a network interface that can handle out-of-order packet arrival. Events are sequenced, and order is reconstructed on receive.

# Specifying Computation

Initialize the MPIJob to use all the nodes in our topology:

`ep = EmberMPIJob(0, topo.getNumNodes())`
The network interface then needs to be linked to the `ep` variable
`ep.network_interface = networkif()`
Then a series of Motifs can be queued for computation.
```
    ep.addMotif("Init")
    ep.addMotif("Allreduce")
    ep.addMotif("Fini")
```
The `addMotif` function adds the specified Motif to a queue. The Motif is named through an SST\_ELI\_REGISTER\_SUBCOMPONENT\_DERIVED command in the C++ Motif definition (usually in the include file). The SST\_ELI\_REGISTER\_SUBCOMPONENT\_DERIVED parameter follows the naming convention "ExampleMotif", and to add the Motif to `ep` using `ep.addMotif("Example")` The "Motif" portion is implied in the naming.
Parameters can be passed to motifs through the string. The parameters are read as a list of assignments, separated by whitespace. For example, a motif 'Sum' that takes three integers as a parameter named x, y, and z
The Motif would be invoked `ep.addMotif("Sum x=4 y=5 z=6")` would pass the arguments as args.x, args.y, and args.z with assigned values 4, 5, and 6, respectively. The arguments are passed in a Param object to the motif generator to be parsed. 

Some additional functions that can be called on an endpoint or the `ep` variable:
* getName()
Returns the name of the ep. i.e., it will return "EmberMPIJob"
* enableMotifLog()
Starts logging the Motifs as they are executed.

Some common MPI-function call motifs that exist in Ember:

# Running the simulation

Finally, we create the 'system', which then runs the simulation. The topology is set with the setTopology function, and the endpoint is specified by system.allocateNodes. Build executes the motifs in the endpoint.
```
    system = System()
    system.setTopology(topo)
    system.allocateNodes(ep,"linear")
    system.build()
```
While most of the System is self-explanatory, the system allows the user to specify how threads/processes are assigned to nodes in the allocateNodes function.
The allocateNodes function allows the user to specify how the nodes are allocated. In the example, "linear" is chosen. In linear, the nodes are sorted, and threads are placed in linear order onto the simulated nodes. There are additional ways to allocate nodes, such as "random", "interval", and "indexed".
