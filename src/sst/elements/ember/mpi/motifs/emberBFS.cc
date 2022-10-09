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
    uint32_t rng_seed = (uint32_t) params.find("arg.seed", 1);

    // TODO: Does every rank need to initialize? It seems like the answer
    // is yes, as they need to tell the system the name of the global
    // object they want to use.

    comm_filename = params.find<std::string>("arg.comm_model", "bfs-comm.model");
    comp_filename = params.find<std::string>("arg.comp_model", "bfs-comp.model");

    /*
    comm_model.initialize("comm_model", Shared::SharedObject::NO_VERIFY);
    comp_model.initialize("comp_model", Shared::SharedObject::NO_VERIFY);


    if (rank() == 0) {
        std::ifstream comm_file( params.find<std::string>("arg.comm_model", "bfs-comm.model") );
        std::ifstream comp_file( params.find<std::string>("arg.comp_model", "bfs-comp.model") );

        if (not (comp_file.good() and comm_file.good())) {
            out.fatal(CALL_INFO, 1, "Bad model file(s)");
        }

        // Read the communication size model into a SharedMap
        // Each line of the file is
        //   callsite coeff1 coeff2 coeff3 [...]
        std::string line;
        while (getline(comm_file, line)) {
            std::istringstream iss(line);
            int cs;
            iss >> cs;
            std::vector<float> coeff((std::istream_iterator<float>(iss)),
                                      std::istream_iterator<float>());

            comm_model.write(cs, PolyModel(coeff));
        }

        // Read the compute time model into a SharedMap
        // Each line of the file is
        //   src_callsite dst_callsite coeff1 coeff2 coeff3 [...]
        while (getline(comp_file, line)) {
            std::istringstream iss(line);
            int src_cs, dst_cs;
            iss >> src_cs >> dst_cs;
            std::vector<float> coeff((std::istream_iterator<float>(iss)),
                                      std::istream_iterator<float>());

            comp_model.write(std::pair<int,int>(src_cs, dst_cs), PolyModel(coeff));
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

void EmberBFSGenerator::init(unsigned int phase) {

    out.output("Info - this line executed");

    comm_model.initialize("comm_model", Shared::SharedObject::NO_VERIFY);
    comp_model.initialize("comp_model", Shared::SharedObject::NO_VERIFY);

    if (rank() == 0) {
        std::ifstream comm_file( comm_filename );
        std::ifstream comp_file( comp_filename );

        if (not (comp_file.good() and comm_file.good())) {
            out.fatal(CALL_INFO, 1, "Bad model file(s)");
        }

        // Read the communication size model into a SharedMap
        // Each line of the file is
        //   callsite coeff1 coeff2 coeff3 [...]
        std::string line;
        while (getline(comm_file, line)) {
            std::istringstream iss(line);
            int cs;
            iss >> cs;
            std::vector<float> coeff((std::istream_iterator<float>(iss)),
                                      std::istream_iterator<float>());

            comm_model.write(cs, PolyModel(coeff));
        }

        // Read the compute time model into a SharedMap
        // Each line of the file is
        //   src_callsite dst_callsite coeff1 coeff2 coeff3 [...]
        while (getline(comp_file, line)) {
            std::istringstream iss(line);
            int src_cs, dst_cs;
            iss >> src_cs >> dst_cs;
            std::vector<float> coeff((std::istream_iterator<float>(iss)),
                                      std::istream_iterator<float>());

            comp_model.write(std::pair<int,int>(src_cs, dst_cs), PolyModel(coeff));
        }

    }
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


    //if (rank() == 0) {
    //    printf("%d %.1f %.3f %.3f %d\n", rank(), meanSize, roll, initRoll, ret);
    //}
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
        printf("Rank %d Iter = %lld State = %lld Time=%llu\n", rank(),
               iter, state, s_time);
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
            // find our 'local' ranks
            enQ_rank( evQ, comm2, &comm2_rank );
            enQ_rank( evQ, comm0, &comm0_rank );
            //enQ_Allreduce(evQ,comm0);
            // don't know what the actual operator is
            enQ_allreduce( evQ, nullBuf, nullBuf, 1, DOUBLE, MP::MAX, comm0 );

            enQ_compute(evQ,49*1000);

            state = 2;
        }
        break;
    case 2:
        {
            // we do some initialization for state 48 here since we
            // now have the comm0_rank
            if (iter == 0) {
                s_partner_48 = r_partner_48 = comm0_rank;
            }
            
            //enQ_Allreduce(evQ,comm0);
            enQ_allreduce( evQ, nullBuf, nullBuf, 1, DOUBLE, MP::MAX, comm0 );

            enQ_compute(evQ,28*1000);

            state = 3;
        }
        break;
    case 3:
        {
            //enQ_Barrier(evQ,comm1);
            enQ_barrier( evQ, comm1 );
             
            enQ_compute(evQ,18*1000);
            
            state = 4;
        }
        break;
    case 4:
        {
            //enQ_Allreduce(evQ,comm1);
            enQ_allreduce( evQ, nullBuf, nullBuf, 1, DOUBLE, MP::MAX, comm1 );
            
            enQ_compute(evQ,17*1000);

            state = 5;
        }
        break;
    case 5:
        {
            //enQ_Allreduce(evQ,comm0);
            enQ_allreduce( evQ, nullBuf, nullBuf, 1, DOUBLE, MP::MAX, comm0 );

            enQ_compute(evQ,18*1000);

            state = 6;
        }
        break;
    case 6:
        {
            //enQ_Bcast(evQ,comm1);
            enQ_bcast( evQ, nullBuf, 1, INT, 0, comm1 );
            assert(sizeofDataType(INT)==4);
            
            enQ_compute(evQ,15*1000);

            state = 7;
        }
        break;
    case 7:
        {
            //enQ_Bcast(evQ,comm1);
            enQ_bcast( evQ, nullBuf, 1, DOUBLE, 0, comm1 );

            enQ_compute(evQ,17*1000);

            state = 8;
        }
        break;
    case 8:
        {
            //enQ_Bcast(evQ,comm1);
            enQ_bcast( evQ, nullBuf, 1, DOUBLE, 0, comm1 );

            enQ_compute(evQ,16*1000);

            state = 9;
        }
        break;
    case 9:
        {
            //enQ_Bcast(evQ,comm1);
            enQ_bcast( evQ, nullBuf, 1, DOUBLE, 0, comm1 );

            enQ_compute(evQ,114*1000);

            state = 10;
        }
        break;
    case 10:
        {
            //enQ_Comm_dup\n

            enQ_compute(evQ,38*1000);

            state = 11;
        }
        break;
    case 11:
        {
            //enQ_Comm_dup\n

            enQ_compute(evQ,40*1000);

            state = 12;
        }
        break;
    case 12:
        {
            //enQ_Comm_dup\n

            enQ_compute(evQ,40*1000);

            state = 13;
        }
        break;
    case 13:
        {
            //enQ_Comm_dup\n
            // it appears that non-diagonals don't do this comm_dup

            enQ_compute(evQ,17*1000);

            state = 14;
        }
        break;
    case 14:
        {
            //enQ_Sendrecv\n
            enQ_sendrecv( evQ,
                          nullBuf, 1, INT, opposite, 0, //send (tag=0)
                          nullBuf, 1, INT, opposite, 0, //recv (tag=0)
                          GroupWorld, &m_resp );
 
            enQ_compute(evQ,16*1000);

            state = 15;
        }
        break;
    case 15:
        {
            //enQ_Allgather(evQ,comm2);
            enQ_allgather( evQ,
                           nullBuf, 1, INT,
                           nullBuf, 1, INT,
                           comm2);

            enQ_compute(evQ,15*1000);

            state = 16;
        }
        break;
    case 16:
        {
            //enQ_Sendrecv\n
            enQ_sendrecv( evQ,
                          nullBuf, 1, INT, opposite, 0, //send (tag=0)
                          nullBuf, 1, INT, opposite, 0, //recv (tag=0)
                          GroupWorld, &m_resp );

            enQ_compute(evQ,58*1000);

            state = 17;
        }
        break;
    case 17:
        {
            //enQ_Comm_dup\n

            enQ_compute(evQ,37*1000);

            state = 18;
        }
        break;
    case 18:
        {
            //enQ_Comm_dup\n

            enQ_compute(evQ,38*1000);

            state = 19;
        }
        break;
    case 19:
        {
            //enQ_Comm_dup\n

            enQ_compute(evQ,38*1000);

            state = 20;
        }
        break;
    case 20:
        {
            //enQ_Comm_dup\n
            // it appears that non-diagonals don't do this comm_dup

            enQ_compute(evQ,17*1000);

            state = 21;
        }
        break;
    case 21:
        {
            //enQ_Allgather(evQ,comm0);
            enQ_allgather( evQ,
                           nullBuf, 1, INT,
                           nullBuf, 1, INT,
                           comm0);

            enQ_compute(evQ,16*1000);

            state = 22;
        }
        break;
    case 22:
        {
            //enQ_Bcast(evQ,comm0);
            enQ_bcast( evQ, nullBuf, 1, DOUBLE, 0, comm0 );

            double szScale = pow(1.84,(m_sz-18.0));
            double nodeScale = 16.0/double(size());
            double base = adjustUniformRand(1.0, 0.25, rng_rank);
            enQ_compute(evQ,212*1000*szScale*nodeScale*base);

            state = 23;
        }
        break;
    case 23:
        {
            //enQ_Allreduce(evQ,comm1);
            enQ_allreduce( evQ, nullBuf, nullBuf, 1, DOUBLE, MP::MAX, comm1 );
            
            enQ_compute(evQ,34*1000);

            state = 24;
        }
        break;
    case 24:
        {
            //enQ_Allreduce(evQ,comm1);
            enQ_allreduce( evQ, nullBuf, nullBuf, 1, DOUBLE, MP::MAX, comm1 );

            enQ_compute(evQ,73*1000);

            state = 25;
        }
        break;
    case 25:
        {
            //enQ_Allreduce(evQ,comm0);
            enQ_allreduce( evQ, nullBuf, nullBuf, 1, DOUBLE, MP::MAX, comm0 );

            enQ_compute(evQ,18*1000);

            state = 26;
        }
        break;
    case 26:
        {
            //enQ_Sendrecv\n
            enQ_sendrecv( evQ,
                          nullBuf, 1, INT, opposite, 0, //send (tag=0)
                          nullBuf, 1, INT, opposite, 0, //recv (tag=0)
                          GroupWorld, &m_resp );
     
            enQ_compute(evQ,14*1000);

            state = 27;
        }
        break;
    case 27:
        {
            //enQ_Sendrecv\n
            enQ_sendrecv( evQ,
                          nullBuf, 1, INT, opposite, 0, //send (tag=0)
                          nullBuf, 1, INT, opposite, 0, //recv (tag=0)
                          GroupWorld, &m_resp );
            
            enQ_compute(evQ,14*1000);            

            state = 28;
        }
        break;
    case 28:
        {
            //enQ_Sendrecv\n
            enQ_sendrecv( evQ,
                          nullBuf, 1, DOUBLE, opposite, 0, //send (tag=0)
                          nullBuf, 1, DOUBLE, opposite, 0, //recv (tag=0)
                          GroupWorld, &m_resp );

            enQ_compute(evQ,146*1000);

            state = 29;
        }
        break;
    case 29:
        {
            //enQ_Sendrecv\n  size varies
            computeSR_29(sendElem_29, recvElem_29);
            enQ_sendrecv( evQ,
                          nullBuf, sendElem_29, INT, opposite, 0, //send (tag=0)
                          nullBuf, recvElem_29, INT, opposite, 0, //recv (tag=0)
                          GroupWorld, &m_resp );

            enQ_compute(evQ,22*1000);

            state = 30;
        }
        break;
    case 30:
        {
            //enQ_Allgather(evQ,comm2);
            enQ_allgather( evQ,
                           nullBuf, 1, INT,
                           nullBuf, 1, INT,
                           comm2);
            
            enQ_compute(evQ,18*1000);

            state = 31;
        }
        break;
    case 31:
        {
            //enQ_Allgatherv(evQ,comm2);

            computeAG_31();

            enQ_allgatherv( evQ,
                            nullBuf, sendCount_31, LONG,
                            nullBuf, recvCounts_31, nullDispMap, LONG,
                            comm2 );

            enQ_compute(evQ,15*1000);

            state = 32;
        }
        break;
    case 32:
        {
            //enQ_Bcast(evQ,comm2);
            enQ_bcast( evQ, nullBuf, 1, DOUBLE, 0, comm2 );
            
            enQ_compute(evQ,17*1000);

            state = 33;
        }
        break;
    case 33:
        {
            //enQ_Allreduce(evQ,comm2);
            enQ_allreduce( evQ, nullBuf, nullBuf, 1, DOUBLE, MP::MAX, comm2 );
            
            enQ_compute(evQ,107*1000);

            state = 34;
        }
        break;
    case 34:
        {
            //enQ_Alltoall(evQ,comm0);
            enQ_alltoall( evQ,
                          nullBuf, 1, INT,
                          nullBuf, 1, INT, comm0 );
            
            enQ_compute(evQ,20*1000);

            state = 35;
        }
        break;
    case 35:
        {
            //enQ_Alltoallv(evQ,comm0);
            computeAA_35();
            enQ_alltoallv( evQ,
                           nullBuf, sendCounts_35, nullDispMap, LONG,
                           nullBuf, recvCounts_35, nullDispMap, LONG,
                           comm0 );
   
            enQ_compute(evQ,20*1000);

            state = 36;
        }
        break;
    case 36:
        {
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

            state = 37;
        }
        break;
    case 37:
        {
            //enQ_Allreduce(evQ,comm1);
            enQ_allreduce( evQ, nullBuf, nullBuf, 1, DOUBLE, MP::MAX, comm1 );
            
            enQ_compute(evQ,65*1000);

            state = 38;
        }
        break;
    case 38:
        {
            //enQ_Allreduce(evQ,comm1);
            enQ_allreduce( evQ, nullBuf, nullBuf, 1, DOUBLE, MP::MAX, comm1 );

            enQ_compute(evQ,21*1000);

            state = 39;
        }
        break;
    case 39:
        {
            //enQ_Allreduce(evQ,comm1);
            enQ_allreduce( evQ, nullBuf, nullBuf, 1, DOUBLE, MP::MAX, comm1 );

            enQ_compute(evQ,24*1000);

            // the next state is determined by a pattern for each
            //iteration. It goes 25, 40 then a 67% chance of 25 again,
            //then 51. 
            switch(idx_39) {
            case 0:
                state = 25;
                break;
            case 1:
                state = 40;
                break;
            case 2:
                if (rng->nextUniform() <= 0.67) {
                    state = 25;
                } else {
                    state = 51;
                }
                break;
            case 3:
                    state = 51;
                    break;
            }
            idx_39++;
        }
        break;
    case 40:
        {
            enQ_barrier(evQ,comm1);

            double szScale = pow(1.875,(m_sz-18.0));
            double nodeScale = pow((double(size())/16.0),-0.5); //wrong!
            double base = adjustUniformRand(1.0, 0.3, rng_rank);
            enQ_compute(evQ,326*1000*szScale*nodeScale*base);

            state = 41;
        }
        break;
    case 41:
        {
            //enQ_Sendrecv\n
            enQ_sendrecv( evQ,
                          nullBuf, 1, DOUBLE, opposite, 0, //send (tag=0)
                          nullBuf, 1, DOUBLE, opposite, 0, //recv (tag=0)
                          GroupWorld, &m_resp );

            enQ_compute(evQ,16*1000);

            state = 42;
        }
        break;
    case 42:
        {
            //enQ_Bcast(evQ,comm2);
            enQ_bcast( evQ, nullBuf, 1, DOUBLE, 0, comm2 );

            enQ_compute(evQ,15*1000);

            state = 43;
        }
        break;
    case 43:
        {
            //enQ_Bcast(evQ,comm0);
            enQ_bcast( evQ, nullBuf, 1, DOUBLE, 0, comm0 );

            enQ_compute(evQ,14*1000);

            state = 44;
        }
        break;
    case 44:
        {
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

            state = 45;
        }
        break;
    case 45:
        {
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

            state = 46;
        }
        break;
    case 46:
        {
            //enQ_Allgather(evQ,comm2);
            enQ_allgather( evQ,
                           nullBuf, 1, DOUBLE,
                           nullBuf, 1, DOUBLE,
                           comm2);

            double szScale = pow(1.0953,(m_sz-18.0));
            double nodeScale = pow(16.0/double(size()),0.7);
            double base = adjustUniformRand(1.0, 0.15, rng_rank);
            enQ_compute(evQ,1554*1000*szScale*nodeScale*base);

            state = 47;
        }
        break;
    case 47:
        {
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

            state = 48;
        }
        break;
    case 48:
        {
            //enQ_Sendrecv\n
            // iterate through row, starting with ourself
            enQ_sendrecv( evQ,
                          nullBuf, 1, INT, s_partner_48, 0, //send (tag=0)
                          nullBuf, 1, INT, r_partner_48, 0, //recv (tag=0)
                          comm0, &m_resp );


            enQ_compute(evQ,21*1000);

            state = 49;
        }
        break;
    case 49:
        {
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
                double szScale = pow(1.26,(m_sz-18.0));
                enQ_compute(evQ,45*1000*szScale);
                state = 50;                
                idx_49 = 0;
            } else {
                double szScale = pow(1.203,(m_sz-18.0));
                enQ_compute(evQ,70*1000*szScale);

                state = 47;
                idx_49++;
            }
        }
        break;
    case 50:
        {
            //enQ_Allreduce(evQ,comm1);
            enQ_allreduce( evQ, nullBuf, nullBuf, 1, DOUBLE, MP::MAX, comm1);

            enQ_compute(evQ,28*1000);

            // goes to 41 2 (70%) or 3 (30%) times then 25
            switch(idx_50) {
            case 0:
                state = 41;
                idx_50 = 1;
                break;
            case 1:
                state = 41;
                idx_50 = 2;
                break;
            case 2:
                if (rng->nextUniform() <= 0.3) {
                    state = 41;
                    idx_50 = 3;
                } else {
                    state = 25;
                    idx_50 = 0;
                }
                break;
            case 3:
                state = 25;
                idx_50 = 0;
                break;
            default:
                printf("idx_50 incorrect (%llu)\n", idx_50);
                break;
            }
        }
        break;
    case 51:
        {
            enQ_barrier(evQ,comm1);

            double szScale = pow(1.8981,(m_sz-18.0));
            double nodeScale = (double(size())/16.0);
            double base = adjustUniformRand(1.0, 0.1, rng_rank);
            enQ_compute(evQ,1327.0*1000.0*szScale*nodeScale);

            state = 52;
        }
        break;
    case 52:
        {
            //enQ_Allreduce(evQ,comm1);
            enQ_allreduce( evQ, nullBuf, nullBuf, 1, DOUBLE, MP::MAX, comm1);

            double szScale = pow(1.7716,(m_sz-18.0));
            double nodeScale = (double(size())/16.0);
            double base = adjustUniformRand(1.0, 0.10, rng_rank);
            enQ_compute(evQ,181*1000*szScale*nodeScale*base);

            state = 53;
        }
        break;
    case 53:
        {
            //enQ_Allreduce(evQ,comm1);
            enQ_allreduce( evQ, nullBuf, nullBuf, 1, DOUBLE, MP::MAX, comm1);

            enQ_compute(evQ,20*1000);

            state = 54;
        }
        break;
    case 54:
        {
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
