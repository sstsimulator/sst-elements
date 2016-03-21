// Copyright 2013-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2015, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _H_HERMES
#define _H_HERMES

#include <sst/core/module.h>
#include <sst/core/component.h>
#include <sst/core/subcomponent.h>

#include "functor.h"

using namespace SST;

namespace SST {

namespace Hermes {

class NodePerf : public Module {
  public:
    virtual double getFlops() { assert(0); }
    virtual double getBandwidth() { assert(0); }
    virtual double calcTimeNS_flops( int instructions ) { assert(0); } 
    virtual double calcTimeNS_bandwidth( int bytes ) { assert(0); } 
};

class OS : public SubComponent {
  public:
	OS( Component *owner ) : SubComponent( owner ) {}
    virtual void _componentInit( unsigned int phase ) {}
    virtual void _componentSetup( void ) {}
    virtual void printStatus( Output& ) {}
    virtual int  getNid() { assert(0); }
    virtual void finish() {}
    virtual NodePerf* getNodePerf() { assert(0); }
};

class Interface : public Module {
  public:
    virtual void setup() {} 
    virtual void finish() {} 
    virtual void setOS( OS* ) { assert(0); }
    virtual std::string getName() { assert(0); }
    void setSize(int val) { _size = val; }
    void setRank(int val ) { _rank = val; }
    int getSize() { return _size; }
    int getRank() { return _rank; }
  private:
	int _rank;
	int _size;
};

}

}

#endif
