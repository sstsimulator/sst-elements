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

#ifndef _SSTDISKSIM_TRACEREADER_H
#define _SSTDISKSIM_TRACEREADER_H

#include "sstdisksim_event.h"

#include <sst/core/log.h>
#include <sst/core/component.h>
#include <sst/core/simulation.h>
#include <stdlib.h>
#include <stddef.h>

#include "syssim_driver.h"
#include <disksim_interface.h>
#include <disksim_rand48.h>

#include <sstdisksim.h>
#include <sstdisksim_event.h>
#include "sstdisksim_tau_parser.h"

using namespace std;
using namespace SST;

#ifndef DISKSIM_DBG
#define DISKSIM_DBG 0
#endif

class sstdisksim_tracereader : public Component {

 public:

  sstdisksim_tracereader( ComponentId_t id, Params_t& params );
  ~sstdisksim_tracereader();
//  int Setup();  // Renamed per Issue 70 - ALevine
//  int Finish();
  void setup(); 
  void finish();

  bool clock(Cycle_t current);

 private:

  std::string traceFile;
  std::string edfFile;

  Params_t __params;
  ComponentId_t __id;
  sstdisksim_tau_parser* __parser;

  /* To be removed later-this is just to test the component
     before we start having trace-reading functionality. */

  Log< DISKSIM_DBG >&  __dbg;
  
  sstdisksim_tracereader( const sstdisksim_tracereader& c );
  
  SST::Link* diskmodel;
  
  friend class boost::serialization::access;
  template<class Archive>
    void save(Archive & ar, const unsigned int version) const
    {
      ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
      ar & BOOST_SERIALIZATION_NVP(link);
    }
  
  template<class Archive>
    void load(Archive & ar, const unsigned int version)
    {
      ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
      ar & BOOST_SERIALIZATION_NVP(diskmodel);
    }
	
  BOOST_SERIALIZATION_SPLIT_MEMBER()    
};

#endif /* _SSTDISKSIM_TRACEREADER_H */
