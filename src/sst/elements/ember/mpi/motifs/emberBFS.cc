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
#include "emberBFS.h"

#include <cmath>

using namespace SST::Ember;

EmberBFSGenerator::EmberBFSGenerator(SST::ComponentId_t id,
                                     Params& params) :
    EmberMessagePassingGenerator(id, params, "BFS"), state(0), sendCount_31(0),
    recvCounts_31(0), sendCounts_35(0), recvCounts_35(0)
{
    m_sz = (int32_t) params.find("arg.sz", 1);
    m_threads = (int32_t) params.find("arg.threads", 1);
    uint32_t rng_seed = (uint32_t) params.find("arg.seed", 1);

    m_ranks_per_node = 1; // TODO: get this from ember config and add to model

    // Parse model files
    std::ifstream comm_file( params.find<std::string>("arg.comm_model", "bfs-comm.model") );
    std::ifstream comp_file( params.find<std::string>("arg.comp_model", "bfs-comp.model") );

    if (not (comp_file.good() and comm_file.good())) {
        out.fatal(CALL_INFO, 1, "Bad model file(s)");
    }

    // Read the communication size model into a SharedMap
    // Each line of the file is
    //   callsite coeff1 coeff2 coeff3 [...]
    std::string line;
    int skip_first = true;
    while (getline(comm_file, line)) {
        if (skip_first) {
            skip_first = false;
            continue;
        }
        std::istringstream iss(line);
        int cs, num_nodes, threads_per_rank;
        iss >> cs >> num_nodes >> threads_per_rank;
        std::vector<double> coeff((std::istream_iterator<double>(iss)),
                                  std::istream_iterator<double>());

        //comm_model.write(cs, PolyModel(coeff));
        auto key = std::make_tuple(cs, num_nodes, threads_per_rank);
        comm_model.insert(std::pair<std::tuple<int,int,int>,PolyModel>(key,PolyModel(coeff)));
    }

    // Read the compute time model into a SharedMap
    // Each line of the file is
    //   src_callsite dst_callsite coeff1 coeff2 coeff3 [...]
    skip_first = true;
    while (getline(comp_file, line)) {
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
        comp_model.insert(std::pair<std::tuple<int,int,int,int>,PolyModel>(key,PolyModel(coeff)));
        //comp_model.insert(std::pair<std::pair<int,int>,PolyModel>(std::pair<int,int>(src_cs, dst_cs), PolyModel(coeff)));
    }

    // Print out the first five models to see if they look right
    /*
    if (rank() == 0) {
        int counter = 0;
        std::cout << "COMM MODELS" << std::endl;
        for (auto const& [key, pm] : comm_model) {
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
        for (auto const& [key, pm] : comp_model) {
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
    rng_rank = new SST::RNG::MersenneRNG(rng_seed+rank());
    rng_opposite
        = new SST::RNG::MersenneRNG(rng_seed
                                    + ((opposite<rank()) ? opposite : rank()));
    rng_comm0
        = new SST::RNG::MersenneRNG(rng_seed + myRow);
    rng_comm2
        = new SST::RNG::MersenneRNG(rng_seed + myCol);


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
    delete rng_rank;
    delete rng_opposite;
    delete rng_comm0;
    delete rng_comm2;
    delete nullDispMap;
}

void EmberBFSGenerator::initIter() {
    prevState = -1;
    state = 1;
    idx_39 = 0;
    idx_49 = 0;
    idx_50 = 0;
}

int EmberBFSGenerator::msgSize_29(double meanSize) {
    // piecewise. if < 0.8 use pareto for "small messages', if >=0.8
    // use other distribution
    double roll = rng_opposite->nextUniform();
    if (roll < 0.8) {
        // pareto
        return int(1.0/pow(1.0-(roll*1.25),1.0/1.5625)-0.5);
    } else {
        // other distribution
        return int((pow(2.0,roll)-1.7411)*45.96*meanSize);
    }
}

void EmberBFSGenerator::computeSR_29(int &sendElem, int &recvElem) {
    // assumptions:
    // mean message size increas 1.75x per graph size
    // mean message size scales inversely with #nodes
    double meanSize = 76.3; // size 22, 4 nodes in elements
    // adjust for graph size
    meanSize *= pow(1.75, double(int(m_sz)-22.0));
    // adjsut for # nodes
    meanSize *= (4.0 / size());

    /*if (rank() == 0) printf("meanSize sz%d ranks%d %.1f\n", m_sz, size(),
      meanSize);*/

    // generate two message sizes
    int a = msgSize_29(meanSize);
    int b = msgSize_29(meanSize);

    // assign send and recv
    if (rank() < opposite) {
        sendElem = a;
        recvElem = b;
        //printf("X %d\n", a);
        //printf("X %d\n", b);
    } else if (rank() == opposite) {
        sendElem = recvElem = a;
        //printf("X %d\n", a);
    } else {
        sendElem = b;
        recvElem = a;
    }

    //printf("%d: send:%d recv%d\n", rank(), sendElem, recvElem);
}

int EmberBFSGenerator::msgSize_31(double meanSize, double initRoll) {
    // two pieces. if <0.8 use pareto for small messages. if >0.8 use
    // exponential for large messages

    // communications are correlated, so the roll we use is based on a
    // small deelta from the original
    double roll = (rng_comm2->nextUniform()*0.05 + initRoll*0.95);
    int ret;
    if (roll < 0.8) {
        // pareto
        roll *= 1.25; // expand from 0 to 1
        //printf("p");
        ret = int(1.0/(pow(1.0-roll,1.0/1.7299))-1.0);
    } else {
        roll = (roll-0.8)*5.0;
        //printf("x(%.1f)", log(roll));
        ret = int(-meanSize*log(roll));
    }
    //printf("%d %.1f %.3f %.3f %d\n", rank(), meanSize, roll, initRoll, ret);
    return ret;
}

