// Copyright 2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2022, NTESS
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
#include <fstream>
#include <iterator>
#include <algorithm>
#include "emberBFS.h"

using namespace SST::Ember;

EmberBFSGenerator::EmberBFSGenerator(SST::ComponentId_t id,
                                     Params& params) :
    EmberMessagePassingGenerator(id, params, "BFS"), state(0),
    recvCounts_31(0), sendCounts_35(0), recvCounts_35(0)
{
    m_sz = (int32_t) params.find("arg.sz", 1);
    m_threads = (int32_t) params.find("arg.threads", 1);
    uint32_t rng_seed = (uint32_t) params.find("arg.seed", 1);
    m_nodes = (uint32_t) params.find("arg.nodes", 1);

    // Parse model files
    std::ifstream msg_model_file( params.find<std::string>("arg.msg_model", "msg_size.model") );
    std::ifstream exec_model_file( params.find<std::string>("arg.exec_model", "exec_time.model") );

    if (not (exec_model_file.good() and msg_model_file.good())) {
        out.fatal(CALL_INFO, 1, "Bad model file(s)");
    }

    // Read the communication size model into a SharedMap
    // Each line of the file is
    //   callsite coeff1 coeff2 coeff3 [...]
    std::string line;
    int skip_first = true;
    double scale = 1.0;
    double min   = 4.0; // the minimum communication size should be 1 float/int
    while (getline(msg_model_file, line)) {
        if (skip_first) {
            skip_first = false;
            continue;
        }
        std::istringstream iss(line);
        int cs, num_nodes, threads_per_rank;
        iss >> cs >> num_nodes >> threads_per_rank;
        std::vector<double> coeff((std::istream_iterator<double>(iss)),
                                  std::istream_iterator<double>());

        auto key = std::make_tuple(cs, num_nodes, threads_per_rank);
        msg_model.insert(std::pair<std::tuple<int,int,int>,PolyModel>(key,PolyModel(coeff,scale,min)));
    }

    // Read the compute time model into a SharedMap
    // Each line of the file is
    //   src_callsite dst_callsite coeff1 coeff2 coeff3 [...]
    skip_first = true;
    scale=1000.0; // adjust timescale
    min = 0.0;    // no negative times
    while (getline(exec_model_file, line)) {
        if (skip_first) {
            skip_first = false;
            continue;
        }
        std::istringstream iss(line);
        int src_cs, dst_cs, num_nodes, threads_per_rank;
        iss >> src_cs >> dst_cs >> num_nodes >> threads_per_rank;
        std::vector<double> coeff((std::istream_iterator<double>(iss)),
                                  std::istream_iterator<double>());

        auto key = std::make_tuple(src_cs, dst_cs, num_nodes, threads_per_rank);
        exec_model.insert(std::pair<std::tuple<int,int,int,int>,PolyModel>(key,PolyModel(coeff,scale,min)));
    }

    // Print out the first five models to see if they look right
    /*
    if (rank() == 0) {
        int counter = 0;
        std::cout << "COMM MODELS" << std::endl;
        for (auto const& [key, pm] : msg_model) {
            if (++counter > 5) {
                break;
            }
            std::cout << std::get<0>(key) << ' ' << std::get<1>(key) << ' ' << std::get<2>(key) << ": ";
            for (auto const p : pm.coeff) {
                std::cout << " " << p;
            }
            std::cout << std::endl;
        }

        std::cout << "COMP MODELS" << std::endl;
        counter = 0;
        for (auto const& [key, pm] : exec_model) {
            if (++counter > 5) {
                break;
            }
            //std::cout << transition.first << " -> " << transition.second << ": ";
            std::cout << std::get<0>(key) << "->" << std::get<1>(key) << ' ' << std::get<2>(key) << ' ' << std::get<3>(key) << ": ";
            for (auto const p : pm.coeff) {
                std::cout << " " << p;
            }
            std::cout << std::endl;
        }

    }
    */

    // initial state
    iter = 0;
    maxIter = 64;
    square = sqrt(size());
    initIter();
    state = 0;
    prevState = 0;
    s_time = 0;

    // init comm rank structures
    unsigned myRow = rank() / square;
    unsigned myCol = rank() % square;

    for (int i = 0; i < square; ++i) {
        comm0_ranks.push_back(i+myRow*square); // row
        comm2_ranks.push_back(myCol+i*square); // col
        /*if (myCol+i*square == rank()){
            comm2_rank = comm2_ranks.size()-1;
            //printf("%d c2r %d\n", rank(), comm2_rank);
            }*/
    }
    for (int i = 0; i < size(); ++i) {
        comm1_ranks.push_back(i); // COMM_WORLD
    }
#if 0
    printf("%d row:", rank());
    for (int i = 0; i < square; ++i) {
        printf("%d ", comm0_ranks[i]);
    }
    printf("\n");
    printf("%d col:", rank());
    for (int i = 0; i < square; ++i) {
        printf("%d ", comm2_ranks[i]);
    }
    printf("\n");
#endif

    // find 'oppoosite'
    // if I am (myRow,myCol), my opposite is at (myCol,myRow);
    opposite = myCol*square + myRow;
    //printf("%d opposite: %llu\n", rank(), opposite);

    // init RNGs
    rng = new SST::RNG::MersenneRNG(rng_seed);


    // setup empty buffers
    nullBuf = 0;
    nullDispMap = new int[size()];
    std::memset(nullDispMap, 0, sizeof(nullDispMap[0])*size());
}

