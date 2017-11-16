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


#ifndef COMPONENTS_FIREFLY_SIMPLE_MEMORY_MODEL_H
#define COMPONENTS_FIREFLY_SIMPLE_MEMORY_MODEL_H

class SimpleMemoryModel {

  public:

   struct MemOp {
        enum Op { NotInit, BusLoad, BusStore, LocalLoad, LocalStore, HostLoad, HostStore, HostCopy, BusDmaToHost, BusDmaFromHost };

		MemOp( ) : addr(0), length(0), type(NotInit) {}
		MemOp( Hermes::Vaddr addr, size_t length, Op op) : addr(addr), length(length), type(op) {}
		MemOp( Hermes::Vaddr dest, Hermes::Vaddr src, size_t length, Op op) : dest(dest), src(src), length(length), type(op) {}

 		Op type;
        Hermes::Vaddr addr;
        Hermes::Vaddr src;
        Hermes::Vaddr dest;

        size_t   length;
    };

	virtual SimTime_t calcHostDelay( std::vector< MemOp >& ops ) {
		return 0;
	}

	virtual SimTime_t calcNicDelay( std::vector< MemOp >& ops ) {
		SimTime_t delay = 0;
#if 0 
		printf("%s() size=%lu\n",__func__, ops.size() );
#endif
		for ( size_t i = 0; i < ops.size(); i++ ) {
			switch ( ops[i].type ) {
			case MemOp::BusLoad:
				delay += 150; 
				break;
			case MemOp::BusStore:
				delay += 1; 
				break;
			case MemOp::LocalLoad:
				delay += 1; 
				break;
			case MemOp::LocalStore:
				delay += 1; 
				break;
			case MemOp::BusDmaToHost:
				delay += 150; 
				break;
			case MemOp::BusDmaFromHost:
				delay += 150; 
				break;
			default:
				assert( 0);
			}
 
#if 0 
			printf("%s() addr=%#lx length=%lu op=%s\n",__func__, ops[i].addr, ops[i].length, getName( ops[i].type ) );
#endif
		}
		return delay;
	}

  private:
	const char* getName( MemOp::Op type ) {
		switch( type ) {
		case MemOp::BusLoad: return "BusLoad";
		case MemOp::BusStore: return "BusStore";
		case MemOp::LocalLoad: return "LocalLoad";
		case MemOp::LocalStore: return "LocalStore";
		case MemOp::HostLoad: return "HostLoad";
		case MemOp::HostStore: return "HostStore";
		case MemOp::NotInit: return "NotInit"; 
		case MemOp::HostCopy: return "HostCopy"; 
		case MemOp::BusDmaToHost: return "BusDmaToHost"; 
		case MemOp::BusDmaFromHost: return "BusDmaFromHost";	
		}
		return "";
	}
}; 

#endif
