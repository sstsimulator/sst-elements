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

#include "sst/elements/SysC/common/TLMgenerator.h"
#include "sst/elements/SysC/common/TLManouncer.h"

struct Top: public sc_module{
  TLMGenerator* generator;
  TLMAnnouncer*  anouncer;

  SC_CTOR(Top){
      generator=new TLMGenerator("generator");
      anouncer=new TLMAnnouncer("anouncer");
      generator->socket.bind(anouncer->socket);
  }
  
  void printData(){
  generator->printData();
  anouncer->printData();
  }

};

int sc_main(int argc, char* argv[])
{
  Top top("top");
  sc_start();
  top.printData();
  return 0;
}
