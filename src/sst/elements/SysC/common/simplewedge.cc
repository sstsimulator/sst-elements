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

#include <sst_config.h>
#include "sst/elements/SysC/common/simplewedge.h"
#include <sst/core/params.h>
#include <stdio.h>

using namespace SST;

SimpleWedge_base::SimpleWedge_base(ComponentId_t _id, Params& _params)
  : BaseType(_id,_params)
{
  out.verbose(CALL_INFO,1,0,"Constructing SimpleWedge_base\n");
  base_address = 
      _params.find_integer(SW_BASE_ADDRESS,DEF_SW_BASE_ADDRESS);
  out.verbose(CALL_INFO,2,0,
              SW_BASE_ADDRESS " = 0x%lX\n",base_address);
  size_multiplier = 
      _params.find_integer(SW_SIZE_MULTIPLIER,DEF_SW_SIZE_MULTIPLIER);
  out.verbose(CALL_INFO,2,0,
              SW_SIZE_MULTIPLIER " = %d\n",size_multiplier);
  size_exponent = 
      _params.find_integer(SW_SIZE_EXP,DEF_SW_SIZE_EXP);
  out.verbose(CALL_INFO,2,0,
              SW_SIZE_EXP " = %d\n",size_exponent);
  size=(uint64_t(2)<<size_exponent)*uint64_t(size_multiplier);
  out.verbose(CALL_INFO,2,0,
              "Store Size = %d*2^%d = %ld bytes\n",
              size_multiplier,size_exponent,size
              );
  out.verbose(CALL_INFO,2,0,
              "Valid address range = [%lX - %lX]\n",
              base_address,base_address+size
              );
  data_store=NULL;
}

void SimpleWedge_base::setup(){
  data_store = (unsigned char *)malloc(sizeof(unsigned char)*size);
  if(!data_store)
    out.fatal(CALL_INFO,-1,
              "SimpleWedge: Could not malloc data_store");
  BaseType::setup();
}
void SimpleWedge_base::finish(){
  BaseType::finish();
  free (data_store);
}

void SimpleWedge_base::readFromFile(std::string _filename){
  if(""==_filename){
  out.output("SimpleWedge::readFromFile() - no file to read");
  }
  else{
    FILE *input = fopen(_filename.c_str(),"rb");
    if(!input)
      out.fatal(CALL_INFO,-2,
                "SimpleWedge: Could not open source file");
    assert(size == fread(data_store,sizeof(unsigned char),size,input));
    fclose(input);
  }
}

void SimpleWedge_base::writeToFile(std::string _filename){
  if(""==_filename){
  out.output("SimpleWedge::writeToFile() - no file to write");
  }
  else{
    FILE *output = fopen(_filename.c_str(),"wb");
    if(!output)
      out.fatal(CALL_INFO,-3,
                "SimpleWedge: Could not open destination file");
    fwrite(data_store,sizeof(unsigned char),size,output);
    fclose(output);
  }
}
