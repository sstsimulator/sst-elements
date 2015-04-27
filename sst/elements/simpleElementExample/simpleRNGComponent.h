// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _SIMPLERNGCOMPONENT_H
#define _SIMPLERNGCOMPONENT_H

#include "sst/core/component.h"
#include "sst/core/rng/sstrand.h"

using namespace SST;
using namespace SST::RNG;

namespace SST {
namespace SimpleRNGComponent {

class simpleRNGComponent : public SST::Component 
{
public:
    simpleRNGComponent(SST::ComponentId_t id, SST::Params& params);
    ~simpleRNGComponent();
    void setup()  { }
    void finish() { }

private:
    simpleRNGComponent();  // for serialization only
    simpleRNGComponent(const simpleRNGComponent&); // do not implement
    void operator=(const simpleRNGComponent&); // do not implement
    
    virtual bool tick(SST::Cycle_t);

    Output* output;
    SSTRandom*  rng;
    std::string rng_type;
    int rng_max_count;
    int rng_count;
    
    friend class boost::serialization::access;
    template<class Archive>
    void save(Archive & ar, const unsigned int version) const
    {
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
        ar & BOOST_SERIALIZATION_NVP(rng_count);
        ar & BOOST_SERIALIZATION_NVP(rng_max_count);
        ar & BOOST_SERIALIZATION_NVP(rng_type);
    }
    
    template<class Archive>
    void load(Archive & ar, const unsigned int version)
    {
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
        ar & BOOST_SERIALIZATION_NVP(rng_count);
        ar & BOOST_SERIALIZATION_NVP(rng_max_count);
        ar & BOOST_SERIALIZATION_NVP(rng_type);
    }
    
    BOOST_SERIALIZATION_SPLIT_MEMBER()
};

} // namespace SimpleRNGComponent
} // namespace SST

#endif /* _SIMPLERNGCOMPONENT_H */
