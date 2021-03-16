// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>

#include "emberlqcd.h"

using namespace SST::Ember;


EmberLQCDGenerator::EmberLQCDGenerator(SST::ComponentId_t id, Params& params) :
	EmberMessagePassingGenerator(id, params, "LQCD"),
	m_loopIndex(0)
{
	nx  = (uint32_t) params.find("arg.nx", 64);
	ny  = (uint32_t) params.find("arg.ny", 64);
	nz  = (uint32_t) params.find("arg.nz", 64);
	nt  = (uint32_t) params.find("arg.nt", 64);

    num_nodes = size();
    sites_on_node = nx*ny*nz*nt/num_nodes;
    even_sites_on_node = sites_on_node/2;
    odd_sites_on_node = sites_on_node/2;

    // 3X3 matrix of complex values -> 18 (8 Byte) doubles
    // This represents the long links or fat links aka gluons aka force carriers
    sizeof_su3matrix = 18*8;

    // 3 component complex vector -> 6 (8 Byte) doubles
    // represents the matter fields (quarks)
    sizeof_su3vector = 6*8;

    // With data compression, a Xeon Phi (KNL) could get 300 GF/s
    // see "Staggered Dslash Performance on Intel Xeon Phi Architecture"
	uint64_t pe_flops = (uint64_t) params.find("arg.peflops", 20ULL*1000*1000*1000);

	const uint64_t total_sites = (uint64_t) (nx * ny * nz * nt);

    // The MILC code for CG gave this calculation sites_on_node*(offset*15+1205)
    // Offset is also referred to as "number of masses" with referenced values being 9 or 11.
    // Some algorithms compress data before writing it to memory.
    // The equation below accounts only for "useful" flops.
    // Turn this into an optional argument
	//const uint64_t flops_per_iter = sites_on_node*(11*15 + 1205)/2;
    const uint64_t flops_per_iter = 0;

    // From the portion of code we are looking at there is 157(sites_on_node)/2 for the resid calc
    // (12*num_offsets+12 +12 +1) * V/2
	const uint64_t flops_resid = sites_on_node*157/2;
    // Per SG: multiplication of 3X3 matrix by 3-vector:
    // 36 multiplies and 30 adds, i.e. 66 flops.
    // 3-vector add is 6 adds, so one call to this function is 4*72 flops.
    // Each call is, therefore, 288 flops and there is loop over sites,
    // but that loop is only over one parity.
    // But, there are two calls to mult_su3_mat_vec_sum_4dir, once with fatback4 and once with longback4.
	const uint64_t flops_mmvs4d = sites_on_node*288;

    // This is overly simplified, but divides the flops per iteration evenly across the compute segments
    // You have 4 approximately equal compute segments after the even/odd, pos/neg gathers,
    // then the resid calculation which we assign two segments to.
    // Getting computation segments to represent all architectures optimizations is not possible.
    // Arithmetic intensity can vary widely machine to machine.
    // This motif represents a starting point, given our profiled systems.

	// Converts FLOP/s into nano seconds of compute
	compute_nseconds_resid = ( flops_resid / ( pe_flops / 1000000000.0 ) );
	compute_nseconds_mmvs4d = ( flops_mmvs4d / ( pe_flops / 1000000000.0 ) );

    if(flops_per_iter){
        const uint64_t compute_nseconds = ( flops_per_iter / ( pe_flops / 1000000000.0 ) );
        nsCompute  = (uint64_t) params.find("arg.computetime", (uint64_t) compute_nseconds);
	    output("nsCompute set:%" PRIu64 " ns\n", nsCompute);
    }
    else{
        nsCompute = 0;
        if (rank() == 0){
            output("LQCD compute time resid: %" PRIu64 " mmvs4d: %" PRIu64 " ns\n", compute_nseconds_resid, compute_nseconds_mmvs4d);
        }
    }

	iterations = (uint32_t) params.find("arg.iterations", 1);

    coll_start = 0;
    coll_time = 0;
    comp_start = 0;
    comp_time = 0;
    gather_pos_start = 0;
    gather_pos_time = 0;
    gather_neg_start = 0;
    gather_neg_time = 0;

	n_ranks[XDOWN] = -1;
	n_ranks[XUP]   = -1;
	n_ranks[YDOWN] = -1;
	n_ranks[YUP]   = -1;
	n_ranks[ZDOWN] = -1;
	n_ranks[ZUP]   = -1;
	n_ranks[TDOWN] = -1;
	n_ranks[TUP]   = -1;

	configure();
}


