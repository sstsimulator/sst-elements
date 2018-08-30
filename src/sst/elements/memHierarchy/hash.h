// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
// 
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

/*
 * File:   hash.h
 * Author: Caesar De la Paz III
 * Email:  caesar.sst@gmail.com
 */


#ifndef HASH_H
#define	HASH_H

#include <sst_config.h>
#include <stdint.h>


namespace SST{ namespace MemHierarchy{

class HashFunction{
public:
    HashFunction() {};
    virtual ~HashFunction() {};

    virtual uint64_t hash(uint32_t ID, uint64_t value) = 0;
};

/* Simplest ID hashing */
class PureIdHashFunction : public HashFunction {
public:
    inline uint64_t hash(uint32_t ID, uint64_t value) {
        return value;
    }
};

/* This function is taken from the C99 standard's RNG and should uniquely map
   each input to an output. */
class LinearHashFunction : public HashFunction {
public:
  uint64_t hash(uint32_t ID, uint64_t x) {
      return 1103515245*x + 12345;
  }
};

/* Just a simple xor-based hash. */
class XorHashFunction : public HashFunction {
public:
  uint64_t hash(uint32_t ID, uint64_t x) {
    unsigned char b[8];
    for (unsigned i = 0; i < 8; ++i)
      b[i] = (x >> (i*8))&0xff;

    for (unsigned i = 0; i < 7; ++i)
      b[i] ^= b[i + 1];

    uint64_t result = 0;
    for (unsigned i = 0; i < 8; ++i)
      result |= (b[i]<<(i*8));
    
    return result;
  }
};

}}
#endif	
/* HASH_H */
