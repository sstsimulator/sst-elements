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


#include <sst_config.h>
#include <sst/core/link.h>
#include "sst/elements/memHierarchy/util.h"
#include "membackend/simpleDRAMBackend.h"

using namespace SST;
using namespace SST::MemHierarchy;

/*------------------------------- Simple DRAM ------------------------------- */
/* SimpleDRAM is a very simplified DRAM timing model. It considers only the
 * latency differences between accessing open/closed rows and implements banking.
 * The latencies are as follows:
 *      Correct row already open: tCAS
 *      No row open: tRCD + tCAS
 *      Wrong row open: tRP + tRCD + tCAS
 *
 *  SimpleDRAM maps address to banks and rows as follows
 *      Example: 64B lines, 8 banks, 8KiB rows (256 columns/row)
 *      |...  19|18     9|8    6|5    0|
 *      |  Row  | Column | Bank | Line |
 *
 * Implementation notes:
 *  SimpleDRAM relies on external (memController or other backend) request limiting. SimpleDRAM will block banks but will accept multiple requests per cycle if they don't have a bank conflict.
 *  SimpleDRAM assumes transfer granularity is a cache line -> this can be changed by changing 'cache_line_size_in_bytes'
 *  SimpleDRAM does not model bus contention back to the controller -> possible for multiple requests to return in the same cycle
 *
 *  TODO
 *   * Randomize bank bits?
 *   * Decide whether to consider tRAS
 *   * Allow multiple ranks?
 */


SimpleDRAM::SimpleDRAM(ComponentId_t id, Params &params) : SimpleMemBackend(id, params){ 
    // Get parameters
    tCAS = params.find<uint64_t>("tCAS", 9);
    tRCD = params.find<uint64_t>("tRCD", 9);
    tRP = params.find<uint64_t>("tRP", 9);
    std::string cycTime = params.find<std::string>("cycle_time", "4ns");
    uint64_t banks = params.find<uint64_t>("banks", 8);
    UnitAlgebra lineSize(params.find<std::string>("bank_interleave_granularity", "64B"));
    UnitAlgebra rowSize(params.find<std::string>("row_size", "8KiB"));
    bool found = false;
    std::string policyStr = params.find<std::string>("row_policy", "closed", found);
    int verbose = params.find<int>("verbose", 0);

    output = new Output("SimpleDRAM[@p:@l]: ", verbose, 0, Output::STDOUT);

    // Check parameters
    // Supported policies are 'open', 'closed' or 'dynamic'
    if (policyStr != "closed" && policyStr != "open") {
        output->fatal(CALL_INFO, -1, "Invalid param(%s): row_policy - must be 'closed' or 'open'. You specified '%s'.\n", getName().c_str(), policyStr.c_str());
    }

    if (policyStr == "closed") policy = RowPolicy::CLOSED;
    else policy = RowPolicy::OPEN;

    // banks needs to be a power of 2 -> use to set bank mask
    if (!isPowerOfTwo(banks)) {
        output->fatal(CALL_INFO, -1, "Invalid param(%s): banks - must be a power of two. You specified %" PRIu64 ".\n", getName().c_str(), banks);
    }
    bankMask = banks - 1;

    // line size needs to be a power of 2 and have units of bytes
    if (!(lineSize.hasUnits("B"))) {
        output->fatal(CALL_INFO, -1, "Invalid param(%s): cache_line_size_in_bytes - must have units of 'B' (bytes). The units you specified were '%s'.\n", getName().c_str(), lineSize.toString().c_str());
    }
    if (!isPowerOfTwo(lineSize.getRoundedValue())) {
        output->fatal(CALL_INFO, -1, "Invalid param(%s): cache_line_size_in_bytes - must be a power of two. You specified %s.\n", getName().c_str(), lineSize.toString().c_str());
    }
    lineOffset = log2Of(lineSize.getRoundedValue());

    // row size (# columns) needs to be power of 2 and have units of bytes
    if (!(rowSize.hasUnits("B"))) {
        output->fatal(CALL_INFO, -1, "Invalid param(%s): row_size - must have units of 'B' (bytes). You specified %s.\n", getName().c_str(), rowSize.toString().c_str());
    }
    if (!isPowerOfTwo(rowSize.getRoundedValue())) {
        output->fatal(CALL_INFO, -1, "Invalid param(%s): row_size - must be a power of two. You specified %s.\n", getName().c_str(), rowSize.toString().c_str());
    }
    rowOffset = log2Of(rowSize.getRoundedValue());

    // Bookkeeping for bank/row state
    busy = (bool*) malloc(sizeof(bool) * banks);
    openRow = (int*) malloc(sizeof(int) * banks);
    for (int i = 0; i < banks; i++) {
        openRow[i] = -1;
        busy[i] = false;
    }

    // Self link for timing requests
    self_link = configureSelfLink("Self", cycTime, new Event::Handler<SimpleDRAM>(this, &SimpleDRAM::handleSelfEvent));

    // Some statistics
    statRowHit = registerStatistic<uint64_t>("row_already_open");
    statRowMissNoRP = registerStatistic<uint64_t>("no_row_open");
    statRowMissRP = registerStatistic<uint64_t>("wrong_row_open");
}

/*
 *  Return response and free bank
 */
void SimpleDRAM::handleSelfEvent(SST::Event *event){
    MemCtrlEvent *ev = static_cast<MemCtrlEvent*>(event);
    if ( ! ev->close ) {
        if (policy == RowPolicy::CLOSED) {
            self_link->send(tRP, new MemCtrlEvent(ev->bank));
        } else {
            busy[ev->bank] = false;
        }
        handleMemResponse(ev->reqId);
        delete event;
    } else {
        openRow[ev->bank] = -1;
        busy[ev->bank] = false;
        delete event;
    }
}

bool SimpleDRAM::issueRequest( ReqId reqId, Addr addr, bool isWrite, unsigned numBytes ){

    // Determine bank & row for address
    //  Basic mapping: interleave cache lines across banks
    int bank = (addr >> lineOffset) & bankMask;
    int row = addr >> rowOffset;

#ifdef __SST_DEBUG_OUTPUT__
    output->debug(_L10_, "SimpleDRAM (%s) received request for address %" PRIx64 " which maps to bank: %d, row: %d. Bank status: %s, open row is %d\n",
           getName().c_str(), addr, bank, row, (busy[bank] ? "busy" : "idle"), openRow[bank]);
#endif

    // If bank is busy -> return false;
    if (busy[bank]) return false;

    int latency = tCAS;
    if (openRow[bank] != row) {
        latency += tRCD;
        if (openRow[bank] != -1) {
            latency += tRP;
            statRowMissRP->addData(1);
        } else {
            statRowMissNoRP->addData(1);
        }
        openRow[bank] = row;
    } else {
        statRowHit->addData(1);
    }
    busy[bank] = true;
    self_link->send(latency, new MemCtrlEvent( bank, reqId ));

    return true;
}
