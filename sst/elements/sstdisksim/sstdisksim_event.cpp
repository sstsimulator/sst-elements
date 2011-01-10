#include "sstdisksim_event.h"

/******************************************************************************/
void 
opFinishedCallback()
{
}

/******************************************************************************/
sstdisksim_event::sstdisksim_event()
  :SST::Event()
{
  finishedCall = &opFinishedCallback;
}
