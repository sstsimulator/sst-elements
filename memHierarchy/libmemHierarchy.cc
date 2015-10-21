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

#include <sst_config.h>

#include "sst/core/serialization.h"
#include "sst/core/element.h"
#include "sst/core/component.h"

#include "memEvent.h"
#include "memNIC.h"
#include "cacheController.h"
#include "Sieve/sieveController.h"
#include "bus.h"
#include "trivialCPU.h"
#include "streamCPU.h"
#include "memoryController.h"
#include "directoryController.h"
#include "dmaEngine.h"
#include "memHierarchyInterface.h"
#include "memNIC.h"
#include "membackend/memBackend.h"
#include "membackend/simpleMemBackend.h"
#include "membackend/vaultSimBackend.h"
#include "networkMemInspector.h"

#ifdef HAVE_GOBLIN_HMCSIM
#include "membackend/goblinHMCBackend.h"
#endif

#ifdef HAVE_LIBDRAMSIM
#include "membackend/dramSimBackend.h"
#include "membackend/pagedMultiBackend.h"
#endif

#ifdef HAVE_LIBHYBRIDSIM
#include "membackend/hybridSimBackend.h"
#endif

#ifdef HAVE_FDSIM
#include "membackend/flashSimBackend.h"
#endif

using namespace SST;
using namespace SST::MemHierarchy;

static const char * memEvent_port_events[] = {"memHierarchy.MemEvent", NULL};
static const char * net_port_events[] = {"memHierarchy.MemRtrEvent", NULL};

static Component* create_Cache(ComponentId_t id, Params& params)
{
	return Cache::cacheFactory(id, params);
}

static const ElementInfoParam cache_params[] = {
    /* Required */
    {"cache_frequency",         "Required, string   - Clock frequency with units. For L1s, this is usually the same as the CPU's frequency."},
    {"cache_size",              "Required, string   - Cache size with units. Eg. 4KB or 1MB"},
    {"associativity",           "Required, int      - Associativity of the cache. In set associative mode, this is the number of ways."},
    {"access_latency_cycles",   "Required, int      - Latency (in cycles) to access the cache array."},
    {"coherence_protocol",      "Required, string   - Coherence protocol. Options: MESI, MSI, NONE"},
    {"cache_line_size",         "Required, int      - Size of a cache line (aka cache block) in bytes."},
    {"hash_function",           "Optional, int      - 0 - none (default), 1 - linear, 2 - XOR"},
    {"L1",                      "Required, int      - Required for L1s, specifies whether cache is an L1. Options: 0[not L1], 1[L1]"},
    {"LLC",                     "Required, int      - Required for LLCs, specifies whether cache is a last-level cache. Options: 0[not LLC], 1[LLC]"},
    {"LL",                      "Required, int      - Required for LLCs, specifies whether an LLC is also the lowest-level coherence entity in the system (e.g., no dir below). Options: 0[not LL entity], 1[LL entity]"},
    /* Not required */
    {"replacement_policy",      "Optional, string   - Replacement policy of the cache array. Options:  LRU[least-recently-used], LFU[least-frequently-used], Random, MRU[most-recently-used], or NMRU[not-most-recently-used]. ", "lru"},
    {"cache_type",              "Optional, string   - Cache type. Options: inclusive cache ('inclusive', required for L1s), non-inclusive cache ('noninclusive') or non-inclusive cache with a directory ('noninclusive_with_directory', required for non-inclusive caches with multiple upper level caches directly above them),", "inclusive"},
    {"noninclusive_directory_repl",    "Optional, string   - If non-inclusive directory exists, its replacement policy. LRU, LFU, MRU, NMRU, or RANDOM. (not case-sensitive).", "LRU"},
    {"noninclusive_directory_entries", "Optional, int  - Number of entries in the directory. Must be at least 1 if the non-inclusive directory exists.", "0"},
    {"noninclusive_directory_associativity", "Optional, int    - For a set-associative directory, number of ways.", "1"},
    {"mshr_num_entries",        "Optional, int      - Number of MSHR entries. Not valid for L1s because L1 MSHRs assumed to be sized for the CPU's load/store queue. Setting this to -1 will create a very large MSHR.", "-1"},
    {"stat_group_ids",          "Optional, int list - Stat grouping. Instructions with same IDs will be grouped for stats. Separated by commas.", ""},
    {"tag_access_latency_cycles", "Optional, int     - Latency (in cycles) to access tag portion only of cache. If not specified, defaults to access_latency_cycles","access_latency_cycles"},
    {"mshr_latency_cycles",     "Optional, int      - Latency (in cycles) to process responses in the cache (MSHR response hits). If not specified, simple intrapolation is used based on the cache access latency", "-1"},
    {"idle_max",                "Optional, int      - Cache temporarily turns off its clock after this many idle cycles", "6"},
    {"prefetcher",              "Optional, string   - Name of prefetcher module", ""},
    {"directory_at_next_level", "Optional, int      - Specifies if there is a directory-controller as the next lower memory level; deprecated - set 'bottom_network' to 'directory' instead", "0"},
    {"bottom_network",          "Optional, string   - Specifies whether the cache is connected to a network below and the entity type of the connection. Options: cache, directory, ''[no network below]", ""},
    {"top_network",             "Optional, string   - Specifies whether the cache is connected to a network above and the entity type of the connection. Options: cache, ''[no network above]", ""},
    {"num_cache_slices",        "Optional, int      - For a distributed, shared cache, total number of cache slices", "1"},
    {"slice_id",                "Optional, int      - For distributed, shared caches, unique ID for this cache slice", "0"},
    {"slice_allocation_policy", "Optional, string   - Policy for allocating addresses among distributed shared cache. Options: rr[round-robin]", "rr"},
    {"statistics",              "Optional, int      - Print cache stats at end of simulation. Options: 0[off], 1[on]", "0"},
    {"network_bw",              "Optional, int      - Network link bandwidth.", "1GB/s"},
    {"network_address",         "Optional, int      - When connected to a network, the network address of this cache.", "0"},
    {"network_num_vc",          "Optional, int      - When connected to a network, the number of VCS on the on-chip network.", "3"},
    {"network_input_buffer_size", "Optional, int      - Size of the network's input buffer.", "1KB"},
    {"network_output_buffer_size","Optional, int      - Size of the network's output buffer.", "1KB"},
    {"debug",                   "Optional, int      - Print debug information. Options: 0[no output], 1[stdout], 2[stderr], 3[file]", "0"},
    {"debug_level",             "Optional, int      - Debugging level. Between 0 and 10", "0"},
    {"debug_addr",              "Optional, int      - Address (in decimal) to be debugged, if not specified or specified as -1, debug output for all addresses will be printed","-1"},
    {"force_noncacheable_reqs", "Optional, int      - Used for verification purposes. All requests are considered to be 'noncacheable'. Options: 0[off], 1[on]", "0"},
    {"maxRequestDelay",         "Optional, int      - Set an error timeout if memory requests take longer than this in ns (0: disable)", "0"},
    {"snoop_l1_invalidations",  "Optional, int      - Forward invalidations from L1s to processors. Options: 0[off], 1[on]", "0"},
    {"lower_is_noninclusive",      "Optional, int      - Next lower level cache is non-inclusive, changes some coherence decisions (e.g., write back clean data)", "0"},
    {NULL, NULL, NULL}
};