//returns the rank that holds the site x,y,z,t
int EmberLQCDGenerator::get_node_index(int x, int y, int z, int t){
    int i = -1;
    i = x + nx * (y + ny * (z + nz) * t);
    return i % num_nodes;
}

//copies coordinates with an offset in the specified dimension
void EmberLQCDGenerator::neighbor_coords(const int coords[], int n_coords[], const int dim, const int offset){
    n_coords[XUP] = coords[XUP];
    n_coords[YUP] = coords[YUP];
    n_coords[ZUP] = coords[ZUP];
    n_coords[TUP] = coords[TUP];
    n_coords[dim] = n_coords[dim]+offset;
}

/*------------------------------------------------------------------*/
/* Convert rank to machine coordinates */
void EmberLQCDGenerator::lex_coords(int coords[], const int dim, const int size[], const uint32_t rank){
  int d;
  uint32_t r = rank;

  for(d = 0; d < dim; d++){
    coords[d] = r % size[d];
    r /= size[d];
  }
}

/*------------------------------------------------------------------*/
/* Parity of the coordinate */
int EmberLQCDGenerator::coord_parity(int r[]){
  return (r[0] + r[1] + r[2] + r[3]) % 2;
}

/*------------------------------------------------------------------*/
/* Convert machine coordinate to linear rank (inverse of
   lex_coords) */
//added two checks to return -1 if we are trying to access a neighbor that
//is beyond the scope of the lattice.

int EmberLQCDGenerator::lex_rank(const int coords[], int dim, int size[])
{
    int d;
    int x = coords[XUP];
    int y = coords[YUP];
    int z = coords[ZUP];
    int t = coords[TUP];
    size_t rank = coords[dim-1];
    if (x == -1 || y == -1 || z == -1 || t == -1){
        return(-1);
    }
    if (x >= nsquares[XUP] || y >= nsquares[YUP] || z >= nsquares[ZUP] || t >= nsquares[TUP]){
        return(-1);
    }

    for(d = dim-2; d >= 0; d--){
        rank = rank * size[d] + coords[d];
    }
    return rank;
}

void EmberLQCDGenerator::setup_hyper_prime(){
    int prime[] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53};
    int dir, i, j, k;
    int len_primes = sizeof(prime)/sizeof(int);
    /* Figure out dimensions of rectangle */
    squaresize[XUP] = nx; squaresize[YUP] = ny;
    squaresize[ZUP] = nz; squaresize[TUP] = nt;
    nsquares[XUP] = nsquares[YUP] = nsquares[ZUP] = nsquares[TUP] = 1;

    i = 1;	/* current number of hypercubes */
    while(i<num_nodes){
        /* figure out which prime to divide by starting with largest */
        k = len_primes-1;
        while( (num_nodes/i)%prime[k] != 0 && k>0 ) --k;
        /* figure out which direction to divide */

        /* find largest dimension of h-cubes divisible by prime[k] */
        for(j=0,dir=XUP;dir<=TUP;dir++)
            if( squaresize[dir]>j && squaresize[dir]%prime[k]==0 )
                j=squaresize[dir];

        /* if one direction with largest dimension has already been
           divided, divide it again.  Otherwise divide first direction
           with largest dimension. */
        for(dir=XUP;dir<=TUP;dir++)
            if( squaresize[dir]==j && nsquares[dir]>1 )break;
        if( dir > TUP)for(dir=XUP;dir<=TUP;dir++)
            if( squaresize[dir]==j )break;
        /* This can fail if I run out of prime factors in the dimensions */
        if(dir > TUP){
            if(rank() == 0)
                printf("LAYOUT: Can't lay out this lattice, not enough factors of %d\n"
               ,prime[k]);
        }

        /* do the surgery */
        i*=prime[k]; squaresize[dir] /= prime[k]; nsquares[dir] *= prime[k];
    }
    if(0 == rank()) {
        output("LQCD nsquares size: x%" PRIu32 "y%" PRIu32 "z%" PRIu32 "t%" PRIu32"\n", nsquares[XUP], nsquares[YUP], nsquares[ZUP], nsquares[TUP]);
        output("LQCD square size: x%" PRIu32 "y%" PRIu32 "z%" PRIu32 "t%" PRIu32"\n", squaresize[XUP], squaresize[YUP], squaresize[ZUP], squaresize[TUP]);
    }
}

//returns the node number on which a site lives.
int EmberLQCDGenerator::node_number_from_lex(int x, int y, int z, int t) {
    int i;
    i = x + nsquares[XUP]*( y + nsquares[YUP]*( z + nsquares[ZUP]*( t )));
    return(i);
}

