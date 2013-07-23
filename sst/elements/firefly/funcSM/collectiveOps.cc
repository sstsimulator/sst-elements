// Copyright 2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "funcSM/collectiveOps.h"

#define DBGX( fmt, args... ) \
{\
    fprintf( stderr, "%s():%d: "fmt, __func__, __LINE__, ##args);\
}

namespace SST {
namespace Firefly {

void EEE( std::vector<void*>& input, int numIn, void* result, int count,
            Hermes::PayloadDataType dType, Hermes::ReductionOperation op )
{
    DBGX("%d\n",numIn);
    
   *((int*)result ) = *((int*)input[0]);
    DBGX("%#010x\n",*((int*)result ));
    for ( int i = 1; i < numIn; i++ ) {
        DBGX( "%#010x\n", *((int*)input[i])  );
        *((int*)result ) |= *((int*)input[i]);
    }
    DBGX("%#010x\n",*((int*)result ));
}


template <class Type>
void EEE( std::vector<Type*>& input, int numIn, Type* result, int count,
                Hermes::ReductionOperation op )
{
    DBGX("numIn=%d count=%d op=%d\n",numIn, count, op);
    for ( int i = 0; i < numIn; i++ ) {
    }
}

}
}