void EmberBFSGenerator::computeAG_31() {
    int csize = comm2_ranks.size();
    if (0 == recvCounts_31) {
        // allocate if needed
        recvCounts_31 = new int[csize];
    }

    //zero memory
    sendCount_31 = 0;
    std::memset(recvCounts_31, 0, sizeof(recvCounts_31[0])*csize);

    // assumptions for mean 'large' message:
    // mean message size increas 1.75x per graph size
    // mean message size scales inversely with #nodes
    double meanSize = 357.227; // size 22, 4 nodes in elements
    // adjust for graph size
    meanSize *= pow(1.75, double(m_sz-22.0));
    // adjsut for # nodes
    meanSize *= (4.0 / size());

    // figure out recvs
    double initRoll = rng_comm2->nextUniform();
    for (int i = 0;i < csize;++i) {
        recvCounts_31[i] = msgSize_31(meanSize, initRoll);
    }
    sendCount_31 = recvCounts_31[comm2_rank];

#if 0
    printf("%d/%d_31: ", rank(), comm2_rank);
    for (int i = 0;i < csize;++i) {
        printf(" %d", recvCounts_31[i]);
    }
    printf("\n");
#endif
}

int EmberBFSGenerator::msgSize_35(double meanSize, double initRoll) {
    //two pieces. if <0.55 use exponential for small messages. if >, use
    //log-uniform

    // communications are corelated
    double roll = (rng_comm0->nextUniform()*0.03 + initRoll*0.97);
    int ret;
    if (roll < 0.55) {
        ret = int(-log(1-roll)/0.7715);
    } else {
        roll = roll - 0.55;
        roll /= 0.45;
        double log2size = roll*11.262+4.284+meanSize;
        ret = int(pow(2.0,log2size));
    }

    return ret;
}

void EmberBFSGenerator::computeAA_35() {
    int csize = comm0_ranks.size();
    if (0 == recvCounts_35) {
        // allocate if needed
        recvCounts_35 = new int[csize];
        sendCounts_35 = new int[csize];
    }

    // zero memory
    std::memset(sendCounts_35, 0, sizeof(sendCounts_35[0])*csize);
    std::memset(recvCounts_35, 0, sizeof(recvCounts_35[0])*csize);

    // assumptions for mean 'large' message:
    // mean message size increas 1.75x per graph size
    // mean message size scales inversely with #nodes
    double meanSize = 1; // size 22, 4 nodes in elements
    // adjust for graph size
    meanSize *= pow(1.75, double(m_sz-22.0));
    // adjsut for # nodes
    meanSize *= (4.0 / size());
    meanSize = log(meanSize)/log(2.0);

    // generate send array: how much each person sends to each other.
    // we generate this on each rank, even though it should be the
    // same for all.
    int sendArray[csize][csize];

    double initRoll = rng_comm0->nextUniform();
    for(int i=0;i<csize;++i) {
        for(int j=0;j<csize;++j) {
            // [sender][reciever]
            sendArray[i][j] = msgSize_35(meanSize, initRoll);
        }
    }

    // compute our send count
    for(int i=0;i<csize;++i) {
        sendCounts_35[i] = sendArray[comm0_rank][i];
    }
    // compute our recieve count
    for(int i=0;i<csize;++i) {
        recvCounts_35[i] = sendArray[i][comm0_rank];
    }

#if 0
    if (rank() == 0||1) {
        for(int i=0;i<csize;++i) {
            for(int j=0;j<csize;++j) {
                printf("%7d ", sendArray[i][j]);
            }
            printf("\n");
        }
        printf("Send: ");
        for(int i=0;i<csize;++i) {
            printf("%7d ", sendCounts_35[i]);
        }
        printf("\nRecv: ");
        for(int i=0;i<csize;++i) {
            printf("%7d ", recvCounts_35[i]);
        }
        printf("\n");
    }
#endif
}

void EmberBFSGenerator::computeSR_49(int &msgElem) {
    // three pices
    double roll = rng_comm0->nextUniform();

    double log2Sz;
    if (roll < 0.12) {
        log2Sz = 3.29857*pow(3368.147,roll); // based on 4x1-22
    } else if (roll < 0.4) {
        log2Sz = 7.8796+roll*16.7359;
    } else { //>0.4
        log2Sz = 14.754+roll*8.15658;
    }

    // adjusts for # ranks, graph size
    // 1.926x per graph sz log_2 = .945683
    log2Sz += (.945683 * double(int(m_sz)-22));
    // adjust for num nodes
    log2Sz += log(4/size())/log(2);

    //adjust for doubles
    msgElem = pow(2.0, log2Sz) / 8.0;
}