//returns the surface volume to be transfered in a gather (1st neighbor)
int EmberLQCDGenerator::get_transfer_size(int dimension){
    int sitestrans = 1;
    int dim = dimension;
    // squaresize[neg] should == squaresize[pos]
    if (dimension >= TDOWN){
        dim = OPP_DIR(dim);
    }
    // for positive directions
    for (int d = XUP; d <= TUP; d++){
        if (d != dim){
            sitestrans *= squaresize[d];
        }
    }
    return sitestrans;
}

//returns the node number on which a site lives (given lattice coords).
/*------------------------------------------------------------------*/
int EmberLQCDGenerator::node_number(int x, int y, int z, int t) {
    int i;
    x /= squaresize[XUP]; y /= squaresize[YUP];
    z /= squaresize[ZUP]; t /= squaresize[TUP];
    i = x + nsquares[XUP]*( y + nsquares[YUP]*( z + nsquares[ZUP]*( t )));
    return( i );
}

void EmberLQCDGenerator::configure()
{

    //determine the problem size given to each node
    //code from MILC setup_hyper_prime()
    setup_hyper_prime();

    //the coordinates of this rank in pe_x, pe_y, pe_z, pe_t
    lex_coords(machine_coordinates, 4, nsquares, rank());

    /* Number of sites on node */
    sites_on_node = squaresize[XUP]*squaresize[YUP]*squaresize[ZUP]*squaresize[TUP];
    even_sites_on_node = sites_on_node/2;
    odd_sites_on_node = sites_on_node/2;
    if(0 == rank()) {
		output("LQCD problem size: x%" PRIu32 "y%" PRIu32 "z%" PRIu32 "t%" PRIu32"\n", nx, ny, nz, nt);
		output("LQCD gather num_elements (1st and Naik): %" PRIu64  " %" PRIu64 "\n", (uint64_t) nx*ny*nz*9, (uint64_t) nx*ny*nz*27);
        if (nsCompute != 0) output("LQCD compute time per segment: %" PRIu64 " ns\n", nsCompute);
        else output("LQCD compute time resid: %" PRIu64 " mmvs4d: %" PRIu64 " ns\n", compute_nseconds_resid, compute_nseconds_mmvs4d);
		output("LQCD iterations: %" PRIu32 "\n", iterations);
		output("LQCD sites per node: %" PRIu32 "\n", sites_on_node);
	}
    //determine rank of neighbors in each direction x,y,z,t
    int tmp_coords[4];
    //positive direction
    for (int d = XUP; d <= TUP; d++){
        neighbor_coords(machine_coordinates, tmp_coords, d, 1);
        n_ranks[d] = lex_rank(tmp_coords, 4, nsquares);
    }
    //negative direction
    for (int d = XUP; d <= TUP; d++){
        neighbor_coords(machine_coordinates, tmp_coords, d, -1);
        n_ranks[OPP_DIR(d)] = lex_rank(tmp_coords, 4, nsquares);
    }

	verbose(CALL_INFO, 1, 0, "Rank: %" PRIu32 ", World=%" PRId32 ", X=%" PRId32 ", Y=%" PRId32 ", Z=%" PRId32 ", T=%" PRId32 ", Px=%" PRId32 ", Py=%" PRId32 ", Pz=%" PRId32 ", Pt=%" PRId32 "\n",
		rank(), size(),
        machine_coordinates[XUP], machine_coordinates[YUP], machine_coordinates[ZUP], machine_coordinates[TUP],
        nsquares[XUP],nsquares[YUP],nsquares[ZUP],nsquares[TUP]);
	verbose(CALL_INFO, 1, 0, "Rank: %" PRIu32 ", X+: %" PRId32 ", X-: %" PRId32 "\n", rank(), n_ranks[XUP], n_ranks[XDOWN]);
	verbose(CALL_INFO, 1, 0, "Rank: %" PRIu32 ", Y+: %" PRId32 ", Y-: %" PRId32 "\n", rank(), n_ranks[YUP], n_ranks[YDOWN]);
	verbose(CALL_INFO, 1, 0, "Rank: %" PRIu32 ", Z+: %" PRId32 ", Z-: %" PRId32 "\n", rank(), n_ranks[ZUP], n_ranks[ZDOWN]);
	verbose(CALL_INFO, 1, 0, "Rank: %" PRIu32 ", T+: %" PRId32 ", T-: %" PRId32 "\n", rank(), n_ranks[TUP], n_ranks[TDOWN]);
}

