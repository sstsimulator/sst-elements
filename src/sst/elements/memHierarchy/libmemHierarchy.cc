// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2017, Sandia Corporation
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>

#define SST_ELI_COMPILE_OLD_ELI_WITHOUT_DEPRECATION_WARNINGS

#include "sst/core/element.h"
#include "sst/core/component.h"

#include "memEvent.h"
#include "memNIC.h"
#include "memLink.h"
#include "cacheController.h"
#include "Sieve/sieveController.h"
#include "Sieve/broadcastShim.h"
#include "bus.h"
#include "testcpu/trivialCPU.h"
#include "testcpu/scratchCPU.h"
#include "testcpu/streamCPU.h"
#include "memoryController.h"
#include "directoryController.h"
#include "dmaEngine.h"
#include "memHierarchyInterface.h"
#include "memHierarchyScratchInterface.h"
#include "coherencemgr/coherenceController.h"
#include "coherencemgr/MESICoherenceController.h"
#include "coherencemgr/MESIInternalDirectory.h"
#include "coherencemgr/IncoherentController.h"
#include "coherencemgr/L1CoherenceController.h"
#include "coherencemgr/L1IncoherentController.h"
#include "membackend/memBackend.h"
#include "membackend/simpleMemBackend.h"
#include "membackend/simpleDRAMBackend.h"
#include "membackend/vaultSimBackend.h"
#include "membackend/MessierBackend.h"
#include "membackend/requestReorderSimple.h"
#include "membackend/requestReorderByRow.h"
#include "membackend/delayBuffer.h"
#include "membackend/simpleMemBackendConvertor.h"
#include "membackend/flagMemBackendConvertor.h"
#include "membackend/timingDRAMBackend.h"
#include "membackend/timingTransaction.h"
#include "membackend/timingPagePolicy.h"
#include "membackend/timingAddrMapper.h"
#include "membackend/simpleMemScratchBackendConvertor.h"
#include "membackend/cramSimBackend.h"
#include "networkMemInspector.h"
#include "memNetBridge.h"
#include "multithreadL1Shim.h"
#include "scratchpad.h"
#include "coherentMemoryController.h"
#include "customcmd/amoCustomCmdHandler.h"

#ifdef HAVE_GOBLIN_HMCSIM
#include "membackend/extMemBackendConvertor.h"
#include "membackend/goblinHMCBackend.h"
#endif

#ifdef HAVE_LIBRAMULATOR
#include "membackend/ramulatorBackend.h"
#endif

#ifdef HAVE_LIBDRAMSIM
#include "membackend/dramSimBackend.h"
#include "membackend/pagedMultiBackend.h"
#endif

#ifdef HAVE_LIBHYBRIDSIM
#include "membackend/hybridSimBackend.h"
#endif

#ifdef HAVE_LIBFDSIM
#include "membackend/flashSimBackend.h"
#endif

#ifdef HAVE_HBMLIBDRAMSIM
#include "membackend/HBMdramSimBackend.h"
#include "membackend/HBMpagedMultiBackend.h"
#endif

using namespace SST;
using namespace SST::MemHierarchy;

static const char * memEvent_port_events[] = {"memHierarchy.MemEventBase", NULL};
static const char * arielAlloc_port_events[] = {"ariel.arielAllocTrackEvent", NULL};
static const char * net_port_events[] = {"memHierarchy.MemRtrEvent", NULL};


/*****************************************************************************************
 *  Component: Cache
 *  Purpose: Cache controller
 ****************************************************************************************/
static Component* create_Cache(ComponentId_t id, Params& params)
{
	return Cache::cacheFactory(id, params);
}

static const ElementInfoParam cache_params[] = {
    /* Required */
    {"cache_frequency",         "(string) Clock frequency or period with units (Hz or s; SI units OK). For L1s, this is usually the same as the CPU's frequency."},
    {"cache_size",              "(string) Cache size with units. Eg. 4KiB or 1MiB"},
    {"associativity",           "(int) Associativity of the cache. In set associative mode, this is the number of ways."},
    {"access_latency_cycles",   "(int) Latency (in cycles) to access the cache data array. This latency is paid by cache hits and coherence requests that need to return data."},
    {"L1",                      "(bool) Required for L1s, specifies whether cache is an L1. Options: 0[not L1], 1[L1]", "false"},
    /* Not required */
    {"cache_line_size",         "(uint) Size of a cache line (aka cache block) in bytes.", "64"},
    {"hash_function",           "(int) 0 - none (default), 1 - linear, 2 - XOR", "0"},
    {"coherence_protocol",      "(string) Coherence protocol. Options: MESI, MSI, NONE", "MESI"},
    {"replacement_policy",      "(string) Replacement policy of the cache array. Options:  LRU[least-recently-used], LFU[least-frequently-used], Random, MRU[most-recently-used], or NMRU[not-most-recently-used]. ", "lru"},
    {"cache_type",              "(string) - Cache type. Options: inclusive cache ('inclusive', required for L1s), non-inclusive cache ('noninclusive') or non-inclusive cache with a directory ('noninclusive_with_directory', required for non-inclusive caches with multiple upper level caches directly above them),", "inclusive"},
    {"max_requests_per_cycle",  "(int) Maximum number of requests to accept per cycle. 0 or negative is unlimited.", "-1"},
    {"request_link_width",      "(string) Limits number of request bytes sent per cycle. Use 'B' units. '0B' is unlimited.", "0B"},
    {"response_link_width",     "(string) Limits number of response bytes sent per cycle. Use 'B' units. '0B' is unlimited.", "0B"},
    {"noninclusive_directory_repl",    "(string) If non-inclusive directory exists, its replacement policy. LRU, LFU, MRU, NMRU, or RANDOM. (not case-sensitive).", "LRU"},
    {"noninclusive_directory_entries", "(uint) Number of entries in the directory. Must be at least 1 if the non-inclusive directory exists.", "0"},
    {"noninclusive_directory_associativity", "(uint) For a set-associative directory, number of ways.", "1"},
    {"mshr_num_entries",        "(int) Number of MSHR entries. Not valid for L1s because L1 MSHRs assumed to be sized for the CPU's load/store queue. Setting this to -1 will create a very large MSHR.", "-1"},
    {"tag_access_latency_cycles", "(uint) Latency (in cycles) to access tag portion only of cache. Paid by misses and coherence requests that don't need data. If not specified, defaults to access_latency_cycles","access_latency_cycles"},
    {"mshr_latency_cycles",     "(uint) Latency (in cycles) to process responses in the cache and replay requests. Paid on the return/response path for misses instead of access_latency_cycles. If not specified, simple intrapolation is used based on the cache access latency", "1"},
    {"prefetcher",              "(string) Name of prefetcher subcomponent", ""},
    {"max_outstanding_prefetch","(uint) Maximum number of prefetch misses that can be outstanding, additional prefetches will be dropped/NACKed. Default is 1/2 of MSHR entries.", "0.5*mshr_num_entries"},
    {"drop_prefetch_mshr_level","(uint) Drop/NACK prefetches if the number of in-use mshrs is greater than or equal to this number. Default is mshr_num_entries - 2.", "mshr_num_entries-2"},
    {"num_cache_slices",        "(uint) For a distributed, shared cache, total number of cache slices", "1"},
    {"slice_id",                "(uint) For distributed, shared caches, unique ID for this cache slice", "0"},
    {"slice_allocation_policy", "(string) Policy for allocating addresses among distributed shared cache. Options: rr[round-robin]", "rr"},
    {"maxRequestDelay",         "(uint) Set an error timeout if memory requests take longer than this in ns (0: disable)", "0"},
    {"snoop_l1_invalidations",  "(bool) Forward invalidations from L1s to processors. Options: 0[off], 1[on]", "false"},
    {"debug",                   "(uint) Where to send output. Options: 0[no output], 1[stdout], 2[stderr], 3[file]", "0"},
    {"debug_level",             "(uint) Output/debug verbosity level. Between 0 (no output) and 10 (everything). 1-3 gives warnings/info; 4-10 gives debug.", "1"},
    {"debug_addr",          "(comma separated uint) Address(es) to be debugged. Leave empty for all, otherwise specify one or more, comma-separated values. Start and end string with brackets",""},
    {"force_noncacheable_reqs", "(bool) Used for verification purposes. All requests are considered to be 'noncacheable'. Options: 0[off], 1[on]", "false"},
    {"min_packet_size",         "(string) Number of bytes in a request/response not including payload (e.g., addr + cmd). Specify in B.", "8B"},
    /* Old parameters - deprecated or moved */
    {"LL",                          "DEPRECATED - Now auto-detected during init."},
    {"LLC",                         "DEPRECATED - Now auto-detected by configure."},
    {"lower_is_noninclusive",       "DEPRECATED - Now auto-detected during init."},
    {"statistics",                  "DEPRECATED - Use Statistics API to get statistics for caches."},
    {"stat_group_ids",              "DEPRECATED - Use Statistics API to get statistics for caches."},
    {"network_num_vc",              "DEPRECATED - Number of virtual channels (VCs) on the on-chip network. memHierarchy only uses one VC.", "1"},
    {"directory_at_next_level",     "DEPRECATED - Now auto-detected by configure."},
    {"bottom_network",              "DEPRECATED - Now auto-detected by configure."},
    {"top_network",                 "DEPRECATED - Now auto-detected by configure."},
    {"network_address",             "DEPRECATED - Now auto-detected by link control."},
    {"network_bw",                  "MOVED - Now a member of the MemNIC subcomponent.", "80GiB/s"},
    {"network_input_buffer_size",   "MOVED - Now a member of the MemNIC subcomponent.", "1KiB"},
    {"network_output_buffer_size",  "MOVED - Now a member of the MemNIC subcomponent.", "1KiB"},
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
    {"GetSXHit_Arrival",   "GetSX was handled at arrival and was a cache hit", "count", 1},
    {"GetSXHit_Blocked",   "GetSX was blocked in MSHR at arrival and  later was a cache hit", "count", 1},
    {"GetSMiss_Arrival",    "GetS was handled at arrival and was a cache miss", "count", 1},
    {"GetSMiss_Blocked",    "GetS was blocked in MSHR at arrival and later was a cache miss", "count", 1},
    {"GetXMiss_Arrival",    "GetX was handled at arrival and was a cache miss", "count", 1},
    {"GetXMiss_Blocked",    "GetX was blocked in MSHR at arrival and  later was a cache miss", "count", 1},
    {"GetSXMiss_Arrival",  "GetSX was handled at arrival and was a cache miss", "count", 1},
    {"GetSXMiss_Blocked",  "GetSX was blocked in MSHR at arrival and  later was a cache miss", "count", 1},
    {"TotalEventsReceived", "Total number of events received by this cache", "events", 1},
    {"TotalEventsReplayed", "Total number of events that were initially blocked and then were replayed", "events", 1},
    {"TotalNoncacheableEventsReceived", "Total number of non-cache or noncacheable cache events that were received by this cache and forward", "events", 1},
    {"MSHR_occupancy",      "Number of events in MSHR each cycle", "events", 1},
    {"Prefetch_requests",   "Number of prefetches received from prefetcher at this cache", "events", 1},
    {"Prefetch_hits",       "Number of prefetches that were cancelled due to cache or MSHR hit", "events", 1},
    {"Prefetch_drops",      "Number of prefetches that were cancelled because the cache was too busy or too many prefetches were outstanding", "events", 1},
    /* Coherence events - break down GetS between S/E */
    {"SharedReadResponse",      "Coherence: Received shared response to a GetS request", "count", 2},
    {"ExclusiveReadResponse",   "Coherence: Received exclusive response to a GetS request", "count", 2},
    /* Event receives */
    {"GetS_recv",               "Event received: GetS", "count", 2},
    {"GetX_recv",               "Event received: GetX", "count", 2},
    {"GetSX_recv",             "Event received: GetSX", "count", 2},
    {"GetSResp_recv",           "Event received: GetSResp", "count", 2},
    {"GetXResp_recv",           "Event received: GetXResp", "count", 2},
    {"PutM_recv",               "Event received: PutM", "count", 2},
    {"PutS_recv",               "Event received: PutS", "count", 2},
    {"PutE_recv",               "Event received: PutE", "count", 2},
    {"FetchInv_recv",           "Event received: FetchInv", "count", 2},
    {"FetchInvX_recv",          "Event received: FetchInvX", "count", 2},
    {"Inv_recv",                "Event received: Inv", "count", 2},
    {"NACK_recv",               "Event: NACK received", "count", 2},
    {NULL, NULL, NULL, 0}
};

