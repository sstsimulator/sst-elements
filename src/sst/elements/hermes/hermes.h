// Copyright 2013-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2017, Sandia Corporation
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _H_HERMES
#define _H_HERMES

#include <sst/core/module.h>
#include <sst/core/subcomponent.h>
#include <sst/core/component.h>
#include <sst/core/subcomponent.h>
#include "sst/elements/thornhill/detailedCompute.h"
#include "sst/elements/thornhill/memoryHeapLink.h"

#include "functor.h"

using namespace SST;

namespace SST {

namespace Hermes {

class MemAddr {

  public:
    enum Type { Normal, Shmem }; 
    MemAddr( uint64_t sim, void* back, Type type = Normal) : 
        type(type), simVAddr(sim), backing(back) {} 
    MemAddr( Type type = Normal) : type(type), simVAddr(0), backing(NULL) {} 
    MemAddr( void* backing ) : type(Normal), simVAddr(0), backing(backing) {} 

    char& operator[](size_t index) {
        return ((char*)backing)[index];
    }
    MemAddr offset( size_t val ) {
        MemAddr addr = *this;
        addr.simVAddr += val; 
        if ( addr.backing ) {
            addr.backing = (char*) addr.backing + val;
        }
        return addr;
    }
    uint64_t getSimVAddr( ) {
        return simVAddr;
    }
    void setSimVAddr( uint64_t addr ) {
        simVAddr = addr;
    }
    void* getBacking( ) {
        return backing;
    }
    void setBacking( void* ptr ) {
        backing = ptr;
    }
            
  private:
    Type type;
    uint64_t simVAddr;
    void*	 backing;
};

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
    virtual Thornhill::DetailedCompute* getDetailedCompute() { assert(0); }
	virtual Thornhill::MemoryHeapLink*  getMemHeapLink() { assert(0); }
};

class Interface : public SubComponent {
  public:
    Interface( Component* owner ) : SubComponent(owner), _rank(-1), _size(0) {}
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
