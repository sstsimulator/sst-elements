// Copyright 2013-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2025, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef COMPONENTS_FIREFLY_FUNCSM_COLLECTIVEOPS_H
#define COMPONENTS_FIREFLY_FUNCSM_COLLECTIVEOPS_H

#include "sst/elements/hermes/msgapi.h"

#include <cstdint>

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
    switch( op->type ) {
      case MP::ReductionOpType::Sum:
        retval = sum( x, y );
        break;
      case MP::ReductionOpType::Min:
        retval = min( x, y );
        break;
      case MP::ReductionOpType::Max:
        retval = max( x, y );
        break;
      case MP::ReductionOpType::Nop:
        assert(0);
        break;
      case MP::ReductionOpType::Func:
        assert(0);
        break;
    }
//    printf("%x %x %x\n",retval,x,y);
    return retval;
}

template<typename T >
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
    if ( op->type == MP::ReductionOpType::Func ) {
        op->userFunction( input, result, &count, &dtype );
        return;
    }

    switch ( dtype  ) {
      case MP::CHAR:
            collectiveOp( (char**)(input), numIn, static_cast<char*>(result),
                        count, op );
            break;
      case MP::SIGNED_CHAR:
            collectiveOp( (signed char**)(input), numIn, static_cast<signed char*>(result),
                        count, op );
            break;
      case MP::UNSIGNED_CHAR:
            collectiveOp( (unsigned char**)(input), numIn, static_cast<unsigned char*>(result),
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
      case MP::LONG_LONG:
            collectiveOp( (long long**)(input), numIn, static_cast<long long*>(result),
                        count, op );
            break;
      case MP::UNSIGNED_INT:
            collectiveOp( (unsigned int**)(input), numIn, static_cast<unsigned int*>(result),
                        count, op );
            break;
      case MP::UNSIGNED_LONG:
            collectiveOp( (unsigned long**)(input), numIn, static_cast<unsigned long*>(result),
                        count, op );
            break;
      case MP::UNSIGNED_LONG_LONG:
            collectiveOp( (unsigned long long**)(input), numIn, static_cast<unsigned long long*>(result),
                        count, op );
            break;
      case MP::INT8_T:
            collectiveOp( (std::int8_t**)(input), numIn, static_cast<std::int8_t*>(result),
                        count, op );
            break;
      case MP::INT16_T:
            collectiveOp( (std::int16_t**)(input), numIn, static_cast<std::int16_t*>(result),
                        count, op );
            break;
      case MP::INT32_T:
            collectiveOp( (std::int32_t**)(input), numIn, static_cast<std::int32_t*>(result),
                        count, op );
            break;
      case MP::INT64_T:
            collectiveOp( (std::int64_t**)(input), numIn, static_cast<std::int64_t*>(result),
                        count, op );
            break;
      case MP::UINT8_T:
            collectiveOp( (std::uint8_t**)(input), numIn, static_cast<std::uint8_t*>(result),
                        count, op );
            break;
      case MP::UINT16_T:
            collectiveOp( (std::uint16_t**)(input), numIn, static_cast<std::uint16_t*>(result),
                        count, op );
            break;
      case MP::UINT32_T:
            collectiveOp( (std::uint32_t**)(input), numIn, static_cast<std::uint32_t*>(result),
                        count, op );
            break;
      case MP::UINT64_T:
            collectiveOp( (std::uint64_t**)(input), numIn, static_cast<std::uint64_t*>(result),
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