static const ElementInfoStatistic coherence_statistics[] = {
    /* Event sends */
    {"eventSent_GetS",          "Number of GetS requests sent", "events", 2},
    {"eventSent_GetX",          "Number of GetX requests sent", "events", 2},
    {"eventSent_GetSX",        "Number of GetSX requests sent", "events", 2},
    {"eventSent_GetSResp",      "Number of GetSResp responses sent", "events", 2},
    {"eventSent_GetXResp",      "Number of GetXResp responses sent", "events", 2},
    {"eventSent_PutS",          "Number of PutS requests sent", "events", 2},
    {"eventSent_PutE",          "Number of PutE requests sent", "events", 2},
    {"eventSent_PutM",          "Number of PutM requests sent", "events", 2},
    {"eventSent_Inv",           "Number of Inv requests sent", "events", 2},
    {"eventSent_Fetch",         "Number of Fetch requests sent", "events", 2},
    {"eventSent_FetchInv",      "Number of FetchInv requests sent", "events", 2},
    {"eventSent_FetchInvX",     "Number of FetchInvX requests sent", "events", 2},
    {"eventSent_FetchResp",     "Number of FetchResp requests sent", "events", 2},
    {"eventSent_FetchXResp",    "Number of FetchXResp requests sent", "events", 2},
    {"eventSent_AckInv",        "Number of AckInvs sent", "events", 2},
    {"eventSent_AckPut",        "Number of AckPuts sent", "events", 2},
    {"eventSent_NACK_up",       "Number of NACKs sent up (towards CPU)", "events", 2},
    {"eventSent_NACK_down",     "Number of NACKs sent down (towards main memory)", "events", 2},
    {"eventSent_FlushLine",     "Number of FlushLine requests sent", "events", 2},
    {"eventSent_FlushLineInv",  "Number of FlushLineInv requests sent", "events", 2},
    {"eventSent_FlushLineResp", "Number of FlushLineResp responses sent", "events", 2},
    /* Event/State combinations - Count how many times an event was seen in particular state */
    {"stateEvent_GetS_I",           "Event/State: Number of times a GetS was seen in state I (Miss)", "count", 3},
    {"stateEvent_GetS_S",           "Event/State: Number of times a GetS was seen in state S (Hit)", "count", 3},
    {"stateEvent_GetS_E",           "Event/State: Number of times a GetS was seen in state E (Hit)", "count", 3},
    {"stateEvent_GetS_M",           "Event/State: Number of times a GetS was seen in state M (Hit)", "count", 3},
    {"stateEvent_GetX_I",           "Event/State: Number of times a GetX was seen in state I (Miss)", "count", 3},
    {"stateEvent_GetX_S",           "Event/State: Number of times a GetX was seen in state S (Miss)", "count", 3},
    {"stateEvent_GetX_E",           "Event/State: Number of times a GetX was seen in state E (Hit)", "count", 3},
    {"stateEvent_GetX_M",           "Event/State: Number of times a GetX was seen in state M (Hit)", "count", 3},
    {"stateEvent_GetSX_I",         "Event/State: Number of times a GetSX was seen in state I (Miss)", "count", 3},
    {"stateEvent_GetSX_S",         "Event/State: Number of times a GetSX was seen in state S (Miss)", "count", 3},
    {"stateEvent_GetSX_E",         "Event/State: Number of times a GetSX was seen in state E (Hit)", "count", 3},
    {"stateEvent_GetSX_M",         "Event/State: Number of times a GetSX was seen in state M (Hit)", "count", 3},
    {"stateEvent_GetSResp_IS",      "Event/State: Number of times a GetSResp was seen in state IS", "count", 3},
    {"stateEvent_GetXResp_IS",      "Event/State: Number of times a GetXResp was seen in state IS", "count", 3},
    {"stateEvent_GetSResp_IM",      "Event/State: Number of times a GetSResp was seen in state IM", "count", 3},
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
    {"stateEvent_PutS_IB",          "Event/State: Number of times a PutS was seen in state I_B", "count", 3},
    {"stateEvent_PutS_SB",          "Event/State: Number of times a PutS was seen in state S_B", "count", 3},
    {"stateEvent_PutS_SBInv",       "Event/State: Number of times a PutS was seen in state SB_Inv", "count", 3},
    {"stateEvent_PutE_I",           "Event/State: Number of times a PutE was seen in state I", "count", 3},
    {"stateEvent_PutE_E",           "Event/State: Number of times a PutE was seen in state E", "count", 3},
    {"stateEvent_PutE_M",           "Event/State: Number of times a PutE was seen in state M", "count", 3},
    {"stateEvent_PutE_IM",          "Event/State: Number of times a PutE was seen in state IM", "count", 3},
    {"stateEvent_PutE_IS",          "Event/State: Number of times a PutE was seen in state IS", "count", 3},
    {"stateEvent_PutE_EI",          "Event/State: Number of times a PutE was seen in state EI", "count", 3},
    {"stateEvent_PutE_MI",          "Event/State: Number of times a PutE was seen in state MI", "count", 3},
    {"stateEvent_PutE_EInv",        "Event/State: Number of times a PutE was seen in state E_Inv", "count", 3},
    {"stateEvent_PutE_MInv",        "Event/State: Number of times a PutE was seen in state M_Inv", "count", 3},
    {"stateEvent_PutE_EInvX",       "Event/State: Number of times a PutE was seen in state E_InvX", "count", 3},
    {"stateEvent_PutE_MInvX",       "Event/State: Number of times a PutE was seen in state M_InvX", "count", 3},
    {"stateEvent_PutE_IB",          "Event/State: Number of times a PutE was seen in state I_B", "count", 3},
    {"stateEvent_PutE_SB",          "Event/State: Number of times a PutE was seen in state S_B", "count", 3},
    {"stateEvent_PutM_I",           "Event/State: Number of times a PutM was seen in state I", "count", 3},
    {"stateEvent_PutM_E",           "Event/State: Number of times a PutM was seen in state E", "count", 3},
    {"stateEvent_PutM_M",           "Event/State: Number of times a PutM was seen in state M", "count", 3},
    {"stateEvent_PutM_IS",          "Event/State: Number of times a PutM was seen in state IS", "count", 3},
    {"stateEvent_PutM_IM",          "Event/State: Number of times a PutM was seen in state IM", "count", 3},
    {"stateEvent_PutM_EI",          "Event/State: Number of times a PutM was seen in state EI", "count", 3},
    {"stateEvent_PutM_MI",          "Event/State: Number of times a PutM was seen in state MI", "count", 3},
    {"stateEvent_PutM_EInv",        "Event/State: Number of times a PutM was seen in state E_Inv", "count", 3},
    {"stateEvent_PutM_MInv",        "Event/State: Number of times a PutM was seen in state M_Inv", "count", 3},
    {"stateEvent_PutM_EInvX",       "Event/State: Number of times a PutM was seen in state E_InvX", "count", 3},
    {"stateEvent_PutM_MInvX",       "Event/State: Number of times a PutM was seen in state M_InvX", "count", 3},
    {"stateEvent_PutM_IB",          "Event/State: Number of times a PutM was seen in state I_B", "count", 3},
    {"stateEvent_PutM_SB",          "Event/State: Number of times a PutM was seen in state S_B", "count", 3},
    {"stateEvent_Inv_I",            "Event/State: Number of times an Inv was seen in state I", "count", 3},
    {"stateEvent_Inv_IS",           "Event/State: Number of times an Inv was seen in state IS", "count", 3},
    {"stateEvent_Inv_IM",           "Event/State: Number of times an Inv was seen in state IM", "count", 3},
    {"stateEvent_Inv_IB",           "Event/State: Number of times an Inv was seen in state I_B", "count", 3},
    {"stateEvent_Inv_S",            "Event/State: Number of times an Inv was seen in state S", "count", 3},
    {"stateEvent_Inv_SM",           "Event/State: Number of times an Inv was seen in state SM", "count", 3},
    {"stateEvent_Inv_SInv",         "Event/State: Number of times an Inv was seen in state S_Inv", "count", 3},
    {"stateEvent_Inv_SI",           "Event/State: Number of times an Inv was seen in state SI", "count", 3},
    {"stateEvent_Inv_SMInv",        "Event/State: Number of times an Inv was seen in state SM_Inv", "count", 3},
    {"stateEvent_Inv_SD",           "Event/State: Number of times an Inv was seen in state S_D", "count", 3},
    {"stateEvent_Inv_SB",           "Event/State: Number of times an Inv was seen in state S_B", "count", 3},
    {"stateEvent_Inv_ED",           "Event/State: Number of times an Inv was seen in state E_D", "count", 3},
    {"stateEvent_Inv_EI",           "Event/State: Number of times an Inv was seen in state EI", "count", 3},
    {"stateEvent_Inv_EInv",         "Event/State: Number of times an Inv was seen in state E_Inv", "count", 3},
    {"stateEvent_Inv_EInvX",        "Event/State: Number of times an Inv was seen in state E_InvX", "count", 3},
    {"stateEvent_Inv_MD",           "Event/State: Number of times an Inv was seen in state M_D", "count", 3},
    {"stateEvent_Inv_MI",           "Event/State: Number of times an Inv was seen in state MI", "count", 3},
    {"stateEvent_Inv_MInv",         "Event/State: Number of times an Inv was seen in state M_Inv", "count", 3},
    {"stateEvent_Inv_MInvX",        "Event/State: Number of times an Inv was seen in state M_InvX", "count", 3},
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
    {"stateEvent_FetchInv_SInv",    "Event/State: Number of times a FetchInv was seen in state S_Inv", "count", 3},
    {"stateEvent_FetchInv_SD",      "Event/State: Number of times a FetchInv was seen in state S_D", "count", 3},
    {"stateEvent_FetchInv_ED",      "Event/State: Number of times a FetchInv was seen in state E_D", "count", 3},
    {"stateEvent_FetchInv_MD",      "Event/State: Number of times a FetchInv was seen in state M_D", "count", 3},
    {"stateEvent_FetchInv_IB",      "Event/State: Number of times a FetchInv was seen in state I_B", "count", 3},
    {"stateEvent_FetchInv_SB",      "Event/State: Number of times a FetchInv was seen in state S_B", "count", 3},
    {"stateEvent_FetchInvX_I",      "Event/State: Number of times a FetchInvX was seen in state I", "count", 3},
    {"stateEvent_FetchInvX_IS",     "Event/State: Number of times a FetchInvX was seen in state IS", "count", 3},
    {"stateEvent_FetchInvX_IM",     "Event/State: Number of times a FetchInvX was seen in state IM", "count", 3},
    {"stateEvent_FetchInvX_E",      "Event/State: Number of times a FetchInvX was seen in state E", "count", 3},
    {"stateEvent_FetchInvX_M",      "Event/State: Number of times a FetchInvX was seen in state M", "count", 3},
    {"stateEvent_FetchInvX_EI",     "Event/State: Number of times a FetchInvX was seen in state EI", "count", 3},
    {"stateEvent_FetchInvX_MI",     "Event/State: Number of times a FetchInvX was seen in state MI", "count", 3},
    {"stateEvent_FetchInvX_EInv",   "Event/State: Number of times a FetchInvX was seen in state E_Inv", "count", 3},
    {"stateEvent_FetchInvX_EInvX",  "Event/State: Number of times a FetchInvX was seen in state E_InvX", "count", 3},
    {"stateEvent_FetchInvX_MInv",   "Event/State: Number of times a FetchInvX was seen in state M_Inv", "count", 3},
    {"stateEvent_FetchInvX_MInvX",  "Event/State: Number of times a FetchInvX was seen in state M_InvX", "count", 3},
    {"stateEvent_FetchInvX_IB",     "Event/State: Number of times a FetchInvX was seen in state I_B", "count", 3},
    {"stateEvent_FetchInvX_SB",     "Event/State: Number of times a FetchInvX was seen in state S_B", "count", 3},
    {"stateEvent_Fetch_I",          "Event/State: Number of times a Fetch was seen in state I", "count", 3},
    {"stateEvent_Fetch_IS",         "Event/State: Number of times a Fetch was seen in state IS", "count", 3},
    {"stateEvent_Fetch_IM",         "Event/State: Number of times a Fetch was seen in state IM", "count", 3},
    {"stateEvent_Fetch_S",          "Event/State: Number of times a Fetch was seen in state S", "count", 3},
    {"stateEvent_Fetch_SM",         "Event/State: Number of times a Fetch was seen in state SM", "count", 3},
    {"stateEvent_Fetch_SInv",       "Event/State: Number of times a Fetch was seen in state S_Inv", "count", 3},
    {"stateEvent_Fetch_SI",         "Event/State: Number of times a Fetch was seen in state SI", "count", 3},
    {"stateEvent_Fetch_SD",         "Event/State: Number of times a Fetch was seen in state S_D", "count", 3},
    {"stateEvent_Fetch_IB",         "Event/State: Number of times a Fetch was seen in state I_B", "count", 3},
    {"stateEvent_Fetch_SB",         "Event/State: Number of times a Fetch was seen in state S_B", "count", 3},
    {"stateEvent_FetchResp_I",      "Event/State: Number of times a FetchResp was seen in state I", "count", 3},
    {"stateEvent_FetchResp_SI",     "Event/State: Number of times a FetchResp was seen in state SI", "count", 3},
    {"stateEvent_FetchResp_EI",     "Event/State: Number of times a FetchResp was seen in state EI", "count", 3},
    {"stateEvent_FetchResp_MI",     "Event/State: Number of times a FetchResp was seen in state MI", "count", 3},
    {"stateEvent_FetchResp_SInv",   "Event/State: Number of times a FetchResp was seen in state S_Inv", "count", 3},
    {"stateEvent_FetchResp_SMInv",  "Event/State: Number of times a FetchResp was seen in state SM_Inv", "count", 3},
    {"stateEvent_FetchResp_EInv",   "Event/State: Number of times a FetchResp was seen in state E_Inv", "count", 3},
    {"stateEvent_FetchResp_EInvX",  "Event/State: Number of times a FetchResp was seen in state E_Inv", "count", 3},
    {"stateEvent_FetchResp_MInv",   "Event/State: Number of times a FetchResp was seen in state M_Inv", "count", 3},
    {"stateEvent_FetchResp_MInvX",  "Event/State: Number of times a FetchResp was seen in state M_InvX", "count", 3},
    {"stateEvent_FetchResp_SD",     "Event/State: Number of times a FetchResp was seen in state S_D", "count", 3},
    {"stateEvent_FetchResp_SMD",    "Event/State: Number of times a FetchResp was seen in state SM_D", "count", 3},
    {"stateEvent_FetchResp_ED",     "Event/State: Number of times a FetchResp was seen in state E_D", "count", 3},
    {"stateEvent_FetchResp_MD",     "Event/State: Number of times a FetchResp was seen in state M_D", "count", 3},
    {"stateEvent_FetchXResp_I",     "Event/State: Number of times a FetchXResp was seen in state I", "count", 3},
    {"stateEvent_FetchXResp_EInv",  "Event/State: Number of times a FetchXResp was seen in state E_Inv", "count", 3},
    {"stateEvent_FetchXResp_EInvX", "Event/State: Number of times a FetchXResp was seen in state E_InvX", "count", 3},
    {"stateEvent_FetchXResp_MInvX", "Event/State: Number of times a FetchXResp was seen in state M_InvX", "count", 3},
    {"stateEvent_FetchXResp_MInv",  "Event/State: Number of times a FetchXResp was seen in state M_Inv", "count", 3},
    {"stateEvent_FetchXResp_MI",    "Event/State: Number of times a FetchXResp was seen in state MI", "count", 3},
    {"stateEvent_FetchXResp_EI",    "Event/State: Number of times a FetchXResp was seen in state EI", "count", 3},
    {"stateEvent_FetchXResp_SI",    "Event/State: Number of times a FetchXResp was seen in state SI", "count", 3},
    {"stateEvent_FetchXResp_SD",    "Event/State: Number of times a FetchXResp was seen in state S_D", "count", 3},
    {"stateEvent_FetchXResp_ED",    "Event/State: Number of times a FetchXResp was seen in state E_D", "count", 3},
    {"stateEvent_FetchXResp_MD",    "Event/State: Number of times a FetchXResp was seen in state M_D", "count", 3},
    {"stateEvent_FetchXResp_SMD",   "Event/State: Number of times a FetchXResp was seen in state SM_D", "count", 3},
    {"stateEvent_FetchXResp_SInv",  "Event/State: Number of times a FetchXResp was seen in state S_Inv", "count", 3},
    {"stateEvent_FetchXResp_SMInv", "Event/State: Number of times a FetchXResp was seen in state SM_Inv", "count", 3},
    {"stateEvent_AckInv_I",         "Event/State: Number of times an AckInv was seen in state I", "count", 3},
    {"stateEvent_AckInv_SInv",      "Event/State: Number of times an AckInv was seen in state S_Inv", "count", 3},
    {"stateEvent_AckInv_SMInv",     "Event/State: Number of times an AckInv was seen in state SM_Inv", "count", 3},
    {"stateEvent_AckInv_SI",        "Event/State: Number of times an AckInv was seen in state SI", "count", 3},
    {"stateEvent_AckInv_EI",        "Event/State: Number of times an AckInv was seen in state EI", "count", 3},
    {"stateEvent_AckInv_MI",        "Event/State: Number of times an AckInv was seen in state MI", "count", 3},
    {"stateEvent_AckInv_EInv",      "Event/State: Number of times an AckInv was seen in state E_Inv", "count", 3},
    {"stateEvent_AckInv_EInvX",     "Event/State: Number of times an AckInv was seen in state E_InvX", "count", 3},
    {"stateEvent_AckInv_MInv",      "Event/State: Number of times an AckInv was seen in state M_Inv", "count", 3},
    {"stateEvent_AckInv_MInvX",     "Event/State: Number of times an AckInv was seen in state M_InvX", "count", 3},
    {"stateEvent_AckInv_SBInv",     "Event/State: Number of times an AckInv was seen in state SB_Inv", "count", 3},
    {"stateEvent_AckPut_I",         "Event/State: Number of times an AckPut was seen in state I", "count", 3},
    {"stateEvent_FlushLine_I",      "Event/State: Number of times a FlushLine was seen in state I", "count", 3},
    {"stateEvent_FlushLine_S",      "Event/State: Number of times a FlushLine was seen in state S", "count", 3},
    {"stateEvent_FlushLine_E",      "Event/State: Number of times a FlushLine was seen in state E", "count", 3},
    {"stateEvent_FlushLine_M",      "Event/State: Number of times a FlushLine was seen in state M", "count", 3},
    {"stateEvent_FlushLine_IS",     "Event/State: Number of times a FlushLine was seen in state IS", "count", 3},
    {"stateEvent_FlushLine_IM",     "Event/State: Number of times a FlushLine was seen in state IM", "count", 3},
    {"stateEvent_FlushLine_SM",     "Event/State: Number of times a FlushLine was seen in state SM", "count", 3},
    {"stateEvent_FlushLine_MInv",   "Event/State: Number of times a FlushLine was seen in state M_Inv", "count", 3},
    {"stateEvent_FlushLine_MInvX",  "Event/State: Number of times a FlushLine was seen in state M_InvX", "count", 3},
    {"stateEvent_FlushLine_EInv",   "Event/State: Number of times a FlushLine was seen in state E_Inv", "count", 3},
    {"stateEvent_FlushLine_EInvX",  "Event/State: Number of times a FlushLine was seen in state E_InvX", "count", 3},
    {"stateEvent_FlushLine_SInv",   "Event/State: Number of times a FlushLine was seen in state S_Inv", "count", 3},
    {"stateEvent_FlushLine_SMInv",  "Event/State: Number of times a FlushLine was seen in state SM_Inv", "count", 3},
    {"stateEvent_FlushLine_SD",     "Event/State: Number of times a FlushLine was seen in state S_D", "count", 3},
    {"stateEvent_FlushLine_ED",     "Event/State: Number of times a FlushLine was seen in state E_D", "count", 3},
    {"stateEvent_FlushLine_MD",     "Event/State: Number of times a FlushLine was seen in state M_D", "count", 3},
    {"stateEvent_FlushLine_SMD",    "Event/State: Number of times a FlushLine was seen in state SM_D", "count", 3},
    {"stateEvent_FlushLine_MI",     "Event/State: Number of times a FlushLine was seen in state MI", "count", 3},
    {"stateEvent_FlushLine_EI",     "Event/State: Number of times a FlushLine was seen in state EI", "count", 3},
    {"stateEvent_FlushLine_SI",     "Event/State: Number of times a FlushLine was seen in state SI", "count", 3},
    {"stateEvent_FlushLine_IB",     "Event/State: Number of times a FlushLine was seen in state I_B", "count", 3},
    {"stateEvent_FlushLine_SB",     "Event/State: Number of times a FlushLine was seen in state S_B", "count", 3},
    {"stateEvent_FlushLineInv_I",       "Event/State: Number of times a FlushLineInv was seen in state I", "count", 3},
    {"stateEvent_FlushLineInv_S",       "Event/State: Number of times a FlushLineInv was seen in state S", "count", 3},
    {"stateEvent_FlushLineInv_E",       "Event/State: Number of times a FlushLineInv was seen in state E", "count", 3},
    {"stateEvent_FlushLineInv_M",       "Event/State: Number of times a FlushLineInv was seen in state M", "count", 3},
    {"stateEvent_FlushLineInv_IS",      "Event/State: Number of times a FlushLineInv was seen in state IS", "count", 3},
    {"stateEvent_FlushLineInv_IM",      "Event/State: Number of times a FlushLineInv was seen in state IM", "count", 3},
    {"stateEvent_FlushLineInv_SM",      "Event/State: Number of times a FlushLineInv was seen in state SM", "count", 3},
    {"stateEvent_FlushLineInv_MInv",    "Event/State: Number of times a FlushLineInv was seen in state M_Inv", "count", 3},
    {"stateEvent_FlushLineInv_MInvX",   "Event/State: Number of times a FlushLineInv was seen in state M_InvX", "count", 3},
    {"stateEvent_FlushLineInv_EInv",    "Event/State: Number of times a FlushLineInv was seen in state E_Inv", "count", 3},
    {"stateEvent_FlushLineInv_EInvX",   "Event/State: Number of times a FlushLineInv was seen in state E_InvX", "count", 3},
    {"stateEvent_FlushLineInv_SInv",    "Event/State: Number of times a FlushLineInv was seen in state S_Inv", "count", 3},
    {"stateEvent_FlushLineInv_SMInv",   "Event/State: Number of times a FlushLineInv was seen in state SM_Inv", "count", 3},
    {"stateEvent_FlushLineInv_SD",      "Event/State: Number of times a FlushLineInv was seen in state S_D", "count", 3},
    {"stateEvent_FlushLineInv_ED",      "Event/State: Number of times a FlushLineInv was seen in state E_D", "count", 3},
    {"stateEvent_FlushLineInv_MD",      "Event/State: Number of times a FlushLineInv was seen in state M_D", "count", 3},
    {"stateEvent_FlushLineInv_SMD",     "Event/State: Number of times a FlushLineInv was seen in state SM_D", "count", 3},
    {"stateEvent_FlushLineInv_MI",      "Event/State: Number of times a FlushLineInv was seen in state MI", "count", 3},
    {"stateEvent_FlushLineInv_EI",      "Event/State: Number of times a FlushLineInv was seen in state EI", "count", 3},
    {"stateEvent_FlushLineInv_SI",      "Event/State: Number of times a FlushLineInv was seen in state SI", "count", 3},
    {"stateEvent_FlushLineInv_IB",      "Event/State: Number of times a FlushLineInv was seen in state I_B", "count", 3},
    {"stateEvent_FlushLineInv_SB",      "Event/State: Number of times a FlushLineInv was seen in state S_B", "count", 3},
    {"stateEvent_FlushLineResp_I",      "Event/State: Number of times a FlushLineResp was seen in state I", "count", 3},
    {"stateEvent_FlushLineResp_IB",     "Event/State: Number of times a FlushLineResp was seen in state I_B", "count", 3},
    {"stateEvent_FlushLineResp_SB",     "Event/State: Number of times a FlushLineResp was seen in state S_B", "count", 3},

    
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
    {"evict_IB",                "Eviction: Attempted to evict a block in state S_B", "count", 3},
    {"evict_SB",                "Eviction: Attempted to evict a block in state I_B", "count", 3},
    /* Latency for different kinds of misses*/
    {"latency_GetS_IS",         "Latency for read misses in I state", "cycles", 1},
    {"latency_GetS_M",          "Latency for read misses that find the block owned by another cache in M state", "cycles", 1},
    {"latency_GetX_IM",         "Latency for write misses in I state", "cycles", 1},
    {"latency_GetX_SM",         "Latency for write misses in S state", "cycles", 1},
    {"latency_GetX_M",          "Latency for write misses that find the block owned by another cache in M state", "cycles", 1},
    {"latency_GetSX_IM",       "Latency for read-exclusive misses in I state", "cycles", 1},
    {"latency_GetSX_SM",       "Latency for read-exclusive misses in S state", "cycles", 1},
    {"latency_GetSX_M",        "Latency for read-exclusive misses that find the block owned by another cache in M state", "cycles", 1},
    /* Miscellaneous */
    {"EventStalledForLockedCacheline",  "Number of times an event (FetchInv, FetchInvX, eviction, Fetch, etc.) was stalled because a cache line was locked", "instances", 1},
    {NULL, NULL, NULL, 0}
};

