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

#define SC_INCLUDE_DYNAMIC_PROCESSES
//#include <sst_config.h>
//#include <sst/elements/memHierarchy/memEvent.h>
//#include <sst/elements/SysC/TLM/memEventAdapterFunctions.h>
//#include <sst/elements/SysC/TLM/adapter.h>

#include <sst/elements/SysC/common/TLMgenerator.h>

#include <systemc.h>
#include "tlm.h"
#include "tlm_utils/simple_initiator_socket.h"

#include "HMCSim.h"
#include <fstream>

#include <vector>
#include <iostream>
#include <string>
using std::cout;
using std::endl;
typedef Micron::Internal::HMC::HMCWrapper                 HMCWrapper_t;
typedef Micron::Internal::HMC::HMCController              HMCController_t;
typedef tlm_utils::simple_target_socket<HMCController_t>  TargetSocket_t;
typedef std::vector<TargetSocket_t*>                      TargetSocketVector_t;
typedef Micron::Internal::HMC::Config                     Config_t;

//using namespace SST;
//using namespace SysC;
using namespace Micron::Internal::HMC;


void usage(){
  // Prints each argument on the command line.
  cout<<"== Usage : Tester.o <Config File>"<<endl;
}
int sc_main(int argc, char* argv[])
{
  if(argc<2)
    {
      usage();
    }
  std::string configFileName = std::string(argv[1]);
  Config_t* cfg = GetConfig(configFileName);
  std::ofstream log_file;
  log_file.open("infogather.cdf");
  HMCWrapper_t *hmc = Micron::Internal::HMC::getHMCWrapper(*cfg,log_file);
  TargetSocketVector_t sockets=hmc->GetControllerSockets();
  cout<<"Number of sockets: "<<sockets.size()<<endl;
  TLMGenerator *generator=new TLMGenerator("Generator");
  generator->socket.bind(*sockets.front());

  sc_start(100000,SC_NS);
  //top.printData();
  return 0;
}
