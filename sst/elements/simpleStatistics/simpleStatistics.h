// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _SIMPLESTATISTICS_H
#define _SIMPLESTATISTICS_H

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
namespace SimpleStatistics {

class simpleStatistics : public SST::Component {
public:
  simpleStatistics(ComponentId_t id, Params& params);
  void setup()  { }
  void finish() { }

private:
  simpleStatistics();  // for serialization only
  simpleStatistics(const simpleStatistics&); // do not implement
  void operator=(const simpleStatistics&); // do not implement

  virtual bool Clock1Tick(SST::Cycle_t);
  virtual bool Clock2Tick(SST::Cycle_t, uint32_t);
  virtual bool Clock3Tick(SST::Cycle_t, uint32_t);

  virtual void Oneshot1Callback(uint32_t);
  virtual void Oneshot2Callback();
  
  SSTRandom* rng;
  std::string rng_type;
  int rng_max_count;
  int rng_count;
  
  TimeConverter*      tc;
  Clock::HandlerBase* Clock3Handler;

  // Histogram Statistics
  Statistic<uint32_t>*  histoU32; 
  Statistic<uint64_t>*  histoU64; 
  Statistic<int32_t>*   histoI32; 
  Statistic<int64_t>*   histoI64; 

  // Accumulator Statistics
  Statistic<uint32_t>*  accumU32; 
  Statistic<uint64_t>*  accumU64; 
  Statistic<uint32_t>*  accumU32_NOTUSED; 
  
  // Variables to store OneShot Callback Handlers
  OneShot::HandlerBase* callback1Handler;
  OneShot::HandlerBase* callback2Handler;

  friend class boost::serialization::access;
  template<class Archive>
  void save(Archive & ar, const unsigned int version) const
  {
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
    ar & BOOST_SERIALIZATION_NVP(rng_count);
    ar & BOOST_SERIALIZATION_NVP(rng_max_count);
    ar & BOOST_SERIALIZATION_NVP(rng_type);

    ar & BOOST_SERIALIZATION_NVP(histoU32); 
    ar & BOOST_SERIALIZATION_NVP(histoU64); 
    ar & BOOST_SERIALIZATION_NVP(histoI32); 
    ar & BOOST_SERIALIZATION_NVP(histoI64); 
    ar & BOOST_SERIALIZATION_NVP(accumU32); 
    ar & BOOST_SERIALIZATION_NVP(accumU64); 
    ar & BOOST_SERIALIZATION_NVP(accumU32_NOTUSED); 
  }

  template<class Archive>
  void load(Archive & ar, const unsigned int version)
  {
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
    ar & BOOST_SERIALIZATION_NVP(rng_count);
    ar & BOOST_SERIALIZATION_NVP(rng_max_count);
    ar & BOOST_SERIALIZATION_NVP(rng_type);

    ar & BOOST_SERIALIZATION_NVP(histoU32); 
    ar & BOOST_SERIALIZATION_NVP(histoU64); 
    ar & BOOST_SERIALIZATION_NVP(histoI32); 
    ar & BOOST_SERIALIZATION_NVP(histoI64); 
    ar & BOOST_SERIALIZATION_NVP(accumU32); 
    ar & BOOST_SERIALIZATION_NVP(accumU64); 
    ar & BOOST_SERIALIZATION_NVP(accumU32_NOTUSED); 
  }

  BOOST_SERIALIZATION_SPLIT_MEMBER()
};

} // namespace SimpleStatistics
} //namespace SST

#endif /* _SIMPLESTATISTICS_H */