bool EmberBFSGenerator::generate( std::queue<EmberEvent*>& evQ) {
    bool done = 0;

    enQ_getTime( evQ, &s_time );

    if (rank() == 0 && state == 54) {
        /*
        printf("Rank %d Iter = %lld State = %lld Time=%llu\n", rank(),
               iter, state, s_time);
               */
    }

    // initial timings based on 16x1 sz18

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
            auto msg_size  = (int)comm_model[std::make_tuple(state,m_ranks_per_node,m_threads)].eval(m_sz);
            auto exec_time = comp_model[std::make_tuple(state,next_state,m_ranks_per_node,m_threads)].eval(m_sz);

            // find our 'local' ranks
            enQ_rank( evQ, comm2, &comm2_rank );
            enQ_rank( evQ, comm0, &comm0_rank );
            //enQ_Allreduce(evQ,comm0);
            // don't know what the actual operator is
            enQ_allreduce( evQ, nullBuf, nullBuf, 1, DOUBLE, MP::MAX, comm0 );

            enQ_compute(evQ,49*1000);

            if (rank() == 0 && iter==0) {
                std::cout << "Rank " << rank() << ": " \
                    << "(" << state << "->" << next_state  << ")" \
                    << " old: " << 49*1000 << ", new: " << exec_time << std::endl;
            }

            state = next_state;
        }
        break;
    case 2:
        {
            int next_state = 3;
            auto msg_size  = (int)comm_model[std::make_tuple(state,m_ranks_per_node,m_threads)].eval(m_sz);
            auto exec_time = comp_model[std::make_tuple(state,next_state,m_ranks_per_node,m_threads)].eval(m_sz);

            // we do some initialization for state 48 here since we
            // now have the comm0_rank
            if (iter == 0) {
                s_partner_48 = r_partner_48 = comm0_rank;
            }

            enQ_allreduce( evQ, nullBuf, nullBuf, 1, DOUBLE, MP::MAX, comm0 );
            enQ_compute(evQ,28*1000);

            if (rank() == 0 && iter == 0) {
                std::cout << "Rank " << rank() << ": " \
                    << "(" << state << "->" << next_state  << ")" \
                    << " old: " << 28*1000 << ", new: " << exec_time << std::endl;
            }

            state = next_state;
        }
        break;
    case 3:
        {
            int next_state = 4;
            //auto msg_size  = (int)comm_model[std::make_tuple(state,m_ranks_per_node,m_threads)].eval(m_sz);
            auto exec_time = comp_model[std::make_tuple(state,next_state,m_ranks_per_node,m_threads)].eval(m_sz);

            enQ_barrier( evQ, comm1 );
            enQ_compute(evQ,18*1000);

            if (rank() == 0 && iter==0) {
                std::cout << "Rank " << rank() << ": " \
                    << "(" << state << "->" << next_state  << ")" \
                    << " old: " << 18*1000 << ", new: " << exec_time << std::endl;
            }

            state = next_state;
        }
        break;
    case 4:
        {
            int next_state = 5;
            auto msg_size  = (int)comm_model[std::make_tuple(state,m_ranks_per_node,m_threads)].eval(m_sz);
            auto exec_time = comp_model[std::make_tuple(state,next_state,m_ranks_per_node,m_threads)].eval(m_sz);

            enQ_allreduce( evQ, nullBuf, nullBuf, 1, DOUBLE, MP::MAX, comm1 );
            enQ_compute(evQ,17*1000);

            if (rank() == 0 && iter==0) {
                std::cout << "Rank " << rank() << ": " \
                    << "(" << state << "->" << next_state  << ")" \
                    << " old: " << 17*1000 << ", new: " << exec_time << std::endl;
            }

            state = next_state;
        }
        break;
    case 5:
        {
            int next_state = 6;
            auto msg_size  = (int)comm_model[std::make_tuple(state,m_ranks_per_node,m_threads)].eval(m_sz);
            auto exec_time = comp_model[std::make_tuple(state,next_state,m_ranks_per_node,m_threads)].eval(m_sz);

            enQ_allreduce( evQ, nullBuf, nullBuf, 1, DOUBLE, MP::MAX, comm0 );
            enQ_compute(evQ,18*1000);

            state = next_state;
        }
        break;
    case 6:
        {
            int next_state = 7;
            auto msg_size  = (int)comm_model[std::make_tuple(state,m_ranks_per_node,m_threads)].eval(m_sz);
            auto exec_time = comp_model[std::make_tuple(state,next_state,m_ranks_per_node,m_threads)].eval(m_sz);

            enQ_bcast( evQ, nullBuf, 1, INT, 0, comm1 );
            assert(sizeofDataType(INT)==4);

            enQ_compute(evQ,15*1000);

            state = next_state;
        }
        break;
    case 7:
        {
            int next_state = 8;
            auto msg_size  = (int)comm_model[std::make_tuple(state,m_ranks_per_node,m_threads)].eval(m_sz);
            auto exec_time = comp_model[std::make_tuple(state,next_state,m_ranks_per_node,m_threads)].eval(m_sz);

            enQ_bcast( evQ, nullBuf, 1, DOUBLE, 0, comm1 );
            enQ_compute(evQ,17*1000);

            state = next_state;
        }
        break;
    case 8:
        {
            int next_state = 9;
            auto msg_size  = (int)comm_model[std::make_tuple(state,m_ranks_per_node,m_threads)].eval(m_sz);
            auto exec_time = comp_model[std::make_tuple(state,next_state,m_ranks_per_node,m_threads)].eval(m_sz);

            enQ_bcast( evQ, nullBuf, 1, DOUBLE, 0, comm1 );
            enQ_compute(evQ,16*1000);

            state = next_state;
        }
        break;
    case 9:
        {
            int next_state = 10;
            auto msg_size  = (int)comm_model[std::make_tuple(state,m_ranks_per_node,m_threads)].eval(m_sz);
            auto exec_time = comp_model[std::make_tuple(state,next_state,m_ranks_per_node,m_threads)].eval(m_sz);

            enQ_bcast( evQ, nullBuf, 1, DOUBLE, 0, comm1 );
            enQ_compute(evQ,114*1000);

            if (rank() == 0 && iter==0) {
                std::cout << "Rank " << rank() << ": " \
                    << "(" << state << "->" << next_state  << ")" \
                    << " old: " << 114*1000 << ", new: " << exec_time << std::endl;
            }

            state = next_state;
        }
        break;
    case 10:
        {
            int next_state = 11;
            //auto msg_size  = (int)comm_model[std::make_tuple(state,m_ranks_per_node,m_threads)].eval(m_sz);
            auto exec_time = comp_model[std::make_tuple(state,next_state,m_ranks_per_node,m_threads)].eval(m_sz);

            // comm_dup
            enQ_compute(evQ,38*1000);

            state = next_state;
        }
        break;
    case 11:
        {
            int next_state = 12;
            auto exec_time = comp_model[std::make_tuple(state,next_state,m_ranks_per_node,m_threads)].eval(m_sz);

            // comm_dup
            enQ_compute(evQ,40*1000);

            state = next_state;
        }
        break;
    case 12:
        {
            int next_state = 13;
            auto exec_time = comp_model[std::make_tuple(state,next_state,m_ranks_per_node,m_threads)].eval(m_sz);

            // comm_dup
            enQ_compute(evQ,40*1000);

            state = next_state;
        }
        break;
    case 13:
        {
            int next_state = 14;
            auto exec_time = comp_model[std::make_tuple(state,next_state,m_ranks_per_node,m_threads)].eval(m_sz);

            // comm_dup
            enQ_compute(evQ,17*1000);

            state = next_state;
        }
        break;
    case 14:
        {
            int next_state = 15;
            auto msg_size  = (int)comm_model[std::make_tuple(state,m_ranks_per_node,m_threads)].eval(m_sz);
            auto exec_time = comp_model[std::make_tuple(state,next_state,m_ranks_per_node,m_threads)].eval(m_sz);

            enQ_sendrecv( evQ,
                          nullBuf, 1, INT, opposite, 0, //send (tag=0)
                          nullBuf, 1, INT, opposite, 0, //recv (tag=0)
                          GroupWorld, &m_resp );

            enQ_compute(evQ,16*1000);

            state = next_state;
        }
        break;
    case 15:
        {
            int next_state = 16;
            auto msg_size  = (int)comm_model[std::make_tuple(state,m_ranks_per_node,m_threads)].eval(m_sz);
            auto exec_time = comp_model[std::make_tuple(state,next_state,m_ranks_per_node,m_threads)].eval(m_sz);

            enQ_allgather( evQ,
                           nullBuf, 1, INT,
                           nullBuf, 1, INT,
                           comm2);

            enQ_compute(evQ,15*1000);

            state = next_state;
        }
        break;
    case 16:
        {
            int next_state = 17;
            auto msg_size  = (int)comm_model[std::make_tuple(state,m_ranks_per_node,m_threads)].eval(m_sz);
            auto exec_time = comp_model[std::make_tuple(state,next_state,m_ranks_per_node,m_threads)].eval(m_sz);

            enQ_sendrecv( evQ,
                          nullBuf, 1, INT, opposite, 0, //send (tag=0)
                          nullBuf, 1, INT, opposite, 0, //recv (tag=0)
                          GroupWorld, &m_resp );

            enQ_compute(evQ,58*1000);

            state = next_state;
        }
        break;
    case 17:
        {
            int next_state = 18;
            auto exec_time = comp_model[std::make_tuple(state,next_state,m_ranks_per_node,m_threads)].eval(m_sz);

            // comm_dup
            enQ_compute(evQ,37*1000);

            state = next_state;
        }
        break;
    case 18:
        {
            int next_state = 19;
            auto exec_time = comp_model[std::make_tuple(state,next_state,m_ranks_per_node,m_threads)].eval(m_sz);

            // comm_dup
            enQ_compute(evQ,38*1000);

            state = next_state;
        }
        break;
    case 19:
        {
            int next_state = 20;
            auto msg_size  = (int)comm_model[std::make_tuple(state,m_ranks_per_node,m_threads)].eval(m_sz);
            auto exec_time = comp_model[std::make_tuple(state,next_state,m_ranks_per_node,m_threads)].eval(m_sz);

            enQ_compute(evQ,38*1000);

            state = next_state;
        }
        break;
    case 20:
        {
            int next_state = 21;
            auto exec_time = comp_model[std::make_tuple(state,next_state,m_ranks_per_node,m_threads)].eval(m_sz);

            // comm_dup
            enQ_compute(evQ,17*1000);

            state = next_state;
        }
        break;
    case 21:
        {
            int next_state = 22;
            auto msg_size  = (int)comm_model[std::make_tuple(state,m_ranks_per_node,m_threads)].eval(m_sz);
            auto exec_time = comp_model[std::make_tuple(state,next_state,m_ranks_per_node,m_threads)].eval(m_sz);

            enQ_allgather( evQ,
                           nullBuf, 1, INT,
                           nullBuf, 1, INT,
                           comm0);

            enQ_compute(evQ,16*1000);

            state = next_state;
        }
        break;
    case 22:
        {
            int next_state = 23;
            auto msg_size  = (int)comm_model[std::make_tuple(state,m_ranks_per_node,m_threads)].eval(m_sz);
            auto exec_time = comp_model[std::make_tuple(state,next_state,m_ranks_per_node,m_threads)].eval(m_sz);

            enQ_bcast( evQ, nullBuf, 1, DOUBLE, 0, comm0 );

            double szScale = pow(1.84,(m_sz-18.0));
            double nodeScale = 16.0/double(size());
            double base = adjustUniformRand(1.0, 0.25, rng_rank);
            enQ_compute(evQ,212*1000*szScale*nodeScale*base);

            if (rank() == 0 && iter==0) {
                std::cout << "Rank " << rank() << ": " \
                    << "(" << state << "->" << next_state  << ")" \
                    << " old: " << 212*1000*szScale*nodeScale*base << ", new: " << exec_time << std::endl;
            }

            state = next_state;
        }
        break;
    case 23:
        {
            int next_state = 24;
            auto msg_size  = (int)comm_model[std::make_tuple(state,m_ranks_per_node,m_threads)].eval(m_sz);
            auto exec_time = comp_model[std::make_tuple(state,next_state,m_ranks_per_node,m_threads)].eval(m_sz);

            enQ_allreduce( evQ, nullBuf, nullBuf, 1, DOUBLE, MP::MAX, comm1 );
            enQ_compute(evQ,34*1000);

            if (rank() == 0 && iter==0) {
                std::cout << "Rank " << rank() << ": " \
                    << "(" << state << "->" << next_state  << ")" \
                    << " old: " << 34*1000 << ", new: " << exec_time << std::endl;
            }

            state = next_state;
        }
        break;
    case 24:
        {
            int next_state = 25;
            auto msg_size  = (int)comm_model[std::make_tuple(state,m_ranks_per_node,m_threads)].eval(m_sz);
            auto exec_time = comp_model[std::make_tuple(state,next_state,m_ranks_per_node,m_threads)].eval(m_sz);

            enQ_allreduce( evQ, nullBuf, nullBuf, 1, DOUBLE, MP::MAX, comm1 );
            enQ_compute(evQ,73*1000);

            state = next_state;
        }
        break;
    case 25:
        {
            int next_state = 26;
            auto msg_size  = (int)comm_model[std::make_tuple(state,m_ranks_per_node,m_threads)].eval(m_sz);
            auto exec_time = comp_model[std::make_tuple(state,next_state,m_ranks_per_node,m_threads)].eval(m_sz);

            enQ_allreduce( evQ, nullBuf, nullBuf, 1, DOUBLE, MP::MAX, comm0 );
            enQ_compute(evQ,18*1000);

            state = next_state;
        }
        break;
    case 26:
        {
            int next_state = 27;
            auto msg_size  = (int)comm_model[std::make_tuple(state,m_ranks_per_node,m_threads)].eval(m_sz);
            auto exec_time = comp_model[std::make_tuple(state,next_state,m_ranks_per_node,m_threads)].eval(m_sz);

            enQ_sendrecv( evQ,
                          nullBuf, 1, INT, opposite, 0, //send (tag=0)
                          nullBuf, 1, INT, opposite, 0, //recv (tag=0)
                          GroupWorld, &m_resp );
            enQ_compute(evQ,14*1000);

            if (rank() == 0 && iter==0) {
                std::cout << "Rank " << rank() << ": " \
                    << "(" << state << "->" << next_state  << ")" \
                    << " old: " << 14*1000 << ", new: " << exec_time << std::endl;
            }

            state = next_state;
        }
        break;
    case 27:
        {
            int next_state = 28;
            auto msg_size  = (int)comm_model[std::make_tuple(state,m_ranks_per_node,m_threads)].eval(m_sz);
            auto exec_time = comp_model[std::make_tuple(state,next_state,m_ranks_per_node,m_threads)].eval(m_sz);

            enQ_sendrecv( evQ,
                          nullBuf, 1, INT, opposite, 0, //send (tag=0)
                          nullBuf, 1, INT, opposite, 0, //recv (tag=0)
                          GroupWorld, &m_resp );
            enQ_compute(evQ,14*1000);

            state = next_state;
        }
        break;
    case 28:
        {
            int next_state = 29;
            auto msg_size  = (int)comm_model[std::make_tuple(state,m_ranks_per_node,m_threads)].eval(m_sz);
            auto exec_time = comp_model[std::make_tuple(state,next_state,m_ranks_per_node,m_threads)].eval(m_sz);

            enQ_sendrecv( evQ,
                          nullBuf, 1, DOUBLE, opposite, 0, //send (tag=0)
                          nullBuf, 1, DOUBLE, opposite, 0, //recv (tag=0)
                          GroupWorld, &m_resp );

            enQ_compute(evQ,146*1000);

            state = next_state;
        }
        break;
    case 29:
        {
            int next_state = 30;
            auto msg_size  = (int)comm_model[std::make_tuple(state,m_ranks_per_node,m_threads)].eval(m_sz);
            auto exec_time = comp_model[std::make_tuple(state,next_state,m_ranks_per_node,m_threads)].eval(m_sz);

            computeSR_29(sendElem_29, recvElem_29);
            enQ_sendrecv( evQ,
                          nullBuf, sendElem_29, INT, opposite, 0, //send (tag=0)
                          nullBuf, recvElem_29, INT, opposite, 0, //recv (tag=0)
                          GroupWorld, &m_resp );
            enQ_compute(evQ,22*1000);

            state = next_state;
        }
        break;
    case 30:
        {
            int next_state = 31;
            auto msg_size  = (int)comm_model[std::make_tuple(state,m_ranks_per_node,m_threads)].eval(m_sz);
            auto exec_time = comp_model[std::make_tuple(state,next_state,m_ranks_per_node,m_threads)].eval(m_sz);

            enQ_allgather( evQ,
                           nullBuf, 1, INT,
                           nullBuf, 1, INT,
                           comm2);
            enQ_compute(evQ,18*1000);

            state = next_state;
        }
        break;
    case 31:
        {
            int next_state = 32;
            auto msg_size  = (int)comm_model[std::make_tuple(state,m_ranks_per_node,m_threads)].eval(m_sz);
            auto exec_time = comp_model[std::make_tuple(state,next_state,m_ranks_per_node,m_threads)].eval(m_sz);

            computeAG_31();
            enQ_allgatherv( evQ,
                            nullBuf, sendCount_31, LONG,
                            nullBuf, recvCounts_31, nullDispMap, LONG,
                            comm2 );
            enQ_compute(evQ,15*1000);

            state = next_state;
        }
        break;
    case 32:
        {
            int next_state = 33;
            auto msg_size  = (int)comm_model[std::make_tuple(state,m_ranks_per_node,m_threads)].eval(m_sz);
            auto exec_time = comp_model[std::make_tuple(state,next_state,m_ranks_per_node,m_threads)].eval(m_sz);

            enQ_bcast( evQ, nullBuf, 1, DOUBLE, 0, comm2 );
            enQ_compute(evQ,17*1000);

            state = next_state;
        }
        break;
    case 33:
        {
            int next_state = 34;
            auto msg_size  = (int)comm_model[std::make_tuple(state,m_ranks_per_node,m_threads)].eval(m_sz);
            auto exec_time = comp_model[std::make_tuple(state,next_state,m_ranks_per_node,m_threads)].eval(m_sz);

            enQ_allreduce( evQ, nullBuf, nullBuf, 1, DOUBLE, MP::MAX, comm2 );
            enQ_compute(evQ,107*1000);

            state = next_state;
        }
        break;
    case 34:
        {
            int next_state = 35;
            auto msg_size  = (int)comm_model[std::make_tuple(state,m_ranks_per_node,m_threads)].eval(m_sz);
            auto exec_time = comp_model[std::make_tuple(state,next_state,m_ranks_per_node,m_threads)].eval(m_sz);

            enQ_alltoall( evQ,
                          nullBuf, 1, INT,
                          nullBuf, 1, INT, comm0 );
            enQ_compute(evQ,20*1000);

            state = next_state;
        }
        break;
    case 35:
        {
            int next_state = 36;
            auto msg_size  = (int)comm_model[std::make_tuple(state,m_ranks_per_node,m_threads)].eval(m_sz);
            auto exec_time = comp_model[std::make_tuple(state,next_state,m_ranks_per_node,m_threads)].eval(m_sz);

            computeAA_35();
            enQ_alltoallv( evQ,
                           nullBuf, sendCounts_35, nullDispMap, LONG,
                           nullBuf, recvCounts_35, nullDispMap, LONG,
                           comm0 );
            enQ_compute(evQ,20*1000);

            state = next_state;
        }
        break;
    case 36:
        {
            int next_state = 37;
            auto msg_size  = (int)comm_model[std::make_tuple(state,m_ranks_per_node,m_threads)].eval(m_sz);
            auto exec_time = comp_model[std::make_tuple(state,next_state,m_ranks_per_node,m_threads)].eval(m_sz);

            // 36 repeats the same message pattern as 35, so no need
            // to recompute

            //enQ_Alltoallv(evQ,comm0);
            enQ_alltoallv( evQ,
                           nullBuf, sendCounts_35, nullDispMap, LONG,
                           nullBuf, recvCounts_35, nullDispMap, LONG,
                           comm0 );

            double szScale = pow(1.5074,(m_sz-18.0));
            double nodeScale = pow((double(size())/16.0), -0.5);
            // this distribution is definitely not uniform. But for
            // now...
            double base = adjustUniformRand(1.0, 0.5, rng_rank);
            if (rng_rank->nextUniform() > .85) {
                base *= 3.0;
            }
            enQ_compute(evQ,208*1000*szScale*nodeScale);

            state = next_state;
        }
        break;
    case 37:
        {
            int next_state = 38;
            auto msg_size  = (int)comm_model[std::make_tuple(state,m_ranks_per_node,m_threads)].eval(m_sz);
            auto exec_time = comp_model[std::make_tuple(state,next_state,m_ranks_per_node,m_threads)].eval(m_sz);

            enQ_allreduce( evQ, nullBuf, nullBuf, 1, DOUBLE, MP::MAX, comm1 );
            enQ_compute(evQ,65*1000);

            state = next_state;
        }
        break;
    case 38:
        {
            int next_state = 39;
            auto msg_size  = (int)comm_model[std::make_tuple(state,m_ranks_per_node,m_threads)].eval(m_sz);
            auto exec_time = comp_model[std::make_tuple(state,next_state,m_ranks_per_node,m_threads)].eval(m_sz);

            enQ_allreduce( evQ, nullBuf, nullBuf, 1, DOUBLE, MP::MAX, comm1 );
            enQ_compute(evQ,21*1000);

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

            auto msg_size  = (int)comm_model[std::make_tuple(state,m_ranks_per_node,m_threads)].eval(m_sz);
            auto exec_time = comp_model[std::make_tuple(state,next_state,m_ranks_per_node,m_threads)].eval(m_sz);

            enQ_allreduce( evQ, nullBuf, nullBuf, 1, DOUBLE, MP::MAX, comm1 );
            enQ_compute(evQ,24*1000);

            state = next_state;

        }
        break;
    case 40:
        {
            int next_state = 41;
            auto msg_size  = (int)comm_model[std::make_tuple(state,m_ranks_per_node,m_threads)].eval(m_sz);
            auto exec_time = comp_model[std::make_tuple(state,next_state,m_ranks_per_node,m_threads)].eval(m_sz);

            enQ_barrier(evQ,comm1);

            double szScale = pow(1.875,(m_sz-18.0));
            double nodeScale = pow((double(size())/16.0),-0.5); //wrong!
            double base = adjustUniformRand(1.0, 0.3, rng_rank);
            enQ_compute(evQ,326*1000*szScale*nodeScale*base);

            state = next_state;
        }
        break;
    case 41:
        {
            int next_state = 42;
            auto msg_size  = (int)comm_model[std::make_tuple(state,m_ranks_per_node,m_threads)].eval(m_sz);
            auto exec_time = comp_model[std::make_tuple(state,next_state,m_ranks_per_node,m_threads)].eval(m_sz);

            enQ_sendrecv( evQ,
                          nullBuf, 1, DOUBLE, opposite, 0, //send (tag=0)
                          nullBuf, 1, DOUBLE, opposite, 0, //recv (tag=0)
                          GroupWorld, &m_resp );
            enQ_compute(evQ,16*1000);

            state = next_state;
        }
        break;
    case 42:
        {
            int next_state = 43;
            auto msg_size  = (int)comm_model[std::make_tuple(state,m_ranks_per_node,m_threads)].eval(m_sz);
            auto exec_time = comp_model[std::make_tuple(state,next_state,m_ranks_per_node,m_threads)].eval(m_sz);

            enQ_bcast( evQ, nullBuf, 1, DOUBLE, 0, comm2 );
            enQ_compute(evQ,15*1000);

            state = next_state;
        }
        break;
    case 43:
        {
            int next_state = 44;
            auto msg_size  = (int)comm_model[std::make_tuple(state,m_ranks_per_node,m_threads)].eval(m_sz);
            auto exec_time = comp_model[std::make_tuple(state,next_state,m_ranks_per_node,m_threads)].eval(m_sz);

            enQ_bcast( evQ, nullBuf, 1, DOUBLE, 0, comm0 );
            enQ_compute(evQ,14*1000);

            state = next_state;
        }
        break;
    case 44:
        {
            int next_state = 45;
            auto msg_size  = (int)comm_model[std::make_tuple(state,m_ranks_per_node,m_threads)].eval(m_sz);
            auto exec_time = comp_model[std::make_tuple(state,next_state,m_ranks_per_node,m_threads)].eval(m_sz);

            //enQ_Sendrecv - exchange w/ opposite
            // size increases by 1.92x per graph size
            // size varies linearly w/ num nodes
            double msgSize = 74896.0/4.0; // 4x1-22
            msgSize *= pow(1.92, double(m_sz-22.0));
            msgSize *= (4.0 / size());

            enQ_sendrecv( evQ,
                          nullBuf, int(msgSize), INT, opposite, 0, //send(tag=0)
                          nullBuf, int(msgSize), INT, opposite, 0, //recv(tag=0)
                          GroupWorld, &m_resp );

            enQ_compute(evQ,26*1000);

            state = next_state;
        }
        break;
    case 45:
        {
            int next_state = 46;
            auto msg_size  = (int)comm_model[std::make_tuple(state,m_ranks_per_node,m_threads)].eval(m_sz);
            auto exec_time = comp_model[std::make_tuple(state,next_state,m_ranks_per_node,m_threads)].eval(m_sz);
            //enQ_Allgatherv(evQ,comm2); this is an allgatherv, but we
            // treat it as an allgather because the variation is very
            // small.
            double msgSize = 9361; // 4x1-22
            msgSize *= pow(1.92, double(m_sz-22.0));
            msgSize *= (4.0 / size());

            enQ_allgather( evQ,
                           nullBuf, msgSize, INT,
                           nullBuf, msgSize, INT,
                           comm2);

            enQ_compute(evQ,17*1000);

            state = next_state;
        }
        break;
    case 46:
        {
            int next_state = 47;
            auto msg_size  = (int)comm_model[std::make_tuple(state,m_ranks_per_node,m_threads)].eval(m_sz);
            auto exec_time = comp_model[std::make_tuple(state,next_state,m_ranks_per_node,m_threads)].eval(m_sz);

            //enQ_Allgather(evQ,comm2);
            enQ_allgather( evQ,
                           nullBuf, 1, DOUBLE,
                           nullBuf, 1, DOUBLE,
                           comm2);

            double szScale = pow(1.0953,(m_sz-18.0));
            double nodeScale = pow(16.0/double(size()),0.7);
            double base = adjustUniformRand(1.0, 0.15, rng_rank);
            enQ_compute(evQ,1554*1000*szScale*nodeScale*base);

            state = next_state;
        }
        break;
    case 47:
        {
            int next_state = 48;
            auto msg_size  = (int)comm_model[std::make_tuple(state,m_ranks_per_node,m_threads)].eval(m_sz);
            auto exec_time = comp_model[std::make_tuple(state,next_state,m_ranks_per_node,m_threads)].eval(m_sz);

            //enQ_Sendrecv\n
            // shift within row
            // make sure send and recv partner should be the same
            int s_partner = (comm0_rank + 1) % comm0_ranks.size();
            int r_partner = (comm0_rank - 1);
            if (r_partner < 0) {
                r_partner = comm0_ranks.size()-1;
            }

            double msgSize = 18724; // 4x1-22
            msgSize *= pow(1.92667, double(m_sz-22.0));
            msgSize *= (4.0 / size());

            enQ_sendrecv( evQ,
                          nullBuf, msgSize, INT, s_partner, 0, //send (tag=0)
                          nullBuf, msgSize, INT, r_partner, 0, //recv (tag=0)
                          comm0, &m_resp );

            enQ_compute(evQ,18*1000);

            state = next_state;
        }
        break;
    case 48:
        {
            int next_state = 49;
            auto msg_size  = (int)comm_model[std::make_tuple(state,m_ranks_per_node,m_threads)].eval(m_sz);
            auto exec_time = comp_model[std::make_tuple(state,next_state,m_ranks_per_node,m_threads)].eval(m_sz);

            // iterate through row, starting with ourself
            enQ_sendrecv( evQ,
                          nullBuf, 1, INT, s_partner_48, 0, //send (tag=0)
                          nullBuf, 1, INT, r_partner_48, 0, //recv (tag=0)
                          comm0, &m_resp );


            enQ_compute(evQ,21*1000);

            state = next_state;
        }
        break;
    case 49:
        {
            int next_state = -1;

            //enQ_Sendrecv\n
            // iterate through row, starting with ourself
            int msgElem;
            computeSR_49(msgElem);

            enQ_sendrecv( evQ,
                          nullBuf, msgElem, DOUBLE, s_partner_48, 0,//snd(tag=0)
                          nullBuf, msgElem, DOUBLE, r_partner_48, 0,//rcv(tag=0)
                          comm0, &m_resp );

            // advance iterators
            s_partner_48 =  (s_partner_48 + 1) % comm0_ranks.size();
            r_partner_48  = r_partner_48 - 1;
            if (r_partner_48  < 0) {
                r_partner_48 = comm0_ranks.size()-1;
            }


            // goes 47 X times (where X is (sqrt(nodes)-1)) then 50
            if (idx_49 == (square-1)) {
                next_state = 50;
                auto msg_size  = (int)comm_model[std::make_tuple(state,m_ranks_per_node,m_threads)].eval(m_sz);
                auto exec_time = comp_model[std::make_tuple(state,next_state,m_ranks_per_node,m_threads)].eval(m_sz);
                double szScale = pow(1.26,(m_sz-18.0));
                enQ_compute(evQ,45*1000*szScale);
                idx_49 = 0;
            } else {
                next_state = 47;
                auto msg_size  = (int)comm_model[std::make_tuple(state,m_ranks_per_node,m_threads)].eval(m_sz);
                auto exec_time = comp_model[std::make_tuple(state,next_state,m_ranks_per_node,m_threads)].eval(m_sz);
                double szScale = pow(1.203,(m_sz-18.0));
                enQ_compute(evQ,70*1000*szScale);

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

            auto msg_size  = (int)comm_model[std::make_tuple(state,m_ranks_per_node,m_threads)].eval(m_sz);
            auto exec_time = comp_model[std::make_tuple(state,next_state,m_ranks_per_node,m_threads)].eval(m_sz);

            enQ_allreduce( evQ, nullBuf, nullBuf, 1, DOUBLE, MP::MAX, comm1);
            enQ_compute(evQ,28*1000);


            state = next_state;
        }
        break;
    case 51:
        {
            int next_state = 52;
            auto msg_size  = (int)comm_model[std::make_tuple(state,m_ranks_per_node,m_threads)].eval(m_sz);
            auto exec_time = comp_model[std::make_tuple(state,next_state,m_ranks_per_node,m_threads)].eval(m_sz);

            enQ_barrier(evQ,comm1);

            double szScale = pow(1.8981,(m_sz-18.0));
            double nodeScale = (double(size())/16.0);
            double base = adjustUniformRand(1.0, 0.1, rng_rank);
            enQ_compute(evQ,1327.0*1000.0*szScale*nodeScale);

            state = next_state;
        }
        break;
    case 52:
        {
            int next_state = 53;
            auto msg_size  = (int)comm_model[std::make_tuple(state,m_ranks_per_node,m_threads)].eval(m_sz);
            auto exec_time = comp_model[std::make_tuple(state,next_state,m_ranks_per_node,m_threads)].eval(m_sz);

            //enQ_Allreduce(evQ,comm1);
            enQ_allreduce( evQ, nullBuf, nullBuf, 1, DOUBLE, MP::MAX, comm1);

            double szScale = pow(1.7716,(m_sz-18.0));
            double nodeScale = (double(size())/16.0);
            double base = adjustUniformRand(1.0, 0.10, rng_rank);
            enQ_compute(evQ,181*1000*szScale*nodeScale*base);

            state = next_state;
        }
        break;
    case 53:
        {
            int next_state = 54;
            auto msg_size  = (int)comm_model[std::make_tuple(state,m_ranks_per_node,m_threads)].eval(m_sz);
            auto exec_time = comp_model[std::make_tuple(state,next_state,m_ranks_per_node,m_threads)].eval(m_sz);

            //enQ_Allreduce(evQ,comm1);
            enQ_allreduce( evQ, nullBuf, nullBuf, 1, DOUBLE, MP::MAX, comm1);

            enQ_compute(evQ,20*1000);

            state = next_state;
        }
        break;
    case 54:
        {
            int next_state = 1;
            auto msg_size  = (int)comm_model[std::make_tuple(state,m_ranks_per_node,m_threads)].eval(m_sz);
            auto exec_time = comp_model[std::make_tuple(state,next_state,m_ranks_per_node,m_threads)].eval(m_sz);

            //enQ_Bcast(evQ,comm1);
            enQ_bcast( evQ, nullBuf, 1, DOUBLE, 0, comm1 );

            // must enque something or it assumes we're done
            enQ_compute(evQ,20*1000);

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