static const ElementInfoPort cache_ports[] = {
    {"low_network_%(low_network_ports)d",  "Ports connected to lower level caches (closer to main memory)", memEvent_port_events},
    {"high_network_%(high_network_ports)d", "Ports connected to higher level caches (closer to CPU)", memEvent_port_events},
    {"directory",   "Network link port to directory", net_port_events},
    {"cache",       "Network link port to cache", net_port_events},
    {NULL, NULL, NULL}
};

static const ElementInfoStatistic cache_statistics[] = {
    /* Cache hits and misses */
    {"CacheHits",           "Total number of cache hits", "count", 1},
    {"CacheMisses",         "Total number of cache misses", "count", 1},
    {"GetSHit_Arrival",     "GetS was handled at arrival and was a cache hit", "count", 1},
    {"GetSHit_Blocked",     "GetS was blocked in MSHR at arrival and later was a cache hit", "count", 1},
    {"GetXHit_Arrival",     "GetX was handled at arrival and was a cache hit", "count", 1},
    {"GetXHit_Blocked",     "GetX was blocked in MSHR at arrival and  later was a cache hit", "count", 1},
    {"GetSExHit_Arrival",   "GetSEx was handled at arrival and was a cache hit", "count", 1},
    {"GetSExHit_Blocked",   "GetSEx was blocked in MSHR at arrival and  later was a cache hit", "count", 1},
    {"GetSMiss_Arrival",    "GetS was handled at arrival and was a cache miss", "count", 1},
    {"GetSMiss_Blocked",    "GetS was blocked in MSHR at arrival and later was a cache miss", "count", 1},
    {"GetXMiss_Arrival",    "GetX was handled at arrival and was a cache miss", "count", 1},
    {"GetXMiss_Blocked",    "GetX was blocked in MSHR at arrival and  later was a cache miss", "count", 1},
    {"GetSExMiss_Arrival",  "GetSEx was handled at arrival and was a cache miss", "count", 1},
    {"GetSExMiss_Blocked",  "GetSEx was blocked in MSHR at arrival and  later was a cache miss", "count", 1},
    /* Coherence events - hits */
    {"GetSHit_S",           "Coherence: GetS handled in state: S", "count", 2},
    {"GetSHit_E",           "Coherence: GetS handled in state: E", "count", 2},
    {"GetSHit_M",           "Coherence: GetS handled in state: M", "count", 2},
    {"GetXHit_E",           "Coherence: GetX handled in state: E", "count", 2},
    {"GetXHit_M",           "Coherence: GetX handled in state: M", "count", 2},
    {"GetSExHit_E",         "Coherence: GetSEx handled in state: E", "count", 2},
    {"GetSExHit_M",         "Coherence: GetSEx handled in state: M", "count", 2},
    /* Coherence events - misses */
    {"GetSMiss_IS",         "Coherence: GetS caused I->S transition", "count", 2},  // Name, description, unit, level
    {"GetXMiss_IM",         "Coherence: GetX caused I->M transition", "count", 2},
    {"GetXMiss_SM",         "Coherence: GetX caused S->M transition", "count", 2},
    {"GetXMiss_MM",         "Coherence: GetX caused ownership transition (M->M)", "count", 2},
    {"GetSExMiss_IM",       "Coherence: GetSEx caused I->M transition", "count", 2},
    {"GetSExMiss_SM",       "Coherence: GetSEx caused S->M transition", "count", 2},
    {"GetSExMiss_MM",       "Coherence: GetSEx caused ownership transition (M->M)", "count", 2},
    {"SharedReadResponse",  "Coherence: Received shared response to a GetS request", "count", 1},
    {"ExclusiveReadResponse",   "Coherence: Received exclusive response to a GetS request", "count", 1},
    /* Event receives */
    {"GetS_recv",               "Event received: GetS", "count", 1},
    {"GetX_recv",               "Event received: GetX", "count", 1},
    {"GetSEx_recv",             "Event received: GetSEx", "count", 1},
    {"GetSResp_recv",           "Event received: GetSResp", "count", 1},
    {"GetXResp_recv",           "Event received: GetXResp", "count", 1},
    {"PutM_recv",               "Event received: PutM", "count", 1},
    {"PutS_recv",               "Event received: PutS", "count", 1},
    {"PutE_recv",               "Event received: PutE", "count", 1},
    {"FetchInv_recv",           "Event received: FetchInv", "count", 1},
    {"FetchInvX_recv",          "Event received: FetchInvX", "count", 1},
    {"Inv_recv",                "Event received: Inv", "count", 1},
    {"NACK_recv",               "Event: NACK received", "count", 1},
    /* Event sends */
    {"PutM_Sent_Evict",         "Event: PutM sent due to eviction", "count", 1},
    {"PutS_Sent_Evict",         "Event: PutS sent due to eviction", "count", 1},
    {"PutE_Sent_Evict",         "Event: PutE sent due to eviction", "count", 1},
    {"PutM_Sent_Inv",           "Event: PutM sent due to invalidation", "count", 1},
    {"PutS_Sent_Inv",           "Event: PutS sent due to invalidation", "count", 1},
    {"PutE_Sent_Inv",           "Event: PutE sent due to invalidation", "count", 1},
    {"Inv_Sent",                "Event: Inv sent", "count", 1},
    {"FetchInv_Sent",           "Event: FetchInv sent", "count", 1},
    {"FetchInvX_Sent",          "Event: FetchInvX sent", "count", 1},
    {"NACK_Sent",               "Event: NACK sent", "count", 1},
    /* Event/State combinations - Count how many times an event was seen in particular state */
    {"stateEvent_GetS_I",           "Event/State: Number of times a GetS was seen in state I", "count", 3},
    {"stateEvent_GetS_S",           "Event/State: Number of times a GetS was seen in state S", "count", 3},
    {"stateEvent_GetS_E",           "Event/State: Number of times a GetS was seen in state E", "count", 3},
    {"stateEvent_GetS_M",           "Event/State: Number of times a GetS was seen in state M", "count", 3},
    {"stateEvent_GetX_I",           "Event/State: Number of times a GetX was seen in state I", "count", 3},
    {"stateEvent_GetX_S",           "Event/State: Number of times a GetX was seen in state S", "count", 3},
    {"stateEvent_GetX_E",           "Event/State: Number of times a GetX was seen in state E", "count", 3},
    {"stateEvent_GetX_M",           "Event/State: Number of times a GetX was seen in state M", "count", 3},
    {"stateEvent_GetSEx_I",         "Event/State: Number of times a GetSEx was seen in state I", "count", 3},
    {"stateEvent_GetSEx_S",         "Event/State: Number of times a GetSEx was seen in state S", "count", 3},
    {"stateEvent_GetSEx_E",         "Event/State: Number of times a GetSEx was seen in state E", "count", 3},
    {"stateEvent_GetSEx_M",         "Event/State: Number of times a GetSEx was seen in state M", "count", 3},
    {"stateEvent_GetSResp_IS",      "Event/State: Number of times a GetSResp was seen in state IS", "count", 3},
    {"stateEvent_GetSResp_IM",      "Event/State: Number of times a GetSResp was seen in state IM", "count", 3},
    {"stateEvent_GetSResp_SMInv",   "Event/State: Number of times a GetSResp was seen in state SM_Inv", "count", 3},
    {"stateEvent_GetSResp_SM",      "Event/State: Number of times a GetSResp was seen in state SM", "count", 3},
    {"stateEvent_GetXResp_IM",      "Event/State: Number of times a GetXResp was seen in state IM", "count", 3},
    {"stateEvent_GetXResp_SM",      "Event/State: Number of times a GetXResp was seen in state SM", "count", 3},
    {"stateEvent_GetXResp_SMInv",   "Event/State: Number of times a GetXResp was seen in state SM_Inv", "count", 3},
    {"stateEvent_PutS_I",           "Event/State: Number of times a PutS was seen in state I", "count", 3},
    {"stateEvent_PutS_S",           "Event/State: Number of times a PutS was seen in state S", "count", 3},
    {"stateEvent_PutS_E",           "Event/State: Number of times a PutS was seen in state E", "count", 3},
    {"stateEvent_PutS_M",           "Event/State: Number of times a PutS was seen in state M", "count", 3},
    {"stateEvent_PutS_SD",          "Event/State: Number of times a PutS was seen in state S_D", "count", 3},
    {"stateEvent_PutS_ED",          "Event/State: Number of times a PutS was seen in state E_D", "count", 3},
    {"stateEvent_PutS_MD",          "Event/State: Number of times a PutS was seen in state M_D", "count", 3},
    {"stateEvent_PutS_SMD",         "Event/State: Number of times a PutS was seen in state SM_D", "count", 3},
    {"stateEvent_PutS_SI",          "Event/State: Number of times a PutS was seen in state SI", "count", 3},
    {"stateEvent_PutS_EI",          "Event/State: Number of times a PutS was seen in state EI", "count", 3},
    {"stateEvent_PutS_MI",          "Event/State: Number of times a PutS was seen in state MI", "count", 3},
    {"stateEvent_PutS_SInv",        "Event/State: Number of times a PutS was seen in state S_Inv", "count", 3},
    {"stateEvent_PutS_EInv",        "Event/State: Number of times a PutS was seen in state E_Inv", "count", 3},
    {"stateEvent_PutS_MInv",        "Event/State: Number of times a PutS was seen in state M_Inv", "count", 3},
    {"stateEvent_PutS_SMInv",       "Event/State: Number of times a PutS was seen in state SM_Inv", "count", 3},
    {"stateEvent_PutS_EInvX",       "Event/State: Number of times a PutS was seen in state E_InvX", "count", 3},
    {"stateEvent_PutE_I",           "Event/State: Number of times a PutE was seen in state I", "count", 3},
    {"stateEvent_PutE_E",           "Event/State: Number of times a PutE was seen in state E", "count", 3},
    {"stateEvent_PutE_M",           "Event/State: Number of times a PutE was seen in state M", "count", 3},
    {"stateEvent_PutE_EI",          "Event/State: Number of times a PutE was seen in state EI", "count", 3},
    {"stateEvent_PutE_MI",          "Event/State: Number of times a PutE was seen in state MI", "count", 3},
    {"stateEvent_PutE_EInv",        "Event/State: Number of times a PutE was seen in state E_Inv", "count", 3},
    {"stateEvent_PutE_MInv",        "Event/State: Number of times a PutE was seen in state M_Inv", "count", 3},
    {"stateEvent_PutE_EInvX",       "Event/State: Number of times a PutE was seen in state E_InvX", "count", 3},
    {"stateEvent_PutE_MInvX",       "Event/State: Number of times a PutE was seen in state M_InvX", "count", 3},
    {"stateEvent_PutM_I",           "Event/State: Number of times a PutM was seen in state I", "count", 3},
    {"stateEvent_PutM_E",           "Event/State: Number of times a PutM was seen in state E", "count", 3},
    {"stateEvent_PutM_M",           "Event/State: Number of times a PutM was seen in state M", "count", 3},
    {"stateEvent_PutM_EI",          "Event/State: Number of times a PutM was seen in state EI", "count", 3},
    {"stateEvent_PutM_MI",          "Event/State: Number of times a PutM was seen in state MI", "count", 3},
    {"stateEvent_PutM_EInv",        "Event/State: Number of times a PutM was seen in state E_Inv", "count", 3},
    {"stateEvent_PutM_MInv",        "Event/State: Number of times a PutM was seen in state M_Inv", "count", 3},
    {"stateEvent_PutM_EInvX",       "Event/State: Number of times a PutM was seen in state E_InvX", "count", 3},
    {"stateEvent_PutM_MInvX",       "Event/State: Number of times a PutM was seen in state M_InvX", "count", 3},
    {"stateEvent_Inv_I",            "Event/State: Number of times an Inv was seen in state I", "count", 3},
    {"stateEvent_Inv_IS",           "Event/State: Number of times an Inv was seen in state IS", "count", 3},
    {"stateEvent_Inv_IM",           "Event/State: Number of times an Inv was seen in state IM", "count", 3},
    {"stateEvent_Inv_S",            "Event/State: Number of times an Inv was seen in state S", "count", 3},
    {"stateEvent_Inv_SM",           "Event/State: Number of times an Inv was seen in state SM", "count", 3},
    {"stateEvent_Inv_SInv",         "Event/State: Number of times an Inv was seen in state S_Inv", "count", 3},
    {"stateEvent_Inv_SI",           "Event/State: Number of times an Inv was seen in state SI", "count", 3},
    {"stateEvent_Inv_SMInv",        "Event/State: Number of times an Inv was seen in state SM_Inv", "count", 3},
    {"stateEvent_Inv_SD",           "Event/State: Number of times an Inv was seen in state S_D", "count", 3},
    {"stateEvent_FetchInv_I",       "Event/State: Number of times a FetchInv was seen in state I", "count", 3},
    {"stateEvent_FetchInv_IS",      "Event/State: Number of times a FetchInv was seen in state IS", "count", 3},
    {"stateEvent_FetchInv_IM",      "Event/State: Number of times a FetchInv was seen in state IM", "count", 3},
    {"stateEvent_FetchInv_SM",      "Event/State: Number of times a FetchInv was seen in state SM", "count", 3},
    {"stateEvent_FetchInv_S",       "Event/State: Number of times a FetchInv was seen in state S", "count", 3},
    {"stateEvent_FetchInv_E",       "Event/State: Number of times a FetchInv was seen in state E", "count", 3},
    {"stateEvent_FetchInv_M",       "Event/State: Number of times a FetchInv was seen in state M", "count", 3},
    {"stateEvent_FetchInv_EI",      "Event/State: Number of times a FetchInv was seen in state EI", "count", 3},
    {"stateEvent_FetchInv_MI",      "Event/State: Number of times a FetchInv was seen in state MI", "count", 3},
    {"stateEvent_FetchInv_EInv",    "Event/State: Number of times a FetchInv was seen in state E_Inv", "count", 3},
    {"stateEvent_FetchInv_EInvX",   "Event/State: Number of times a FetchInv was seen in state E_InvX", "count", 3},
    {"stateEvent_FetchInv_MInv",    "Event/State: Number of times a FetchInv was seen in state M_Inv", "count", 3},
    {"stateEvent_FetchInv_MInvX",   "Event/State: Number of times a FetchInv was seen in state M_InvX", "count", 3},
    {"stateEvent_FetchInv_SD",      "Event/State: Number of times a FetchInv was seen in state S_D", "count", 3},
    {"stateEvent_FetchInv_ED",      "Event/State: Number of times a FetchInv was seen in state E_D", "count", 3},
    {"stateEvent_FetchInv_MD",      "Event/State: Number of times a FetchInv was seen in state M_D", "count", 3},
    {"stateEvent_FetchInvX_I",      "Event/State: Number of times a FetchInvX was seen in state I", "count", 3},
    {"stateEvent_FetchInvX_IS",     "Event/State: Number of times a FetchInvX was seen in state IS", "count", 3},
    {"stateEvent_FetchInvX_IM",     "Event/State: Number of times a FetchInvX was seen in state IM", "count", 3},
    {"stateEvent_FetchInvX_SM",     "Event/State: Number of times a FetchInvX was seen in state SM", "count", 3},
    {"stateEvent_FetchInvX_E",      "Event/State: Number of times a FetchInvX was seen in state E", "count", 3},
    {"stateEvent_FetchInvX_M",      "Event/State: Number of times a FetchInvX was seen in state M", "count", 3},
    {"stateEvent_FetchInvX_EI",     "Event/State: Number of times a FetchInvX was seen in state EI", "count", 3},
    {"stateEvent_FetchInvX_MI",     "Event/State: Number of times a FetchInvX was seen in state MI", "count", 3},
    {"stateEvent_FetchInvX_EInv",   "Event/State: Number of times a FetchInvX was seen in state E_Inv", "count", 3},
    {"stateEvent_FetchInvX_EInvX",  "Event/State: Number of times a FetchInvX was seen in state E_InvX", "count", 3},
    {"stateEvent_FetchInvX_MInv",   "Event/State: Number of times a FetchInvX was seen in state M_Inv", "count", 3},
    {"stateEvent_FetchInvX_MInvX",  "Event/State: Number of times a FetchInvX was seen in state M_InvX", "count", 3},
    {"stateEvent_FetchInvX_ED",     "Event/State: Number of times a FetchInvX was seen in state E_D", "count", 3},
    {"stateEvent_FetchInvX_MD",     "Event/State: Number of times a FetchInvX was seen in state M_D", "count", 3},
    {"stateEvent_Fetch_I",          "Event/State: Number of times a Fetch was seen in state I", "count", 3},
    {"stateEvent_Fetch_IS",         "Event/State: Number of times a Fetch was seen in state IS", "count", 3},
    {"stateEvent_Fetch_IM",         "Event/State: Number of times a Fetch was seen in state IM", "count", 3},
    {"stateEvent_Fetch_S",          "Event/State: Number of times a Fetch was seen in state S", "count", 3},
    {"stateEvent_Fetch_SM",         "Event/State: Number of times a Fetch was seen in state SM", "count", 3},
    {"stateEvent_Fetch_SInv",       "Event/State: Number of times a Fetch was seen in state S_Inv", "count", 3},
    {"stateEvent_Fetch_SI",         "Event/State: Number of times a Fetch was seen in state SI", "count", 3},
    {"stateEvent_Fetch_SD",         "Event/State: Number of times a Fetch was seen in state S_D", "count", 3},
    {"stateEvent_FetchResp_I",      "Event/State: Number of times a FetchResp was seen in state I", "count", 3},
    {"stateEvent_FetchResp_SI",     "Event/State: Number of times a FetchResp was seen in state SI", "count", 3},
    {"stateEvent_FetchResp_EI",     "Event/State: Number of times a FetchResp was seen in state EI", "count", 3},
    {"stateEvent_FetchResp_MI",     "Event/State: Number of times a FetchResp was seen in state MI", "count", 3},
    {"stateEvent_FetchResp_SInv",   "Event/State: Number of times a FetchResp was seen in state S_Inv", "count", 3},
    {"stateEvent_FetchResp_SMInv",  "Event/State: Number of times a FetchResp was seen in state SM_Inv", "count", 3},
    {"stateEvent_FetchResp_EInv",   "Event/State: Number of times a FetchResp was seen in state E_Inv", "count", 3},
    {"stateEvent_FetchResp_MInv",   "Event/State: Number of times a FetchResp was seen in state M_Inv", "count", 3},
    {"stateEvent_FetchResp_SD",     "Event/State: Number of times a FetchResp was seen in state S_D", "count", 3},
    {"stateEvent_FetchResp_SMD",    "Event/State: Number of times a FetchResp was seen in state SM_D", "count", 3},
    {"stateEvent_FetchResp_ED",     "Event/State: Number of times a FetchResp was seen in state E_D", "count", 3},
    {"stateEvent_FetchResp_MD",     "Event/State: Number of times a FetchResp was seen in state M_D", "count", 3},
    {"stateEvent_FetchXResp_I",     "Event/State: Number of times a FetchXResp was seen in state I", "count", 3},
    {"stateEvent_FetchXResp_EInvX", "Event/State: Number of times a FetchXResp was seen in state E_InvX", "count", 3},
    {"stateEvent_FetchXResp_MInvX", "Event/State: Number of times a FetchXResp was seen in state M_InvX", "count", 3},
    {"stateEvent_AckInv_I",         "Event/State: Number of times an AckInv was seen in state I", "count", 3},
    {"stateEvent_AckInv_SInv",      "Event/State: Number of times an AckInv was seen in state S_Inv", "count", 3},
    {"stateEvent_AckInv_SMInv",     "Event/State: Number of times an AckInv was seen in state SM_Inv", "count", 3},
    {"stateEvent_AckInv_SI",        "Event/State: Number of times an AckInv was seen in state SI", "count", 3},
    {"stateEvent_AckInv_EI",        "Event/State: Number of times an AckInv was seen in state EI", "count", 3},
    {"stateEvent_AckInv_MI",        "Event/State: Number of times an AckInv was seen in state MI", "count", 3},
    {"stateEvent_AckInv_EInv",      "Event/State: Number of times an AckInv was seen in state E_Inv", "count", 3},
    {"stateEvent_AckInv_MInv",      "Event/State: Number of times an AckInv was seen in state M_Inv", "count", 3},
    {"stateEvent_AckPut_I",         "Event/State: Number of times an AckPut was seen in state I", "count", 3},
    /* Eviction - count attempts to evict in a particular state */
    {"evict_I",                 "Eviction: Attempted to evict a block in state I", "count", 3},
    {"evict_S",                 "Eviction: Attempted to evict a block in state S", "count", 3},
    {"evict_E",                 "Eviction: Attempted to evict a block in state E", "count", 3},
    {"evict_M",                 "Eviction: Attempted to evict a block in state M", "count", 3},
    {"evict_IS",                "Eviction: Attempted to evict a block in state IS", "count", 3},
    {"evict_IM",                "Eviction: Attempted to evict a block in state IM", "count", 3},
    {"evict_SM",                "Eviction: Attempted to evict a block in state SM", "count", 3},
    {"evict_SInv",              "Eviction: Attempted to evict a block in state S_Inv", "count", 3},
    {"evict_EInv",              "Eviction: Attempted to evict a block in state E_Inv", "count", 3},
    {"evict_MInv",              "Eviction: Attempted to evict a block in state M_Inv", "count", 3},
    {"evict_SMInv",             "Eviction: Attempted to evict a block in state SM_Inv", "count", 3},
    {"evict_EInvX",             "Eviction: Attempted to evict a block in state E_InvX", "count", 3},
    {"evict_MInvX",             "Eviction: Attempted to evict a block in state M_InvX", "count", 3},
    {"evict_SI",                "Eviction: Attempted to evict a block in state SI", "count", 3},
    /* Latency for different kinds of misses*/
    {"latency_GetS_IS",         "Latency for read misses in I state", "cycles", 1},
    {"latency_GetS_M",          "Latency for read misses that find the block owned by another cache in M state", "cycles", 1},
    {"latency_GetX_IM",         "Latency for write misses in I state", "cycles", 1},
    {"latency_GetX_SM",         "Latency for write misses in S state", "cycles", 1},
    {"latency_GetX_M",          "Latency for write misses that find the block owned by another cache in M state", "cycles", 1},
    {"latency_GetSEx_IM",       "Latency for read-exclusive misses in I state", "cycles", 1},
    {"latency_GetSEx_SM",       "Latency for read-exclusive misses in S state", "cycles", 1},
    {"latency_GetSEx_M",        "Latency for read-exclusive misses that find the block owned by another cache in M state", "cycles", 1},
    {NULL, NULL, NULL, 0}
};