#ifdef HAVE_GOBLIN_HMCSIM
static SubComponent* create_Mem_ExtBackendConvertor(Component* comp, Params& params){
    return new ExtMemBackendConvertor(comp, params);
}

static const ElementInfoStatistic extMemBackendConvertor_statistics[] = {
    /* Cache hits and misses */
    { "cycles_with_issue",                  "Total cycles with successful issue to back end",   "cycles",   1 },
    { "cycles_attempted_issue_but_rejected","Total cycles where an attempt to issue to backend was rejected (indicates backend full)", "cycles", 1 },
    { "total_cycles",                       "Total cycles called at the memory controller",     "cycles",   1 },
    { "requests_received_GetS",             "Number of GetS (read) requests received",          "requests", 1 },
    { "requests_received_GetSX",           "Number of GetSX (read) requests received",        "requests", 1 },
    { "requests_received_GetX",             "Number of GetX (read) requests received",          "requests", 1 },
    { "requests_received_PutM",             "Number of PutM (write) requests received",         "requests", 1 },
    { "outstanding_requests",               "Total number of outstanding requests each cycle",  "requests", 1 },
    { "latency_GetS",                       "Total latency of handled GetS requests",           "cycles",   1 },
    { "latency_GetSX",                     "Total latency of handled GetSX requests",         "cycles",   1 },
    { "latency_GetX",                       "Total latency of handled GetX requests",           "cycles",   1 },
    { "latency_PutM",                       "Total latency of handled PutM requests",           "cycles",   1 },
    { NULL, NULL, NULL, 0 }
};

