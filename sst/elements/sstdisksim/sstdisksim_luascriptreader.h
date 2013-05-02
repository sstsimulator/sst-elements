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

#ifndef _SSTDISKSIM_H
#define _SSTDISKSIM_H

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

extern "C"
{
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

enum lua_value_types
{
  L_INT=1,
  L_DOUBLE,
  L_LONG,
  L_STRING,
  L_BOOLEAN
};

using namespace std;
using namespace SST;

#ifndef DISKSIM_DBG
#define DISKSIM_DBG 0
#endif

class sstdisksim_luascriptreader : public Component {

 public:

  sstdisksim_luascriptreader( ComponentId_t id, Params_t& params );
  ~sstdisksim_luascriptreader();
//  int Setup();  // Renamed per Issue 70 - ALevine
//  int Finish();
  void setup(); 
  void finish();

  int luaRead(int count, int pos, int devno);
  int luaWrite(int count, int pos, int devno);
  lua_State* __L;
  lua_State* __otherthread;

 private:

  std::string traceFile;

  bool __done;
  Params_t __params;
  ComponentId_t __id;

  Log< DISKSIM_DBG >&  __dbg;
  
  sstdisksim_luascriptreader( const sstdisksim_luascriptreader& c );
  
  SST::Link* link;
  
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
      ar & BOOST_SERIALIZATION_NVP(link);
    }
	
  BOOST_SERIALIZATION_SPLIT_MEMBER()    
};

#endif /* _SSTDISKSIM_H */