static Component* create_Sieve(ComponentId_t id, Params& params)
{
	return Sieve::sieveFactory(id, params);
}

static const ElementInfoParam sieve_params[] = {
    /* Required */
    {"cache_size",              "Required, string   - Cache size with units. Eg. 4KB or 1MB"},
    {"associativity",           "Required, int      - Associativity of the cache. In set associative mode, this is the number of ways."},
    {"cache_line_size",         "Required, int      - Size of a cache line (aka cache block) in bytes."},
    /* Not required */
    {"prefetcher",              "Optional, string   - Name of prefetcher module", ""},
    {"debug",                   "Optional, int      - Print debug information. Options: 0[no output], 1[stdout], 2[stderr], 3[file]", "0"},
    {"debug_level",             "Optional, int      - Debugging level. Between 0 and 10", "0"},
    {NULL, NULL, NULL}
};

static const ElementInfoPort sieve_ports[] = {
    {"cpu_link", "Connection to the CPU", memEvent_port_events},
    {NULL, NULL, NULL}
};



static Component* create_Bus(ComponentId_t id, Params& params)
{
    return new Bus( id, params );
}

static const ElementInfoParam bus_params[] = {
    {"bus_frequency",       "Bus clock frequency"},
    {"broadcast",           "If set, messages are broadcasted to all other ports", "0"},
    {"fanout",              "If set, messages from the high network are replicated and sent to all low network ports", "0"},
    {"bus_latency_cycles",  "Number of ports on the bus", "0"},
    {"idle_max",            "Bus temporarily turns off clock after this amount of idle cycles", "6"},
    {"debug",               "Prints debug statements --0[No debugging], 1[STDOUT], 2[STDERR], 3[FILE]--", "0"},
    {"debug_level",         "Debugging level: 0 to 10", "0"},
    {"debug_addr",          "Optional, int      - Address (in decimal) to be debugged, if not specified or specified as -1, debug output for all addresses will be printed","-1"},
    {NULL, NULL}
};


