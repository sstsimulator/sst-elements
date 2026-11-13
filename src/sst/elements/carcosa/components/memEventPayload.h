// Copyright 2009-2026 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.

#ifndef SST_ELEMENTS_CARCOSA_MEM_EVENT_PAYLOAD_H
#define SST_ELEMENTS_CARCOSA_MEM_EVENT_PAYLOAD_H

#include "sst/elements/memHierarchy/memEvent.h"

#include <cstdint>

namespace SST::Carcosa {

/**
 * Address of payload byte zero on memory-facing links. Cacheable read/fetch
 * responses carry a complete line, while addr and vAddr retain the original
 * request's byte offset. Noncacheable responses and writes start at addr.
 * This helper does not inspect or materialize the event payload.
 */
inline uint64_t memEventPayloadAddress(MemHierarchy::MemEvent& event,
                                       bool prefer_virtual = true) {
    using MemHierarchy::Command;
    const Command command = event.getCmd();
    const bool line_response =
        !event.queryFlag(MemHierarchy::MemEvent::F_NONCACHEABLE) &&
        (command == Command::GetSResp || command == Command::GetXResp ||
         command == Command::FetchResp || command == Command::FetchXResp);
    const uint64_t addr = event.getAddr();
    const uint64_t vaddr = event.getVirtualAddress();
    if (prefer_virtual && vaddr != 0)
        return line_response ? vaddr - (addr - event.getBaseAddr()) : vaddr;
    return line_response ? event.getBaseAddr() : addr;
}

} // namespace SST::Carcosa

#endif
