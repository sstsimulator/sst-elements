// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
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

#ifndef _H_EMBER_BFS_MOTIF
#define _H_EMBER_BFS_MOTIF

#include "mpi/embermpigen.h"
#include <cstdint>
#include <iostream>
#include <ostream>
#include <vector>
#include <sst/core/rng/mersenne.h>
#include <sst/core/shared/sharedMap.h>
#include <sst/core/output.h>

#include <cmath>
#include <map>
#include <tuple>

namespace SST {
namespace Ember {

struct Model : public SST::Core::Serialization::serializable
{
    double scale = 1.0;
    virtual double eval(int nodes, int size) = 0;

    // TODO: Remove setter or make `scale` private and add getter
    void setScale(double newScale)
    {
        scale = newScale;
    }
};

struct ConstModel : public Model
{
    double val;

    ConstModel() : val(0) {}

    ConstModel(double val) : val(val) {}

    double eval(int nodes, int size) override {
        return val*scale;
    }

    bool operator==(const ConstModel& lhs) const
    {
        return (val == lhs.val);
    }

    bool operator!=(const ConstModel& lhs) const
    {
        return (val != lhs.val);
    }

    void serialize_order(SST::Core::Serialization::serializer& ser) override
    {
        SST_SER(val);
    }

    ImplementSerializable(SST::Ember::ConstModel);

};

struct BilinearModel : public Model
{
    double A, B, C, D;

    BilinearModel() : A(0), B(0), C(0), D(0) {}

    BilinearModel(double A, double B, double C, double D)
        : A(A)
        , B(B)
        , C(C)
        , D(D)
    {}

    double eval(int nodes, int size) override {
        return std::nearbyint(scale*(A*nodes*size + B*nodes + C*size + D));
    }

    bool operator==(const BilinearModel& lhs) const
    {
        return (A == lhs.A && B == lhs.B && C == lhs.C && D == lhs.D);
    }

    bool operator!=(const BilinearModel& lhs) const
    {
        return (A != lhs.A || B != lhs.B || C != lhs.C || D != lhs.D);
    }

    void serialize_order(SST::Core::Serialization::serializer& ser) override
    {
        SST_SER(A);
        SST_SER(B);
        SST_SER(C);
        SST_SER(D);
    }

    ImplementSerializable(SST::Ember::BilinearModel);

};

struct TraceModel : public Model
{
    std::vector<double> val;
    int idx = 0;

    TraceModel() : val(0) {}

    TraceModel(std::vector<double> val) : val(val)  {}

    double eval(int nodes, int size) override
    {
        int _idx = idx;
        idx = (idx + 1) == val.size() ? 0 : idx+1;
        return std::nearbyint(scale*val[_idx]);
    }

    bool operator==(const TraceModel& lhs) const
    {
        return (val == lhs.val);
    }

    bool operator!=(const TraceModel& lhs) const
    {
        return (val != lhs.val);
    }

    void serialize_order(SST::Core::Serialization::serializer& ser) override
    {
        SST_SER(val);
    }

    ImplementSerializable(SST::Ember::TraceModel);

};

struct ExpModel : public Model
{
    double A, B, C, D, memo = -1;

    ExpModel() : A(0), B(0), C(0), D(0) {}

    ExpModel(double A, double B, double C, double D)
        : A(A)
        , B(B)
        , C(C)
        , D(D)
    {}

    double _eval(int nodes, int size) {
        return std::nearbyint(scale*exp(A*nodes*size + B*nodes + C*size + D));
    }

    double eval(int nodes, int size) override {
        // memoize result
        if (memo > -1) {
            return memo;
        }
        memo = _eval(nodes, size);
        return memo;
    }

    bool operator==(const ExpModel& lhs) const
    {
        return (A == lhs.A && B == lhs.B && C == lhs.C && D == lhs.D);
    }

    bool operator!=(const ExpModel& lhs) const
    {
        return (A != lhs.A || B != lhs.B || C != lhs.C || D != lhs.D);
    }

    void serialize_order(SST::Core::Serialization::serializer& ser) override
    {
        SST_SER(A);
        SST_SER(B);
        SST_SER(C);
        SST_SER(D);
    }

