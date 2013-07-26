#include <sst_config.h>
#include <stdio.h>
#include <stdlib.h>
#include "sst/core/serialization.h"
#include "cpu.h"
#include <stdlib.h>

static unsigned int missRate[][3] = {{0,51,32},   //app 0
				     {0,18,15}};  //app 1
static unsigned int isLoad[] = {3,32}; // out of 64

cpu::memChan_t::event_t *cpu::getInst(int cacheLevel, int app, int core) {
  /*
app: 		MD(0)	PHD(1)
l/s ratio	21:1	1:1
L1 miss/1Kinst  51	18
L2 miss/inst	32	15
  */
  //printf("using missrate %d\n", missRate[app][cacheLevel]);

  unsigned int roll1K = random() & 0x3ff;
  if (roll1K <= missRate[app][cacheLevel]) {
    //is a memory access
    unsigned int roll = random();
    unsigned int memRoll = roll & 0x3f;

    memChan_t::event_t *event = new memChan_t::event_t;
    if ((memRoll & 0x1) == 0) {
      // stride
      event->addr = coreAddr[core] + (1 << 6);  
      event->addr = (event->addr >> 6) << 6;    
    } else {
      //random
      event->addr = (roll >> 6) << 6;    
    }
    coreAddr[core] = event->addr;
    if (memRoll <= isLoad[app]) {
      event->reqType = memChan_t::event_t::WRITE; 
    } else {
      event->reqType = memChan_t::event_t::READ; 
    }
    event->msgType = memChan_t::event_t::REQUEST; 

    return event;
  } else {
    return 0;
  }
}