EmberBFSGenerator::~EmberBFSGenerator() {
    if (recvCounts_31) {
        delete recvCounts_31;
    }
    if (recvCounts_35) {
        delete sendCounts_35;
        delete recvCounts_35;
    }
    delete rng;
    delete nullDispMap;
}

void EmberBFSGenerator::initIter() {
    prevState = -1;
    state = 1;
    idx_39 = 0;
    idx_49 = 0;
    idx_50 = 0;
}

bool EmberBFSGenerator::generate( std::queue<EmberEvent*>& evQ) {
    bool done = 0;

    enQ_getTime( evQ, &s_time );

    switch(state) {
    case 0: // special init state
        // create communicators
        if (iter == 0) {
            enQ_commCreate( evQ, GroupWorld, comm0_ranks, &comm0 );
            enQ_commCreate( evQ, GroupWorld, comm1_ranks, &comm1 );
            enQ_commCreate( evQ, GroupWorld, comm2_ranks, &comm2 );
        }
        state = 1;
        break;
    case 1:
        {
            int next_state = 2;
            auto msg_size  = msg_model[std::make_tuple(state,m_nodes,m_threads)].eval(m_sz);
            auto exec_time = exec_model[std::make_tuple(state,next_state,m_nodes,m_threads)].eval(m_sz);

            // find our 'local' ranks
            enQ_rank( evQ, comm2, &comm2_rank );
            enQ_rank( evQ, comm0, &comm0_rank );

            enQ_allreduce( evQ, nullBuf, nullBuf, 8, CHAR, MP::MAX, comm0 );
            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 2:
        {
            int next_state = 3;
            auto msg_size  = msg_model[std::make_tuple(state,m_nodes,m_threads)].eval(m_sz);
            auto exec_time = exec_model[std::make_tuple(state,next_state,m_nodes,m_threads)].eval(m_sz);

            // we do some initialization for state 48 here since we
            // now have the comm0_rank
            if (iter == 0) {
                s_partner_48 = r_partner_48 = comm0_rank;
            }

            enQ_allreduce( evQ, nullBuf, nullBuf, msg_size, CHAR, MP::MAX, comm0 );
            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 3:
        {
            int next_state = 4;
            auto exec_time = exec_model[std::make_tuple(state,next_state,m_nodes,m_threads)].eval(m_sz);

            enQ_barrier( evQ, comm1 );
            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 4:
        {
            int next_state = 5;
            auto msg_size  = msg_model[std::make_tuple(state,m_nodes,m_threads)].eval(m_sz);
            auto exec_time = exec_model[std::make_tuple(state,next_state,m_nodes,m_threads)].eval(m_sz);

            enQ_allreduce( evQ, nullBuf, nullBuf, msg_size, CHAR, MP::MAX, comm1 );
            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 5:
        {
            int next_state = 6;
            auto msg_size  = msg_model[std::make_tuple(state,m_nodes,m_threads)].eval(m_sz);
            auto exec_time = exec_model[std::make_tuple(state,next_state,m_nodes,m_threads)].eval(m_sz);

            enQ_allreduce( evQ, nullBuf, nullBuf, msg_size, CHAR, MP::MAX, comm0 );
            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 6:
        {
            int next_state = 7;
            auto msg_size  = msg_model[std::make_tuple(state,m_nodes,m_threads)].eval(m_sz);
            auto exec_time = exec_model[std::make_tuple(state,next_state,m_nodes,m_threads)].eval(m_sz);

            enQ_bcast( evQ, nullBuf, msg_size, CHAR, 0, comm1 );
            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 7:
        {
            int next_state = 8;
            auto msg_size  = msg_model[std::make_tuple(state,m_nodes,m_threads)].eval(m_sz);
            auto exec_time = exec_model[std::make_tuple(state,next_state,m_nodes,m_threads)].eval(m_sz);

            enQ_bcast( evQ, nullBuf, msg_size, CHAR, 0, comm1 );
            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 8:
        {
            int next_state = 9;
            auto msg_size  = msg_model[std::make_tuple(state,m_nodes,m_threads)].eval(m_sz);
            auto exec_time = exec_model[std::make_tuple(state,next_state,m_nodes,m_threads)].eval(m_sz);

            enQ_bcast( evQ, nullBuf, msg_size, CHAR, 0, comm1 );
            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 9:
        {
            int next_state = 10;
            auto msg_size  = msg_model[std::make_tuple(state,m_nodes,m_threads)].eval(m_sz);
            auto exec_time = exec_model[std::make_tuple(state,next_state,m_nodes,m_threads)].eval(m_sz);

            enQ_bcast( evQ, nullBuf, msg_size, CHAR, 0, comm1 );
            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 10:
        {
            int next_state = 11;
            auto exec_time = exec_model[std::make_tuple(state,next_state,m_nodes,m_threads)].eval(m_sz);

            // comm_dup
            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 11:
        {
            int next_state = 12;
            auto exec_time = exec_model[std::make_tuple(state,next_state,m_nodes,m_threads)].eval(m_sz);

            // comm_dup
            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 12:
        {
            int next_state = 13;
            auto exec_time = exec_model[std::make_tuple(state,next_state,m_nodes,m_threads)].eval(m_sz);

            // comm_dup
            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 13:
        {
            int next_state = 14;
            auto exec_time = exec_model[std::make_tuple(state,next_state,m_nodes,m_threads)].eval(m_sz);

            // comm_dup
            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 14:
        {
            int next_state = 15;
            auto msg_size  = msg_model[std::make_tuple(state,m_nodes,m_threads)].eval(m_sz);
            auto exec_time = exec_model[std::make_tuple(state,next_state,m_nodes,m_threads)].eval(m_sz);

            enQ_sendrecv( evQ,
                          nullBuf, msg_size, CHAR, opposite, 0, //send (tag=0)
                          nullBuf, msg_size, CHAR, opposite, 0, //recv (tag=0)
                          GroupWorld, &m_resp );

            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 15: //COMMTODO
        {
            int next_state = 16;
            auto msg_size  = msg_model[std::make_tuple(state,m_nodes,m_threads)].eval(m_sz);
            auto exec_time = exec_model[std::make_tuple(state,next_state,m_nodes,m_threads)].eval(m_sz);

            enQ_allgather( evQ,
                           nullBuf, msg_size, CHAR,
                           nullBuf, msg_size, CHAR,
                           comm2);

            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 16: //COMMTODO
        {
            int next_state = 17;
            auto msg_size  = msg_model[std::make_tuple(state,m_nodes,m_threads)].eval(m_sz);
            auto exec_time = exec_model[std::make_tuple(state,next_state,m_nodes,m_threads)].eval(m_sz);

            enQ_sendrecv( evQ,
                          nullBuf, msg_size, CHAR, opposite, 0, //send (tag=0)
                          nullBuf, msg_size, CHAR, opposite, 0, //recv (tag=0)
                          GroupWorld, &m_resp );

            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 17:
        {
            int next_state = 18;
            auto exec_time = exec_model[std::make_tuple(state,next_state,m_nodes,m_threads)].eval(m_sz);

            // comm_dup
            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 18:
        {
            int next_state = 19;
            auto exec_time = exec_model[std::make_tuple(state,next_state,m_nodes,m_threads)].eval(m_sz);

            // comm_dup
            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 19:
        {
            int next_state = 20;
            auto exec_time = exec_model[std::make_tuple(state,next_state,m_nodes,m_threads)].eval(m_sz);

            // comm_dup
            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 20:
        {
            int next_state = 21;
            auto exec_time = exec_model[std::make_tuple(state,next_state,m_nodes,m_threads)].eval(m_sz);

            // comm_dup
            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 21:
        {
            int next_state = 22;
            auto msg_size  = msg_model[std::make_tuple(state,m_nodes,m_threads)].eval(m_sz);
            auto exec_time = exec_model[std::make_tuple(state,next_state,m_nodes,m_threads)].eval(m_sz);

            enQ_allgather( evQ,
                           nullBuf, msg_size, CHAR,
                           nullBuf, msg_size, CHAR,
                           comm0);

            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 22:
        {
            int next_state = 23;
            auto msg_size  = msg_model[std::make_tuple(state,m_nodes,m_threads)].eval(m_sz);
            auto exec_time = exec_model[std::make_tuple(state,next_state,m_nodes,m_threads)].eval(m_sz);

            enQ_bcast( evQ, nullBuf, msg_size, CHAR, 0, comm0 );
            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 23:
        {
            int next_state = 24;
            auto msg_size  = msg_model[std::make_tuple(state,m_nodes,m_threads)].eval(m_sz);
            auto exec_time = exec_model[std::make_tuple(state,next_state,m_nodes,m_threads)].eval(m_sz);

            enQ_allreduce( evQ, nullBuf, nullBuf, msg_size, CHAR, MP::MAX, comm1 );
            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 24:
        {
            int next_state = 25;
            auto msg_size  = msg_model[std::make_tuple(state,m_nodes,m_threads)].eval(m_sz);
            auto exec_time = exec_model[std::make_tuple(state,next_state,m_nodes,m_threads)].eval(m_sz);

            enQ_allreduce( evQ, nullBuf, nullBuf, msg_size, CHAR, MP::MAX, comm1 );
            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 25:
        {
            int next_state = 26;
            auto msg_size  = msg_model[std::make_tuple(state,m_nodes,m_threads)].eval(m_sz);
            auto exec_time = exec_model[std::make_tuple(state,next_state,m_nodes,m_threads)].eval(m_sz);

            enQ_allreduce( evQ, nullBuf, nullBuf, msg_size, CHAR, MP::MAX, comm0 );
            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 26:
        {
            int next_state = 27;
            auto msg_size  = msg_model[std::make_tuple(state,m_nodes,m_threads)].eval(m_sz);
            auto exec_time = exec_model[std::make_tuple(state,next_state,m_nodes,m_threads)].eval(m_sz);

            enQ_sendrecv( evQ,
                          nullBuf, msg_size, CHAR, opposite, 0, //send (tag=0)
                          nullBuf, msg_size, CHAR, opposite, 0, //recv (tag=0)
                          GroupWorld, &m_resp );

            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 27:
        {
            int next_state = 28;
            auto msg_size  = msg_model[std::make_tuple(state,m_nodes,m_threads)].eval(m_sz);
            auto exec_time = exec_model[std::make_tuple(state,next_state,m_nodes,m_threads)].eval(m_sz);

            enQ_sendrecv( evQ,
                          nullBuf, msg_size, CHAR, opposite, 0, //send (tag=0)
                          nullBuf, msg_size, CHAR, opposite, 0, //recv (tag=0)
                          GroupWorld, &m_resp );

            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 28:
        {
            int next_state = 29;
            auto msg_size  = msg_model[std::make_tuple(state,m_nodes,m_threads)].eval(m_sz);
            auto exec_time = exec_model[std::make_tuple(state,next_state,m_nodes,m_threads)].eval(m_sz);

            enQ_sendrecv( evQ,
                          nullBuf, msg_size, CHAR, opposite, 0, //send (tag=0)
                          nullBuf, msg_size, CHAR, opposite, 0, //recv (tag=0)
                          GroupWorld, &m_resp );

            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 29:
        {
            int next_state = 30;
            auto msg_size  = msg_model[std::make_tuple(state,m_nodes,m_threads)].eval(m_sz);
            auto exec_time = exec_model[std::make_tuple(state,next_state,m_nodes,m_threads)].eval(m_sz);

            enQ_sendrecv( evQ,
                          nullBuf, msg_size, CHAR, opposite, 0, //send (tag=0)
                          nullBuf, msg_size, CHAR, opposite, 0, //recv (tag=0)
                          GroupWorld, &m_resp );

            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 30:
        {
            int next_state = 31;
            auto msg_size  = msg_model[std::make_tuple(state,m_nodes,m_threads)].eval(m_sz);
            auto exec_time = exec_model[std::make_tuple(state,next_state,m_nodes,m_threads)].eval(m_sz);

            enQ_allgather( evQ,
                           nullBuf, msg_size, CHAR,
                           nullBuf, msg_size, CHAR,
                           comm2);

            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 31:
        {
            int next_state = 32;
            auto msg_size  = msg_model[std::make_tuple(state,m_nodes,m_threads)].eval(m_sz);
            auto exec_time = exec_model[std::make_tuple(state,next_state,m_nodes,m_threads)].eval(m_sz);

            int csize = comm2_ranks.size();
            if (0 == recvCounts_31) {
                recvCounts_31 = new int[csize];
            }
            std::fill_n(recvCounts_31, csize, msg_size);

            enQ_allgatherv( evQ,
                            nullBuf, msg_size, CHAR,
                            nullBuf, recvCounts_31, nullDispMap, CHAR,
                            comm2 );

            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 32:
        {
            int next_state = 33;
            auto msg_size  = msg_model[std::make_tuple(state,m_nodes,m_threads)].eval(m_sz);
            auto exec_time = exec_model[std::make_tuple(state,next_state,m_nodes,m_threads)].eval(m_sz);

            enQ_bcast( evQ, nullBuf, msg_size, CHAR, 0, comm2 );
            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 33:
        {
            int next_state = 34;
            auto msg_size  = msg_model[std::make_tuple(state,m_nodes,m_threads)].eval(m_sz);
            auto exec_time = exec_model[std::make_tuple(state,next_state,m_nodes,m_threads)].eval(m_sz);

            enQ_allreduce( evQ, nullBuf, nullBuf, msg_size, CHAR, MP::MAX, comm2 );
            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 34:
        {
            int next_state = 35;
            auto msg_size  = msg_model[std::make_tuple(state,m_nodes,m_threads)].eval(m_sz);
            auto exec_time = exec_model[std::make_tuple(state,next_state,m_nodes,m_threads)].eval(m_sz);

            enQ_alltoall( evQ,
                          nullBuf, msg_size, CHAR,
                          nullBuf, msg_size, CHAR, comm0 );

            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 35:
        {
            int next_state = 36;
            auto msg_size  = msg_model[std::make_tuple(state,m_nodes,m_threads)].eval(m_sz);
            auto exec_time = exec_model[std::make_tuple(state,next_state,m_nodes,m_threads)].eval(m_sz);

            int csize = comm0_ranks.size();
            if (0 == sendCounts_35) {
                sendCounts_35 = new int[csize];
                recvCounts_35 = new int[csize];
            }
            std::fill_n(sendCounts_35, csize, msg_size);
            std::fill_n(recvCounts_35, csize, msg_size);

            enQ_alltoallv( evQ,
                           nullBuf, sendCounts_35, nullDispMap, CHAR,
                           nullBuf, recvCounts_35, nullDispMap, CHAR,
                           comm0 );

            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 36:
        {
            int next_state = 37;
            auto msg_size  = msg_model[std::make_tuple(state,m_nodes,m_threads)].eval(m_sz);
            auto exec_time = exec_model[std::make_tuple(state,next_state,m_nodes,m_threads)].eval(m_sz);

            // Reuse buffers from 35
            // They should be allocated but we'll check just in case
            int csize = comm0_ranks.size();
            if (0 == sendCounts_35) {
                sendCounts_35 = new int[csize];
                recvCounts_35 = new int[csize];
            }
            std::fill_n(sendCounts_35, csize, msg_size);
            std::fill_n(recvCounts_35, csize, msg_size);

            enQ_alltoallv( evQ,
                           nullBuf, sendCounts_35, nullDispMap, CHAR,
                           nullBuf, recvCounts_35, nullDispMap, CHAR,
                           comm0 );

            double szScale = pow(1.5074,(m_sz-18.0));
            double nodeScale = pow((double(size())/16.0), -0.5);
            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 37:
        {
            int next_state = 38;
            auto msg_size  = msg_model[std::make_tuple(state,m_nodes,m_threads)].eval(m_sz);
            auto exec_time = exec_model[std::make_tuple(state,next_state,m_nodes,m_threads)].eval(m_sz);

            enQ_allreduce( evQ, nullBuf, nullBuf, msg_size, CHAR, MP::MAX, comm1 );
            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 38:
        {
            int next_state = 39;
            auto msg_size  = msg_model[std::make_tuple(state,m_nodes,m_threads)].eval(m_sz);
            auto exec_time = exec_model[std::make_tuple(state,next_state,m_nodes,m_threads)].eval(m_sz);

            enQ_allreduce( evQ, nullBuf, nullBuf, msg_size, CHAR, MP::MAX, comm1 );
            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 39:
        {
            int next_state;

            // the next state is determined by a pattern for each
            //iteration. It goes 25, 40 then a 67% chance of 25 again,
            //then 51.
            switch(idx_39) {
            case 0:
                next_state = 25;
                break;
            case 1:
                next_state = 40;
                break;
            case 2:
                if (rng->nextUniform() <= 0.67) {
                    next_state = 25;
                } else {
                    next_state = 51;
                }
                break;
            case 3:
                    next_state = 51;
                    break;
            }
            idx_39++;

            auto msg_size  = msg_model[std::make_tuple(state,m_nodes,m_threads)].eval(m_sz);
            auto exec_time = exec_model[std::make_tuple(state,next_state,m_nodes,m_threads)].eval(m_sz);

            enQ_allreduce( evQ, nullBuf, nullBuf, msg_size, CHAR, MP::MAX, comm1 );
            enQ_compute(evQ, exec_time);


            state = next_state;

        }
        break;
    case 40:
        {
            int next_state = 41;
            auto msg_size  = msg_model[std::make_tuple(state,m_nodes,m_threads)].eval(m_sz);
            auto exec_time = exec_model[std::make_tuple(state,next_state,m_nodes,m_threads)].eval(m_sz);

            enQ_barrier(evQ,comm1);
            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 41:
        {
            int next_state = 42;
            auto msg_size  = msg_model[std::make_tuple(state,m_nodes,m_threads)].eval(m_sz);
            auto exec_time = exec_model[std::make_tuple(state,next_state,m_nodes,m_threads)].eval(m_sz);

            enQ_sendrecv( evQ,
                          nullBuf, msg_size, CHAR, opposite, 0, //send (tag=0)
                          nullBuf, msg_size, CHAR, opposite, 0, //recv (tag=0)
                          GroupWorld, &m_resp );

            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 42:
        {
            int next_state = 43;
            auto msg_size  = msg_model[std::make_tuple(state,m_nodes,m_threads)].eval(m_sz);
            auto exec_time = exec_model[std::make_tuple(state,next_state,m_nodes,m_threads)].eval(m_sz);

            enQ_bcast( evQ, nullBuf, msg_size, CHAR, 0, comm2 );
            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 43:
        {
            int next_state = 44;
            auto msg_size  = msg_model[std::make_tuple(state,m_nodes,m_threads)].eval(m_sz);
            auto exec_time = exec_model[std::make_tuple(state,next_state,m_nodes,m_threads)].eval(m_sz);

            enQ_bcast( evQ, nullBuf, msg_size, CHAR, 0, comm0 );
            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 44:
        {
            int next_state = 45;
            auto msg_size  = msg_model[std::make_tuple(state,m_nodes,m_threads)].eval(m_sz);
            auto exec_time = exec_model[std::make_tuple(state,next_state,m_nodes,m_threads)].eval(m_sz);

            enQ_sendrecv( evQ,
                          nullBuf, msg_size, CHAR, opposite, 0, //send(tag=0)
                          nullBuf, msg_size, CHAR, opposite, 0, //recv(tag=0)
                          GroupWorld, &m_resp );

            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 45:
        {
            int next_state = 46;
            auto msg_size  = msg_model[std::make_tuple(state,m_nodes,m_threads)].eval(m_sz);
            auto exec_time = exec_model[std::make_tuple(state,next_state,m_nodes,m_threads)].eval(m_sz);
            //this is an allgatherv, but we
            // treat it as an allgather because the variation is very
            // small.
            enQ_allgather( evQ,
                           nullBuf, msg_size, CHAR,
                           nullBuf, msg_size, CHAR,
                           comm2);

            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 46:
        {
            int next_state = 47;
            auto msg_size  = msg_model[std::make_tuple(state,m_nodes,m_threads)].eval(m_sz);
            auto exec_time = exec_model[std::make_tuple(state,next_state,m_nodes,m_threads)].eval(m_sz);

            enQ_allgather( evQ,
                           nullBuf, msg_size, CHAR,
                           nullBuf, msg_size, CHAR,
                           comm2);

            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 47:
        {
            int next_state = 48;
            auto msg_size  = msg_model[std::make_tuple(state,m_nodes,m_threads)].eval(m_sz);
            auto exec_time = exec_model[std::make_tuple(state,next_state,m_nodes,m_threads)].eval(m_sz);

            // shift within row
            // make sure send and recv partner should be the same
            int s_partner = (comm0_rank + 1) % comm0_ranks.size();
            int r_partner = (comm0_rank - 1);
            if (r_partner < 0) {
                r_partner = comm0_ranks.size()-1;
            }

            enQ_sendrecv( evQ,
                          nullBuf, msg_size, CHAR, s_partner, 0, //send (tag=0)
                          nullBuf, msg_size, CHAR, r_partner, 0, //recv (tag=0)
                          comm0, &m_resp );

            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 48:
        {
            int next_state = 49;
            auto msg_size  = msg_model[std::make_tuple(state,m_nodes,m_threads)].eval(m_sz);
            auto exec_time = exec_model[std::make_tuple(state,next_state,m_nodes,m_threads)].eval(m_sz);

            enQ_sendrecv( evQ,
                          nullBuf, msg_size, CHAR, s_partner_48, 0, //send (tag=0)
                          nullBuf, msg_size, CHAR, r_partner_48, 0, //recv (tag=0)
                          comm0, &m_resp );


            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 49:
        {
            int next_state = -1;

            auto msg_size  = msg_model[std::make_tuple(state,m_nodes,m_threads)].eval(m_sz);
            int msgElem;

            enQ_sendrecv( evQ,
                          nullBuf, msg_size, CHAR, s_partner_48, 0,//snd(tag=0)
                          nullBuf, msg_size, CHAR, r_partner_48, 0,//rcv(tag=0)
                          comm0, &m_resp );

            // iterate through row, starting with ourself
            // advance iterators
            s_partner_48 =  (s_partner_48 + 1) % comm0_ranks.size();
            r_partner_48  = r_partner_48 - 1;
            if (r_partner_48  < 0) {
                r_partner_48 = comm0_ranks.size()-1;
            }


            // goes 47 X times (where X is (sqrt(nodes)-1)) then 50
            if (idx_49 == (square-1)) {
                next_state = 50;
                auto exec_time = exec_model[std::make_tuple(state,next_state,m_nodes,m_threads)].eval(m_sz);
                enQ_compute(evQ, exec_time);

                idx_49 = 0;
            } else {
                next_state = 47;
                auto exec_time = exec_model[std::make_tuple(state,next_state,m_nodes,m_threads)].eval(m_sz);
                enQ_compute(evQ, exec_time);

                idx_49++;
            }

            state = next_state;
        }
        break;
    case 50:
        {
            // goes to 41 2 (70%) or 3 (30%) times then 25
            int next_state;
            switch(idx_50) {
            case 0:
                next_state = 41;
                idx_50 = 1;
                break;
            case 1:
                next_state = 41;
                idx_50 = 2;
                break;
            case 2:
                if (rng->nextUniform() <= 0.3) {
                    next_state = 41;
                    idx_50 = 3;
                } else {
                    next_state = 25;
                    idx_50 = 0;
                }
                break;
            case 3:
                next_state = 25;
                idx_50 = 0;
                break;
            default:
                printf("idx_50 incorrect (%llu)\n", idx_50);
                break;
            }

            auto msg_size  = msg_model[std::make_tuple(state,m_nodes,m_threads)].eval(m_sz);
            auto exec_time = exec_model[std::make_tuple(state,next_state,m_nodes,m_threads)].eval(m_sz);

            enQ_allreduce( evQ, nullBuf, nullBuf, msg_size, CHAR, MP::MAX, comm1);
            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 51:
        {
            int next_state = 52;
            auto msg_size  = msg_model[std::make_tuple(state,m_nodes,m_threads)].eval(m_sz);
            auto exec_time = exec_model[std::make_tuple(state,next_state,m_nodes,m_threads)].eval(m_sz);

            enQ_barrier(evQ,comm1);
            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 52:
        {
            int next_state = 53;
            auto msg_size  = msg_model[std::make_tuple(state,m_nodes,m_threads)].eval(m_sz);
            auto exec_time = exec_model[std::make_tuple(state,next_state,m_nodes,m_threads)].eval(m_sz);

            enQ_allreduce( evQ, nullBuf, nullBuf, msg_size, CHAR, MP::MAX, comm1);
            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 53:
        {
            int next_state = 54;
            auto msg_size  = msg_model[std::make_tuple(state,m_nodes,m_threads)].eval(m_sz);
            auto exec_time = exec_model[std::make_tuple(state,next_state,m_nodes,m_threads)].eval(m_sz);

            enQ_allreduce( evQ, nullBuf, nullBuf, msg_size, CHAR, MP::MAX, comm1);
            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 54:
        {
            int next_state = 1;
            auto msg_size  = msg_model[std::make_tuple(state,m_nodes,m_threads)].eval(m_sz);
            auto exec_time = exec_model[std::make_tuple(state,next_state,m_nodes,m_threads)].eval(m_sz);

            enQ_bcast( evQ, nullBuf, msg_size, CHAR, 0, comm1 );

            // must enque something or it assumes we're done
            enQ_compute(evQ, exec_time); // Old time 20*1000

            // done?
            if (iter == maxIter-1) {
                done = 1;
            } else {
                // start a new iteration
                iter++;
                initIter();
            }
        }
        break;
    default:
        printf("error\n Bad State %llu in rank %d\n", state, rank());
        break;
    }

    if (rank()==0&&0) {
        printf("TRACE %lld -> %lld 0\n", prevState, state);
        prevState = state;
    }

    // signal if we are done
    if (done) {
        return true;
    } else {
        return false;
    }
}
