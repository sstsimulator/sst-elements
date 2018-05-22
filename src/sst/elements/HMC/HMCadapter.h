// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef HMCADAPTER_H
#define HMCADAPTER_H
#define SC_INCLUDE_DYNAMIC_PROCESSES
#include <sst/core/sst_types.h>
#include <systemc.h>
#include "tlm.h"
#include "tlm_utils/simple_target_socket.h"
#include "HMCSim.h"

#include <sst/core/sst_types.h>
#include <sst/elements/memHierarchy/memEvent.h>
#include <../SysC/TLM/adapter.h>

#include <sst/core/component.h>
#include <sst/core/output.h>

#include <vector>
namespace SST{
namespace SysC{
namespace HMC{
class HMCAdapter
    : public SST::Component
{
 private:
  typedef SST::SysC::HMC::HMCAdapter        ThisType;
  typedef SST::Component              BaseType;
 protected:
  typedef Micron::Internal::HMC::Config         Config_t;
  typedef Micron::Internal::HMC::HMCWrapper     HMCWrapper_t;
  typedef Micron::Internal::HMC::HMCController  HMCController_t;
  typedef tlm_utils::simple_target_socket<HMCController_t> TargetSocket_t;
  typedef std::vector<TargetSocket_t*>          TargetSocketVector_t;
  typedef SST::MemHierarchy::MemEvent           Event_t;
  typedef DownStreamAdapter<Event_t>            Adapter_t;
 public:
  HMCAdapter(SST::ComponentId_t _id,SST::Params& _params);
  void setup(){}
  void finish(){hmc->DumpStats();}
  ~HMCAdapter();
  TargetSocketVector_t    target_sockets;
  Link*                   links[4];
  Adapter_t*              adapters[4];
  HMCWrapper_t            *hmc;
  Output                  out;
  uint32_t                cube_id;
  std::ofstream           *log_file;
};
}//namespace HMC
}//namespace SysC
}//namespace SST
#define STR_EXPAND(tok) #tok
#define STR(tok) STR_EXPAND(tok)

#define HMC_CUBE_ID         "cube_id"          
#define DEF_HMC_CUBE_ID     0
#define HMC_CONFIG_FILE     "config_file"      
#define DEF_HMC_CONFIG_FILE config.def
#define HMC_OUTPUT_FILE     "output_file"
#define DEF_HMC_OUTPUT_FILE powerOutputLog.cdf
#define HMC_IGNORE_INIT     "ignore_init_data" 
#define DEF_HMC_IGNORE_INIT 0
#define HMC_TIME_BASE       "time_base"
#define DEF_HMC_TIME_BASE   1ps
#define HMC_PARAMS                                                            \
{HMC_CUBE_ID,                                                                 \
  "Integer id of this particular cube",                                       \
  STR(DEF_HMC_CUBE_ID)},                                                      \
{HMC_CONFIG_FILE,                                                             \
  "file name to load HMC config from",                                        \
  STR(DEF_HMC_CONFIG_FILE)},                                                  \
{HMC_OUTPUT_FILE,                                                             \
  "file name to save power info to",                                          \
  STR(DEF_HMC_OUTPUT_FILE)},                                                  \
{HMC_IGNORE_INIT,                                                             \
  "Determine if we ignore init data or error on it",                          \
  STR(DEF_HMC_IGNORE_INIT)},                                                  \
{HMC_TIME_BASE,                                                               \
  "The default time base to use",                                             \
  STR(DEF_HMC_TIME_BASE)}
//ADAPTER_BASE_PARAMS_1

#define HMC_LINK_PREFIX "HMCLink"
#define HMC_PORT(x)                                                           \
{HMC_LINK_PREFIX #x,                                                          \
  "Link to HMC link #" #x,                                                    \
  NULL}

#define HMC_PORTS                                                             \
    HMC_PORT(0),                                                              \
HMC_PORT(1),                                                                  \
HMC_PORT(2),                                                                  \
HMC_PORT(3)

#endif //HMCADAPTER_H
