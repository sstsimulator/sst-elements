// Copyright 2009-2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2013, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _SIMPLECLOCKER_H
#define _SIMPLECLOCKER_H

#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>

#include <cstring>
#include <string>

namespace SST {
namespace SimpleClockerComponent {

class simpleClocker : public SST::Component {
public:

  simpleClocker(SST::ComponentId_t id, SST::Component::Params_t& params);
//  int Setup()  { return 0; }  // Renamed per Issue 70 - ALevine
//  int Finish() { return 0; }
  void setup()  { }
  void finish() { }

private:
  simpleClocker();  // for serialization only
  simpleClocker(const simpleClocker&); // do not implement
  void operator=(const simpleClocker&); // do not implement

  virtual bool tick( SST::Cycle_t );

  std::string clock_frequency_str;
  int clock_count;

  friend class boost::serialization::access;
  template<class Archive>
  void save(Archive & ar, const unsigned int version) const
  {
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
    ar & BOOST_SERIALIZATION_NVP(clock_frequency_str);
    ar & BOOST_SERIALIZATION_NVP(clock_count);
  }

  template<class Archive>
  void load(Archive & ar, const unsigned int version) 
  {
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
    ar & BOOST_SERIALIZATION_NVP(clock_frequency_str);
    ar & BOOST_SERIALIZATION_NVP(clock_count);
  }

  BOOST_SERIALIZATION_SPLIT_MEMBER()

};

}
}

#endif /* _SIMPLECLOCKER_H */
