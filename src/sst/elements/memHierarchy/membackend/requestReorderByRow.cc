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
#include "sst/elements/memHierarchy/util.h"
#include "membackend/requestReorderByRow.h"

using namespace SST;
using namespace SST::MemHierarchy;

/*------------------------------- Simple Backend ------------------------------- */
RequestReorderRow::RequestReorderRow(ComponentId_t id, Params &params) : SimpleMemBackend(id, params){ 

    fixupParams( params, "clock", "backend.clock" );

    // Get parameters
    reqsPerCycle = params.find<int>("max_issue_per_cycle", -1);

    banks = params.find<unsigned int>("banks", 8);
    UnitAlgebra rowSize(params.find<std::string>("row_size", "8KiB"));
    maxReqsPerRow = params.find<unsigned int>("reorder_limit", 1);    // No re-ordering
    UnitAlgebra requestSize(params.find<std::string>("bank_interleave_granularity", "64B"));

    // Check parameters
    if (banks == 0) {
        output->fatal(CALL_INFO, -1, "Invalid param(%s): banks - must be at least 1. You specified '0'.\n", getName().c_str());
    }
    if (!(rowSize.hasUnits("B"))) {
        output->fatal(CALL_INFO, -1, "Invalid param(%s): row_size - must have units of 'B' (bytes). You specified %s.\n", getName().c_str(), rowSize.toString().c_str());
    }
    if (!isPowerOfTwo(rowSize.getRoundedValue())) {
        output->fatal(CALL_INFO, -1, "Invalid param(%s): row_size - must be a power of two. You specified %s.\n", getName().c_str(), rowSize.toString().c_str());
    }
    if (maxReqsPerRow == 0) maxReqsPerRow = 1;
    if (!(requestSize.hasUnits("B"))) {
        output->fatal(CALL_INFO, -1, "Invalid param(%s): bank_interleave_granularity - must have units of 'B' (bytes). You specified '%s'.\n", getName().c_str(), requestSize.toString().c_str());
    }
    if (!isPowerOfTwo(requestSize.getRoundedValue())) {
        output->fatal(CALL_INFO, -1, "Invalid param(%s): bank_interleave_granularity - must be a power of two. You specified '%s'.\n", getName().c_str(), requestSize.toString().c_str());
    }

    // Create our backend & copy 'mem_size' through for now
    backend = loadUserSubComponent<SimpleMemBackend>("backend");
    if (!backend) {
        std::string backendName = params.find<std::string>("backend", "memHierarchy.simpleDRAM");
        Params backendParams = params.get_scoped_params("backend");
        backendParams.insert("mem_size", params.find<std::string>("mem_size"));
        backend = loadAnonymousSubComponent<SimpleMemBackend>(backendName, "backend", 0, ComponentInfo::INSERT_STATS | ComponentInfo::SHARE_PORTS, backendParams);
    }
    using std::placeholders::_1;
    backend->setResponseHandler( std::bind( &RequestReorderRow::handleMemResponse, this, _1 )  );
    m_memSize = backend->getMemSize(); // inherit from backend

    // Set up local variables
    nextBank = 0;
    bankMask = banks - 1;
    rowOffset = log2Of(rowSize.getRoundedValue());
    lineOffset = log2Of(requestSize.getRoundedValue());
    for (unsigned int i = 0; i < banks; i++) {
        std::list<Req >* bankList = new std::list<Req>;
        requestQueue.push_back(bankList);
        lastRow.push_back(-1);  // No last request to this bank
        reorderCount.push_back(maxReqsPerRow);  // No requests reordered to this row
    }

}

bool RequestReorderRow::issueRequest(ReqId id, Addr addr, bool isWrite, unsigned numBytes ) {
#ifdef __SST_DEBUG_OUTPUT__
    output->debug(_L10_, "Reorderer received request for 0x%" PRIx64 "\n", (Addr)addr);
#endif
    int bank = (addr >> lineOffset) & bankMask;

    requestQueue[bank]->push_back(Req(id,addr,isWrite,numBytes));
    return true;
}

/*
 * Issue as many requests as we can up to requestsPerCycle
 * by searching up to searchWindowSize requests
 */
bool RequestReorderRow::clock(Cycle_t cycle) {

    if (!requestQueue.empty()) {

        int reqsIssuedThisCycle = 0;
        int reqsSearchedThisCycle = 0;
        // For current bank
        unsigned int bank = nextBank;
        for (unsigned int i = 0; i < banks; i++) {
            if (requestQueue[bank]->empty()) {
                bank = (bank + 1) % banks;
                continue;
            }

            // Decide whether to try to re-order a request to this bank or issue a new row
            bool reorderIssued = false;
            if (reorderCount[bank] != maxReqsPerRow) {
                std::list<Req>* bankList = requestQueue[bank];
                for (std::list<Req>::iterator it = bankList->begin(); it != bankList->end(); it++) {
                    unsigned int row = (*it).addr >> rowOffset;

                    if (row == lastRow[bank]) {
                        // Attempt issue, if we're blocked, this bank is busy & move to next bank
                        bool issued = backend->issueRequest((*it).id,(*it).addr,(*it).isWrite,(*it).numBytes);
                        reorderIssued = true;
                        if (issued) {
                            reqsIssuedThisCycle++;
                            nextBank = (bank + 1) % banks;
                            reorderCount[bank]++;
                            bankList->erase(it);
                            break;
                        } else {
                            break;
                        }
                    }
                }
            }

            if (!reorderIssued) {
                // Try to issue oldest request
				Req& req = *requestQueue[bank]->begin();
                if (backend->issueRequest( req.id, req.addr, req.isWrite, req.numBytes ) ) {
                    reqsIssuedThisCycle++;
                    nextBank = (bank + 1) % banks;
                    reorderCount[bank] = 1;
                    lastRow[bank] = req.addr >> rowOffset;
                    requestQueue[bank]->erase(requestQueue[bank]->begin());
                }
            }

            if (reqsIssuedThisCycle == reqsPerCycle) {
                break;  // Can't issue any more
            }

            bank = (bank + 1) % banks;
        }
    }

    bool unclock = backend->clock(cycle);
    return false;
}


/*
 * Call throughs to our backend
 */

void RequestReorderRow::setup() {
    backend->setup();
}

void RequestReorderRow::finish() {
    backend->finish();
}