    ImplementSerializable(SST::Ember::ExpModel);

};

struct PolyModelND : public Model
{
    // coeff is a map from powers to coefficients, e.g.
    // [0,2,2]:0.4 means that the coefficient of x0^0 * x1^2 * x2^2 is 0.4
    // We will not worry about making sure all powers are there. We need only
    // evaluate what we find in this list
    std::map<std::vector<int>, double> coeff;
    double min; // minimum value returned by the model

    PolyModelND() : coeff(), min(0) {}

    PolyModelND(std::map<std::vector<int>, double> coeff, double min)
        : coeff(coeff)
        , min(min)
    {}

    // Evaluate the model at x
    double _eval(std::vector<int> x)
    {
        double val = 0.0;
        for (const auto &co : coeff) {         // loop over our map of powers->coefficients
            double term = co.second;             // value in map is the coefficient
            if ( x.size() != co.first.size() ) {
                std::cout << "x: " << x.size() << ", co: " << co.first.size() << std::endl;
            }
            for (int i = 0; i < x.size(); i++) {       // for each power, evaluate that power and multiple
                term *= pow(x[i], co.first[i]);
            }
            val += term;                        // sum terms
        }
        val *= scale;                           // scale value (used for time conversion)
        val = std::nearbyint(val);              // round so we don't get fractional message sizes
        return val > min ? val : min;           // don't return less than min, useful to remove negatives
    }

    double _eval(std::vector<int> x, int verbose)
    {

        //std::cout << "[" << x[0] << "," << x[1] << "]" << std::endl;

        double val = 0.0;
        for (const auto &co : coeff) {         // loop over our map of powers->coefficients
            //std::cout << "  [" << co.first[0] << "," << co.first[1] << "]: " << co.second << std::endl;
            double term = co.second;             // value in map is the coefficient
            for (int i = 0; i < x.size(); i++) {       // for each power, evaluate that power and multiple
                term *= pow(x[i], co.first[i]);
            }
            val += term;                        // sum terms
        }
        val *= scale;                           // scale value (used for time conversion)
        val = std::nearbyint(val);              // round so we don't get fractional message sizes
        val = val > min ? val : min;           // don't return less than min, useful to remove negatives
        //std::cout << "  -> " << val << std::endl;;
        return val;
    }

    double eval(int nodes, int size) override
    {
        std::vector<int> x;
        x.push_back(size);
        x.push_back(nodes);
        return _eval(x);
    }

    bool operator==(const PolyModelND& lhs) const
    {
        return (coeff == lhs.coeff);
    }

    bool operator!=(const PolyModelND& lhs) const
    {
        return (coeff != lhs.coeff);
    }

    void serialize_order(SST::Core::Serialization::serializer& ser) override
    {
        SST_SER(coeff);
    }

    ImplementSerializable(SST::Ember::PolyModelND);

};

struct PolyModel : public SST::Core::Serialization::serializable
{
    std::vector<double> coeff; // polynomial coefficients
    double scale; // scale the result of eval
    double min; // minimum value returned by the model

    PolyModel() : coeff(0), scale(1), min(0) {}

    PolyModel(std::vector<double> coeff, double scale, double min)
        : coeff(coeff)
        , scale(scale)
        , min(min)
    {}

    // Evaluate the model at x
    // x should be between 0 and 1
    double eval(double x)
    {
        double xt = 1.0;  // holds powers of x
        double res = 0.0; // accumulator
        for (auto c : coeff) {
            res += c * xt;
            xt  *= x;
        }
        res *= scale;
        res = std::nearbyint(res);
        return res > min ? res : min;
    }

    bool operator==(const PolyModel& lhs) const
    {
        return (coeff == lhs.coeff);
    }

    bool operator!=(const PolyModel& lhs) const
    {
        return (coeff != lhs.coeff);
    }

    void serialize_order(SST::Core::Serialization::serializer& ser) override
    {
        SST_SER(coeff);
    }

    ImplementSerializable(SST::Ember::PolyModel);

};

class EmberBFSGenerator : public EmberMessagePassingGenerator {

public:
    SST_ELI_REGISTER_SUBCOMPONENT(
        EmberBFSGenerator,
        "ember",
        "BFSMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "motif for the ComBLAS BFS code.",
        SST::Ember::EmberGenerator
    )

