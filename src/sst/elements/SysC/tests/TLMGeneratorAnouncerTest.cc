// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2016, Sandia Corporation
// All rights reserved.
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
