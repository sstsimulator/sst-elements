# What is an Ember Motif? 
Ember Motifs can be used to compare the effects of different hardware parameters on a representative workflow.
Ember Motifs provide allow the user to provide representative workloads 
This guide provides a directions on how to:
- Create and vary hardware parameters in SST simulations.
- Implement code to simulate representative workloads.

To implement and run Ember Motif there have to be three different files: 
1) Python File
Provides information on the hardware to be simulated when running an Ember Motif
This file holds directions to SST on how to setup the simulated hardware componenets.
2) C++ file
Decides how the computation is structured and computed.
3) C++ header file
The header file contains primarily naming information

We have a Quick-Start guide to creating an Ember Motif.
Details on how to make and implement the Python file
Details on how to implement the C++ file and the C++ header file



# Quick Start Guide
This guide gives a simple example of how to create a 'Foomake' Motif and how to run a simulation.

First ensure SST-core and SST-elements are properly installed on your machine. For download and installation see [SST-Downloads].

Next we navigate to `sst-elements/src/elements/ember` inside the elements library.
First, create a new directory to hold our FooMotif.

`mkdir fooMotif`



Next, create a FooMotif/foo.h file

<code>
    #ifndef _H_EMBER_FOO
    #define _H_EMBER_FOO

    #include "../../mpi/embermpigen.h"

    namespace SST {
    namespace Ember {

    class FooGenerator : public EmberMessagePassingGenerator {
       public:

        SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
          Foo,
          "ember",
          "FooMotif",
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

#endif /* _H_EMBER_FOO */
</code>
This creates a class called FooGenerator that inherits from EmberMessagePassingGenerator. The Python code we provide later creates and invokes the generate function on a EmberMessagePassingGenerator until the generate function returns true.
The SST Document Statistics provides tracking for initialization and various MPI function calls.


Then we create a file fooMotif/foo.cc
With the contents:
<code>

    #include <sst_config.h>
    #include "foo.h"
    using namespace SST:Ember;
    FooGenerator::FooGenerator(SST::ComponentId_t id, Params& params) :
	    	EmberMessagePassingGenerator(id, params, "Null" )
    	{
        }

        bool FooGenerator::generate( std::queue<EmberEvent*>& evQ)
	    { // Code here is repeated until true is returned. 
	    	return true;
	    }
</code>

The C++ file loads the SST:Ember namespace and has an constructor and generate function. 

Then we add the `.cc` and `.h` file to `Makefile.am`

i.e.

<code>
    
     embermemoryev.h \
	 fooMotif/foo.h \
	 fooMotif/foo.cc \
	 libs/emberLib.h \
</code>



[SST]: http://sst-simulator.org/
[SST-Downloads]: http://sst-simulator.org/SSTPages/SSTMainDownloads/
