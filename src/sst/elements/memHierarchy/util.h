// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef MEMHIERARCHY_UTIL_H
#define	MEMHIERARCHY_UTIL_H

#include <sst/core/stringize.h>
#include <sst/core/params.h>
#include <string>

using namespace std;

namespace SST {
namespace MemHierarchy {

/* Debug macros */
#ifdef __SST_DEBUG_OUTPUT__ /* From sst-core, enable with --enable-debug */
#define is_debug_addr(addr) (DEBUG_ADDR.empty() || DEBUG_ADDR.find(addr) != DEBUG_ADDR.end())
#define is_debug_event(ev) (DEBUG_ADDR.empty() || ev->doDebug(DEBUG_ADDR))
#define is_debug true
#else
#define is_debug_addr(addr) false
#define is_debug_event(ev) false
#define is_debug false
#endif

#define _INFO_ CALL_INFO,1,0
#define _L2_ CALL_INFO,2,0     //Important messages:  incoming requests, state changes, etc
#define _L3_ CALL_INFO,3,0     //Important messages:  incoming requests, state changes, etc
#define _L4_ CALL_INFO,4,0     //Important messages:  send request, forward request, send response
#define _L5_ CALL_INFO,5,0     //
#define _L6_ CALL_INFO,6,0     //BottomCC messages
#define _L7_ CALL_INFO,7,0     //TopCC messages
#define _L8_ CALL_INFO,8,0     //Atomics
#define _L9_ CALL_INFO,9,0     //MSHR messages
#define _L10_ CALL_INFO,10,0   //Directory controller, Bus, Memory Controller
#define _L20_ CALL_INFO,20,0   //Debug at function call granularity

// Type conversions - TODO are these used anywhere?
const unsigned int kibi = 1024;
const unsigned int mebi = kibi * 1024;
const unsigned int gibi = mebi * 1024;
const unsigned int tebi = gibi * 1024;
const unsigned int pebi = tebi * 1024;
const unsigned int exbi = pebi * 1024;

typedef uint64_t Addr;

// Event attributes
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

/*
 * copy oldKey to newKey but don't overwrite if newKey already exists
 * return whether parameter was fixed
 */
inline bool fixupParam( SST::Params& params, const std::string oldKey, const std::string newKey ) {
    bool found;
    if (params.contains(newKey)) return false;

    std::string value = params.find<std::string>(oldKey, found);
    if (found) {
        params.insert(newKey, value);
        return true;
    //    params.erase( oldKey );
    }
    return false;
}

inline void fixupParams( Params& params, const std::string oldKey, const std::string newKey ) {
    // Params tmp = params.find_prefix_params( oldKey );

    // std::set<std::string> keys = tmp.getKeys();
    // std::set<std::string>::iterator iter = keys.begin();

    std::set<std::string> keys = params.getKeys();
    std::set<std::string>::iterator iter = keys.begin();
    for ( ; iter != keys.end(); ++iter ) {
        std::string prefix_key = iter->substr(0,oldKey.length());
        if ( prefix_key == oldKey ) {
            std::string value = params.find<std::string>( (*iter) );
            params.insert( newKey + (iter->substr(oldKey.length())), value );
            //    params.erase( oldKey + (*iter) );
        }
    }
}
/*
 *  IGNORE - ignore this request, drop it, do not retry any waiting requests
 *  DONE - this request finished, should retry
 *  STALL - this request is being handled and should be stalled in the MSHRs
 *  BLOCK - this request is blocked by a current outstanding request and should stall in the MSHRs
 *  REJECT - this request cannot be handled
 */
typedef enum {IGNORE, DONE, STALL, BLOCK, REJECT} CacheAction;

enum class CoherenceProtocol {MSI, MESI, NONE};

enum class Endpoint { CPU, Cache, Memory, Directory, Scratchpad };

}}
#endif	/* UTIL_H */