// This code is a simplified representation of Rational Hybrid Monte Carlo (RHMC)
// in MILC lattice QCD.
// This is mostly focused on the Conjugate Gradient and Dslash
bool EmberLQCDGenerator::generate( std::queue<EmberEvent*>& evQ )
{
    verbose(CALL_INFO, 1, 0, "loop=%d\n", m_loopIndex );
	std::vector<MessageRequest*> pos_requests;
	std::vector<MessageRequest*> neg_requests;

    // Even/Odd preconditioning, organizes sites into two sets (if enabled odd_even == 2)
    // If enabled we do two loops of this function with one-half transfer size
    // int even_odd = 1;
    int even_odd = 2;
    // Enqueue gather from positive and negative directions (8 total)  su3_vector
    int transsz = 500;

    for (int parity = 1; parity <= even_odd; parity++){

        for (int d = XUP; d <= TUP; d++){
            if (n_ranks[d] != -1){
                MessageRequest*  req  = new MessageRequest();
                pos_requests.push_back(req);
                transsz = get_transfer_size(d)/even_odd;
                if (rank() == 0 || n_ranks[d] == 0)
                    verbose(CALL_INFO, 2, 0, "Rank: %" PRIu32 "receiving pos elements: %" PRIu32 "\n", rank(), transsz);
                enQ_irecv( evQ, n_ranks[d],
                        sizeof_su3vector * transsz, 0, GroupWorld, req);
            }
        }
        for (int d = XUP; d <= TUP; d++){
            if ( n_ranks[d] != -1 ){
                transsz = get_transfer_size(d)/even_odd;
                if (rank() == 0 || n_ranks[d] == 0)
                    verbose(CALL_INFO, 2, 0, "Rank: %" PRIu32 "sending pos elements: %" PRIu32 "\n", rank(), transsz);
                enQ_send( evQ, n_ranks[d] ,sizeof_su3vector * transsz, 0, GroupWorld);
            }
        }

        // And start the 3-step gather (4 total)
        // In the Greg Bauer MILC performance model paper they list trans_sz as
        // Fetch Naik term or three link term (points 1, 2 and 3 planes into the next domain)
        // multiplied by 6 elements multiplied by surface volume.
        // We don't need to gather the first site twice (hence get_transfer_size(d)*2)

        for (int d = XUP; d <= TUP; d++){
            if (n_ranks[d] != -1){
                MessageRequest*  req  = new MessageRequest();
                pos_requests.push_back(req);
                transsz = get_transfer_size(d)*2/even_odd;
                if (rank() == 0 || n_ranks[d] == 0)
                    verbose(CALL_INFO, 2, 0, "Rank: %" PRIu32 "receiving pos (Naik) elements: %" PRIu32 "\n", rank(), transsz);
                enQ_irecv( evQ, n_ranks[d],
                        sizeof_su3vector * transsz, 0, GroupWorld, req);
            }
        }
        for (int d = XUP; d <= TUP; d++){
            if ( n_ranks[d] != -1 ){
                transsz = get_transfer_size(d)*2/even_odd;
                if (rank() == 0 || n_ranks[d] == 0)
                    verbose(CALL_INFO, 2, 0, "Rank: %" PRIu32 "sending pos (Naik) elements: %" PRIu32 "\n", rank(), transsz);
                enQ_send( evQ, n_ranks[d] ,sizeof_su3vector * transsz, 0, GroupWorld);
            }
        }

        // Enqueue gather from negative directions (4 total)
        for (int d = TDOWN; d <= XDOWN; d++){
            if (n_ranks[d] != -1){
                MessageRequest*  req  = new MessageRequest();
                neg_requests.push_back(req);
                transsz = get_transfer_size(d)/even_odd;
                if (rank() == 0 || n_ranks[d] == 0)
                    verbose(CALL_INFO, 2, 0, "Rank: %" PRIu32 "receiving neg elements: %" PRIu32 "\n", rank(), transsz);
                enQ_irecv( evQ, n_ranks[d],
                        sizeof_su3vector * transsz, 0, GroupWorld, req);
            }
        }
        for (int d = TDOWN; d <= XDOWN; d++){
            if ( n_ranks[d] != -1 ){
                transsz = get_transfer_size(d)/even_odd;
                if (rank() == 0 || n_ranks[d] == 0)
                    verbose(CALL_INFO, 2, 0, "Rank: %" PRIu32 "sending neg elements: %" PRIu32 "\n", rank(), transsz);
                enQ_send( evQ, n_ranks[d] ,sizeof_su3vector * transsz, 0, GroupWorld);
            }
        }

        // Enqueue 3-step gather (4 total)
        for (int d = TDOWN; d <= XDOWN; d++){
            if (n_ranks[d] != -1){
                MessageRequest*  req  = new MessageRequest();
                neg_requests.push_back(req);
                transsz = get_transfer_size(d)*2/even_odd;
                if (rank() == 0 || n_ranks[d] == 0)
                    verbose(CALL_INFO, 2, 0, "Rank: %" PRIu32 "receiving neg (Naik) elements: %" PRIu32 "\n", rank(), transsz);
                enQ_irecv( evQ, n_ranks[d],
                        sizeof_su3vector * transsz, 0, GroupWorld, req);
            }
        }
        for (int d = TDOWN; d <= XDOWN; d++){
            if ( n_ranks[d] != -1 ){
                transsz = get_transfer_size(d)*2/even_odd;
                if (rank() == 0 || n_ranks[d] == 0)
                    verbose(CALL_INFO, 2, 0, "Rank: %" PRIu32 "sending neg (Naik) elements: %" PRIu32 "\n", rank(), transsz);
                enQ_send( evQ, n_ranks[d] ,sizeof_su3vector * transsz, 0, GroupWorld);
            }
        }

        // Wait on positive gathers (8 total)
	    verbose(CALL_INFO, 1, 0, "Rank: %" PRIu32 ", +Messages to wait on: %zu\n", rank(), pos_requests.size());
        for(uint32_t i = 0; i < pos_requests.size(); ++i) {
            enQ_wait( evQ, pos_requests[i]);
        }
        pos_requests.clear();

        // Compute Matrix Vector Multiply Sum in 4 directions
        // C <- A[0]*B[0]+A[1]*B[1]+A[2]*B[2]+A[3]*B[3]
        // This is done for both positive "fat" and "long" links
        comp_start = Simulation::getSimulation()->getCurrentSimCycle();
        //output("Rank %" PRIu32 ", Compute start: %" PRIu64 "\n", rank(), comp_start);
        if (nsCompute) enQ_compute(evQ, nsCompute);
        else enQ_compute(evQ, compute_nseconds_mmvs4d);
        comp_time += Simulation::getSimulation()->getCurrentSimCycle() - comp_start;
        //output("Rank %" PRIu32 ", Compute end: %" PRIu64 "\n", rank(), Simulation::getSimulation()->getCurrentSimCycle());

        //Wait on negative gathers (8 total)
	    verbose(CALL_INFO, 1, 0, "Rank: %" PRIu32 ", -Messages to wait on: %zu\n", rank(), neg_requests.size());
        for(uint32_t i = 0; i < neg_requests.size(); ++i) {
            enQ_wait( evQ, neg_requests[i]);
        }

        neg_requests.clear();

        // Compute Matrix Vector Multiply Sum in 4 directions
        // C <- A[0]*B[0]+A[1]*B[1]+A[2]*B[2]+A[3]*B[3]
        // This is done for both negative "fat" and "long" links
        if (nsCompute) enQ_compute(evQ, nsCompute);
        else enQ_compute(evQ, compute_nseconds_mmvs4d);

        //output("Iteration on rank %" PRId32 " completed generation, %d events in queue\n",
        //    rank(), (int)evQ.size());
    } // END even-odd preconditioning loop

    // Allreduce MPI_DOUBLE
    coll_start = Simulation::getSimulation()->getCurrentSimCycle();
    enQ_allreduce( evQ, NULL, NULL, 1, DOUBLE, MP::SUM, GroupWorld);
    coll_time += Simulation::getSimulation()->getCurrentSimCycle() - coll_start;
    // Compute ResidSq
    // Our profiling shows this is ~ 1/4th the FLOPS of the
    // compute segments above, but is more memory intensive (and takes longer).
    // Depending on whether the problem is small enough to utilize memory
    // effectively, the time spent here can vary widely.
    if (nsCompute) enQ_compute(evQ, nsCompute);
    else enQ_compute(evQ, compute_nseconds_resid);
    // Allreduce MPI_DOUBLE
    coll_start = Simulation::getSimulation()->getCurrentSimCycle();
    //output("Rank %" PRIu32 ", Collective start: %" PRIu64 "\n", rank(), coll_start);
    enQ_allreduce( evQ, NULL, NULL, 1, DOUBLE, MP::SUM, GroupWorld);
    coll_time += Simulation::getSimulation()->getCurrentSimCycle() - coll_start;
    //output("Rank %" PRIu32 ", Collective end: %" PRIu64 "\n", rank(), Simulation::getSimulation()->getCurrentSimCycle());

    if ( ++m_loopIndex == iterations ) {
        //output("Rank %" PRIu32 ", Time spent in collectives (ns): %" PRIu64 "\n", rank(), coll_time);
        return true;
    } else {
        return false;
    }
}

