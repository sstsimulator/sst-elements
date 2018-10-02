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

/* 
 * File:   cacheFactory.cc
 * Author: Branden Moore / Si Hammond
 */


#ifndef _H_MEMHIERARCHY_CACHE_LISTENER
#define _H_MEMHIERARCHY_CACHE_LISTENER

#include <sst/core/simulation.h>
#include <sst/core/element.h>
#include <sst/core/event.h>
#include <sst/core/subcomponent.h>
#include <sst/core/warnmacros.h>

#include "sst/elements/memHierarchy/memEvent.h"

using namespace SST;

namespace SST {
class Output;

namespace MemHierarchy {

    enum NotifyAccessType{ READ, WRITE, EVICT };
    enum NotifyResultType{ HIT, MISS, NA };

class CacheListenerNotification {
public:
    CacheListenerNotification(const Addr tAddr, const Addr pAddr, const Addr vAddr,
                              const Addr iPtr, const uint32_t reqSize,
                              NotifyAccessType accessT,
                              NotifyResultType resultT) :
        size(reqSize), targAddr(tAddr), physAddr(pAddr), virtAddr(vAddr), instPtr(iPtr),
        access(accessT), result(resultT) {}

    /** the target address is the underlying address from the
        LOAD/STORE, not the baseAddr (which is usually he cache line
        address). For an evict they are the same. */
        Addr getTargetAddress() const {return targAddr;}
	Addr getPhysicalAddress() const { return physAddr; }
	Addr getVirtualAddress() const { return virtAddr; }
	Addr getInstructionPointer() const { return instPtr; }
	NotifyAccessType getAccessType() const { return access; }
	NotifyResultType getResultType() const { return result; }
	uint32_t getSize() const { return size; }
private:
	uint32_t size;
        Addr targAddr;
	Addr physAddr;
	Addr virtAddr;
	Addr instPtr;
	NotifyAccessType access;
	NotifyResultType result;
};

class CacheListener : public SubComponent {
public:

    CacheListener(Component* owner, Params& UNUSED(params)) : SubComponent(owner) {}
    virtual ~CacheListener() {}

    virtual void printStats(Output &UNUSED(out)) {}
    virtual void notifyAccess(const CacheListenerNotification& UNUSED(notify)) {}
    virtual void registerResponseCallback(Event::HandlerBase *handler) { delete handler; }
};

}}

#endif