static const ElementInfoPort bus_ports[] = {
    {"low_network_%(low_network_ports)d",  "Ports connected to lower level caches (closer to main memory)", memEvent_port_events},
    {"high_network_%(high_network_ports)d", "Ports connected to higher level caches (closer to CPU)", memEvent_port_events},
    {NULL, NULL, NULL}
};


static Component* create_trivialCPU(ComponentId_t id, Params& params){
	return new trivialCPU( id, params );
}

static const ElementInfoPort cpu_ports[] = {
    {"mem_link", "Connection to caches.", NULL},
    {NULL, NULL, NULL}
};

static const ElementInfoParam cpu_params[] = {
    {"verbose",                 "Determine how verbose the output from the CPU is", "1"},
    {"rngseed",                 "Set a seed for the random generation of addresses", "7"},
    {"commFreq",                "How often to do a memory operation."},
    {"memSize",                 "Size of physical memory."},
    {"do_write",                "Enable writes to memory (versus just reads).", "1"},
    {"num_loadstore",           "Stop after this many reads and writes.", "-1"},
    {"noncacheableRangeStart",  "Beginning of range of addresses that are noncacheable.", "0x0"},
    {"noncacheableRangeEnd",    "End of range of addresses that are noncacheable.", "0x0"},
    {"addressoffset",           "Apply an offset to a calculated address to check for non-alignment issues", "0"},
    {NULL, NULL, NULL}
};