static const ElementInfoStatistic goblinhmcsim_statistics[] = {
    {"WR16",            "Operation count for HMC WR16",       "count", 1},
    {"WR32",            "Operation count for HMC WR32",       "count", 1},
    {"WR48",            "Operation count for HMC WR48",       "count", 1},
    {"WR64",            "Operation count for HMC WR64",       "count", 1},
    {"WR80",            "Operation count for HMC WR80",       "count", 1},
    {"WR96",            "Operation count for HMC WR96",       "count", 1},
    {"WR112",           "Operation count for HMC WR112",      "count", 1},
    {"WR128",           "Operation count for HMC WR128",      "count", 1},
    {"WR256",           "Operation count for HMC WR256",      "count", 1},
    {"RD16",            "Operation count for HMC RD16",       "count", 1},
    {"RD32",            "Operation count for HMC RD32",       "count", 1},
    {"RD48",            "Operation count for HMC RD48",       "count", 1},
    {"RD64",            "Operation count for HMC RD64",       "count", 1},
    {"RD80",            "Operation count for HMC RD80",       "count", 1},
    {"RD96",            "Operation count for HMC RD96",       "count", 1},
    {"RD112",           "Operation count for HMC RD112",      "count", 1},
    {"RD128",           "Operation count for HMC RD128",      "count", 1},
    {"RD256",           "Operation count for HMC RD256",      "count", 1},
    {"MD_WR",           "Operation count for HMC MD_WR",      "count", 1},
    {"MD_RD",           "Operation count for HMC MD_RD",      "count", 1},
    {"BWR",             "Operation count for HMC BWR",        "count", 1},
    {"2ADD8",           "Operation count for HMC 2ADD8",      "count", 1},
    {"ADD16",           "Operation count for HMC ADD16",      "count", 1},
    {"P_WR16",          "Operation count for HMC P_WR16",     "count", 1},
    {"P_WR32",          "Operation count for HMC P_WR32",     "count", 1},
    {"P_WR48",          "Operation count for HMC P_WR48",     "count", 1},
    {"P_WR64",          "Operation count for HMC P_WR64",     "count", 1},
    {"P_WR80",          "Operation count for HMC P_WR80",     "count", 1},
    {"P_WR96",          "Operation count for HMC P_WR96",     "count", 1},
    {"P_WR112",         "Operation count for HMC P_WR112",    "count", 1},
    {"P_WR128",         "Operation count for HMC P_WR128",    "count", 1},
    {"P_WR256",         "Operation count for HMC P_WR256",    "count", 1},
    {"2ADDS8R",         "Operation count for HMC 2ADDS8R",    "count", 1},
    {"ADDS16",          "Operation count for HMC ADDS16",     "count", 1},
    {"INC8",            "Operation count for HMC INC8",       "count", 1},
    {"P_INC8",          "Operation count for HMC P_INC8",     "count", 1},
    {"XOR16",           "Operation count for HMC XOR16",      "count", 1},
    {"OR16",            "Operation count for HMC OR16",       "count", 1},
    {"NOR16",           "Operation count for HMC NOR16",      "count", 1},
    {"AND16",           "Operation count for HMC AND16",      "count", 1},
    {"NAND16",          "Operation count for HMC NAND16",     "count", 1},
    {"CASGT8",          "Operation count for HMC CASGT8",     "count", 1},
    {"CASGT16",         "Operation count for HMC CASGT16",    "count", 1},
    {"CASLT8",          "Operation count for HMC CASLT8",     "count", 1},
    {"CASLT16",         "Operation count for HMC CASLT16",    "count", 1},
    {"CASEQ8",          "Operation count for HMC CASEQ8",     "count", 1},
    {"CASZERO16",       "Operation count for HMC CASZER016",  "count", 1},
    {"EQ8",             "Operation count for HMC EQ8",        "count", 1},
    {"EQ16",            "Operation count for HMC EQ16",       "count", 1},
    {"BWR8R",           "Operation count for HMC BWR8R",      "count", 1},
    {"SWAP16",          "Operation count for HMC SWAP16",     "count", 1},
    {"XbarRqstStall",   "HMC Crossbar request stalls",        "count", 1},
    {"XbarRspStall",    "HMC Crossbar response stalls",       "count", 1},
    {"VaultRqstStall",  "HMC Vault request stalls",           "count", 1},
    {"VaultRspStall",   "HMC Vault response stalls",          "count", 1},
    {"RouteRqstStall",  "HMC Route request stalls",           "count", 1},
    {"RouteRspStall",   "HMC Route response stalls",          "count", 1},
    {"UndefStall",      "HMC Undefined stall events",         "count", 1},
    {"BankConflict",    "HMC Bank conflicts",                 "count", 1},
    {"XbarLatency",     "HMC Crossbar latency events",        "count", 1},

    {"LinkPhyPower",        "HMC Link phy power",                   "milliwatts", 1},
    {"LinkLocalRoutePower", "HMC Link local quadrant route power",  "milliwatts", 1},
    {"LinkRemoteRoutePower","HMC Link remote quadrante route power","milliwatts", 1},
    {"XbarRqstSlotPower",   "HMC Crossbar request slot power",      "milliwatts", 1},
    {"XbarRspSlotPower",    "HMC Crossbar response slot power",     "milliwatts", 1},
    {"XbarRouteExternPower","HMC Crossbar external route power",    "milliwatts", 1},
    {"VaultRqstSlotPower",  "HMC Vault request slot power",         "milliwatts", 1},
    {"VaultRspSlotPower",   "HMC Vault response slot power",        "milliwatts", 1},
    {"VaultCtrlPower",      "HMC Vault control power",              "milliwatts", 1},
    {"RowAccessPower",      "HMC DRAM row access power",            "milliwatts", 1},

    {"LinkPhyTherm",        "HMC Link phy thermal",                   "btus", 1},
    {"LinkLocalRouteTherm", "HMC Link local quadrant route thermal",  "btus", 1},
    {"LinkRemoteRouteTherm","HMC Link remote quadrante route thermal","btus", 1},
    {"XbarRqstSlotTherm",   "HMC Crossbar request slot thermal",      "btus", 1},
    {"XbarRspSlotTherm",    "HMC Crossbar response slot thermal",     "btus", 1},
    {"XbarRouteExternTherm","HMC Crossbar external route thermal",    "btus", 1},
    {"VaultRqstSlotTherm",  "HMC Vault request slot thermal",         "btus", 1},
    {"VaultRspSlotTherm",   "HMC Vault response slot thermal",        "btus", 1},
    {"VaultCtrlTherm",      "HMC Vault control thermal",              "btus", 1},
    {"RowAccessTherm",      "HMC DRAM row access thermal",            "btus", 1},

    {NULL, NULL, NULL, 0}
};
#endif // HAVE_GOBLIN_HMCSIM

#ifdef HAVE_HBMLIBDRAMSIM
static const ElementInfoStatistic hbmDramSim_statistics[] = {
    {"TotalBandwidth",      "Total Bandwidth",              "GB/s",   1},
    {"BytesTransferred",    "Total Bytes Transferred",      "bytes",  1},
    {"TotalReads",          "Total Queued Reads",           "count",  1},
    {"TotalWrites",         "Total Queued Writes",          "count",  1},
    {"TotalTransactions",   "Total Number of Transactions", "count",  1},
    {"PendingReads",        "Pending Transactions",         "count",  1},
    {"PendingReturns",      "Pending Returns",              "count",  1},

    {NULL, NULL, NULL, 0}
};
#endif // HAVE_HBMLIBDRAMSIM

/*****************************************************************************************
 *  SubComponent: MESIController
 *  Purpose: Non-L1 cache coherence controller for MSI or MESI protocol. 
 *  Loaded by a 'Cache' component
 *  May be used for inclusive or private non-inclusive caches
 ****************************************************************************************/
static SubComponent* create_MESICoherenceController(Component * comp, Params& params) {
    return new MESIController(comp, params);
}

/*****************************************************************************************
 *  SubComponent: MESIInternalDirectory
 *  Purpose: Non-L1 cache coherence controller for non-inclusive shared caches
 *  MSI or MESI protocol
 *  Loaded by a 'Cache' component
 ****************************************************************************************/
static SubComponent* create_MESICacheDirectoryCoherenceController(Component * comp, Params& params) {
    return new MESIInternalDirectory(comp, params);
}

/*****************************************************************************************
 *  SubComponent: IncoherentController
 *  Purpose: Non-L1 cache coherence controller for non-coherent caches
 *  Loaded by a 'Cache' component
 ****************************************************************************************/
static SubComponent* create_IncoherentController(Component * comp, Params& params) {
    return new IncoherentController(comp, params);
}

/*****************************************************************************************
 *  SubComponent: IncoherentController
 *  Purpose: L1 cache coherence controller for MSI/MESI protocol
 *  Loaded by a 'Cache' component
 ****************************************************************************************/
static SubComponent* create_L1CoherenceController(Component * comp, Params& params) {
    return new L1CoherenceController(comp, params);
}

/*****************************************************************************************
 *  SubComponent: IncoherentController
 *  Purpose: L1 cache coherence controller for non-coherent caches
 *  Loaded by a 'Cache' component
 ****************************************************************************************/
static SubComponent* create_L1IncoherentController(Component * comp, Params& params) {
    return new L1IncoherentController(comp, params);
}

/*****************************************************************************************
 *  Component: MultiThreadL1
 *  Purpose: Shim between cores & an L1 to simulate a core with multiple hardware threads
 *****************************************************************************************/
static Component* create_multithreadL1(ComponentId_t id, Params& params)
{
    return new MultiThreadL1(id, params);
}

static const ElementInfoParam multithreadL1_params[] = {
    {"clock",               "(string) Clock frequency or period with units (Hz or s; SI units OK)."},
    {"requests_per_cycle",  "(uint) Number of requests to forward to L1 each cycle (for all threads combined). 0 indicates unlimited", "0"},
    {"responses_per_cycle", "(uint) Number of responses to forward to threads each cycle (for all threads combined). 0 indicates unlimited", "0"},
    {"debug",               "(uint) Where to print debug output. Options: 0[no output], 1[stdout], 2[stderr], 3[file]", "0"},
    {"debug_level",         "(uint) Debug verbosity level. Between 0 and 10", "0"},
    {"debug_addr",          "(comma separated uint) Address(es) to be debugged. Leave empty for all, otherwise specify one or more, comma-separated values. Start and end string with brackets",""},
    {NULL, NULL, NULL}
};

static const ElementInfoPort multithreadL1_ports[] = {
    {"cache", "Link to L1 cache", memEvent_port_events},
    {"thread%(port)d", "Links to threads/cores", memEvent_port_events},
    {NULL, NULL, NULL}
};


/*****************************************************************************************
 *  Component: Scratchpad
 *  Purpose: Scratchpad (processor-managed cache)
 *****************************************************************************************/
static Component* create_scratchpad(ComponentId_t id, Params& params)
{
    return new Scratchpad(id, params);
}

static const ElementInfoParam scratchpad_params[] = {
    {"clock",               "(string) Clock frequency or period with units (Hz or s; SI units OK)."},
    {"size",                "(string) Size of the scratchpad in bytes (B), SI units ok"},
    {"scratch_line_size",   "(string) Number of bytes in a scratch line with units. 'size' must be divisible by this number.", "64B"},
    {"memory_line_size",    "(string) Number of bytes in a remote memory line with units. Used to set base addresses for routing.", "64B"},
    {"do_not_back",         "(bool) Whether to store actual data values in a backing store or not. Options: 0 (do not store), 1 (store). Do not unset unless simulation does not rely on correct data values!", "1"},
    {"memory_addr_offset",  "(uint) Amount to offset remote addresses by. Default is 'size' so that remote memory addresses start at 0", "size"},
    {"response_per_cycle",  "(uint) Maximum number of responses to return to processor each cycle. 0 is unlimited", "0"},
    {"backendConvertor",    "(string) Backend convertor to use for the scratchpad", "memHierarchy.scratchpadBackendConvertor"},
    {"debug",               "(uint) Where to print debug output. Options: 0[no output], 1[stdout], 2[stderr], 3[file]", "0"},
    {"debug_level",         "(uint) Debug verbosity level. Between 0 and 10", "0"},
    {NULL, NULL, NULL}
};

static const ElementInfoPort scratchpad_ports[] = {
    {"cpu", "Link to cpu/cache on the cpu side", memEvent_port_events},
    {"memory", "Direct link to a memory or bus", memEvent_port_events},
    {"network", "Network link to memory", net_port_events},
    {NULL, NULL, NULL}
};

static const ElementInfoStatistic scratchpad_statistics[] = {
    {"request_received_scratch_read",   "Number of scratchpad reads received from CPU", "count", 1},
    {"request_received_scratch_write",  "Number of scratchpad writes received from CPU", "count", 1},
    {"request_received_remote_read",    "Number of remote memory reads received from CPU", "count", 1},
    {"request_received_remote_write",   "Number of remote memory writes received from CPU", "count", 1},
    {"request_received_scratch_get",    "Number of scratchpad Gets received from CPU (copy from memory to scratch)", "count", 1},
    {"request_received_scratch_put",    "Number of scratchpad Puts received from CPU (copy from scratch to memory)", "count", 1},
    {"request_issued_scratch_read",     "Number of scratchpad reads issued to scratchpad", "count", 1},
    {"request_issued_scratch_write",    "Number of scratchpad writes issued to scratchpad", "count", 1},
    { NULL, NULL, NULL, 0 }
};


/*****************************************************************************************
 *  Component: BroadcastShim
 *  Purpose: Used with memSieves to broadcast mallocs to private memSieves
 *****************************************************************************************/
static Component* create_BroadcastShim(ComponentId_t id, Params& params)
{
    return new BroadcastShim(id, params);
}

static const ElementInfoPort broadcastShim_ports[] = {
    {"cpu_alloc_link_%(port)d", "Link to CPU's allocation port", arielAlloc_port_events},
    {"sieve_alloc_link_%(port)d", "Link to sieve's allocation port", arielAlloc_port_events},
    {NULL, NULL, NULL}
};

/*****************************************************************************************
 *  Component: sieve
 *  Purpose: Filters requests for those that miss in the cache hierarchy and maps
 *  them back to application mallocs - used with Ariel support for the sieve link
 *****************************************************************************************/
static Component* create_Sieve(ComponentId_t id, Params& params)
{
	return Sieve::sieveFactory(id, params);
}

static const ElementInfoParam sieve_params[] = {
    /* Required */
    {"cache_size",              "(string) Cache size with units. Eg. 4KiB or 1MiB"},
    {"associativity",           "(uint) Associativity of the cache. In set associative mode, this is the number of ways."},
    /* Not required */
    {"cache_line_size",         "(uint) Size of a cache line (aka cache block) in bytes.", "64"},
    {"profiler",                "(string) Name of profiling subcomponent. Currently only configured to work with cassini.AddrHistogrammer. Add params using 'profiler.paramName'", ""},
    {"debug",                   "(uint) Print debug information. Options: 0[no output], 1[stdout], 2[stderr], 3[file]", "0"},
    {"debug_level",             "(uint) Debugging/verbosity level. Between 0 and 10", "0"},
    {"output_file",             "(string) Name of file to output malloc information to. Will have sequence number (and optional marker number) and .txt appended to it. E.g. sieveMallocRank-3.txt", "sieveMallocRank"},
    {"reset_stats_at_buoy",     "(bool) Whether to reset allocation hit/miss stats when a buoy is found (i.e., when a new output file is dumped). Any value other than 0 is true." "0"},
    {NULL, NULL, NULL}
};

static const ElementInfoPort sieve_ports[] = {
    {"cpu_link_%(port)d", "Ports connected to the CPUs", memEvent_port_events},
    {"alloc_link_%(port)d", "Ports connected to the CPU's allocation/free notification", arielAlloc_port_events},
    {NULL, NULL, NULL}
};

static const ElementInfoStatistic sieve_statistics[] = {
    {"ReadHits",    "Number of read requests that hit in the sieve", "count", 1},
    {"ReadMisses",  "Number of read requests that missed in the sieve", "count", 1},
    {"WriteHits",   "Number of write requests that hit in the sieve", "count", 1},
    {"WriteMisses", "Number of write requests that missed in the sieve", "count", 1},
    {"UnassociatedReadMisses", "Number of read misses that did not match a malloc", "count", 1},
    {"UnassociatedWriteMisses", "Number of write misses that did not match a malloc", "count", 1},
    {NULL, NULL, NULL, 0},
};


/*****************************************************************************************
 *  Component: Bus
 *  Purpose: Connects one or more upper level components to one or more lower level components
 *  over a bus-like interface
 *****************************************************************************************/
static Component* create_Bus(ComponentId_t id, Params& params)
{
    return new Bus( id, params );
}

