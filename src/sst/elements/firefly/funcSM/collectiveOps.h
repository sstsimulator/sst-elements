// Copyright 2013-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef COMPONENTS_FIREFLY_FUNCSM_COLLECTIVEOPS_H
#define COMPONENTS_FIREFLY_FUNCSM_COLLECTIVEOPS_H

#include "sst/elements/hermes/msgapi.h"

namespace SST {
namespace Firefly {

template< class T >
T min( T x, T y )
{
    return x < y ? x : y; 
}

template< class T >
T max( T x, T y )
{
    return x > y ? x : y; 
}

template< class T >
T sum( T x, T y )
{
    return x + y;
}

template< class T>
T doOp( T x, T y, MP::ReductionOperation op )
{
    T retval = 0;
    switch( op ) {
      case MP::SUM: 
        retval = sum( x, y );
        break;
      case MP::MIN: 
        retval = min( x, y );
        break;
      case MP::MAX: 
        retval = max( x, y );
        break;
      case MP::NOP: 
        assert(0);
        break;
    } 
//    printf("%x %x %x\n",retval,x,y);
    return retval;
} 

template< class T >
void collectiveOp( T* input[], int numIn, T result[],
                    int count, MP::ReductionOperation op )
{
         
    for ( int c = 0; c < count; c++ ) {
        result[c] = input[0][c];
        for ( int n = 1; n < numIn; n++ ) {
            result[c] = doOp( result[c], input[n][c], op );  
        }  
    } 
}

inline void collectiveOp( void* input[], int numIn, void* result, int count, 
        MP::PayloadDataType dtype, MP::ReductionOperation op )
{
    switch ( dtype  ) {
      case MP::CHAR: 

            collectiveOp( (char**)(input), numIn, static_cast<char*>(result),
                        count, op );
            break;
      case MP::INT:
            collectiveOp( (int**)(input), numIn, static_cast<int*>(result),
                        count, op );
            break;
      case MP::LONG:
            collectiveOp( (long**)(input), numIn, static_cast<long*>(result),
                        count, op );
            break;
      case MP::DOUBLE:
            collectiveOp( (double**)(input), numIn,static_cast<double*>(result),
                        count, op );
            break;
      case MP::FLOAT:
            collectiveOp( (float**)(input), numIn, static_cast<float*>(result),
                        count, op );
            break;
      case MP::COMPLEX:
            assert(0);
            break;
    }
}

}
}

#endif