static Component* create_streamCPU(ComponentId_t id, Params& params){
	return new streamCPU( id, params );
}



static Component* create_MemController(ComponentId_t id, Params& params){
	return new MemController( id, params );
}

static const ElementInfoParam memctrl_params[] = {
    {"mem_size",            "Size of physical memory in MB (*deprecated*)", "0"},
    {"backend.mem_size",    "Size of physical memory in MB", "0"},
    {"range_start",         "Address Range where physical memory begins", "0"},
    {"interleave_size",     "Size of interleaved pages in KB.", "0"},
    {"interleave_step",     "Distance between sucessive interleaved pages on this controller in KB.", "0"},
    {"memory_file",         "Optional backing-store file to pre-load memory, or store resulting state", "N/A"},
    {"clock",               "Clock frequency of controller", NULL},
    //{"divert_DC_lookups",   "Divert Directory controller table lookups from the memory system, use a fixed latency (access_time). Default:0", "0"},
    {"backend",             "Timing backend to use:  Default to simpleMem", "memHierarchy.simpleMem"},
    {"request_width",       "Size of a DRAM request in bytes.  Should be a power of 2 - default 64", "64"},
    {"direct_link_latency", "Latency when using the 'direct_link', rather than 'snoop_link'", "10 ns"},
    {"debug",               "0 (default): No debugging, 1: STDOUT, 2: STDERR, 3: FILE.", "0"},
    {"debug_level",         "Debugging level: 0 to 10", "0"},
    {"debug_addr",              "Optional, int      - Address (in decimal) to be debugged, if not specified or specified as -1, debug output for all addresses will be printed","-1"},
    {"statistics",          "0 (default): Don't print, 1: STDOUT, 2: STDERR, 3: FILE.", "0"},
    {"trace_file",          "File name (optional) of a trace-file to generate.", ""},
    {"coherence_protocol",  "Coherence protocol.  Supported: MESI (default), MSI"},
    {"listenercount",       "Counts the number of listeners attached to this controller, these are modules for tracing or components like prefetchers", "0"},
    {"listener%(listenercount)d", "Loads a listener module into the controller", ""},
    {"direct_link", "Specifies whether memory is directly connected to a directory/cache (1) or is connected via the network (0)","1"},
    {"network_bw",          "Network link bandwidth.", NULL},
    {"network_address",     "Network address of component.", ""},
    {"network_num_vc",      "The number of VCS on the on-chip network.", "3"},
    {"network_input_buffer_size",   "Size of the network's input buffer.", "1KB"},
    {"network_output_buffer_size",  "Size of the network's output buffer.", "1KB"},
    {NULL, NULL, NULL}
};

