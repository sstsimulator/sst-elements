// Copyright 2023 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2023, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst/core/sst_config.h>

#include "mmioTile.h"
#include "golemUtil.h"

#ifdef MANUAL_COMPUTE_ARRAY
	#include "manualMVMComputeArray.h"
#else
	#include "crossSimComputeArray.h"
#endif

using namespace SST;
using namespace SST::Interfaces;
using namespace SST::Golem;

MMIOTile::MMIOTile(ComponentId_t id, Params& params) : 
    Component(id)
{
    out.init("", params.find<int>("verbose", 0), 0, Output::STDOUT);

    // Required parameters
    bool found;

    mmioStartAddr = params.find<uint64_t>("mmioAddr", 0, found);
    if (!found){
        out.fatal(CALL_INFO, -1,"%s, Error: parameter 'mmioAddr' was not provided\n",
            getName().c_str());
    }

    arrayInputSize = params.find<uint64_t>("arrayInputSize", 0, found);
    if (!found){
        out.fatal(CALL_INFO, -1,"%s, Error: parameter 'arrayInputSize' was not provided\n",
            getName().c_str());
    }

    arrayOutputSize = params.find<uint64_t>("arrayOutputSize", 0, found);
    if (!found){
        out.fatal(CALL_INFO, -1,"%s, Error: parameter 'arrayOutputSize' was not provided\n",
            getName().c_str());
    }

    // Other parameters
    UnitAlgebra clock = params.find<UnitAlgebra>("clock", "1GHz");
    numArrays = params.find<uint64_t>("numArrays", 1);
    inputOperandSize = params.find<uint64_t>("inputOperandSize", 1);
    outputOperandSize = params.find<uint64_t>("outputOperandSize", 1);

    // Set the sizes of the array interface vectors
    arrayIns.resize(numArrays);
    arrayOuts.resize(numArrays);
    for(int i = 0; i < numArrays; i++) {
        arrayIns[i].resize(arrayInputSize);
        arrayOuts[i].resize(arrayOutputSize);
    }
    arrayStates.resize(numArrays);

    // Set the address delimiters
    inputDataSize = inputOperandSize * arrayInputSize;
    inputTotalSize = inputDataSize * numArrays;
    outputDataSize = outputOperandSize * arrayOutputSize;
    outputTotalSize = outputDataSize * numArrays;
    inputStartAddr = mmioStartAddr + numArrays;
    outputStartAddr = inputStartAddr + inputTotalSize;

    // TimeConverter for subcompnents, MMIOTile is not currently clocked
    clockTC = getTimeConverter(clock);

    //Load the subcomponents, memory (StandardMem) and array (ComputeArray)
    memory = loadUserSubComponent<StandardMem>("memory", ComponentInfo::SHARE_NONE, clockTC, new StandardMem::Handler<MMIOTile>(this, &MMIOTile::handleMemEvent));
    if ( !memory ) {
        out.fatal(CALL_INFO, -1, "Unable to load standardInterface subcomponent.\n");
    }

    array = loadUserSubComponent<Golem::ComputeArray>("array", ComponentInfo::SHARE_NONE, clockTC, new SST::Event::Handler<MMIOTile>(this, &MMIOTile::handleArrayEvent), &arrayIns, &arrayOuts, &matrices);
    if ( !array ) {
        out.fatal(CALL_INFO, -1, "Unable to load array model subcomponent.\n");
    }

    // Set up memory handlers and mmio region
    memHandlers = new StandardMemHandlers(this, &out);
    uint64_t totalMMIOSize = numArrays + inputTotalSize + outputTotalSize;
    memory->setMemoryMappedAddressRegion(mmioStartAddr, totalMMIOSize);
}

void MMIOTile::init(unsigned int phase) {
    memory->init(phase);
    array->init(phase);

    for (int i = 0; i < numArrays; i++) {
        arrayStates[i] = 0;
        for (int j = 0; j < arrayInputSize; j++){
            arrayIns[i][j] = 0;
        }
        for (int j = 0; j < arrayOutputSize; j++){
            arrayOuts[i][j] = 0;
        }
    }
}

void MMIOTile::setup() {
    memory->setup();
    array->setup();
}

void MMIOTile::finish() {
    memory->finish();
    array->finish();
}

void MMIOTile::emergencyShutdown() {
    //Standard interface has no emergencyShutdown
    if (array) {
        array->emergencyShutdown();
    }
}

void MMIOTile::handleMemEvent(StandardMem::Request* req) {
    req->handle(memHandlers);
    delete req;
}

void MMIOTile::handleArrayEvent(Event * ev) {
    ArrayEvent* aev = static_cast<ArrayEvent*>(ev);
    uint32_t arrayID = aev->getArrayID();

    out.verbose(CALL_INFO, 1, 0, "%s: Array %d completed array operation\n", getName().c_str(), arrayID);

    arrayStates[arrayID] = 0;
}

