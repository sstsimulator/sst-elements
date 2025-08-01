// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef MEMHIERARCHY_UTIL_H
#define	MEMHIERARCHY_UTIL_H

#include <sst/core/stringize.h>
#include <sst/core/params.h>

#include <iomanip>
#include <limits>
#include <string>

using namespace std;

namespace SST {
namespace MemHierarchy {

/* Debug macros */
#ifdef __SST_DEBUG_OUTPUT__ /* From sst-core, enable with --enable-debug */
#define mem_h_is_debug_addr(addr) (debug_addr_filter_.empty() || debug_addr_filter_.find(addr) != debug_addr_filter_.end())
#define mem_h_is_debug_event(ev) (debug_addr_filter_.empty() || ev->doDebug(debug_addr_filter_))
#define mem_h_is_debug true
#define mem_h_debug_output(level, fmt, ... ) dbg.debug( level, fmt, ##__VA_ARGS__ )
#else
#define mem_h_is_debug_addr(addr) false
#define mem_h_is_debug_event(ev) false
#define mem_h_is_debug false
#define mem_h_debug_output(level, fmt, ... )
#endif

#define _INFO_ CALL_INFO,1,0
#define _L2_ CALL_INFO,2,0      //Debug notes, potential error warnings, etc.
#define _L3_ CALL_INFO,3,0      //External events in
#define _L4_ CALL_INFO,4,0      //External events out
#define _L5_ CALL_INFO,5,0      //Internal state transitions (e.g., coherence)
#define _L6_ CALL_INFO,6,0      //Additional detail
#define _L7_ CALL_INFO,7,0      //Additional detail
#define _L8_ CALL_INFO,8,0      //Additional detail
#define _L9_ CALL_INFO,9,0      //Additional detail
#define _L10_ CALL_INFO,10,0    //Untimed phases
#define _L11_ CALL_INFO,11,0    //Data values
#define _L20_ CALL_INFO,20,0    //Debug at function call granularity

typedef uint64_t Addr;
#ifndef PRI_ADDR
#define PRI_ADDR PRIx64
#endif
#define NO_ADDR std::numeric_limits<uint64_t>::max();

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

inline std::string getDataString(std::vector<uint8_t>* data) {
    std::stringstream value;
    value << std::hex << std::setfill('0');
    for (unsigned int i = 0; i < data->size(); i++) {
        value << std::hex << std::setw(2) << (int)data->at(i);
    }
    return value.str();
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

enum class Endpoint { CPU, Cache, Memory, Directory, Scratchpad, MMIO };

}}
#endif	/* UTIL_H */

