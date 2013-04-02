#ifndef _ptlEventInternal_h
#define _ptlEventInternal_h

#include "portals4.h"

namespace SST {
namespace Portals4 {

struct PtlEventInternal  {
    volatile long  count1;
    ptl_event_t user;
    volatile long  count2;
};

}
}

#endif