static const ElementInfoStatistic memctrl_statistics[] = {
    /* Cache hits and misses */
    { "cycles_with_issue", "Total cycles with successful issue to back end", "cycles", 1 },
    { "cycles_attempted_issue_but_rejected", "Total cycles where an attempt to issue to backend was rejected (indicates backend full)", "cycles", 1 },
    { "total_cycles", "Total cycles called at the memory controller", "cycles", 1 },
    { NULL, NULL, NULL, 0 }
};

static const ElementInfoPort memctrl_ports[] = {
    {"direct_link",     "Directly connect to another component (like a Directory Controller).", memEvent_port_events},
    {"cube_link",       "Link to VaultSim.", NULL}, /* TODO:  Make this generic */
    {"network",         "Network link to another component", net_port_events},
    {NULL, NULL, NULL}
};


static SubComponent* create_Mem_SimpleSim(Component* comp, Params& params){
    return new SimpleMemory(comp, params);
}

static const ElementInfoParam simpleMem_params[] = {
    { "verbose",          "Sets the verbosity of the backend output", "0" },
    {"access_time",     "Constant latency of memory operation.", "100 ns"},
    {NULL, NULL}
};


#if defined(HAVE_LIBDRAMSIM)
static SubComponent* create_Mem_DRAMSim(Component* comp, Params& params){
    return new DRAMSimMemory(comp, params);
}


static const ElementInfoParam dramsimMem_params[] = {
    { "verbose",          "Sets the verbosity of the backend output", "0" },
    {"device_ini",      "Name of DRAMSim Device config file", NULL},
    {"system_ini",      "Name of DRAMSim Device system file", NULL},
    {NULL, NULL, NULL}
};

static SubComponent* create_Mem_pagedMulti(Component* comp, Params& params){
    return new pagedMultiMemory(comp, params);
}


static const ElementInfoParam pagedMultiMem_params[] = {
    { "verbose",          "Sets the verbosity of the backend output", "0" },
    {"device_ini",      "Name of DRAMSim Device config file", NULL},
    {"system_ini",      "Name of DRAMSim Device system file", NULL},
    {"collect_stats",      "Name of DRAMSim Device system file", "0"},
    {"transfer_delay",      "Time (in ns) to transfer page to fast mem", "250"},
    {"threshold",      "Threshold (touches/quantum)", "4"},
    {"scan_threshold",      "scan Threshold (for SC strategies)", "4"},
    {"seed",      "RNG Seed", "1447"},
    {"page_add_strategy",      "Page Addition Strategy", "T"},
    {"page_replace_strategy",      "Page Replacement Strategy", "FIFO"},
    {"access_time", "Constant time memory access for \"fast\" memory", "35ns"},
    {"max_fast_pages", "Number of \"fast\" (constant time) pages", "256"},
    {"page_shift", "Size of page (2^x bytes)", "12"},
    {"quantum", "time period for when page access counts is shifted", "5ms"},
    {"accStatsPrefix","File name for acces pattern statistics",""},
    {NULL, NULL, NULL}
};

static const ElementInfoStatistic pagedMultiMem_statistics[] = {
    {"fast_hits", "Number of accesses that 'hit' a fast page", "count", 1},
    {"fast_swaps", "Number of pages swapped between 'fast' and 'slow' memory", "count", 1},
    {"fast_acc", "Number of total accesses to the memory backend", "count", 1},
    {"t_pages", "Number of total pages", "count", 1},
    { NULL, NULL, NULL, 0 }
};

#endif

