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

#ifndef _SIMPLESTATISTICSCOMPONENT_H
#define _SIMPLESTATISTICSCOMPONENT_H

#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/rng/sstrand.h>
#include <sst/core/rng/mersenne.h>
#include <sst/core/rng/marsaglia.h>
#include <sst/core/statapi/stataccumulator.h>
#include <sst/core/statapi/stathistogram.h>

using namespace SST;
using namespace SST::RNG;
using namespace SST::Statistics;

namespace SST {
namespace SimpleStatisticsComponent {

class simpleStatisticsComponent : public SST::Component {
public:
  simpleStatisticsComponent(ComponentId_t id, Params& params);
  void setup()  { }
  void finish() { }

private:
  simpleStatisticsComponent();  // for serialization only
  simpleStatisticsComponent(const simpleStatisticsComponent&); // do not implement
  void operator=(const simpleStatisticsComponent&); // do not implement

  virtual bool Clock1Tick(SST::Cycle_t);
  
  SSTRandom* rng;
  std::string rng_type;
  int rng_max_count;
  int rng_count;
  
  // Histogram Statistics
  Statistic<uint32_t>*  stat1_U32; 
  Statistic<uint64_t>*  stat2_U64; 
  Statistic<int32_t>*   stat3_I32; 
  Statistic<int64_t>*   stat4_I64; 

  // Accumulator Statistics
  Statistic<uint32_t>*  stat5_U32; 
  Statistic<uint64_t>*  stat6_U64; 
  Statistic<uint32_t>*  stat7_U32_NOTUSED; 

  friend class boost::serialization::access;
  template<class Archive>
  void save(Archive & ar, const unsigned int version) const
  {
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
    ar & BOOST_SERIALIZATION_NVP(rng_count);
    ar & BOOST_SERIALIZATION_NVP(rng_max_count);
    ar & BOOST_SERIALIZATION_NVP(rng_type);

    ar & BOOST_SERIALIZATION_NVP(stat1_U32); 
    ar & BOOST_SERIALIZATION_NVP(stat2_U64); 
    ar & BOOST_SERIALIZATION_NVP(stat3_I32); 
    ar & BOOST_SERIALIZATION_NVP(stat4_I64); 
    ar & BOOST_SERIALIZATION_NVP(stat5_U32); 
    ar & BOOST_SERIALIZATION_NVP(stat6_U64); 
    ar & BOOST_SERIALIZATION_NVP(stat7_U32_NOTUSED); 
  }

  template<class Archive>
  void load(Archive & ar, const unsigned int version)
  {
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
    ar & BOOST_SERIALIZATION_NVP(rng_count);
    ar & BOOST_SERIALIZATION_NVP(rng_max_count);
    ar & BOOST_SERIALIZATION_NVP(rng_type);

    ar & BOOST_SERIALIZATION_NVP(stat1_U32); 
    ar & BOOST_SERIALIZATION_NVP(stat2_U64); 
    ar & BOOST_SERIALIZATION_NVP(stat3_I32); 
    ar & BOOST_SERIALIZATION_NVP(stat4_I64); 
    ar & BOOST_SERIALIZATION_NVP(stat5_U32); 
    ar & BOOST_SERIALIZATION_NVP(stat6_U64); 
    ar & BOOST_SERIALIZATION_NVP(stat7_U32_NOTUSED); 
  }

  BOOST_SERIALIZATION_SPLIT_MEMBER()
};

} // namespace SimpleStatistics
} //namespace SST

#endif /* _SIMPLESTATISTICSCOMPONENT_H */