    SST_ELI_DOCUMENT_PARAMS(
        {   "arg.sz",          "Sets the graph size",              "14"},
        {   "arg.seed",        "Sets the RNG seed",                "1"},
        {   "arg.msg_model",   "Communication volume model file",  "msg.model"},
        {   "arg.exec_model",  "Computation timing model file",    "exec.model"},
    )

    SST_ELI_DOCUMENT_STATISTICS(
                                { "time-Init", "Time spent in Init event",          "ns",  0},
                                { "time-Finalize", "Time spent in Finalize event",  "ns", 0},
                                { "time-Rank", "Time spent in Rank event",          "ns", 0},
                                { "time-Size", "Time spent in Size event",          "ns", 0},
                                { "time-Send", "Time spent in Recv event",          "ns", 0},
                                { "time-Recv", "Time spent in Recv event",          "ns", 0},
                                { "time-Irecv", "Time spent in Irecv event",        "ns", 0},
                                { "time-Isend", "Time spent in Isend event",        "ns", 0},
                                { "time-Wait", "Time spent in Wait event",          "ns", 0},
                                { "time-Waitall", "Time spent in Waitall event",    "ns", 0},
                                { "time-Waitany", "Time spent in Waitany event",    "ns", 0},
                                { "time-Compute", "Time spent in Compute event",    "ns", 0},
                                { "time-Barrier", "Time spent in Barrier event",    "ns", 0},
                                { "time-Alltoallv", "Time spent in Alltoallv event", "ns", 0},
                                { "time-Alltoall", "Time spent in Alltoall event",  "ns", 0},
                                { "time-Allreduce", "Time spent in Allreduce event", "ns", 0},
                                { "time-Reduce", "Time spent in Reduce event",      "ns", 0},
                                { "time-Bcast", "Time spent in Bcast event",        "ns", 0},
                                { "time-Gettime", "Time spent in Gettime event",    "ns", 0},
                                { "time-Commsplit", "Time spent in Commsplit event", "ns", 0},
                                { "time-Commcreate", "Time spent in Commcreate event", "ns", 0}
                                )

public:
    EmberBFSGenerator(SST::ComponentId_t, Params& params);
    ~EmberBFSGenerator();
    bool generate( std::queue<EmberEvent*>& evQ);

private:
    Output out;

    int32_t m_sz; // graph size
    int32_t m_threads; // threads per rank
    int32_t m_nodes;
    int32_t m_ranks_per_node;

    uint64_t s_time;

    uint64_t state; uint64_t prevState;  // for debugging
    uint64_t iter;
    uint64_t maxIter;
    uint64_t idx_39; // state for node 39
    uint64_t idx_49; // state for node 49
    uint64_t idx_50; // state for node 50
    uint64_t square; // sqrt(size())

    uint64_t opposite; // our 'opposite' rank on the other side of the
                     // matrix
    Communicator comm0;  // 'row'
    Communicator comm1;  // comm world
    Communicator comm2;  // 'col'
    MessageResponse m_resp;
    std::vector<int>    comm0_ranks;
    std::vector<int>    comm1_ranks;
    std::vector<int>    comm2_ranks;
    uint32_t comm0_rank; // rank within comm0
    uint32_t comm1_rank; // rank within comm_world
    uint32_t comm2_rank; // rank within comm2

    // since we are not sending data, we use null buffers /
    // displacement maps
    void* nullBuf;
    int* nullDispMap;

    // start a new iteration
    void initIter();

    // all-gather message sizes for site 31
    int *recvCounts_31;

    // all-to-allV message sizessite 35
    int *sendCounts_35;
    int *recvCounts_35;

    // partner iterators for size 48/49;
    int s_partner_48;
    int r_partner_48;

    std::map<std::tuple<int,int>,std::unique_ptr<Model>> msg_model;
    std::map<std::tuple<int,int,int>,std::unique_ptr<Model>> exec_model;

    // Random number generator
    SST::RNG::MersenneRNG* rng;

    int trace; // whether to print a trace
};

}
}


#endif