#if defined(HAVE_LIBHYBRIDSIM)
static SubComponent* create_Mem_HybridSim(Component* comp, Params& params){
    return new HybridSimMemory(comp, params);
}


static const ElementInfoParam hybridsimMem_params[] = {
    { "verbose",          "Sets the verbosity of the backend output", "0" },
    {"device_ini",      "Name of HybridSim Device config file", NULL},
    {"system_ini",      "Name of HybridSim Device system file", NULL},
    {NULL, NULL, NULL}
};

#endif

static SubComponent* create_Mem_VaultSim(Component* comp, Params& params){
    return new VaultSimMemory(comp, params);
}

#ifdef HAVE_GOBLIN_HMCSIM
static SubComponent* create_Mem_GOBLINHMCSim(Component* comp, Params& params){
    return new GOBLINHMCSimBackend(comp, params);
}

static const ElementInfoParam goblin_hmcsim_Mem_params[] = {
	{ "verbose",		"Sets the verbosity of the backend output", "0" },
	{ "device_count",	"Sets the number of HMCs being simulated, default=1, max=8", "1" },
	{ "link_count", 	"Sets the number of links being simulated, min=4, max=8, default=4", "4" },
	{ "vault_count",	"Sets the number of vaults being simulated, min=16, max=32, default=16", "16" },
	{ "queue_depth",	"Sets the depth of the HMC request queue, min=2, max=65536, default=2", "2" },
  	{ "dram_count",         "Sets the number of DRAM blocks per cube\n", "20" },
	{ "xbar_depth",         "Sets the queue depth for the HMC X-bar", "8" },
        { "max_req_size",       "Sets the maximum requests which can be inflight from the controller side at any time", "32" },
	{ "trace-banks", 	"Decides where tracing for memory banks is enabled, \"yes\" or \"no\", default=\"no\"", "no" },
	{ "trace-queue", 	"Decides where tracing for queues is enabled, \"yes\" or \"no\", default=\"no\"", "no" },
	{ "trace-cmds", 	"Decides where tracing for commands is enabled, \"yes\" or \"no\", default=\"no\"", "no" },
	{ "trace-latency", 	"Decides where tracing for latency is enabled, \"yes\" or \"no\", default=\"no\"", "no" },
	{ "trace-stalls", 	"Decides where tracing for memory stalls is enabled, \"yes\" or \"no\", default=\"no\"", "no" },
	{ "tag_count",		"Sets the number of inflight tags that can be pending at any point in time", "16" },
	{ "capacity_per_device", "Sets the capacity of the device being simulated in GB, min=2, max=8, default is 4", "4" },
	{ NULL, NULL, NULL }
};
#endif

#ifdef HAVE_FDSIM

static SubComponent* create_Mem_FDSim(Component* comp, Params& params){
    return new FlashDIMMSimMemory(comp, params);
}

static const ElementInfoParam fdsimMem_params[] = {
    { "device_ini",       "Name of HybridSim Device config file", "" },
    { "verbose",          "Sets the verbosity of the backend output", "0" },
    { "trace",            "Sets the name of a file to record trace output", "" },
    { "max_pending_reqs", "Sets the maximum number of requests that can be outstanding", "32" },
    { NULL, NULL, NULL }
};

#endif

static const ElementInfoParam vaultsimMem_params[] = {
    { "verbose",          "Sets the verbosity of the backend output", "0" },
    {"access_time",     "When not using DRAMSim, latency of memory operation.", "100 ns"},
    {NULL, NULL, NULL}
};



static Module* create_MemInterface(Component *comp, Params &params) {
    return new MemHierarchyInterface(comp, params);
}


static Module* create_MemNIC(Component *comp, Params &params) {
    return new MemNIC(comp);
}


static Component* create_DirectoryController(ComponentId_t id, Params& params){
	return new DirectoryController( id, params );
}

static const ElementInfoParam dirctrl_params[] = {
    {"network_bw",          "Network link bandwidth.", NULL},
    {"network_address",     "Network address of component.", ""},
    {"network_num_vc",      "The number of VCS on the on-chip network.", "3"},
    {"network_input_buffer_size",   "Size of the network's input buffer.", "1KB"},
    {"network_output_buffer_size",  "Size of the network's output buffer.", "1KB"},
    {"addr_range_start",    "Start of Address Range, for this controller.", "0"},
    {"addr_range_end",      "End of Address Range, for this controller.", NULL},
    {"interleave_size",     "(optional) Size of interleaved pages in KB.", "0"},
    {"interleave_step",     "(optional) Distance between sucessive interleaved pages on this controller in KB.", "0"},
    {"clock",               "Clock rate of controller.", "1GHz"},
    {"entry_cache_size",    "Size (in # of entries) the controller will cache.", "0"},
    {"debug",               "0 (default): No debugging, 1: STDOUT, 2: STDERR, 3: FILE.", "0"},
    {"debug_level",         "Debugging level: 0 to 10", "0"},
    {"debug_addr",              "Optional, int      - Address (in decimal) to be debugged, if not specified or specified as -1, debug output for all addresses will be printed","-1"},
    {"statistics",          "0 (default): Don't print, 1: STDOUT, 2: STDERR, 3: FILE.", "0"},
    {"cache_line_size",     "Size of a cache line [aka cache block] in bytes.", "64"},
    {"coherence_protocol",  "Coherence protocol.  Supported --MESI, MSI--"},
    {"mshr_num_entries",    "Number of MSHRs. Set to -1 for almost unlimited number.", "-1"},
    {"direct_mem_link",     "Specifies whether directory has a direct connection to memory (1) or is connected via a network (0)","1"},
    {"net_memory_name",     "For directories connected to a memory over the network: name of the memory this directory owns", ""},
    {"access_latency_cycles", "Latency of directory access in cycles", "0"},
    {"mshr_latency_cycles", "Latency of mshr access in cycles", "0"},
    {NULL, NULL, NULL}
};

static const ElementInfoPort dirctrl_ports[] = {
    {"memory",      "Link to Memory Controller", NULL},
    {"network",     "Network Link", NULL},
    {NULL, NULL, NULL}
};



static Component* create_DMAEngine(ComponentId_t id, Params& params){
	return new DMAEngine( id, params );
}

static const ElementInfoParam dmaengine_params[] = {
    {"debug",           "0 (default): No debugging, 1: STDOUT, 2: STDERR, 3: FILE.", "0"},
    {"debug_level",     "Debugging level: 0 to 10", "0"},
    {"clockRate",       "Clock Rate for processing DMAs.", "1GHz"},
    {"netAddr",         "Network address of component.", NULL},
    {"network_num_vc",  "The number of VCS on the on-chip network.", "3"},
    {"printStats",      "0 (default): Don't print, 1: STDOUT, 2: STDERR, 3: FILE.", "0"},
    {NULL, NULL, NULL}
};