static const ElementInfoParam bus_params[] = {
    {"bus_frequency",       "(string) Bus clock frequency"},
    {"broadcast",           "(bool) If set, messages are broadcasted to all other ports", "0"},
    {"fanout",              "(bool) If set, messages from the high network are replicated and sent to all low network ports", "0"},
    {"bus_latency_cycles",  "(uint) Bus latency in cycles", "0"},
    {"idle_max",            "(uint) Bus temporarily turns off clock after this amount of idle cycles", "6"},
    {"debug",               "(uint) Prints debug statements --0[No debugging], 1[STDOUT], 2[STDERR], 3[FILE]--", "0"},
    {"debug_level",         "(uint) Debugging level: 0 to 10", "0"},
    {"debug_addr",          "(comma separated uint) Address(es) to be debugged. Leave empty for all, otherwise specify one or more, comma-separated values. Start and end string with brackets",""},
    {NULL, NULL}
};


static const ElementInfoPort bus_ports[] = {
    {"low_network_%(low_network_ports)d",  "Ports connected to lower level caches (closer to main memory)", memEvent_port_events},
    {"high_network_%(high_network_ports)d", "Ports connected to higher level caches (closer to CPU)", memEvent_port_events},
    {NULL, NULL, NULL}
};


/*****************************************************************************************
 *  Component: trivialCPU
 *  Purpose: Generates random memory requests - for testing
 *****************************************************************************************/
static Component* create_trivialCPU(ComponentId_t id, Params& params){
	return new trivialCPU( id, params );
}

static const ElementInfoPort cpu_ports[] = {
    {"mem_link", "Connection to caches.", NULL},
    {NULL, NULL, NULL}
};

static const ElementInfoParam cpu_params[] = {
    /* Required */
    {"commFreq",                "(int) How often to do a memory operation."},
    {"memSize",                 "(uint) Size of physical memory."},
    /* Optional */
    {"verbose",                 "(uint) Determine how verbose the output from the CPU is", "0"},
    {"clock",                   "(string) Clock frequency", "1GHz"},
    {"rngseed",                 "(int) Set a seed for the random generation of addresses", "7"},
    {"lineSize",                "(uint) Size of a cache line - used for flushes", "64"},
    {"maxOutstanding",          "(uint) Maximum Number of Outstanding memory requests.", "10"},
    {"num_loadstore",           "(int) Stop after this many reads and writes.", "-1"},
    {"reqsPerIssue",            "(uint) Maximum number of requests to issue at a time", "1"},
    {"do_write",                "(bool) Enable writes to memory (versus just reads).", "1"},
    {"do_flush",                "(bool) Enable flushes", "0"},
    {"noncacheableRangeStart",  "(uint) Beginning of range of addresses that are noncacheable.", "0x0"},
    {"noncacheableRangeEnd",    "(uint) End of range of addresses that are noncacheable.", "0x0"},
    {"addressoffset",           "(uint) Apply an offset to a calculated address to check for non-alignment issues", "0"},
    {NULL, NULL, NULL}
};

static const ElementInfoStatistic cpu_statistics[] = {
    {"pendCycle",               "Number of Pending Requests per Cycle", "count", 1},
    {NULL, NULL, NULL, 0}
};


/*****************************************************************************************
 *  Component: ScratchCPU
 *  Purpose: Generates random memory requests for a scratchpad - for testing
 *****************************************************************************************/
static Component* create_scratchCPU(ComponentId_t id, Params& params){
	return new ScratchCPU( id, params );
}

static const ElementInfoParam scratch_cpu_params[] = {
    /* Required */
    {"scratchSize",             "(uint) Size of the scratchpad in bytes"},
    {"maxAddr",                 "(uint) Maximum address to generate (i.e., scratchSize + size of memory)"},
    /* Optional */
    {"rngseed",                 "(int) Set a seed for the random generator used to create requests", "7"},
    {"scratchLineSize",         "(uint) Line size for scratch, max request size for scratch", "64"},
    {"memLineSize",             "(uint) Line size for memory, max request size for memory", "64"},
    {"clock",                   "(string) Clock frequency in Hz or period in s", "1GHz"},
    {"maxOutstandingRequests",  "(uint) Maximum number of requests outstanding at a time", "8"},
    {"maxRequestsPerCycle",     "(uint) Maximum number of requests to issue per cycle", "2"},
    {"reqsToIssue",             "(uint) Number of requests to issue before ending simulation", "1000"},
    {NULL, NULL, NULL}
};


/*****************************************************************************************
 *  Component: streamCPU
 *  Purpose: Generates sequential memory requests - for testing
 *****************************************************************************************/
static Component* create_streamCPU(ComponentId_t id, Params& params){
	return new streamCPU( id, params );
}



/*****************************************************************************************
 *  Component: MemController
 *  Purpose: Main memory controller, analagous to a single channel...loads a memory
 *  backend for timing
 *****************************************************************************************/
static Component* create_MemController(ComponentId_t id, Params& params){
	return new MemController( id, params );
}

static const ElementInfoParam memctrl_params[] = {
    /* Required parameters */
    {"backend.mem_size",    "(string) Size of physical memory. NEW REQUIREMENT: must include units in 'B' (SI ok). Simple fix: add 'MiB' to old value."},
    {"clock",               "(string) Clock frequency of controller"},
    /* Optional parameters */
    {"backend",             "(string) Timing backend to use:  Default to simpleMem", "memHierarchy.simpleMem"},
    {"request_width",       "(uint) Size of a DRAM request in bytes.", "64"},
    {"max_requests_per_cycle",  "(int) Maximum number of requests to accept per cycle. 0 or negative is unlimited. Default is 1 for simpleMem backend, unlimited otherwise.", "1"},
    {"trace_file",          "(string) File name (optional) of a trace-file to generate.", ""},
    {"debug",               "(uint) 0: No debugging, 1: STDOUT, 2: STDERR, 3: FILE.", "0"},
    {"debug_level",         "(uint) Debugging level: 0 to 10", "0"},
    {"debug_addr",          "(comma separated uint) Address(es) to be debugged. Leave empty for all, otherwise specify one or more, comma-separated values. Start and end string with brackets",""},
    {"listenercount",       "(uint) Counts the number of listeners attached to this controller, these are modules for tracing or components like prefetchers", "0"},
    {"listener%(listenercount)d", "(string) Loads a listener module into the controller", ""},
    {"do_not_back",         "(bool) DO NOT use this parameter if simulation depends on correct memory values. Otherwise, set to '1' to reduce simulation's memory footprint", "0"},
    {"memory_file",         "(string) Optional backing-store file to pre-load memory, or store resulting state", "N/A"},
    {"addr_range_start",    "(uint) Lowest address handled by this memory.", "0"},
    {"addr_range_end",      "(uint) Highest address handled by this memory.", "uint64_t-1"},
    {"interleave_size",     "(string) Size of interleaved chunks. E.g., to interleave 8B chunks among 3 memories, set size=8B, step=24B", "0B"},
    {"interleave_step",     "(string) Distance between interleaved chunks. E.g., to interleave 8B chunks among 3 memories, set size=8B, step=24B", "0B"},
    /* Old parameters - deprecated or moved */
    {"mem_size",            "DEPRECATED. Use 'backend.mem_size' instead. Size of physical memory in MiB", "0"},
    {"statistics",          "DEPRECATED - use Statistics API to get statistics for memory controller","0"},
    {"network_num_vc",      "DEPRECATED. Number of virtual channels (VCs) on the on-chip network. memHierarchy only uses one VC.", "1"},
    {"direct_link",         "DEPRECATED. Now auto-detected by configure. Specifies whether memory is directly connected to a directory/cache (1) or is connected via the network (0)","1"},
    {"coherence_protocol",  "DEPRECATED. No longer needed. Coherence protocol.  Supported: MESI (default), MSI. Only used when a directory controller is not present.", "MESI"},
    {"network_address",     "DEPRECATED - Now auto-detected by link control."},
    {"network_bw",          "MOVED. Now a member of the MemNIC subcomponent.", NULL},
    {"network_input_buffer_size",   "MOVED. Now a member of the MemNIC subcomponent.", "1KiB"},
    {"network_output_buffer_size",  "MOVED. Now a member of the MemNIC subcomponent.", "1KiB"},
    {"direct_link_latency", "MOVED. Now a member of the MemLink subcomponent.", "10 ns"},
    {NULL, NULL, NULL}
};

static const ElementInfoStatistic memctrl_statistics[] = {
    /* Cache hits and misses */
    { NULL, NULL, NULL, 0 }
};


static const ElementInfoPort memctrl_ports[] = {
    {"direct_link",     "Directly connect to another component (like a Directory Controller).", memEvent_port_events},
    {"cube_link",       "Link to VaultSim.", NULL}, /* TODO:  Make this generic */
    {"network",         "Network link to another component", net_port_events},
    {NULL, NULL, NULL}
};

/*****************************************************************************************
 *  Component: CoherentMemController
 *  Purpose: Main memory controller, analagous to a single channel, supports custom memory
 *  instructions which can issue cache line shootdowns
 *****************************************************************************************/
static Component* create_CoherentMemController(ComponentId_t id, Params& params){
	return new CoherentMemController( id, params );
}

static const ElementInfoParam cohmemctrl_params[] = {
    /* Required parameters */
    {"backend.mem_size",    "(string) Size of physical memory. NEW REQUIREMENT: must include units in 'B' (SI ok). Simple fix: add 'MiB' to old value."},
    {"clock",               "(string) Clock frequency of controller"},
    /* Optional parameters */
    {"backend",             "(string) Timing backend to use:  Default to simpleMem", "memHierarchy.simpleMem"},
    {"request_width",       "(uint) Size of a DRAM request in bytes.", "64"},
    {"max_requests_per_cycle",  "(int) Maximum number of requests to accept per cycle. 0 or negative is unlimited. Default is 1 for simpleMem backend, unlimited otherwise.", "1"},
    {"trace_file",          "(string) File name (optional) of a trace-file to generate.", ""},
    {"debug",               "(uint) 0: No debugging, 1: STDOUT, 2: STDERR, 3: FILE.", "0"},
    {"debug_level",         "(uint) Debugging level: 0 to 10", "0"},
    {"debug_addr",          "(comma separated uint) Address(es) to be debugged. Leave empty for all, otherwise specify one or more, comma-separated values. Start and end string with brackets",""},
    {"listenercount",       "(uint) Counts the number of listeners attached to this controller, these are modules for tracing or components like prefetchers", "0"},
    {"listener%(listenercount)d", "(string) Loads a listener module into the controller", ""},
    {"do_not_back",         "(bool) DO NOT use this parameter if simulation depends on correct memory values. Otherwise, set to '1' to reduce simulation's memory footprint", "0"},
    {"memory_file",         "(string) Optional backing-store file to pre-load memory, or store resulting state", "N/A"},
    {"addr_range_start",    "(uint) Lowest address handled by this memory.", "0"},
    {"addr_range_end",      "(uint) Highest address handled by this memory.", "uint64_t-1"},
    {"interleave_size",     "(string) Size of interleaved chunks. E.g., to interleave 8B chunks among 3 memories, set size=8B, step=24B", "0B"},
    {"interleave_step",     "(string) Distance between interleaved chunks. E.g., to interleave 8B chunks among 3 memories, set size=8B, step=24B", "0B"},
    {"customCmdMemHandler", "(string) Name of the custom command handler to load", ""},
    {NULL, NULL, NULL}
};

/*****************************************************************************************
 *  SubComponent: AMOCustomCmdMemHandler
 *  Purpose: (A)tomic (M)emory (O)peration CustomCmdMemHandler for issuing custom
 *  AMO commands to memBackends
 *****************************************************************************************/
static SubComponent* create_Mem_AMOCustomCmdMemHandler(Component* comp, Params& params){
	return new AMOCustomCmdMemHandler(comp, params);
}

/*****************************************************************************************
 *  SubComponent: simpleMemBackendConvertor
 *  Purpose: Converts memEvents from a memory controller into cmd/addr/size for backends
 *  which use the 'simpleMem' backend interface
 *****************************************************************************************/
static SubComponent* create_Mem_SimpleBackendConvertor(Component* comp, Params& params){
    return new SimpleMemBackendConvertor(comp, params);
}

static const ElementInfoStatistic memBackendConvertor_statistics[] = {
    /* Cache hits and misses */
    { "cycles_with_issue",                  "Total cycles with successful issue to back end",   "cycles",   1 },
    { "cycles_attempted_issue_but_rejected","Total cycles where an attempt to issue to backend was rejected (indicates backend full)", "cycles", 1 },
    { "total_cycles",                       "Total cycles called at the memory controller",     "cycles",   1 },
    { "requests_received_GetS",             "Number of GetS (read) requests received",          "requests", 1 },
    { "requests_received_GetSX",           "Number of GetSX (read) requests received",        "requests", 1 },
    { "requests_received_GetX",             "Number of GetX (read) requests received",          "requests", 1 },
    { "requests_received_PutM",             "Number of PutM (write) requests received",         "requests", 1 },
    { "outstanding_requests",               "Total number of outstanding requests each cycle",  "requests", 1 },
    { "latency_GetS",                       "Total latency of handled GetS requests",           "cycles",   1 },
    { "latency_GetSX",                     "Total latency of handled GetSX requests",         "cycles",   1 },
    { "latency_GetX",                       "Total latency of handled GetX requests",           "cycles",   1 },
    { "latency_PutM",                       "Total latency of handled PutM requests",           "cycles",   1 },
    { NULL, NULL, NULL, 0 }
};

/*****************************************************************************************
 *  SubComponent: simpleMemScratchBackendConvertor
 *  Purpose: Converts scratchEvents from a scratchpad into cmd/addr/size for backends
 *  which use the 'simpleMem' backend interface
 *****************************************************************************************/
static SubComponent* create_Mem_SimpleScratchBackendConvertor(Component* comp, Params& params){
    return new SimpleMemScratchBackendConvertor(comp, params);
}

