// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2014, Sandia Corporation
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
#include "sst/core/debug.h"

namespace SST {
namespace MemHierarchy {

//#define N CALL_INFO
#define Z 0

enum {ERROR, WARNING, INFO, L0, L1, L2, L3, L4, L5, L6};
#define _ERROR_ C,ERROR,0
#define _WARNING_ C,WARNING,0
#define _INFO_ C,INFO,0
#define _L0_ C,L0,0
#define _L1_ C,L1,0
#define _L2_ C,L2,0
#define _L3_ C,L3,0
#define _L4_ C,L4,0
#define _L5_ C,L5,0
#define _L6_ C,L5,0


struct mshrType {
    boost::variant<Addr, MemEvent*> elem;
    MemEvent* memEvent_;
    mshrType(MemEvent* _memEvent) : elem(_memEvent), memEvent_(_memEvent) {}
    mshrType(Addr _addr) : elem(_addr) {}
    //~mshrType(){ delete memEvent_; }
    
};

struct CtrlStats{
    uint64_t     TotalRequestsReceived_,
                 TotalMSHRHits_,
                 InvWaitingForUserLock_,
                 updgradeLatency_;
    
    CtrlStats(){
        TotalRequestsReceived_ = TotalMSHRHits_ = InvWaitingForUserLock_ = updgradeLatency_ = 0;
    }
};

#define MAX_CACHE_CHILDREN (512);
    
#define C CALL_INFO

const unsigned int kibi = 1024;
const unsigned int mebi = kibi * 1024;
const unsigned int gibi = mebi * 1024;
const unsigned int tebi = gibi * 1024;
const unsigned int pebi = tebi * 1024;
const unsigned int exbi = pebi * 1024;


using namespace boost::algorithm;
using namespace std;

inline long convertToBytes(std::string componentSize){
    trim(componentSize);
    to_upper(componentSize);

    if(componentSize.size() < 2) _abort(Cache, "Cache size is not correctly specified. \n");
    
    std::string value = componentSize.substr(0, componentSize.size() - 2);
    trim(value);

    std::string units = componentSize.substr(componentSize.size() - 2, componentSize.size());
    trim(units);
    boost::to_upper(units);
    
    long valueLong = atol(value.c_str());

    if (ends_with(units, "PB"))      return valueLong * pebi;
    else if (ends_with(units, "TB")) return valueLong * tebi;
    else if (ends_with(units, "GB")) return valueLong * gibi;
    else if (ends_with(units, "MB")) return valueLong * mebi;
    else if (ends_with(units, "KB")) return valueLong * kibi;
    else if (ends_with(units, "B")){
        string valueStr2 = value.substr(0, value.size() - 1);
        trim(valueStr2);
        return atol(valueStr2.c_str());
    }else{
        _abort(Cache, "Cache size is not correctly specified. String: %s. \n", value.c_str());
    }

    return 0L;
}

inline int log2Of(int x){
    int temp = x;
    int result = 0;
    while(temp >>= 1) result++;
    return result;
}

inline void printData(Output* dbg, string msg, vector<uint8_t>* data){
    /*dbg->debug(_L5_,"%s: ", msg.c_str()); unsigned int  j = 0;
    for( std::vector<uint8_t>::const_iterator i = data->begin(); i != data->end(); ++i, ++j)
        dbg->debug(_L5_,"%x", (int)*i);
    dbg->debug(_L5_, "\n");
    */

}

inline void printData(Output* dbg, string msg, vector<uint8_t>* data, Addr offset, unsigned int size){
    /*dbg->debug(_L5_,"%s: ", msg.c_str()); unsigned int  j = 0;
    for( std::vector<uint8_t>::const_iterator i = data->begin() + offset; i != data->end(); ++i, ++j){
        if(j < size){
            dbg->debug(_L5_,"%x", (int)*i);
        }
        else break;
    }
    dbg->debug(_L5_,"\n");
    */
}


}}
#endif	/* UTIL_H */

