// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

/*
 * File:   util.h
 * Author: Caesar De la Paz III
 * Email:  caesar.sst@gmail.com
 */

#ifndef UTIL_H
#define	UTIL_H

#include <boost/algorithm/string.hpp>
#include <boost/variant.hpp>
#include <string>
#include <assert.h>

#include "memEvent.h"

namespace SST {
namespace MemHierarchy {

#define Z 0

#   define ASSERT_MSG(condition, message) \
    do { \
        if (! (condition)) { \
            std::cerr << "Assertion `" #condition "` failed in " << __FILE__ \
                      << " line " << __LINE__ << ": " << message << std::endl; \
            std::exit(EXIT_FAILURE); \
        } \
    } while (false)

enum {ERROR, WARNING, INFO, L3, L4, L5, L6, L7, L8, L9, L10};
#define _ERROR_ CALL_INFO,ERROR,0
#define _WARNING_ CALL_INFO,WARNING,0
#define _INFO_ CALL_INFO,INFO,0
#define _L3_ CALL_INFO,L3,0     //Important messages:  incoming requests, state changes, etc
#define _L4_ CALL_INFO,L4,0     //Important messages:  send request, forward request, send response
#define _L5_ CALL_INFO,L5,0     //
#define _L6_ CALL_INFO,L6,0     //BottomCC messages
#define _L7_ CALL_INFO,L7,0     //TopCC messages
#define _L8_ CALL_INFO,L8,0     //Atomics
#define _L9_ CALL_INFO,L9,0     //MSHR messages
#define _L10_ CALL_INFO,L10,0   //Directory controller, Bus, Memory Controller

const unsigned int kibi = 1024;
const unsigned int mebi = kibi * 1024;
const unsigned int gibi = mebi * 1024;
const unsigned int tebi = gibi * 1024;
const unsigned int pebi = tebi * 1024;
const unsigned int exbi = pebi * 1024;


using namespace boost::algorithm;
using namespace std;

/*
 *  Replace uB or UB (where u/U is a SI unit)
 *  with uiB or UiB for unit algebra
 */
inline void fixByteUnits(std::string &unitstr) {
    trim(unitstr);
    size_t pos;
    string checkstring = "kKmMgGtTpP";
    if ((pos = unitstr.find_first_of(checkstring)) != string::npos) {
        pos++;
        if (pos < unitstr.length() && unitstr[pos] == 'B') {
            unitstr.insert(pos, "i");
        }
    }
}

inline int log2Of(int x){
    int temp = x;
    int result = 0;
    while(temp >>= 1) result++;
    return result;
}

inline bool isPowerOfTwo(unsigned int x) {
    return !(x & (x-1));   
}

inline void printData(Output* dbg, string msg, vector<uint8_t>* data){
    /*dbg->debug(_L10_,"%s: ", msg.c_str());
    unsigned int  j = 0;
    for( std::vector<uint8_t>::const_iterator i = data->begin(); i != data->end(); ++i, ++j)
        dbg->debug(_L10_,"%x", (int)*i);
    dbg->debug(_L10_, "\n");
    */
}

inline void printData(Output* dbg, string msg, vector<uint8_t>* data, Addr offset, unsigned int size){
    /*dbg->debug(_L10_,"%s: ", msg.c_str()); unsigned int  j = 0;
    dbg->debug(_L10_,"size: %lu", data->size());
    for( std::vector<uint8_t>::const_iterator i = data->begin() + offset; i != data->end(); ++i, ++j){
        if(j < size){
            dbg->debug(_L10_,"%x", (int)*i);
        }
        else break;
    }
    dbg->debug(_L10_,"\n");
    */
}
/*
 *  IGNORE - ignore this request, drop it, do not retry any waiting requests
 *  DONE - this request finished, should retry
 *  STALL - this request is being handled and should be stalled in the MSHRs
 *  BLOCK - this request is blocked by a current outstanding request and should stall in the MSHRs
 */
typedef enum {IGNORE, DONE, STALL, BLOCK } CacheAction;

}}
#endif	/* UTIL_H */