static const ElementInfoStatistic scratchBackendConvertor_statistics[] = {
    { "cycles_with_issue",                      "Total cycles with successful issue to backend",    "cycles", 1},
    { "cycles_attempted_issue_but_rejected",    "Total cycles where an issue was attempted but backed rejected it", "cycles", 1},
    { "total_cycles",                           "Total cycles executed at the scratchpad",          "cycles", 1},
    { "requests_received_GetS",                 "Number of GetS (read) requests received",          "requests", 1 },
    { "requests_received_GetSX",                "Number of GetSX (read) requests received",         "requests", 1 },
    { "requests_received_GetX",                 "Number of GetX (read) requests received",          "requests", 1 },
    { "requests_received_PutM",                 "Number of PutM (write) requests received",         "requests", 1 },
    { "latency_GetS",                           "Total latency of handled GetS requests",           "cycles",   1 },
    { "latency_GetSX",                          "Total latency of handled GetSX requests",          "cycles",   1 },
    { "latency_GetX",                           "Total latency of handled GetX requests",           "cycles",   1 },
    { "latency_PutM",                           "Total latency of handled PutM requests",           "cycles",   1 },
    { NULL, NULL, NULL, 0 }
};

/*****************************************************************************************
 *  SubComponent: simpleMemBackendConvertor
 *  Purpose: Converts memEvents from a memory controller into cmd/addr/size for backends
 *  which use the 'simpleMem' backend interface
 *****************************************************************************************/
static SubComponent* create_Mem_FlagBackendConvertor(Component* comp, Params& params){
    return new FlagMemBackendConvertor(comp, params);
}

/*****************************************************************************************
 *  SubComponent: simpleMem
 *  Purpose: Memory backend, gives each request a constant access time
 *****************************************************************************************/
static SubComponent* create_Mem_SimpleSim(Component* comp, Params& params){
    return new SimpleMemory(comp, params);
}

static const ElementInfoParam simpleMem_params[] = {
    {"verbose",         "(uint) Sets the verbosity of the backend output", "0" },
    {"access_time",     "(string) Constant latency of memory operation.", "100 ns"},
    {NULL, NULL}
};

/*****************************************************************************************
 *  SubComponent: timingDRAM
 *  Purpose: Memory backend, simulates DRAM timing with bank conflicts, close/open row, 
 *  and some queuing
 *****************************************************************************************/
static SubComponent* create_Mem_TimingDRAM(Component* comp, Params& params) {
    return new TimingDRAM(comp, params);
}

/*****************************************************************************************
 *  SubComponent: TransactionQ
 *  Purpose: Used by timingDRAM memory backend, in-order queue
 *****************************************************************************************/
static SubComponent* create_Mem_TransactionQ(Component* comp, Params& params) {
    return new TransactionQ(comp, params);
}

/*****************************************************************************************
 *  SubComponent: ReorderTransactionQ
 *  Purpose: Used by timingDRAM memory backend, re-order queue
 *****************************************************************************************/
static SubComponent* create_Mem_ReorderTransactionQ(Component* comp, Params& params) {
    return new ReorderTransactionQ(comp, params);
}

/*****************************************************************************************
 *  SubComponent: SimplePagePolicy
 *  Purpose: Used by timingDRAM memory backend, implements open/close page policies
 *****************************************************************************************/
static SubComponent* create_Mem_SimplePagePolicy(Component* comp, Params& params) {
    return new SimplePagePolicy(comp, params);
}

/*****************************************************************************************
 *  SubComponent: TimeoutPagePolicy
 *  Purpose: Used by timingDRAM memory backend, implements open page policy with timeout
 *****************************************************************************************/
static SubComponent* create_Mem_TimeoutPagePolicy(Component* comp, Params& params) {
    return new TimeoutPagePolicy(comp, params);
}

/*****************************************************************************************
 *  SubComponent: SimpleDRAM
 *  Purpose: Memory backend, simulates DRAM with banks and open/close pages, use with
 *  stackable backends to achieve request re-ordering
 *****************************************************************************************/
static SubComponent* create_Mem_SimpleDRAM(Component* comp, Params& params) {
    return new SimpleDRAM(comp, params);
}

static const ElementInfoParam simpleDRAM_params[] = {
    {"verbose",     "(uint) Sets the verbosity of the backend output", "0" },
    {"cycle_time",  "(string) Latency of a cycle or clock frequency (e.g., '4ns' and '250MHz' are both accepted)", "4ns"},
    {"tCAS",        "(uint) Column access latency in cycles (i.e., access time if correct row is already open)", "9"},
    {"tRCD",        "(uint) Row access latency in cycles (i.e., time to open a row)", "9"},
    {"tRP",         "(uint) Precharge delay in cycles (i.e., time to close a row)", "9"},
    {"banks",       "(uint) Number of banks", "8"},
    {"bank_interleave_granularity", "(string) Granularity of interleaving in bytes (B), generally a cache line. Must be a power of 2.", "64B"},
    {"row_size",    "(string) Size of a row in bytes (B). Must be a power of 2.", "8KiB"},
    {"row_policy",  "(string) Policy for managing the row buffer - open or closed.", "closed"},
    {NULL, NULL, NULL}
};

static const ElementInfoStatistic simpleDRAM_stats[] = {
    {"row_already_open","Number of times a request arrived and the correct row was open", "count", 1},
    {"no_row_open",     "Number of times a request arrived and no row was open", "count", 1},
    {"wrong_row_open",  "Number of times a request arrived and the wrong row was open", "count", 1},
    { NULL, NULL, NULL, 0 }
};


/*****************************************************************************************
 *  SubComponent: DelayBuffer
 *  Purpose: Memory backend, stacks between memory controller and another backend, 
 *  adds a constant latency to every request
 *****************************************************************************************/
static SubComponent* create_Mem_DelayBuffer(Component * comp, Params& params) {
    return new DelayBuffer(comp, params);
}

static const ElementInfoParam delayBuffer_params[] = {
    {"verbose",     "Sets the verbosity of the backend output", "0" },
    {"backend",     "Backend memory system", "memHierarchy.simpleMem"},
    {"request_delay", "Constant delay to be added to requests with units (e.g., 1us)", "0ns"},
    {NULL, NULL, NULL}
};

/*****************************************************************************************
 *  SubComponent: RequestReorderSimple
 *  Purpose: Memory backend, stacks between memory controller and another backend, 
 *  reorders requests to backend by doing brute-force attempt to issue N requests 
 *****************************************************************************************/
static SubComponent* create_Mem_RequestReorderSimple(Component * comp, Params& params) {
    return new RequestReorderSimple(comp, params);
}

static const ElementInfoParam requestReorderSimple_params[] = {
    {"verbose",                 "Sets the verbosity of the backend output", "0" },
    {"max_issue_per_cycle",  "Maximum number of requests to issue per cycle. 0 or negative is unlimited.", "-1"},
    {"search_window_size",      "Maximum number of request to search each cycle. 0 or negative is unlimited.", "-1"},
    {"backend",                 "Backend memory system", "memHierarchy.simpleDRAM"},
    { NULL, NULL, NULL }
};


/*****************************************************************************************
 *  SubComponent: RequestReorderRow
 *  Purpose: Memory backend, stacks between memory controller and another backend, 
 *  reorders requests to backend by trying to issue requests to open rows first
 *****************************************************************************************/
static SubComponent* create_Mem_RequestReorderRow(Component * comp, Params& params) {
    return new RequestReorderRow(comp, params);
}

static const ElementInfoParam requestReorderRow_params[] = {
    {"verbose",                 "Sets the verbosity of the backend output", "0" },
    {"max_issue_per_cycle",  "Maximum number of requests to issue per cycle. 0 or negative is unlimited.", "-1"},
    {"banks",                   "Number of banks", "8"},
    {"bank_interleave_granularity", "Granularity of interleaving in bytes (B), generally a cache line. Must be a power of 2.", "64B"},
    {"row_size",                "Size of a row in bytes (B). Must be a power of 2.", "8KiB"},
    {"reorder_limit",           "Maximum number of request to reorder to a row before changing rows.", "1"},
    {"backend",                 "Backend memory system.", "memHierarchy.simpleDRAM"},
    { NULL, NULL, NULL }
};


/*****************************************************************************************
 *  SubComponent: ramulatorMemory
 *  Purpose: Memory backend, interface to Ramulator 
 *****************************************************************************************/
#if defined(HAVE_LIBRAMULATOR)
static SubComponent* create_Mem_Ramulator(Component* comp, Params& params){
    return new ramulatorMemory(comp, params);
}

static const ElementInfoParam ramulatorMem_params[] = {
    {"verbose",          "Sets the verbosity of the backend output", "0" },
    {"configFile",      "Name of Ramulator Device config file", NULL},
    {NULL, NULL, NULL}
};

#endif


/*****************************************************************************************
 *  SubComponent: dramsim
 *  Purpose: Memory backend, interface to DRAMSim
 *****************************************************************************************/
#if defined(HAVE_LIBDRAMSIM)
static SubComponent* create_Mem_DRAMSim(Component* comp, Params& params){
    return new DRAMSimMemory(comp, params);
}


static const ElementInfoParam dramsimMem_params[] = {
    {"verbose",          "Sets the verbosity of the backend output", "0" },
    {"device_ini",      "Name of DRAMSim Device config file", NULL},
    {"system_ini",      "Name of DRAMSim Device system file", NULL},
    {NULL, NULL, NULL}
};


/*****************************************************************************************
 *  SubComponent: pagedMultiMemory
 *  Purpose: Memory backend, implements both HMC (via simpleMem) and DDR (via DRAMSim)
 *  and simulates caching between them
 *****************************************************************************************/
static SubComponent* create_Mem_pagedMulti(Component* comp, Params& params){
    return new pagedMultiMemory(comp, params);
}


static const ElementInfoParam pagedMultiMem_params[] = {
    { "verbose",          "Sets the verbosity of the backend output", "0" },
    {"device_ini",      "Name of DRAMSim Device config file", NULL},
    {"system_ini",      "Name of DRAMSim Device system file", NULL},
    {"collect_stats",      "Name of DRAMSim Device system file", "0"},
    {"transfer_delay",      "Time (in ns) to transfer page to fast mem", "250"},
    {"dramBackpressure",    "Don't issue page swaps if DRAM is too busy", "1"},
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
    {"cant_swap", "Number of times a page could not be swapped in because no victim page could be found because all candidates were swapping", "count", 1},
    {"swap_delays", "Number of an access is delayed because the page is swapping", "count", 1},
    { NULL, NULL, NULL, 0 }
};

#endif

/*****************************************************************************************
 *  SubComponent: hbmdramsim
 *  Purpose: Memory backend, interface to DRAMSim using GaTech HBM mods
 *****************************************************************************************/
#if defined(HAVE_HBMLIBDRAMSIM)
static SubComponent* create_Mem_HBMDRAMSim(Component* comp, Params& params){
    return new HBMDRAMSimMemory(comp, params);
}


static const ElementInfoParam HBMdramsimMem_params[] = {
    {"verbose",          "Sets the verbosity of the backend output", "0" },
    {"device_ini",      "Name of DRAMSim Device config file", NULL},
    {"system_ini",      "Name of DRAMSim Device system file", NULL},
    {NULL, NULL, NULL}
};


/*****************************************************************************************
 *  SubComponent: HBMpagedMultiMemory
 *  Purpose: Memory backend, implements both HMC (via simpleMem) and DDR (via HBM DRAMSim)
 *  and simulates caching between them
 *****************************************************************************************/
static SubComponent* HBMcreate_Mem_pagedMulti(Component* comp, Params& params){
    return new HBMpagedMultiMemory(comp, params);
}


static const ElementInfoParam HBMpagedMultiMem_params[] = {
    { "verbose",          "Sets the verbosity of the backend output", "0" },
    {"device_ini",      "Name of DRAMSim Device config file", NULL},
    {"system_ini",      "Name of DRAMSim Device system file", NULL},
    {"collect_stats",      "Name of DRAMSim Device system file", "0"},
    {"transfer_delay",      "Time (in ns) to transfer page to fast mem", "250"},
    {"dramBackpressure",    "Don't issue page swaps if DRAM is too busy", "1"},
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

static const ElementInfoStatistic HBMpagedMultiMem_statistics[] = {
    {"fast_hits", "Number of accesses that 'hit' a fast page", "count", 1},
    {"fast_swaps", "Number of pages swapped between 'fast' and 'slow' memory", "count", 1},
    {"fast_acc", "Number of total accesses to the memory backend", "count", 1},
    {"t_pages", "Number of total pages", "count", 1},
    {"cant_swap", "Number of times a page could not be swapped in because no victim page could be found because all candidates were swapping", "count", 1},
    {"swap_delays", "Number of an access is delayed because the page is swapping", "count", 1},
    { NULL, NULL, NULL, 0 }
};
#endif //HAVE_HBMLIBDRAMSIM


/*****************************************************************************************
 *  SubComponent: hybridSimMemory
 *  Purpose: Memory backend, interface to HybridSim
 *****************************************************************************************/
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


/*****************************************************************************************
 *  SubComponent: vaultSimMemory
 *  Purpose: Memory backend, interface to VaultSim (vaulted memory)
 *****************************************************************************************/
static SubComponent* create_Mem_VaultSim(Component* comp, Params& params){
    return new VaultSimMemory(comp, params);
}

static const ElementInfoParam vaultsimMem_params[] = {
    { "verbose",          "Sets the verbosity of the backend output", "0" },
    {"access_time",     "When not using DRAMSim, latency of memory operation.", "100 ns"},
    {NULL, NULL, NULL}
};


/*****************************************************************************************
 *  SubComponent: cramSimMemory
 *  Purpose: Memory backend, interface to VaultSim (vaulted memory)
 *****************************************************************************************/
static SubComponent* create_Mem_CramSim(Component* comp, Params& params){
    return new CramSimMemory(comp, params);
}

static const ElementInfoParam cramsimMem_params[] = {
        { "verbose",          "Sets the verbosity of the backend output", "0" },
        {"access_time",     "When not using DRAMSim, latency of memory operation.", "100 ns"},
        {"max_outstanding_requests", "maximum number of the outstanding requests", "256"},
        {NULL, NULL, NULL}
};

/*****************************************************************************************
 *  SubComponent: vaultSimMemory
 *  Purpose: Memory backend, interface to Messier (NV memory)
 *****************************************************************************************/
static SubComponent* create_Mem_Messier(Component* comp, Params& params){
    return new Messier(comp, params);
}

static const ElementInfoParam Messier_params[] = {
    { "verbose",          "Sets the verbosity of the backend output", "0" },
    {"access_time",     "When not using DRAMSim, latency of memory operation.", "1 ns"},
    {NULL, NULL, NULL}
};

/*****************************************************************************************
 *  SubComponent: goblinHMCSim
 *  Purpose: Memory backend, interface to HMCSim (HMC memory)
 *****************************************************************************************/
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
  	{ "dram_count",         "Sets the number of DRAM blocks per cube", "20" },
	{ "xbar_depth",         "Sets the queue depth for the HMC X-bar", "8" },
        { "max_req_size",       "Sets the maximum request size, in bytes", "64" },
#ifdef HMC_DEV_DRAM_LATENCY
        { "dram_latency",       "Sets the internal DRAM fetch latency in clock cycles", "2" },
#endif
	{ "trace-banks", 	"Decides where tracing for memory banks is enabled, \"yes\" or \"no\", default=\"no\"", "no" },
	{ "trace-queue", 	"Decides where tracing for queues is enabled, \"yes\" or \"no\", default=\"no\"", "no" },
	{ "trace-cmds", 	"Decides where tracing for commands is enabled, \"yes\" or \"no\", default=\"no\"", "no" },
	{ "trace-latency", 	"Decides where tracing for latency is enabled, \"yes\" or \"no\", default=\"no\"", "no" },
	{ "trace-stalls", 	"Decides where tracing for memory stalls is enabled, \"yes\" or \"no\", default=\"no\"", "no" },
#ifdef HMC_TRACE_POWER
        { "trace-power",        "Decides where tracing for memory power is enabled, \"yes\" or \"no\", default=\"no\"", "no" },
#endif
	{ "tag_count",		"Sets the number of inflight tags that can be pending at any point in time", "16" },
	{ "capacity_per_device", "Sets the capacity of the device being simulated in GiB, min=2, max=8, default is 4", "4" },
        { "cmc-config",         "Enables a CMC library command in HMCSim", "NONE" },
        { "cmd-map",            "Maps an existing HMC or CMC command to the target command type", "NONE" },
	{ NULL, NULL, NULL }
};
#endif

