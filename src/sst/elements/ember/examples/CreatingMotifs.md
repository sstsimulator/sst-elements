# C++ Ember Motifs
## Ember

[Ember](http://sst-simulator.org/SSTPages/SSTElementEmber/) is a library that can represents various network communications.  It is often used in conjunction with motifs.

## Motif

Motifs are condensed, efficient generators for simulating different kinds of communication/computation patterns.
The motif presented here does no real work, but more detailed motifs can be found in sst-elements/src/sst/elements/ember/mpi/motifs/.

Motifs are executed as follows:

1) The motif generator is initialized (The contructor)
2) The generate function is invoked, places events on the event queue, and returns either true or false
3) The events on the eventQueue are processed.
4) If the generate function in step 2 returned false, go to step 2, otherwise the motif is complete.

Motifs are intended to be ran as a 'job submission'.
The generate function models an entire iteration of an application, using the event queue to mix compute, and MPI operations in every iteration.

### Register Subcomponent

To invoke an ember motif it first has to be registered as an SST subcomponent. This typically happens in the header file.
for example:
```
SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
        Example,
        "ember",
        "ExampleMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Performs an idle on the node, no traffic can be generated.",
        SST::Ember::EmberGenerator
    );
```

The parameters to this function are:

1) class associated with this Motif
3) Aspect of elements being used, in the case of Motifs it is "ember"
3) Identifier of the Motif. Note that the end of this parameter must always be Motif
4) SST elements version,
5) Comment describing what the motif does
6) EmberGenerator

### Writing a constructor
```
EmberExample::EmberExample(SST::ComponentId_t id, Params& params) :
	EmberMessagePassingGenerator(id, params, "Example"),

```

The constructor allows initializes data used by the generate function.
The constructor also reads parameters from the python script. 
The params are passed through the python file in the form: `ep.addMotif("Example firstParam=100 secondParam=200")`. Note, no space is allowed before or after the = operator. Parameters read from the python file will be prepended with "arg." before being passed to the C++ file. i.e. "firstParam" becomes "arg.firstParam".

The params can be parsed in the C++ file with `firstParam = params.find<uint32_t>("arg.firstParam", 100);` where the name of the parameter is "firstParam".

### Writing a generate() function
```
bool Example::generate( std::queue<EmberEvent*>& evQ)
```
The generate function is the 'main' function to a Motif. 
If the python file invokes a `addMotif` the generate function will be invoked until the generate function returns true.
Once the generate function returns, the events queued in the evQ variable will be performed. 

The generate function takes a event queue as a parameter. The event queue allows the user to include computation operations and MPI events to the simulation. Motifs are intended to be designed so that every call to generate() mimics an entire iteration of the application. The list of events that can be added to the event queue are listed in subsequent sections. 

.  

# User defined events

These functions allow the user to control how the simulation estimates computation time.

* `enQ_compute(Queue&, uint64_t nanoSecondDelay)`   -- The delay by the expected cost of the compute operation in nanoSeconds (without actually computing it)
* `enQ_compute( Queue& q, std::function<uint64_t()> func)`; -- A function is passed as a parameter and invoked. It return a 64 bit unsigned integer that indicates the simulation delay associated with invoking the function (in nanoseconds).
* `enQ_getTime(evQ, &m_startTime)` -- sets m_startTime to the current time

Two types of 'compute' operations types of compute enqueued. The first is a 'pseudo' compute operation. The amount of time needed to perform the compute is already known and so we can skip the actual computation and just delay as if we had computed. 
The second compute takes a function as a parameter. The function returns an the time delay cause by the function. The time delay could be estimated through a hueristic or measured through representative computation. 


# MPI Events


We list the supported MPI events below. For more detail see MPI API documentation.


* `enQ_init(evQ)`  --  MPI  initialize
* `enQ_fini(evQ)` -- MPI finalize
* `enQ_rank(evQ, m_newComm[0], &m_new_rank)` -- MPI rank
* `enQ_size(evQ, m_newComm[0], &m_new_size)` --   MPI size
* `enQ_send(evQ, x_neg, x_xferSize, 0, GroupWorld)`  --  MPI send
* `enQ_recv(evQ, x_neg, x_xferSize, 0, GroupWorld)`  --  MPI  recv
* `enQ_isend(evQ, next_comm_rank, itemsThisPhase, 0, GroupWorld, &requests[next_request])` --  MPI isend
* `enQ_irecv(evQ, xface_down, items_per_cell * sizeof_cell * ny * nz, 0, GroupWorld, &requests[nextRequest])` --  MPI irecv
* `enQ_cancel(evQ, m_req[i])` -- MPI cancel
* `enQ_sendrecv(evQ, m_sendBuf, m_messageSize, DATA_TYPE, destRank(), 0xdeadbeef, m_recvBuf, m_messageSize, DATA_TYPE, srcRank(),  0xdeadbeef, GroupWorld, &m_resp)`  --  MPI send or recv
* `enQ_wait(evQ,  req)`  --  MPI wait
* `enQ_waitany(evQ, m_req.size(), &m_req[0], &m_indx, &m_resp)` --  MPI waitany
* `enQ_waitall(evQ, 1, &m_req, (MessageResponse**)&m_resp)` --  MPI waitall
* `enQ_barrier(evQ, GroupWorld)` -- MPI barrier
* `enQ_bcast(evQ, m_sendBuf, m_count, DOUBLE, m_root, GroupWorld)` -- MPI broadcast
* `enQ_scatter(evQ, m_sendBuf, m_sendCnt, m_sendDsp.data(), LONG, m_recvBuf, m_count, LONG, m_root, GroupWorld)` 
* `enQ_scatterv(evQ, m_sendBuf, &m_sendCnts[0], m_sendDsp.data(), LONG, m_recvBuf, m_count, LONG, m_root, GroupWorld)` -- MPI Scatter variable message size
* `enQ_reduce( evQ, m_sendBuf, m_recvBuf, m_count, DOUBLE, MP::SUM, m_redRoot, GroupWorld)`
* `enQ_allreduce(evQ, m_sendBuf, m_recvBuf, m_count, DOUBLE, m_op, GroupWorld)` -- MPI allreduce function
* `enQ_alltoall(evQ, m_sendBuf, m_colSendCnt, &m_colSendDsp_f[0], DOUBLE, m_colComm)` -- MPI alltoall with constant message size
* `enQ_alltoallv(evQ, m_sendBuf, &m_colSendCnts_f[0], &m_colSendDsp_f[0], DOUBLE, m_colComm)` -- MPI alltoall with varying message sizes
* `enQ_allgather(evQ, m_sendBuf, m_count, INT, m_recvBuf, m_count, INT, GroupWorld)` --  MPI allgather with messages of constant size
* `enQ_allgatherv(evQ, m_sendBuf, m_sendCnt, INT, m_recvBuf, &m_recvCnts[0], &m_recvDsp[0], INT, GroupWorld)` --  MPI allgather with messages of varying sizes
* `enQ_commSplit(evQ, *comm, color, key, newComm)` -- Splits the MPI comm
* `enQ_commCreate(evQ, GroupWorld, m_rowGrpRanks, &m_rowComm)` -- Creates MPI Com
* `enQ_commDestroy(evQ, m_rowComm)` --Destroys MPI comm
