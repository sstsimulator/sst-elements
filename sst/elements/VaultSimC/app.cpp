#include <sst_config.h>
#include "cpu.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>

#include "sst/core/serialization.h"

static unsigned int missRate[][3] = {{0,51,32},   //app 0
				     {0,18,15}};  //app 1
static unsigned int isLoad[] = {3,32}; // out of 64

using namespace SST::Interfaces;

MemEvent *cpu::getInst(int cacheLevel, int app, int core) {
  /*
app: 		MD(0)	PHD(1)
l/s ratio	21:1	1:1
L1 miss/1Kinst  51	18
L2 miss/inst	32	15
  */
  //printf("using missrate %d\n", missRate[app][cacheLevel]);

  unsigned int roll1K = rng->generateNextUInt32() & 0x3ff;
  if (roll1K <= missRate[app][cacheLevel]) {
    //is a memory access
    unsigned int roll = rng->generateNextUInt32();
    unsigned int memRoll = roll & 0x3f;

    Addr addr;
    Command command;
    if ((memRoll & 0x1) == 0) {
      // stride
      addr = coreAddr[core] + (1 << 6);  
      addr = (addr >> 6) << 6;    
    } else {
      //random
      addr = (roll >> 6) << 6;    
    }
    coreAddr[core] = addr;
    if (memRoll <= isLoad[app]) {
      command = SST::Interfaces::ReadReq;
    } else {
      command = SST::Interfaces::WriteReq;
    }
    MemEvent *event = new MemEvent(this, addr, command);

    return event;
  } else {
    return 0;
  }
}