#ifdef HAVE_LIBFDSIM

/*****************************************************************************************
 *  SubComponent: FlashDimmSimMemory
 *  Purpose: Memory backend, interface to FlashDIMMSim (FLASH memory)
 *****************************************************************************************/
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

/* SimpleMem interfaces */

/*****************************************************************************************
 *  SubComponent: memHierarchyInterface
 *  Purpose: Converts a SST::SimpleMem interface event into a memEvent
 *****************************************************************************************/
static SubComponent* create_MemInterface(Component *comp, Params &params) {
    return new MemHierarchyInterface(comp, params);
}

static Module* create_MemInterfaceModule(Component *comp, Params &params) {
    return new MemHierarchyInterface(comp, params);
}

/*****************************************************************************************
 *  SubComponent: memHierarchyScratchInterface
 *  Purpose: Converts a SST::SimpleMem interface event into a scratchEvent
 *****************************************************************************************/
static SubComponent* create_ScratchInterface(Component *comp, Params &params) {
    return new MemHierarchyScratchInterface(comp, params);
}

/*****************************************************************************************
 *  SubComponent: memNIC
 *  Purpose: Wraps a memEventBase in a simpleNetwork interface event
 *****************************************************************************************/
static SubComponent* create_MemNIC(Component *comp, Params &params) {
    return new MemNIC(comp, params);
}

static const ElementInfoParam memNIC_params[] = {
    /* Optional */
    { "memNIC.group",                       "(int) Group ID. See params 'sources' and 'destinations'. If not specified, the parent component will guess.", "1"},
    { "memNIC.sources",                     "(comma-separated list of ints) List of group IDs that serve as sources for this component. If not specified, defaults to 'group - 1'.", "group-1"},
    { "memNIC.destinations",                "(comma-separated list of ints) List of group IDs that serve as destinations for this component. If not specified, defaults to 'group + 1'.", "group+1"},
    { "memNIC.network_bw",                  "(string) Network bandwidth", "80GiB/s" },
    { "memNIC.network_input_buffer_size",   "(string) Size of input buffer", "1KiB"},
    { "memNIC.network_output_buffer_size",  "(string) Size of output buffer", "1KiB"},
    { "memNIC.min_packet_size",             "(string) Size of a packet without a payload (e.g., control message size)", "8B"},
    { "memNIC.accept_region",               "(bool) Set by parent component but user should unset if region (addr_range_start/end, interleave_size/step) params are provided to memory. Provides backward compatibility for address translation between memory controller and directory.", "0"},
    { "memNIC.port",                        "(string) Set by parent component. Name of port this NIC sits on.", ""},
    { "memNIC.addr_range_start",            "(uint) Set by parent component. Lowest address handled by the parent.", "0"},
    { "memNIC.addr_range_end",              "(uint) Set by parent component. Highest address handled by the parent.", "uint64_t-1"},
    { "memNIC.interleave_size",             "(uint) Set by parent component. Size of interleaved chunks.", "0B"},
    { "memNIC.interleave_step",             "(uint) Set by parent component. Distance between interleaved chunks.", "0B"},
    { NULL, NULL, NULL }
};

/*****************************************************************************************
 *  SubComponent: memLink
 *  Purpose: Base class for memory component links. 
 *           Wraps links in an address-aware subcomponent
 *****************************************************************************************/
static SubComponent* create_MemLink(Component * comp, Params &params) {
    return new MemLink(comp, params);
}

static const ElementInfoParam memLink_params[] = {
    { "{cpu|mem}link.latency",          "(string) Link latency. Prefix 'cpulink' for up-link towards CPU or 'memlink' for down-link towards memory", "50ps"},
    { "{cpu|mem}link.debug",            "(int) Where to print debug output. Options: 0[no output], 1[stdout], 2[stderr], 3[file]", "0"},
    { "{cpu|mem}link.debug_level",      "(int) Debug verbosity level. Between 0 and 10", "0"},
    {"debug_addr",          "(comma separated uint) Address(es) to be debugged. Leave empty for all, otherwise specify one or more, comma-separated values. Start and end string with brackets",""},
    { "{cpu|mem}link.accept_region",    "(bool) Set by parent component but user should unset if region (addr_range_start/end, interleave_size/step) params are provided to memory. Provides backward compatibility for address translation between memory controller and directory.", "0"},
    { "{cpu|mem}link.port",             "(string) Set by parent component. Name of port this memLink sits on.", ""},
    { "{cpu|mem}link.addr_range_start", "(uint) Set by parent component. Lowest address handled by the parent.", "0"},
    { "{cpu|mem}link.addr_range_end",   "(uint) Set by parent component. Highest address handled by the parent.", "uint64_t-1"},
    { "{cpu|mem}link.interleave_size",  "(string) Set by parent component. Size of interleaved chunks.", "0B"},
    { "{cpu|mem}link.interleave_step",  "(string) Set by parent component. Distance between interleaved chunks.", "0B"},
    { NULL, NULL, NULL }
};

/*****************************************************************************************
 *  Module: SandyBridgeAddrMapper
 *  Purpose: Used by timingDRAM memory backend, implements address mapping to DRAM 
 *  according to Sandybridge
 *****************************************************************************************/
static Module* create_SandyBridgeAddrMapper(Params &params) {
    return new SandyBridgeAddrMapper(params);
}

/*****************************************************************************************
 *  Module: SimpleAddrMapper
 *  Purpose: Used by timingDRAM memory backend, basic address mapping into DRAM
 *****************************************************************************************/
static Module* create_SimpleAddrMapper(Params &params) {
    return new SimpleAddrMapper(params);
}

/*****************************************************************************************
 *  Component: DirectoryController
 *  Purpose: Distributed directory for coherence (MSI or MESI). 
 *  Always sits on the NoC and may be co-located with memory
 *****************************************************************************************/
static Component* create_DirectoryController(ComponentId_t id, Params& params){
	return new DirectoryController( id, params );
}

static const ElementInfoParam dirctrl_params[] = {
    /* Optional parameters */
    {"clock",                   "Clock rate of controller.", "1GHz"},
    {"entry_cache_size",        "Size (in # of entries) the controller will cache.", "0"},
    {"debug",                   "0 (default): No debugging, 1: STDOUT, 2: STDERR, 3: FILE.", "0"},
    {"debug_level",             "Debugging level: 0 to 10", "0"},
    {"debug_addr",          "(comma separated uint) Address(es) to be debugged. Leave empty for all, otherwise specify one or more, comma-separated values. Start and end string with brackets",""},
    {"cache_line_size",         "Size of a cache line [aka cache block] in bytes.", "64"},
    {"coherence_protocol",      "Coherence protocol.  Supported --MESI, MSI--", "MESI"},
    {"mshr_num_entries",        "Number of MSHRs. Set to -1 for almost unlimited number.", "-1"},
    {"net_memory_name",         "For directories connected to a memory over the network: name of the memory this directory owns", ""},
    {"access_latency_cycles",   "Latency of directory access in cycles", "0"},
    {"mshr_latency_cycles",     "Latency of mshr access in cycles", "0"},
    {"max_requests_per_cycle",  "Maximum number of requests to process per cycle (0 or negative is unlimited)", "0"},
    {"mem_addr_start",          "Starting memory address for the chunk of memory that this directory controller addresses.", "0"},
    {"addr_range_start",        "Lowest address handled by this directory.", "0"},
    {"addr_range_end",          "Highest address handled by this directory.", "uint64_t-1"},
    {"interleave_size",         "Size of interleaved chunks. E.g., to interleave 8B chunks among 3 directories, set size=8B, step=24B", "0B"},
    {"interleave_step",         "Distance between interleaved chunks. E.g., to interleave 8B chunks among 3 directories, set size=8B, step=24B", "0B"},
    /* Old parameters - deprecated or moved */
    {"direct_mem_link",         "DEPRECATED. Now auto-detected by configure. Specifies whether directory has a direct connection to memory (1) or is connected via a network (0)","1"},
    {"network_num_vc",          "DEPRECATED. Number of virtual channels (VCs) on the on-chip network. memHierarchy only uses one VC.", "1"},
    {"statistics",              "DEPRECATED - Use the Statistics API to get statistics", "0"},
    {"network_address",         "DEPRECATD - Now auto-detected by link control", ""},
    {"network_bw",                  "MOVED. Now a member of the MemNIC/MemLink subcomponent.", "80GiB/s"},
    {"network_input_buffer_size",   "MOVED. Now a member of the MemNIC/MemLink subcomponent.", "1KiB"},
    {"network_output_buffer_size",  "MOVED. Now a member of the MemNIC/MemLink subcomponent.", "1KiB"},
    {NULL, NULL, NULL}
};

static const ElementInfoPort dirctrl_ports[] = {
    {"memory",      "Link to Memory Controller", NULL},
    {"network",     "Network Link", NULL},
    {NULL, NULL, NULL}
};


static const ElementInfoStatistic dirctrl_statistics[] = {
    {"replacement_request_latency",     "Total latency in ns of all replacement (put*) requests handled",       "nanoseconds",  1},
    {"get_request_latency",             "Total latency in ns of all get* requests handled",                     "nanoseconds",  1},
    {"directory_cache_hits",            "Number of requests that hit in the directory cache",                   "requests",     1},
    {"mshr_hits",                       "Number of requests that hit in the MSHRs",                             "requests",     1},
    {"requests_received_GetS",          "Number of GetS (read-shared) requests received",                       "requests",     1},
    {"requests_received_GetX",          "Number of GetX (write-exclusive) requests received",                   "requests",     1},
    {"requests_received_GetSX",        "Number of GetSX (read-exclusive) requests received",                    "requests",     1},
    {"requests_received_PutS",          "Number of PutS (shared replacement) requests received",                "requests",     1},
    {"requests_received_PutE",          "Number of PutE (clean exclusive replacement) requests received",       "requests",     1},
    {"requests_received_PutM",          "Number of PutM (dirty exclusive replacement) requests received",       "requests",     1},
    {"requests_received_noncacheable",  "Number of noncacheable requests that were received and forwarded",     "requests",     1},
    {"responses_received_NACK",         "Number of NACK responses received",                                    "responses",    1},
    {"responses_received_FetchResp",    "Number of FetchResp responses received (response to FetchInv/Fetch)",  "responses",    1},
    {"responses_received_FetchXResp",   "Number of FetchXResp responses received (response to FetchXInv) ",     "responses",    1},
    {"responses_received_PutS",         "Number of PutS (shared replacement) requests received that raced with an Inv/Fetch* and were treated as a response to that Inv/Fetch*",   "requests",     1},
    {"responses_received_PutE",         "Number of PutE (clean exclusive replacement) requests received that raced with a Fetch* and were treated as a response to that Fetch*",   "requests",     1},
    {"responses_received_PutM",         "Number of PutM (dirty exclusive replacement) requests received that raced with a Fetch* and were treated as a response to that Fetch*",   "requests",     1},
    {"memory_requests_directory_entry_read", "Number of read requests for a directory entry sent to memory",    "requests",     1},
    {"memory_requests_directory_entry_write","Number of write requests for a directory entry sent to memory",   "requests",     1},
    {"memory_requests_data_read",       "Number of read requests for data sent to memory",                      "requests",     1},
    {"memory_requests_data_write",      "Number of write requests for data sent to memory",                     "requests",     1},
    {"requests_sent_Inv",               "Number of Inv (invalidate) requests sent to LLCs",                     "requests",     1},
    {"requests_sent_FetchInv",          "Number of FetchInv (invalidate and fetch exclusive data) requests sent to LLCs",   "requests",     1},
    {"requests_sent_FetchInvX",         "Number of FetchInvX (fetch exclusive data and downgrade) requests sent to LLCs",   "requests",     1},
    {"responses_sent_NACK",             "Number of NACK responses sent to LLCs",                                            "responses",    1},
    {"responses_sent_GetSResp",         "Number of GetSResp (data response to GetS or GetSX) responses sent to LLCs",       "responses",    1},
    {"responses_sent_GetXResp",         "Number of GetXResp (data response to GetX) responses sent to LLCs",                "responses",    1},
    {"MSHR_occupancy",                  "Number of events in MSHR each cycle",                                  "events",       1},
    {NULL, NULL, NULL, 0}
};


