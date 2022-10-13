# What is an Ember Motif?

Ember Motifs compare the effects of different hardware parameters on a representative workflow.
Ember Motifs provide representative workloads and then estimate performance on hardware using SST simulations.
This guide provides a directions on how to:

*   Implement a empty motif.
*   Create and run motif in a SST simulation.

To implement and run Ember Motif there are three different files:

1.  Python File
    Provides information on the hardware to be simulated when running an Ember Motif
    This directs SST on the available hardware components and how they are set up.
2.  C++ file
    Decides how the computation is structured and computed.
3.  C++ header file
    The header file contains primarily naming information

# Quick Start Guide

This guide gives a simple example of how to create and run a simple Motif.

First ensure SST-core and SST-elements are properly installed on your machine. For download and installation see [SST-Downloads](http://sst-simulator.org/SSTPages/SSTMainDownloads/).

Navigate to `sst-elements/src/elements/ember` inside the sst-elements library.
First, create a new directory to hold our example Motif.

`mkdir ExampleMotif`

Next, create a `ExampleMotif/exampleMotif.h` file
```
    #ifndef _H_EMBER_EXAMPLE
    #define _H_EMBER_EXAMPLE

    #include "../../mpi/embermpigen.h"

    namespace SST {
    namespace Ember {

    class ExampleGenerator : public EmberMessagePassingGenerator {
       public:

        SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
          Example,
          "ember",
          "ExampleMotif",
           SST_ELI_ELEMENT_VERSION(1,0,0),
          "Performs an idle on the node, no traffic can be generated.",
          SST::Ember::EmberGenerator
    );

        SST_ELI_DOCUMENT_PARAMS(
        );

        SST_ELI_DOCUMENT_STATISTICS(
          { "time-Init", "Time spent in Init event",             "ns", 0},
          { "time-Finalize", "Time spent in Finalize event",     "ns", 0},
          { "time-Rank", "Time spent in Rank event",             "ns", 0},
          { "time-Size", "Time spent in Size event",             "ns", 0},
          { "time-Send", "Time spent in Recv event",             "ns", 0},
          { "time-Recv", "Time spent in Recv event",             "ns", 0},
          { "time-Irecv", "Time spent in Irecv event",           "ns", 0},
          { "time-Isend", "Time spent in Isend event",           "ns", 0},
          { "time-Wait", "Time spent in Wait event",             "ns", 0},
          { "time-Waitall", "Time spent in Waitall event",       "ns", 0},
          { "time-Waitany", "Time spent in Waitany event",       "ns", 0},
          { "time-Compute", "Time spent in Compute event",       "ns", 0},
          { "time-Barrier", "Time spent in Barrier event",       "ns", 0},
          { "time-Alltoallv", "Time spent in Alltoallv event",   "ns", 0},
          { "time-Alltoall", "Time spent in Alltoall event",     "ns", 0},
          { "time-Allreduce", "Time spent in Allreduce event",   "ns", 0},
          { "time-Reduce", "Time spent in Reduce event",         "ns", 0},
          { "time-Bcast", "Time spent in Bcast event",           "ns", 0},
          { "time-Gettime", "Time spent in Gettime event",       "ns", 0},
          { "time-Commsplit", "Time spent in Commsplit event",   "ns", 0},
          { "time-Commcreate", "Time spent in Commcreate event", "ns", 0},
        );

        
    };
    }
    }

    #endif /* _H_EMBER_EXAMPLE */
```
This creates a Motif generator class called ExampleGenerator that inherits from EmberMessagePassingGenerator. The Python code we provide later creates and invokes the generate function.
The SST Document Statistics provides tracking for initialization and various MPI function calls.

Motifs are executed as follows:



1) The motif generator is initialized (The contructor)
2) The generate function is invoked and returns either true or false
3) The events on the eventQueue are processed.
4) If the generate function in step 2 returned false, return to step 2, otherwise the motif is complete.

Here is a motif that returns without doing any additional work.

Then we create a file `ExampleMotif/Example.cc` with the contents:

```
    #include <sst_config.h>
    #include "Example.h"

    using namespace SST:Ember;
    
    ExampleGenerator::ExampleGenerator(SST::ComponentId_t id, Params& params) :
        	EmberMessagePassingGenerator(id, params, "Null" )
    {
    }

    bool ExampleGenerator::generate( std::queue<EmberEvent*>& evQ)
    { 
        // Code here is repeated until true is returned. 
        return true;
    }
```


Motifs are intended to be ran as a 'job submission'.
The generate function models an entire iteration of an application. The event queue allows the motif to mix compute and MPI operations in every iteration.



The C++ file loads the SST:Ember namespace giving it access to other Motifs and EmberEvents. 
Each Motif generator has an constructor and generate function. 


Then we add the `.cc` and `.h` file to `Makefile.am`

i.e.
```
     embermemoryev.h \
     ExampleMotif/example.h \
     ExampleMotif/example.cc \
     libs/emberLib.h \
```

Finally, a python file `example.py` needs to be created:

```
    from email.mime import base
    import sst
    from sst.merlin.base import *
    from sst.merlin.endpoint import *
    from sst.merlin.interface import *
    from sst.merlin.topology import *

    from sst.ember import *

    def example():
        PlatformDefinition.setCurrentPlatform("firefly-defaults")

        ### Setup the topology
        topo = topoDragonFly()
        topo.hosts_per_router = 2
        topo.routers_per_group = 4
        topo.intergroup_links = 2
        topo.num_groups = 4
        topo.algorithm = ["minimal","adaptive-local"]

        # Set up the routers
        router = hr_router()
        router.link_bw = "4GB/s"
        router.flit_size = "8B"
        router.xbar_bw = "6GB/s"
        router.input_latency = "20ns"
        router.output_latency = "20ns"
        router.input_buf_size = "4kB"
        router.output_buf_size = "4kB"
        router.num_vns = 2
        router.xbar_arb = "merlin.xbar_arb_lru"

        topo.router = router
        topo.link_latency = "20ns"

        ### set up the endpoint
        networkif = ReorderLinkControl()
        networkif.link_bw = "4GB/s"
        networkif.input_buf_size = "1kB"
        networkif.output_buf_size = "1kB"

        ep = EmberMPIJob(0,topo.getNumNodes())
        ep.network_interface = networkif
        ep.addMotif("Example")
        ep.nic.nic2host_lat= "100ns"

        system = System()
        system.setTopology(topo)
        system.allocateNodes(ep,"linear")

        system.build()

    if __name__ == "__main__":
        example()
```

This follows common python syntax.
The hardware variables (topology, router and network interface) are created through assignment of a constructor
The fields of the variables are accessed using a dot operator.
Topology, router, and network interface variables need to be created for a simulation. More detailed descriptions of how to configure these variables is in RunningMotifs.md. 
Next an endpoint is created and given the motif to be used using a ep.addMotif("MotifName")call
Note multiple motifs can be added the same endpoint. The intended use for each motif to simulate an entire application. The event queue is used to simulate MPI events and computational workloads. Adding multiple motifs is useful for simulating workflows of a series of applications. 

Finally, a system variable is created and 'built'

To run the python script

```
sst example.py
```