static const ElementInfoPort dmaengine_ports[] = {
    {"netLink",     "Network Link",     net_port_events},
    {NULL, NULL, NULL}
};

static SubComponent* load_networkMemoryInspector(Component* parent, 
                                                 Params& params) {
    return new networkMemInspector(parent);
}

static const ElementInfoSubComponent subcomponents[] = {
    {
        "simpleMem",
        "Simple constant-access time memory",
        NULL, /* Advanced help */
        create_Mem_SimpleSim, /* Module Alloc w/ params */
        simpleMem_params,
        NULL, /* statistics */
        "SST::MemHierarchy::MemBackend"
    },
#if defined(HAVE_LIBDRAMSIM)
    {
        "dramsim",
        "DRAMSim-driven memory timings",
        NULL, /* Advanced help */
        create_Mem_DRAMSim, /* Module Alloc w/ params */
        dramsimMem_params,
        NULL, /* statistics */
        "SST::MemHierarchy::MemBackend"
    },
    {
        "pagedMulti",
        "DRAMSim-driven memory timings with a fixed timing multi-level memory using paging",
        NULL, /* Advanced help */
        create_Mem_pagedMulti, /* Module Alloc w/ params */
        pagedMultiMem_params,
        pagedMultiMem_statistics, /* statistics */
        "SST::MemHierarchy::MemBackend"
    },
#endif
#if defined(HAVE_LIBHYBRIDSIM)
    {
        "hybridsim",
        "HybridSim-driven memory timings",
        NULL, /* Advanced help */
        create_Mem_HybridSim, /* Module Alloc w/ params */
        hybridsimMem_params,
        NULL, /* statistics */
        "SST::MemHierarchy::MemBackend"
    },
#endif
#ifdef HAVE_GOBLIN_HMCSIM
    {
        "goblinHMCSim",
        "GOBLIN HMC Simulator driven memory timings",
        NULL, /* Advanced help */
        create_Mem_GOBLINHMCSim, /* Module Alloc w/ params */
        goblin_hmcsim_Mem_params,
        NULL, /* statistics */
        "SST::MemHierarchy::MemBackend"
    },
#endif
#ifdef HAVE_FDSIM
    {
        "flashDIMMSim",
        "FlashDIMM Simulator driven memory timings",
        NULL, /* Advanced help */
        create_Mem_FDSim, /* Module Alloc w/ params */
        fdsimMem_params,
        NULL, /* statistics */
        "SST::MemHierarchy::MemBackend"
    },
#endif
    {
        "vaultsim",
        "VaultSim Memory timings",
        NULL, /* Advanced help */
        create_Mem_VaultSim, /* Module Alloc w/ params */
        vaultsimMem_params,
        NULL, /* statistics */
        "SST::MemHierarchy::MemBackend"
    },
    { "networkMemoryInspector",
      "Used to classify memory traffic going through a network router",
      NULL,
      load_networkMemoryInspector,
      NULL,
      networkMemoryInspector_statistics,
      "SST::Interfaces::SimpleNetwork::NetworkInspector"
    },
    {NULL, NULL, NULL, NULL, NULL, NULL}
};

static const ElementInfoModule modules[] = {
    {
        "memInterface",
        "Simplified interface to Memory Hierarchy",
        NULL,
        NULL,
        create_MemInterface,
        NULL,
        "SST::Interfaces::SimpleMem"
    },
    {
        "memNIC",
        "Memory-oriented Network Interface",
        NULL, /* Advanced help */
        NULL, /* ModuleAlloc */
        create_MemNIC, /* Module Alloc w/ params */
        NULL, /* Params */
        NULL, /* Interface */
    },
    {NULL, NULL, NULL, NULL, NULL, NULL}
};


static const ElementInfoComponent components[] = {
	{ "Cache",
		"Cache Component",
		NULL,
        create_Cache,
        cache_params,
        cache_ports,
        COMPONENT_CATEGORY_MEMORY,
        cache_statistics
	},
    { "Sieve",
		"Simple Cache Filtering Component",
		NULL,
        create_Sieve,
        sieve_params,
        sieve_ports,
        COMPONENT_CATEGORY_MEMORY
	},
	{ "Bus",
		"Mem Hierarchy Bus Component",
		NULL,
		create_Bus,
        bus_params,
        bus_ports,
        COMPONENT_CATEGORY_MEMORY
	},
	{"MemController",
		"Memory Controller Component",
		NULL,
		create_MemController,
        	memctrl_params,
        	memctrl_ports,
        	COMPONENT_CATEGORY_MEMORY,
		memctrl_statistics
	},
	{"DirectoryController",
		"Coherencey Directory Controller Component",
		NULL,
		create_DirectoryController,
        dirctrl_params,
        dirctrl_ports,
        COMPONENT_CATEGORY_MEMORY
	},
	{"DMAEngine",
		"DMA Engine Component",
		NULL,
		create_DMAEngine,
        dmaengine_params,
        dmaengine_ports,
        COMPONENT_CATEGORY_MEMORY
	},
	{"trivialCPU",
		"Simple Demo CPU for testing",
		NULL,
		create_trivialCPU,
        cpu_params,
        cpu_ports,
        COMPONENT_CATEGORY_PROCESSOR
	},
	{"streamCPU",
		"Simple Demo STREAM CPU for testing",
		NULL,
		create_streamCPU,
        cpu_params,
        cpu_ports,
        COMPONENT_CATEGORY_PROCESSOR
	},
	{ NULL, NULL, NULL, NULL, NULL, NULL, 0}
};

static const ElementInfoEvent memHierarchy_events[] = {
	{ "MemEvent", "Event to interact with the memHierarchy", NULL, NULL },
	{ "DMACommand", "Event to interact with DMA engine", NULL, NULL },
	{ NULL, NULL, NULL, NULL }
};

extern "C" {
	ElementLibraryInfo memHierarchy_eli = {
		"memHierarchy",
		"Cache Hierarchy",
		components,
        	memHierarchy_events, /* Events */
        	NULL, /* Introspectors */
        	modules,
		subcomponents,
		NULL,
		NULL, 
                NULL
	};
}

BOOST_CLASS_EXPORT(SST::MemHierarchy::MemEvent)
BOOST_CLASS_EXPORT(SST::MemHierarchy::DMACommand)


BOOST_CLASS_EXPORT(SST::MemHierarchy::MemNIC::MemRtrEvent)
BOOST_CLASS_EXPORT(SST::MemHierarchy::MemNIC::InitMemRtrEvent)