void MMIOTile::StandardMemHandlers::handle(StandardMem::Read* read) {
    uint64_t baseAddr = read->pAddr;
    uint64_t size = read->size;
    out->verbose(CALL_INFO, 1, 0, "%s: Recieved Read. Addr: 0x%" PRIx64 ", size: %ld \n", tile->getName().c_str(), baseAddr, size);


    std::vector<uint8_t> responseVec(size);
    
    //iterate over the requested address range to build the responseVec
    uint64_t cursor = 0;
    uint64_t addr = baseAddr;

    //This doesn't support accesses which aren't properly aligned to the input or output data size
    while (cursor < size) {

        // Read a tile state
        if (addr < tile->inputStartAddr) {
            uint64_t arrayID = addr - tile->mmioStartAddr;
            responseVec[cursor] = tile->arrayStates[arrayID];

            //status locations are 1B wide
            cursor += 1;
            addr += 1;
        }
        // Read from input registers
        else if (addr < tile->outputStartAddr) {
            uint64_t offset = (addr - tile->inputStartAddr) / tile->inputOperandSize;
            uint64_t arrayID = offset / tile->inputDataSize;
            uint64_t index = offset % tile->inputDataSize;

            // Read correct value, convert to uint8 array, and copy to response vec
            uint64_t val = tile->arrayIns[arrayID][index];
            std::vector<uint8_t> data(tile->inputOperandSize);
            intToData(val, &data, tile->inputOperandSize);
            for (int i = 0; i < tile->inputOperandSize; i++) {
                responseVec[cursor + i] = data[i];
            }
            out->verbose(CALL_INFO, 2, 0, "%s: Read Input (%ld, %ld), Val: %ld \n", tile->getName().c_str(), arrayID, index, val);


            cursor += tile->inputDataSize;
            addr += tile->inputDataSize;
        }
        // Read from output registers
        else {
            uint64_t offset = (addr - tile->outputStartAddr) / tile->outputOperandSize;
            uint64_t arrayID = offset / tile->outputDataSize;
            uint64_t index = offset % tile->outputDataSize;

            // Read correct value, convert to uint8 array, and copy to response vec
            uint64_t val = tile->arrayOuts[arrayID][index];
            std::vector<uint8_t> data(tile->outputOperandSize);
            intToData(val, &data, tile->outputOperandSize);
            for (int i = 0; i < tile->outputOperandSize; i++) {
                responseVec[cursor + i] = data[i];
            }

            out->verbose(CALL_INFO, 2, 0, "%s: Read Output (%ld, %ld), Val: %ld \n", tile->getName().c_str(), arrayID, index, val);

            cursor += tile->outputDataSize;
            addr += tile->outputDataSize;
        }
    }

    StandardMem::ReadResp* resp = static_cast<StandardMem::ReadResp*>(read->makeResponse());
    resp->data = responseVec;
    tile->memory->send(resp);
}

void MMIOTile::StandardMemHandlers::handle(StandardMem::Write* write) {
    uint64_t baseAddr = write->pAddr;
    uint64_t size = write->size;
    out->verbose(CALL_INFO, 1, 0, "%s: Recieved Write. Addr: 0x%" PRIx64 ", size: %ld \n", tile->getName().c_str(), baseAddr, size);


    uint64_t cursor = 0;
    uint64_t addr = baseAddr;

    while (cursor < size) {
        // Start up array computations
        if (addr < tile->inputStartAddr) {
            uint64_t arrayID = addr - tile->mmioStartAddr;
            uint8_t data = write->data[cursor];

            // 4 possibilities:
            // Data = 0, arrayState = 0:
            //  Why? Drop and Warn
            // Data = 0, arrayState != 0:
            //  Already computing...Drop and Warn
            // Data != 0, arrayState = 0:
            //  Intended! Start the computation, store 1
            // Data != 0, arrayState != 0:
            //  Already Computing...Drop and Warn
            //If the array is not already computing
            if (!tile->arrayStates[arrayID] && data) {
                tile->arrayStates[arrayID] = 1;

                out->verbose(CALL_INFO, 1, 0, "%s: Starting Array Computation %ld \n", tile->getName().c_str(), arrayID);

                tile->array->beginComputation(arrayID);
            }
            else {
                out->verbose(CALL_INFO, 0, 0, "%s: Array %ld Recieved Invalid Status Write. Write Ignored! Data: %d, Current State: %d \n", tile->getName().c_str(), arrayID, data, tile->arrayStates[arrayID]);

            }
        }
        // Load input buffers
        else if (addr < tile->outputStartAddr) {
            uint64_t offset = (addr - tile->inputStartAddr) / tile->inputOperandSize;
            uint64_t arrayID = offset / tile->inputDataSize;
            uint64_t index = offset % tile->inputDataSize;

            std::vector<uint8_t> data(tile->inputOperandSize);
            for (int i = 0; i < tile->inputOperandSize; i++) {
                data[i] = write->data[cursor + i];
            }
            tile->arrayIns[arrayID][index] = dataToInt(&data);
            out->verbose(CALL_INFO, 2, 0, "%s: Write Input (%ld, %ld), Val: %ld \n", tile->getName().c_str(), arrayID, index, tile->arrayIns[arrayID][index]);

            cursor += tile->inputDataSize;
            addr += tile->inputDataSize;
        }
        // Can't write outputs
        else {
            out->verbose(CALL_INFO, 0, 0, "%s: Recieved Write to Output Register. Addr: 0x%" PRIx64 " Write Ignored!\n", tile->getName().c_str(), addr);
            //We're going to ignore all of these, so just break here
            break;
        }
    }

    if (!(write->posted)) {
        tile->memory->send(write->makeResponse());
    }

}