/*****************************************************************************************
 *  Component: DMAEngine
 *  NOTE: Not up-to-date with the rest of memH. May or may not work.
 *  Purpose: Manages DMA
 *****************************************************************************************/
static Component* create_DMAEngine(ComponentId_t id, Params& params){
	return new DMAEngine( id, params );
}

static const ElementInfoParam dmaengine_params[] = {
    {"debug",           "0 (default): No debugging, 1: STDOUT, 2: STDERR, 3: FILE.", "0"},
    {"debug_level",     "Debugging level: 0 to 10", "0"},
    {"clockRate",       "Clock Rate for processing DMAs.", "1GHz"},
    {"netAddr",         "Network address of component.", NULL},
    {"network_num_vc",  "DEPRECATED. Number of virtual channels (VCs) on the on-chip network. memHierarchy only uses one VC.", "1"},
    {"printStats",      "0 (default): Don't print, 1: STDOUT, 2: STDERR, 3: FILE.", "0"},
    {NULL, NULL, NULL}
};


static const ElementInfoPort dmaengine_ports[] = {
    {"netLink",     "Network Link",     net_port_events},
    {NULL, NULL, NULL}
};

/*****************************************************************************************
 *  SubComponent: networkMemInspector
 *  Purpose: Sits at a network interface and counts events by command type
 *****************************************************************************************/
static SubComponent* load_networkMemoryInspector(Component* parent, 
                                                 Params& params) {
    return new networkMemInspector(parent);
}


/*****************************************************************************************
 *  SubComponent: memNetBridge
 *  Purpose: Enables multiple networks in a single configuration
 *****************************************************************************************/
static SubComponent* create_MemNetBridge(Component* comp, Params& params){
    return new MemNetBridge(comp, params);
}

static const ElementInfoParam bridge_params[] = {
    {"debug",                   "(int) Print debug information. Options: 0[no output], 1[stdout], 2[stderr], 3[file]", "0"},
    {"debug_level",             "(int) Debugging level. Between 0 and 10", "0"},
    {NULL, NULL}
};



/*****************************************************************************************
 *****************************************************************************************/


static const ElementInfoSubComponent subcomponents[] = {
    {
        "memInterface",
        "Simplified interface to Memory Hierarchy",
        NULL,
        create_MemInterface,
        NULL,
        NULL,
        "SST::Interfaces::SimpleMem"
    },
    {
        "scratchInterface",
        "Interface to a scratchpad",
        NULL,
        create_ScratchInterface,
        NULL,
        NULL,
        "SST::Interfaces::SimpleMem"
    },
    {
        "simpleMemBackendConvertor",
        "Convert MemEventBase to base mem backend",
        NULL, /* Advanced help */
        create_Mem_SimpleBackendConvertor, /* Module Alloc w/ params */
        NULL,
        memBackendConvertor_statistics, /* statistics */
        "SST::MemHierarchy::MemBackendConvertor"
    },
    {
        "simpleMemScratchBackendConvertor",
        "Convert MemEventBase to base mem backend, uses different interface than simpleMemBackendConvertor",
        NULL, /* Advanced help */
        create_Mem_SimpleScratchBackendConvertor, /* Module Alloc w/ params */
        NULL,
        scratchBackendConvertor_statistics,
        "SST::MemHierarchy::MemBackendConvertor"
    },
    {
        "flagMemBackendConvertor",
        "Convert MemEventBase to mem backend, pass flags with request/response",
        NULL,
        create_Mem_FlagBackendConvertor,
        NULL,
        memBackendConvertor_statistics,
        "SST::MemHierarchy::MemBackendConvertor"
    },
    {
        "simpleMem",
        "Simple constant-access time memory",
        NULL, /* Advanced help */
        create_Mem_SimpleSim, /* Module Alloc w/ params */
        simpleMem_params,
        NULL, /* statistics */
        "SST::MemHierarchy::MemBackend"
    },
    {
        "simpleDRAM",
        "Simplified timing model for DRAM",
        NULL,
        create_Mem_SimpleDRAM,
        simpleDRAM_params,
        simpleDRAM_stats,
        "SST::MemHierarchy::MemBackend"
    },
    {
        "timingDRAM",
        "Moderate timing model for DRAM",
        NULL,
        create_Mem_TimingDRAM,
        NULL,
        NULL,
        "SST::MemHierarchy::MemBackend"
    },
    {
        "fifoTransactionQ",
        "fifo transaction queue",
        NULL,
        create_Mem_TransactionQ,
        NULL,
        NULL,
        "SST::MemHierarchy::MemBackend"
    },
    {
        "reorderTransactionQ",
        "reorder transaction queue",
        NULL,
        create_Mem_ReorderTransactionQ,
        NULL,
        NULL,
        "SST::MemHierarchy::MemBackend"
    },
    {
        "simplePagePolicy",
        "static page open or close policy",
        NULL,
        create_Mem_SimplePagePolicy,
        NULL,
        NULL,
        "SST::MemHierarchy::MemBackend"
    },
    {
        "timeoutPagePolicy",
        "timeout based page open or close policy",
        NULL,
        create_Mem_TimeoutPagePolicy,
        NULL,
        NULL,
        "SST::MemHierarchy::MemBackend"
    },
    {
        "DelayBuffer",
        "Delays requests by specified time",
        NULL,
        create_Mem_DelayBuffer,
        delayBuffer_params,
        NULL,
        "SST::MemHierarchy::MemBackend"
    },
    {
        "reorderSimple",
        "Simple request re-orderer, issues the first N requests that are accepted by the backend",
        NULL,
        create_Mem_RequestReorderSimple,
        requestReorderSimple_params,
        NULL,
        "SST::MemHierarchy::MemBackend"
    },
    {
        "reorderByRow",
        "Request re-orderer, groups requests by row.",
        NULL,
        create_Mem_RequestReorderRow,
        requestReorderRow_params,
        NULL,
        "SST::MemHierarchy::MemBackend"
    },
    {
        "amoCustomCmdHandler",
        "AMO custom command handler",
        NULL,
        create_Mem_AMOCustomCmdMemHandler,
        NULL,//amoCustomCmd_params,
        NULL,//amoCustomCmd_statistics,
        "SST::MemHierarchy::AMOCustomCmdMemHandler"
    },
#if defined(HAVE_LIBRAMULATOR)
    {
        "ramulator",
        "Ramulator-driven memory timings",
        NULL, /* Advanced help */
        create_Mem_Ramulator, /* alloc subcomponent */
        ramulatorMem_params,
        NULL, /* stats */
        "SST::MemHierarchy::MemBackend"
    },
#endif
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
#if defined(HAVE_HBMLIBDRAMSIM)
    {
        "hbmdramsim",
        "HBM DRAMSim-driven memory timings",
        NULL, /* Advanced help */
        create_Mem_HBMDRAMSim, /* Module Alloc w/ params */
        HBMdramsimMem_params,
        hbmDramSim_statistics, /* statistics */
        "SST::MemHierarchy::MemBackend"
    },
    {
        "HBMpagedMulti",
        "HBM DRAMSim-driven memory timings with a fixed timing multi-level memory using paging",
        NULL, /* Advanced help */
        HBMcreate_Mem_pagedMulti, /* Module Alloc w/ params */
        HBMpagedMultiMem_params,
        HBMpagedMultiMem_statistics, /* statistics */
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
        "extMemBackendConvertor",
        "Convert MemEventBase to Ext mem backend",
        NULL,
        create_Mem_ExtBackendConvertor,
        NULL,
        memBackendConvertor_statistics,
        "SST::MemHierarchy::MemBackendConvertor"
    },
    {
        "goblinHMCSim",
        "GOBLIN HMC Simulator driven memory timings",
        NULL, /* Advanced help */
        create_Mem_GOBLINHMCSim, /* Module Alloc w/ params */
        goblin_hmcsim_Mem_params,
        goblinhmcsim_statistics, /* statistics */
        "SST::MemHierarchy::MemBackend"
    },
#endif
#ifdef HAVE_LIBFDSIM
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
     {
        "cramsim",
        "CramSim Memory timings",
        NULL, /* Advanced help */
        create_Mem_CramSim, /* Module Alloc w/ params */
        cramsimMem_params,
        NULL, /* statistics */
        "SST::MemHierarchy::MemBackend"
    },
    {
        "Messier",
        "Messier Memory timings",
        NULL, /* Advanced help */
        create_Mem_Messier, /* Module Alloc w/ params */
        Messier_params,
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
    { "MemNetBridge",
        "Merlin::Bridge::Translator for memory network bridging",
        NULL,
        create_MemNetBridge,
        bridge_params,
        NULL,
        "SST::Merlin::Bridge::Translator"
    },
    {   "MESICoherenceController",
        "Coherence controller for MESI or MSI protocol, non-L1",
        NULL,
        create_MESICoherenceController,
        NULL,
        coherence_statistics,
        "SST::MemHierarchy::CoherenceController"
    },
    {   "MESICacheDirectoryCoherenceController",
        "Coherence controller for non-inclusive cache with directory, MESI or MSI protocol, non-L1",
        NULL,
        create_MESICacheDirectoryCoherenceController,
        NULL,
        coherence_statistics,
        "SST::MemHierarchy::CoherenceController"
    },
    {   "IncoherentController",
        "Incoherent controller, non-L1",
        NULL,
        create_IncoherentController,
        NULL,
        coherence_statistics,
        "SST::MemHierarchy::CoherenceController"
    },
    {   "L1CoherenceController",
        "Coherence controller for MESI & MSI protocols, L1 caches",
        NULL,
        create_L1CoherenceController,
        NULL,
        coherence_statistics,
        "SST::MemHierarchy::CoherenceController"
    },
    {   "L1IncoherentController",
        "Incoherent controller for MESI & MSI protocols, L1 caches",
        NULL,
        create_L1IncoherentController,
        NULL,
        coherence_statistics,
        "SST::MemHierarchy::CoherenceController"
    },
    {   "MemLink",
        "Memory-oriented link interface",
        NULL,
        create_MemLink,
        memLink_params,
        NULL,
        "SST::MemLink",
    },
    {   "MemNIC",
        "Memory-oriented Network Interface",
        NULL,
        create_MemNIC,
        memNIC_params,
        NULL,
        "SST::MemLink",
    },
            
    {NULL, NULL, NULL, NULL, NULL, NULL}
};

static const ElementInfoModule modules[] = {
    {
        "memInterface",
        "Simplified interface to Memory Hierarchy",
        NULL,
        NULL,
        create_MemInterfaceModule,
        NULL,
        "SST::Interfaces::SimpleMem",
    },
    {
        "simpleAddrMapper",
        "simple address mapper",
        NULL, /* Advanced help */
        create_SimpleAddrMapper,
        NULL,
        NULL, /* Params */
        NULL, /* Interface */
    },
    {
        "sandyBridgeAddrMapper",
        "Sandy Bridge address mapper",
        NULL, /* Advanced help */
        create_SandyBridgeAddrMapper,
        NULL,
        NULL, /* Params */
        NULL, /* Interface */
    },
    {NULL, NULL, NULL, NULL, NULL, NULL}
};


static const ElementInfoComponent components[] = {
	{   "Cache",
	    "Cache Component",
	    NULL,
            create_Cache,
            cache_params,
            cache_ports,
            COMPONENT_CATEGORY_MEMORY,
            cache_statistics
	},
        {   "multithreadL1",
            "Layer to enable connecting multiple CPUs to a single L1 as if multiple hardware threads",
            NULL,
            create_multithreadL1,
            multithreadL1_params,
            multithreadL1_ports,
            COMPONENT_CATEGORY_MEMORY,
            NULL
        },
        {   "Sieve",
	    "Simple Cache Filtering Component to model LL private caches",
	    NULL,
            create_Sieve,
            sieve_params,
            sieve_ports,
            COMPONENT_CATEGORY_MEMORY,
            sieve_statistics	
        },
        {   "BroadcastShim",
            "Used to connect a processor to multiple Sieves.",
            NULL,
            create_BroadcastShim,
            NULL,
            broadcastShim_ports,
            COMPONENT_CATEGORY_MEMORY,
            NULL
        },
	{   "Bus",
	    "Mem Hierarchy Bus Component",
	    NULL,
	    create_Bus,
            bus_params,
            bus_ports,
            COMPONENT_CATEGORY_MEMORY
	},
	{   "MemController",
	    "Memory Controller Component",
	    NULL,
	    create_MemController,
            memctrl_params,
            memctrl_ports,
            COMPONENT_CATEGORY_MEMORY,
	    memctrl_statistics
	},
        {   "CoherentMemController",
            "Coherent memory controller component",
            NULL,
            create_CoherentMemController,
            cohmemctrl_params,
            memctrl_ports,
            COMPONENT_CATEGORY_MEMORY,
            memctrl_statistics
        },
	{   "DirectoryController",
	    "Coherencey Directory Controller Component",
	    NULL,
	    create_DirectoryController,
            dirctrl_params,
            dirctrl_ports,
            COMPONENT_CATEGORY_MEMORY,
	    dirctrl_statistics
        },
        {   "Scratchpad",
            "Scratchpad memory",
            NULL,
            create_scratchpad,
            scratchpad_params,
            scratchpad_ports,
            COMPONENT_CATEGORY_MEMORY,
            scratchpad_statistics
        },
	{   "DMAEngine",
	    "DMA Engine Component",
	    NULL,
	    create_DMAEngine,
            dmaengine_params,
            dmaengine_ports,
            COMPONENT_CATEGORY_MEMORY
	},
	{   "trivialCPU",
	    "Simple Demo CPU for testing",
	    NULL,
	    create_trivialCPU,
            cpu_params,
            cpu_ports,
            COMPONENT_CATEGORY_PROCESSOR,
            cpu_statistics
	},
	{   "ScratchCPU",
	    "Simple test CPU for scratchpad interface",
	    NULL,
	    create_scratchCPU,
            scratch_cpu_params,
            cpu_ports,
            COMPONENT_CATEGORY_PROCESSOR
	},
        {   "streamCPU",
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
        { "ScratchEvent", "Event to interact with scratchpads", NULL, NULL },
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

