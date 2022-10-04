# C++ Ember Motifs
## Ember

[Ember](http://sst-simulator.org/SSTPages/SSTElementEmber/) is a library that can represents various network communications.  It is often used in conjunction with motifs.

## Motif

Motifs are condensed, efficient generators for different kinds of communication patterns.
We will be rather abstract with our motif.  In this case, we want it to be a "Foo" operation.
But it could just as easily be an MPI Allreduce operation for example.

### Register Subcomponent
To invoke an ember motif it first has to be registered as an SST subcomponent. This typically happens in the header file.
for example:
```
SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
        Foo,
        "ember",
        "FooMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Performs an idle on the node, no traffic can be generated.",
        SST::Ember::EmberGenerator
    );
```
The first parameter is the class associated with this Motif. The second parameter specifies the aspect of elements being used, in the case of Motifs it is "ember". The next parameter specifies the identifier used to index the Motif. Note that the end of this parameter must always be Motif, as this is implied in the name of the Motif. The next parameter specifies the  SST elements version, then a description of what the motif does. The last parameter specifies the EmberGenerator used.

### Writing a constructor
```
EmberFoo::EmberFoo(SST::ComponentId_t id, Params& params) :
	EmberMessagePassingGenerator(id, params, "Foo"),

```
The params can be parsed using `firstParam = params.find<uint32_t>("arg.firstParam", 100);` where the name of the parameter is "firstParam".

The params are created in the python file with the ep.addMotif("Foo firstParam=100 secondParam=200") line. Note that no space is allowed before or after the = operator. Parameters read from the python file will be prepended with "arg." before being passed to the C++ file. i.e. "firstParam" becomes "arg.firstParam".

### Writing a generate() function
```
bool Foo::generate( std::queue<EmberEvent*>& evQ)
```
The generate function is the 'main' function to a Motif. 
If the python file invokes a `addMotif` the generate function will be invoked until the generate function returns true.
Once the generate function returns true, the events queued into the evQ variable will be performed. 

# User defined events

There are a variety of functions that are used to allow the user to control aspects of the program computation.

* `enQ_compute(Queue&, uint64_t nanoSecondDelay)`   -- The delay by the expected cost of the input function in nanoSeconds (without actually computing it)
* `enQ_compute( Queue& q, std::function<uint64_t()> func)`; -- The compute function is passed as a parameter and invoked.
* `enQ_getTime(evQ, &m_startTime)` -- sets m_startTime to the current time

There are two types of 'compute' operations that can be enqueued. The first is a 'pseudo' compute operation. The amount of time needed to perform the compute is already known and so we can skip the actual computation and just delay as if we had computed. 
The other compute actually performs the compute and takes the function as a parameter. 
The user can also measure the time using a enQ_getTime call. Hypothetically the amount of time could be measured with a real invokation and then all subsuquent invocations could be estimated.


# MPI Events


MPI events can also be enqueued. We list the supported MPI events below. For more detail see MPI API documentation.


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
