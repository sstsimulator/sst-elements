#ifndef _cmdQueue_h
#define _cmdQueue_h

#include "cmdQueueEntry.h"

namespace SST {
namespace Portals4 {

#define CMD_QUEUE_SIZE 16
typedef struct {
    volatile int head; 
    volatile int tail;
    cmdQueueEntry_t  queue[CMD_QUEUE_SIZE];
} cmdQueue_t ;    

#define ME_UNLINKED_SIZE 64 

}
}

#endif
