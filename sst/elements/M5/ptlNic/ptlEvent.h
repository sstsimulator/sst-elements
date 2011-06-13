#ifndef _ptlEventInternal_h
#define _ptlEventInternal_h

#include "portals4_types.h"

struct PtlEventInternal  {
    ptl_size_t  count1;
    ptl_event_t event;
    ptl_size_t  count2;
};

#endif
