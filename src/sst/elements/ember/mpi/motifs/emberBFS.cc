// Copyright 2009-2023 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2023, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

// Model parameter files associated with this model can be found at:
// https://github.com/sstsimulator/a-sst/tree/new-bfs-models/ISB-BFS


#include <sst_config.h>
#include <fstream>
#include <iterator>
#include <algorithm>
#include <iostream>
#include <string>
#include <memory>
#include "emberBFS.h"
#include <stdint.h>
#include <regex>

using namespace SST::Ember;

EmberBFSGenerator::EmberBFSGenerator(SST::ComponentId_t id,
                                     Params& params) :
    EmberMessagePassingGenerator(id, params, "BFS"), state(0),
    recvCounts_31(0), sendCounts_35(0), recvCounts_35(0)
{

    trace = false;
    m_sz = (int32_t) params.find("arg.sz", 1);
    m_threads = (int32_t) params.find("arg.threads", 1);
    uint32_t rng_seed = (uint32_t) params.find("arg.seed", 1);
    m_nodes = (uint32_t) params.find("arg.nodes", 1);

    // Parse model files
    std::string msg_model_filename  = params.find<std::string>("arg.msg_model", "msg.model");
    std::string exec_model_filename = params.find<std::string>("arg.exec_model", "exec.model");

    // If it is a trace file and has an X in place of rank_id, each rank should read its own file
    // This code replaces "_X." with "-N.", where N is the rank.
    // This code DOES NOT WORK. The motif will hang, probably due to breaking assumptions about symmetric message sizes.
    // There also appear to be issues with not every exec trace having every model it needs. We could possibly fix that
    // by having a default behavior of a constant model when something is missing from the map.

    /*
    if (msg_model_filename.find("msgtrace") != std::string::npos) {
        if (msg_model_filename.find("-X.txt") != std::string::npos) {
            std::cout << "found it" << std::endl;
            std::string rep = "-" + std::to_string(rank()) + ".";
            //std::string rep = "-0."; // used for bug testing, have everyone use rank 0
            std::cout << "replaced: " << std::regex_replace(msg_model_filename, std::regex("\\-X\\."), rep) << std::endl;
            msg_model_filename = std::regex_replace(msg_model_filename, std::regex("\\-X\\."), rep);
        }
    }
    if (exec_model_filename.find("exectrace") != std::string::npos) {
        if (exec_model_filename.find("-X.txt") != std::string::npos) {
            std::cout << "found it" << std::endl;
            std::string rep = "-" + std::to_string(rank()) + ".";
            std::cout << "replaced: " << std::regex_replace(exec_model_filename, std::regex("\\-X\\."), rep) << std::endl;
            exec_model_filename = std::regex_replace(exec_model_filename, std::regex("\\-X\\."), rep);
        }
    }
    */

    std::ifstream msg_model_file(msg_model_filename);
    std::ifstream exec_model_file(exec_model_filename);

    if (not (exec_model_file.good() and msg_model_file.good())) {
        out.fatal(CALL_INFO, 1, "Bad model file(s)");
    }

    std::string line;

    // Parse the message model file. We have a different parsing scheme for polynomial models and other
    // models. The polynomial models have a header that tells us, for each column, what term the coefficeints
    // in that column are associated with. For instance, a column header of X,Y means that column contains the
    // coefficients for the term size^X*nodes^Y. The constant term is thus 0,0.
    if (msg_model_filename.find("poly") != std::string::npos) {
        // Using polynomial models
        int min = 0;
        std::vector<std::vector<int>> powers; // 0,1 => x0^0 * x1^1

        // Parse column headers
        getline(msg_model_file, line);
        std::istringstream iss(line);

        std::string tok;

        iss >> tok >> tok; // drop first two column headers
        // Each token is x0,x1,...,xn
        // Parse this into a vector => [x0,x1,...,xn]
        while (iss >> tok) {
            std::vector<int> pows;
            std::string delim = ",";
            auto start = 0U;
            auto end = tok.find(delim);
            while (end != std::string::npos) {
                pows.push_back(std::stoi(tok.substr(start, end - start)));
                start = end + delim.length();
                end = tok.find(delim, start);
            }
            pows.push_back(std::stoi(tok.substr(start, end - start)));
            powers.push_back(pows);
        }

        // Parse the rest of the file. Each line is:
        //   src dst threads_per_rank coeff0 coeff1 ...
        while (getline(msg_model_file, line)) {
            std::istringstream iss(line);
            int cs, threads_per_rank;
                iss >> cs >> threads_per_rank;
                double co;
                std::map<std::vector<int>, double> coeff;
                int idx = 0;
                while (iss >> co) {
                    coeff[powers[idx]] = co;
                    idx++;
                }
                msg_model[std::make_tuple(cs, threads_per_rank)] = std::unique_ptr<Model>(new PolyModelND(coeff, min));
        }
    }
    else {
        // Using new models
        while (getline(msg_model_file, line)) {
            std::istringstream iss(line);
            int cs, threads_per_rank;
            std::string model_type;
            iss >> cs >> threads_per_rank >> model_type;

            if (!(model_type.compare("CONSTANT"))) {
                double constant;
                iss >> constant;
                msg_model[std::make_tuple(cs, threads_per_rank)] = std::unique_ptr<Model>(new ConstModel(constant));
            }
            else if (!(model_type.compare("EXPONENTIAL"))) {
                double A, B, C, D;
                iss >> A >>  B >>  C >> D;
                msg_model[std::make_tuple(cs, threads_per_rank)] = std::unique_ptr<Model>(new ExpModel(A, B, C, D));
            }
            else if (!(model_type.compare("TRACE"))) {
                std::vector<double> vals((std::istream_iterator<double>(iss)), std::istream_iterator<double>());
                msg_model[std::make_tuple(cs, threads_per_rank)] = std::unique_ptr<Model>(new TraceModel(vals));
            }
            else {
                std::cout << "ERROR\n";
                exit(1);
            }

        }
    }


    // Parse the execution time models. These are nearly identical to the message size models except that a
    // compute region, or "callsite transition" is defiend by the callsites that come before and after it.
    // Thus, a compute region is identified by a source callsite and a destination callsite at the beginning
    // of each line instead of a single callsite in the case of the message models.
    if (exec_model_filename.find("poly") != std::string::npos) {
        // Using polynomial models
        int min = 0;
        std::vector<std::vector<int>> powers; // 0,1 => x0^0 * x1^1

        // Parse column headers
        getline(exec_model_file, line);
        std::istringstream iss(line);
        std::string tok;

        iss >> tok >> tok >> tok; // drop first three column headers
        // Each token is x0,x1,...,xn
        // Parse this into a vector => [x0,x1,...,xn]
        while (iss >> tok) {
            std::vector<int> pows;
            std::string delim = ",";
            auto start = 0U;
            auto end = tok.find(delim);
            while (end != std::string::npos) {
                pows.push_back(std::stoi(tok.substr(start, end - start)));
                start = end + delim.length();
                end = tok.find(delim, start);
            }
            pows.push_back(std::stoi(tok.substr(start, end - start)));
            powers.push_back(pows);
        }

        while (getline(exec_model_file, line)) {
            std::istringstream iss(line);
            int src, dst, threads_per_rank;
            iss >> src >> dst >> threads_per_rank;
            double co;
            std::map<std::vector<int>, double> coeff;
            int idx = 0;
            while (iss >> co) {
                coeff[powers[idx]] = co;
                idx++;
            }
            exec_model[std::make_tuple(src, dst, threads_per_rank)] = std::unique_ptr<Model>(new PolyModelND(coeff, min));
        exec_model[std::make_tuple(src, dst, threads_per_rank)]->setScale(1000.0);
        }

    } else {
        // new models
        while (getline(exec_model_file, line)) {
            std::istringstream iss(line);
            int src, dst, threads_per_rank;
            std::string model_type;
            iss >> src >> dst >> threads_per_rank >> model_type;

            if (!(model_type.compare("CONSTANT"))) {
                double constant;
                iss >> constant;
                exec_model[std::make_tuple(src, dst, threads_per_rank)] = std::unique_ptr<Model>(new ConstModel(constant));
            }
            else if (!(model_type.compare("BILINEAR"))) {
                double A, B, C, D;
                iss >> A >>  B >>  C >> D;
                exec_model[std::make_tuple(src, dst, threads_per_rank)] = std::unique_ptr<Model>(new BilinearModel(A, B, C, D));
            }
            else if (!(model_type.compare("EXPONENTIAL"))) {
                double A, B, C, D;
                iss >> A >>  B >>  C >> D;
                exec_model[std::make_tuple(src, dst, threads_per_rank)] = std::unique_ptr<Model>(new ExpModel(A, B, C, D));
            }
            else if (!(model_type.compare("TRACE"))) {
                std::vector<double> vals((std::istream_iterator<double>(iss)), std::istream_iterator<double>());
                exec_model[std::make_tuple(src, dst, threads_per_rank)] = std::unique_ptr<Model>(new TraceModel(vals));
            }
            else {
                std::cout << "ERROR\n";
                exit(1);
            }
            exec_model[std::make_tuple(src, dst, threads_per_rank)]->setScale(1000.0);
        }
    }

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

    double msg_size  = 0.0;
    double exec_time = 0.0;

    uint64_t next_state = -1;

    int zero_compute = 0;

    switch(state) {
    case 0: // special init state
        // create communicators
        if (iter == 0) {
            enQ_commCreate( evQ, GroupWorld, comm0_ranks, &comm0 );
            enQ_commCreate( evQ, GroupWorld, comm1_ranks, &comm1 );
            enQ_commCreate( evQ, GroupWorld, comm2_ranks, &comm2 );
            comm1_rank = UINT32_MAX; //inital value
        }
        state = 1;

        break;
    case 1:
        {
            next_state = 2;
            msg_size  = msg_model.at(std::make_tuple(state,m_threads))->eval(m_nodes,m_sz);
            exec_time = exec_model.at(std::make_tuple(state,next_state,m_threads))->eval(m_nodes,m_sz);

            // find our 'local' ranks
            enQ_rank( evQ, comm0, &comm0_rank );
            enQ_rank( evQ, comm1, &comm1_rank ); // comm1 is comm_world
            enQ_rank( evQ, comm2, &comm2_rank );


            enQ_allreduce( evQ, nullBuf, nullBuf, 8, CHAR, MP::MAX, comm0 );
            if (!zero_compute) enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 2:
        {
            next_state = 3;

            msg_size  = msg_model.at(std::make_tuple(state,m_threads))->eval(m_nodes,m_sz);
            exec_time = exec_model.at(std::make_tuple(state,next_state,m_threads))->eval(m_nodes,m_sz);

            // we do some initialization for state 48 here since we
            // now have the comm0_rank
            if (iter == 0) {
                s_partner_48 = r_partner_48 = comm0_rank;
            }

            enQ_allreduce( evQ, nullBuf, nullBuf, msg_size, CHAR, MP::MAX, comm0 );
            if (!zero_compute) enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 3:
        {
            next_state = 4;
            exec_time = exec_model.at(std::make_tuple(state,next_state,m_threads))->eval(m_nodes,m_sz);

            enQ_barrier( evQ, comm1 );
            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 4:
        {
            next_state = 5;
            msg_size  = msg_model.at(std::make_tuple(state,m_threads))->eval(m_nodes,m_sz);
            exec_time = exec_model.at(std::make_tuple(state,next_state,m_threads))->eval(m_nodes,m_sz);

            enQ_allreduce( evQ, nullBuf, nullBuf, msg_size, CHAR, MP::MAX, comm1 );
            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 5:
        {
            next_state = 6;
            msg_size  = msg_model.at(std::make_tuple(state,m_threads))->eval(m_nodes,m_sz);
            exec_time = exec_model.at(std::make_tuple(state,next_state,m_threads))->eval(m_nodes,m_sz);

            enQ_allreduce( evQ, nullBuf, nullBuf, msg_size, CHAR, MP::MAX, comm0 );
            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 6:
        {
            next_state = 7;
            msg_size  = msg_model.at(std::make_tuple(state,m_threads))->eval(m_nodes,m_sz);
            exec_time = exec_model.at(std::make_tuple(state,next_state,m_threads))->eval(m_nodes,m_sz);

            enQ_bcast( evQ, nullBuf, msg_size, CHAR, 0, comm1 );
            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 7:
        {
            next_state = 8;
            msg_size  = msg_model.at(std::make_tuple(state,m_threads))->eval(m_nodes,m_sz);
            exec_time = exec_model.at(std::make_tuple(state,next_state,m_threads))->eval(m_nodes,m_sz);

            enQ_bcast( evQ, nullBuf, msg_size, CHAR, 0, comm1 );
            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 8:
        {
            next_state = 9;
            msg_size  = msg_model.at(std::make_tuple(state,m_threads))->eval(m_nodes,m_sz);
            exec_time = exec_model.at(std::make_tuple(state,next_state,m_threads))->eval(m_nodes,m_sz);

            enQ_bcast( evQ, nullBuf, msg_size, CHAR, 0, comm1 );
            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 9:
        {
            next_state = 10;
            msg_size  = msg_model.at(std::make_tuple(state,m_threads))->eval(m_nodes,m_sz);
            exec_time = exec_model.at(std::make_tuple(state,next_state,m_threads))->eval(m_nodes,m_sz);

            enQ_bcast( evQ, nullBuf, msg_size, CHAR, 0, comm1 );
            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 10:
        {
            next_state = 11;
            exec_time = exec_model.at(std::make_tuple(state,next_state,m_threads))->eval(m_nodes,m_sz);

            // comm_dup
            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 11:
        {
            next_state = 12;
            exec_time = exec_model.at(std::make_tuple(state,next_state,m_threads))->eval(m_nodes,m_sz);

            // comm_dup
            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 12:
        {
            next_state = 13;
            exec_time = exec_model.at(std::make_tuple(state,next_state,m_threads))->eval(m_nodes,m_sz);

            // comm_dup
            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 13:
        {
            next_state = 14;
            exec_time = exec_model.at(std::make_tuple(state,next_state,m_threads))->eval(m_nodes,m_sz);

            // comm_dup
            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 14:
        {
            next_state = 15;
            msg_size  = msg_model.at(std::make_tuple(state,m_threads))->eval(m_nodes,m_sz);
            exec_time = exec_model.at(std::make_tuple(state,next_state,m_threads))->eval(m_nodes,m_sz);

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
            next_state = 16;
            msg_size  = msg_model.at(std::make_tuple(state,m_threads))->eval(m_nodes,m_sz);
            exec_time = exec_model.at(std::make_tuple(state,next_state,m_threads))->eval(m_nodes,m_sz);

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
            next_state = 17;
            msg_size  = msg_model.at(std::make_tuple(state,m_threads))->eval(m_nodes,m_sz);
            exec_time = exec_model.at(std::make_tuple(state,next_state,m_threads))->eval(m_nodes,m_sz);

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
            next_state = 18;
            exec_time = exec_model.at(std::make_tuple(state,next_state,m_threads))->eval(m_nodes,m_sz);

            // comm_dup
            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 18:
        {
            next_state = 19;
            exec_time = exec_model.at(std::make_tuple(state,next_state,m_threads))->eval(m_nodes,m_sz);

            // comm_dup
            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 19:
        {
            next_state = 20;
            exec_time = exec_model.at(std::make_tuple(state,next_state,m_threads))->eval(m_nodes,m_sz);

            // comm_dup
            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 20:
        {
            next_state = 21;
            exec_time = exec_model.at(std::make_tuple(state,next_state,m_threads))->eval(m_nodes,m_sz);

            // comm_dup
            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 21:
        {
            next_state = 22;
            msg_size  = msg_model.at(std::make_tuple(state,m_threads))->eval(m_nodes,m_sz);
            exec_time = exec_model.at(std::make_tuple(state,next_state,m_threads))->eval(m_nodes,m_sz);

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
            next_state = 23;
            msg_size  = msg_model.at(std::make_tuple(state,m_threads))->eval(m_nodes,m_sz);
            exec_time = exec_model.at(std::make_tuple(state,next_state,m_threads))->eval(m_nodes,m_sz);

            enQ_bcast( evQ, nullBuf, msg_size, CHAR, 0, comm0 );
            enQ_compute(evQ, exec_time);

            state = next_state;
    //m_nodes = 1; // TODO: get this from ember config and add to model
        }
        break;
    case 23:
        {
            next_state = 24;
            msg_size  = msg_model.at(std::make_tuple(state,m_threads))->eval(m_nodes,m_sz);
            exec_time = exec_model.at(std::make_tuple(state,next_state,m_threads))->eval(m_nodes,m_sz);

            enQ_allreduce( evQ, nullBuf, nullBuf, msg_size, CHAR, MP::MAX, comm1 );
            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 24:
        {
            next_state = 25;
            msg_size  = msg_model.at(std::make_tuple(state,m_threads))->eval(m_nodes,m_sz);
            exec_time = exec_model.at(std::make_tuple(state,next_state,m_threads))->eval(m_nodes,m_sz);

            enQ_allreduce( evQ, nullBuf, nullBuf, msg_size, CHAR, MP::MAX, comm1 );
            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 25:
        {
            next_state = 26;
            msg_size  = msg_model.at(std::make_tuple(state,m_threads))->eval(m_nodes,m_sz);
            exec_time = exec_model.at(std::make_tuple(state,next_state,m_threads))->eval(m_nodes,m_sz);

            enQ_allreduce( evQ, nullBuf, nullBuf, msg_size, CHAR, MP::MAX, comm0 );
            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 26:
        {
            next_state = 27;
            msg_size  = msg_model.at(std::make_tuple(state,m_threads))->eval(m_nodes,m_sz);
            exec_time = exec_model.at(std::make_tuple(state,next_state,m_threads))->eval(m_nodes,m_sz);

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
            next_state = 28;
            msg_size  = msg_model.at(std::make_tuple(state,m_threads))->eval(m_nodes,m_sz);
            exec_time = exec_model.at(std::make_tuple(state,next_state,m_threads))->eval(m_nodes,m_sz);

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
            next_state = 29;
            msg_size  = msg_model.at(std::make_tuple(state,m_threads))->eval(m_nodes,m_sz);
            exec_time = exec_model.at(std::make_tuple(state,next_state,m_threads))->eval(m_nodes,m_sz);

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
            next_state = 30;
            msg_size  = msg_model.at(std::make_tuple(state,m_threads))->eval(m_nodes,m_sz);
            exec_time = exec_model.at(std::make_tuple(state,next_state,m_threads))->eval(m_nodes,m_sz);

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
            next_state = 31;
            msg_size  = msg_model.at(std::make_tuple(state,m_threads))->eval(m_nodes,m_sz);
            exec_time = exec_model.at(std::make_tuple(state,next_state,m_threads))->eval(m_nodes,m_sz);

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
            next_state = 32;
            msg_size  = msg_model.at(std::make_tuple(state,m_threads))->eval(m_nodes,m_sz);
            exec_time = exec_model.at(std::make_tuple(state,next_state,m_threads))->eval(m_nodes,m_sz);

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
            next_state = 33;
            msg_size  = msg_model.at(std::make_tuple(state,m_threads))->eval(m_nodes,m_sz);
            exec_time = exec_model.at(std::make_tuple(state,next_state,m_threads))->eval(m_nodes,m_sz);

            enQ_bcast( evQ, nullBuf, msg_size, CHAR, 0, comm2 );
            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 33:
        {
            next_state = 34;
            msg_size  = msg_model.at(std::make_tuple(state,m_threads))->eval(m_nodes,m_sz);
            exec_time = exec_model.at(std::make_tuple(state,next_state,m_threads))->eval(m_nodes,m_sz);

            enQ_allreduce( evQ, nullBuf, nullBuf, msg_size, CHAR, MP::MAX, comm2 );
            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 34:
        {
            next_state = 35;
            msg_size  = msg_model.at(std::make_tuple(state,m_threads))->eval(m_nodes,m_sz);
            exec_time = exec_model.at(std::make_tuple(state,next_state,m_threads))->eval(m_nodes,m_sz);

            enQ_alltoall( evQ,
                          nullBuf, msg_size, CHAR,
                          nullBuf, msg_size, CHAR, comm0 );

            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 35:
        {
            next_state = 36;
            msg_size  = msg_model.at(std::make_tuple(state,m_threads))->eval(m_nodes,m_sz);
            exec_time = exec_model.at(std::make_tuple(state,next_state,m_threads))->eval(m_nodes,m_sz);

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
            next_state = 37;
            msg_size  = msg_model.at(std::make_tuple(state,m_threads))->eval(m_nodes,m_sz);
            exec_time = exec_model.at(std::make_tuple(state,next_state,m_threads))->eval(m_nodes,m_sz);

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
            next_state = 38;
            msg_size  = msg_model.at(std::make_tuple(state,m_threads))->eval(m_nodes,m_sz);
            exec_time = exec_model.at(std::make_tuple(state,next_state,m_threads))->eval(m_nodes,m_sz);

            enQ_allreduce( evQ, nullBuf, nullBuf, msg_size, CHAR, MP::MAX, comm1 );
            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 38:
        {
            next_state = 39;
            msg_size  = msg_model.at(std::make_tuple(state,m_threads))->eval(m_nodes,m_sz);
            exec_time = exec_model.at(std::make_tuple(state,next_state,m_threads))->eval(m_nodes,m_sz);

            enQ_allreduce( evQ, nullBuf, nullBuf, msg_size, CHAR, MP::MAX, comm1 );
            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 39:
        {
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

            msg_size  = msg_model.at(std::make_tuple(state,m_threads))->eval(m_nodes,m_sz);
            exec_time = exec_model.at(std::make_tuple(state,next_state,m_threads))->eval(m_nodes,m_sz);

            enQ_allreduce( evQ, nullBuf, nullBuf, msg_size, CHAR, MP::MAX, comm1 );
            enQ_compute(evQ, exec_time);


            state = next_state;

        }
        break;
    case 40:
        {
            next_state = 41;
            msg_size  = msg_model.at(std::make_tuple(state,m_threads))->eval(m_nodes,m_sz);
            exec_time = exec_model.at(std::make_tuple(state,next_state,m_threads))->eval(m_nodes,m_sz);

            enQ_barrier(evQ,comm1);
            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 41:
        {
            next_state = 42;
            msg_size  = msg_model.at(std::make_tuple(state,m_threads))->eval(m_nodes,m_sz);
            exec_time = exec_model.at(std::make_tuple(state,next_state,m_threads))->eval(m_nodes,m_sz);

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
            next_state = 43;
            msg_size  = msg_model.at(std::make_tuple(state,m_threads))->eval(m_nodes,m_sz);
            exec_time = exec_model.at(std::make_tuple(state,next_state,m_threads))->eval(m_nodes,m_sz);

            enQ_bcast( evQ, nullBuf, msg_size, CHAR, 0, comm2 );
            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 43:
        {
            next_state = 44;
            msg_size  = msg_model.at(std::make_tuple(state,m_threads))->eval(m_nodes,m_sz);
            exec_time = exec_model.at(std::make_tuple(state,next_state,m_threads))->eval(m_nodes,m_sz);

            enQ_bcast( evQ, nullBuf, msg_size, CHAR, 0, comm0 );
            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 44:
        {
            next_state = 45;
            msg_size  = msg_model.at(std::make_tuple(state,m_threads))->eval(m_nodes,m_sz);
            exec_time = exec_model.at(std::make_tuple(state,next_state,m_threads))->eval(m_nodes,m_sz);

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
            next_state = 46;
            msg_size  = msg_model.at(std::make_tuple(state,m_threads))->eval(m_nodes,m_sz);
            exec_time = exec_model.at(std::make_tuple(state,next_state,m_threads))->eval(m_nodes,m_sz);
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
            next_state = 47;
            msg_size  = msg_model.at(std::make_tuple(state,m_threads))->eval(m_nodes,m_sz);
            exec_time = exec_model.at(std::make_tuple(state,next_state,m_threads))->eval(m_nodes,m_sz);

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
            next_state = 48;
            msg_size  = msg_model.at(std::make_tuple(state,m_threads))->eval(m_nodes,m_sz);
            exec_time = exec_model.at(std::make_tuple(state,next_state,m_threads))->eval(m_nodes,m_sz);

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
            next_state = 49;
            msg_size  = msg_model.at(std::make_tuple(state,m_threads))->eval(m_nodes,m_sz);
            exec_time = exec_model.at(std::make_tuple(state,next_state,m_threads))->eval(m_nodes,m_sz);

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
            next_state = -1;

            msg_size  = msg_model.at(std::make_tuple(state,m_threads))->eval(m_nodes,m_sz);
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
                exec_time = exec_model.at(std::make_tuple(state,next_state,m_threads))->eval(m_nodes,m_sz);
                enQ_compute(evQ, exec_time);

                idx_49 = 0;
            } else {
                next_state = 47;
                exec_time = exec_model.at(std::make_tuple(state,next_state,m_threads))->eval(m_nodes,m_sz);
                enQ_compute(evQ, exec_time);

                idx_49++;
            }

            state = next_state;
        }
        break;
    case 50:
        {
            // goes to 41 2 (70%) or 3 (30%) times then 25
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
                printf("idx_50 incorrect (%" PRIu64 ")\n", idx_50);
                break;
            }

            msg_size  = msg_model.at(std::make_tuple(state,m_threads))->eval(m_nodes,m_sz);
            exec_time = exec_model.at(std::make_tuple(state,next_state,m_threads))->eval(m_nodes,m_sz);

            enQ_allreduce( evQ, nullBuf, nullBuf, msg_size, CHAR, MP::MAX, comm1);
            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 51:
        {
            next_state = 52;
            msg_size  = msg_model.at(std::make_tuple(state,m_threads))->eval(m_nodes,m_sz);
            exec_time = exec_model.at(std::make_tuple(state,next_state,m_threads))->eval(m_nodes,m_sz);

            enQ_barrier(evQ,comm1);
            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 52:
        {
            next_state = 53;
            msg_size  = msg_model.at(std::make_tuple(state,m_threads))->eval(m_nodes,m_sz);
            exec_time = exec_model.at(std::make_tuple(state,next_state,m_threads))->eval(m_nodes,m_sz);

            enQ_allreduce( evQ, nullBuf, nullBuf, msg_size, CHAR, MP::MAX, comm1);
            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 53:
        {
            next_state = 54;
            msg_size  = msg_model.at(std::make_tuple(state,m_threads))->eval(m_nodes,m_sz);
            exec_time = exec_model.at(std::make_tuple(state,next_state,m_threads))->eval(m_nodes,m_sz);

            enQ_allreduce( evQ, nullBuf, nullBuf, msg_size, CHAR, MP::MAX, comm1);
            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 54:
        {
            next_state = 55;
            msg_size  = msg_model.at(std::make_tuple(state,m_threads))->eval(m_nodes,m_sz);
            exec_time = exec_model.at(std::make_tuple(state,next_state,m_threads))->eval(m_nodes,m_sz);

            enQ_bcast( evQ, nullBuf, msg_size, CHAR, 0, comm1 );
            enQ_compute(evQ, exec_time);

            state = next_state;

        }
        break;
    case 55:
        {
            next_state = 56;
            exec_time = exec_model.at(std::make_tuple(state,next_state,m_threads))->eval(m_nodes,m_sz);

            // MPI_comm_free
            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 56:
        {
            next_state = 57;
            exec_time = exec_model.at(std::make_tuple(state,next_state,m_threads))->eval(m_nodes,m_sz);

            // MPI_comm_free
            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 57:
        {
            next_state = 58;
            exec_time = exec_model.at(std::make_tuple(state,next_state,m_threads))->eval(m_nodes,m_sz);

            //MPI_comm_free
            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 58:
        {
            next_state = 59;
            exec_time = exec_model.at(std::make_tuple(state,next_state,m_threads))->eval(m_nodes,m_sz);

            //MPI_comm_free
            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 59:
        {
            next_state = 60;
            exec_time = exec_model.at(std::make_tuple(state,next_state,m_threads))->eval(m_nodes,m_sz);

            //MPI_comm_free
            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 60:
        {
            next_state = 61;
            exec_time = exec_model.at(std::make_tuple(state,next_state,m_threads))->eval(m_nodes,m_sz);

            //MPI_comm_free
            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 61:
        {
            next_state = 62;
            exec_time = exec_model.at(std::make_tuple(state,next_state,m_threads))->eval(m_nodes,m_sz);

            //MPI_comm_free
            enQ_compute(evQ, exec_time);

            state = next_state;
        }
        break;
    case 62:
        {
            next_state = 1;
            exec_time = exec_model.at(std::make_tuple(state,next_state,m_threads))->eval(m_nodes,m_sz);

            //MPI_comm_free
            enQ_compute(evQ, exec_time);

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
        printf("error\n Bad State %" PRIu64 " in rank %d\n", state, rank());
        break;
    }

    if (rank()==0 && trace) {
        printf("TRACE %" PRIu64 " -> %" PRIu64 " %.1lf %.1lf\n", prevState, next_state, msg_size, exec_time);
        prevState = state;
    }

    // signal if we are done
    if (done) {
        return true;
    } else {
        return false;
    }
}
