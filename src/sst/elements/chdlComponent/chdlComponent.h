// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2016, Sandia Corporation
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTITORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef CHDL_COMPONENT_H
#define CHDL_COMPONENT_H

#include <string>
#include <map>
#include <queue>
#include <vector>

#include <chdl/chdl.h>
#include <chdl/loader.h>

#include <sst/core/sst_types.h>
#include <assert.h>

#include <sst/core/component.h>
#include <sst/core/element.h>
#include <sst/core/event.h>
#include <sst/core/link.h>
#include <sst/core/output.h>
#include <sst/core/timeConverter.h>

#include <sst/core/interfaces/simpleMem.h>

namespace SST {
class Params;

namespace ChdlComponent {
  struct reqdata {
    std::vector<uint64_t> data;
    uint64_t mask, addr, id;
    bool ready, valid, wr, llsc;

  };

  struct respdata {
    uint64_t id;
    std::vector<uint64_t> data;
    bool ready, valid, wr, llsc, llsc_suc;

  };

  class chdlComponent : public Component {
  public:
    chdlComponent(ComponentId_t id, Params &p);
    chdlComponent();

    void setup();
    void init(unsigned phase);
    void finish();


  private:
    void consoleOutput(char c);

    void handleEvent(Interfaces::SimpleMem::Request *);
    bool clockTick(Cycle_t);

    uint32_t mmio_rd(uint64_t addr);
    void mmio_wr(uint32_t data, uint64_t addr);
    void mmio_rd(std::vector<uint8_t> &d, uint64_t addr);
    void mmio_wr(const std::vector<uint8_t> &d, uint64_t addr);

    Interfaces::SimpleMem *memLink;

    std::string clockFreq, netlFile, memFile, outputBuffer;

    std::vector<reqdata> req, req_copy;
    std::vector<respdata> resp;

    std::map<std::string, unsigned> ports;
    std::vector<std::queue<Interfaces::SimpleMem::Request *> > resp_q;
    std::vector<unsigned> responses_this_cycle, byte_sz, data_sz;
    std::map<unsigned long, unsigned long> idMap, portMap;
    std::map<std::string, unsigned long *> counters;
    
    chdl::cdomain_handle_t cd;

    int debugLevel, tog, core_id, core_count;

    Output out;
    std::ofstream vcd;
    bool dumpVcd, stopSim, registered;
};

}}

#endif
