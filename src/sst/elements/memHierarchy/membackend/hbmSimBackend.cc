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
#include "sst/elements/memHierarchy/util.h"
#include "membackend/hbmSimBackend.h"

using namespace SST;
using namespace SST::MemHierarchy;

HBMSimMemory::HBMSimMemory(Component *comp, Params &params)
    : SimpleMemBackend(comp, params) {
  std::string deviceIniFilename =
      params.find<std::string>("device_ini", NO_STRING_DEFINED);
  if (NO_STRING_DEFINED == deviceIniFilename)
    output->fatal(CALL_INFO, -1,
                  "Model must define a 'device_ini' file parameter\n");
  std::string systemIniFilename =
      params.find<std::string>("system_ini", NO_STRING_DEFINED);
  if (NO_STRING_DEFINED == systemIniFilename)
    output->fatal(CALL_INFO, -1,
                  "Model must define a 'system_ini' file parameter\n");

  UnitAlgebra ramSize = UnitAlgebra(params.find<std::string>("mem_size", "0B"));
  if (ramSize.getRoundedValue() % (1024 * 1024) != 0) {
    output->fatal(CALL_INFO, -1,
                  "For HBMSim, backend.mem_size must be a multiple of 1MiB. "
                  "Note: for units in base-10 use 'MB', for base-2 use 'MiB'. "
                  "You specified '%s'\n",
                  ramSize.toString().c_str());
  }
  unsigned int ramSizeMiB = ramSize.getRoundedValue() / (1024 * 1024);

  memSystem = HBMSim::getMemorySystemInstance(
      deviceIniFilename, systemIniFilename, "", ramSizeMiB);

  HBMSim::Callback<HBMSimMemory, void, unsigned int, uint64_t, uint64_t>
      *readDataCB, *writeDataCB;

  readDataCB =
      new HBMSim::Callback<HBMSimMemory, void, unsigned int, uint64_t,
                            uint64_t>(this, &HBMSimMemory::dramSimDone);
  writeDataCB =
      new HBMSim::Callback<HBMSimMemory, void, unsigned int, uint64_t,
                            uint64_t>(this, &HBMSimMemory::dramSimDone);

  memSystem->RegisterCallbacks(readDataCB, writeDataCB);
}

bool HBMSimMemory::issueRequest(ReqId id, Addr addr, bool isWrite, unsigned) {
  bool ok = memSystem->willAcceptTransaction(addr);
  if (!ok) return false;
  ok = memSystem->addTransaction(isWrite, addr);
  if (!ok) return false;  // This *SHOULD* always be ok
#ifdef __SST_DEBUG_OUTPUT__
  output->debug(_L10_, "Issued transaction for address %" PRIx64 "\n",
                (Addr)addr);
#endif
  dramReqs[addr].push_back(id);
  return true;
}

void HBMSimMemory::clock() { memSystem->update(); }

void HBMSimMemory::finish() { memSystem->printStats(true); }

void HBMSimMemory::dramSimDone(unsigned int id, uint64_t addr,
                               uint64_t clockcycle) {
  std::deque<ReqId> &reqs = dramReqs[addr];
#ifdef __SST_DEBUG_OUTPUT__
  output->debug(_L10_, "Memory Request for %" PRIx64 " Finished [%zu reqs]\n",
                (Addr)addr, reqs.size());
#endif
  if (reqs.size() == 0)
    output->fatal(CALL_INFO, -1,
                  "Error: reqs.size() is 0 at HBMSimMemory done\n");
  ReqId reqId = reqs.front();
  reqs.pop_front();
  if (0 == reqs.size()) dramReqs.erase(addr);

  handleMemResponse(reqId);
}
