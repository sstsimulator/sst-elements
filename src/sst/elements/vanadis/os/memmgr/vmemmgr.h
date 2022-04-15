// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _H_VANADIS_MEMORY_MANAGER
#define _H_VANADIS_MEMORY_MANAGER

#include <cassert>
#include <cinttypes>
#include <cstdint>
#include <unordered_set>

namespace SST {
namespace Vanadis {

class VanadisMemoryManager {
public:
    VanadisMemoryManager(int verbosity, uint64_t start, uint64_t end, uint64_t pg_size)
        : region_start(start), region_end(end), region_page_size(pg_size) {

        assert(region_page_size > 0);
        assert((region_start % region_page_size) == 0);
        assert((region_end % region_page_size) == 0);
        assert(region_start < region_end);

        output = new SST::Output("[os-memgmr] ", verbosity, 0, Output::STDOUT);

        for (uint64_t page_start = region_start; page_start < region_end; page_start += region_page_size) {
            free_pages.insert(page_start);
        }

        region_pages_in_use_count = 0;
    }

    ~VanadisMemoryManager() {
        free_pages.clear();
        delete output;
    }

    int deallocateRange(uint64_t allocation_address, uint64_t dealloc_len) {
        output->verbose(CALL_INFO, 16, 0, "requested deallocation from address 0x%llx for %" PRIu64 " bytes.\n",
                        allocation_address, dealloc_len);

        uint64_t allocation_address_page_start = (allocation_address % region_page_size) == 0
                                                     ? allocation_address
                                                     : allocation_address - (allocation_address % region_page_size);

        output->verbose(CALL_INFO, 16, 0, "aligned allocation-address (0x%llx) to page boundary at 0x%llx\n",
                        allocation_address, allocation_address_page_start);

        if (allocation_address_page_start < region_start) {
            output->verbose(CALL_INFO, 16, 0,
                            "attempting to deallocate memory starting at 0x%llx but this is "
                            "lower than heap region at 0x%llx, returns deallocation error.\n",
                            allocation_address_page_start, region_start);
            // Some kind of weird unmap of a non-mapped page occurred, uh-oh.
            return 1;
        }

        for (uint64_t free_len = 0; free_len < dealloc_len; free_len += region_page_size) {
            if ((allocation_address_page_start + (free_len * region_page_size)) < region_end) {
                // add the page being freed back into the free page pool
                // we can then allocate this page for requests at a later date
                free_pages.insert(allocation_address_page_start + (free_len * region_page_size));
            }
        }

        // Successful deallocation occured
        return 0;
    }

    int allocateRange(uint64_t allocation_size, uint64_t* allocation_address) {
        output->verbose(CALL_INFO, 16, 0, "requested allocation size: %" PRIu64 "\n", allocation_size);

        bool found_allocation = false;
        uint64_t allocation_start = 0;

        // if we evenly divide page size, then allocate only what we need
        // or else we need to round up
        const uint64_t allocation_page_count = ((allocation_size % region_page_size) > 0)
                                                   ? (allocation_size / region_page_size) + 1
                                                   : (allocation_size / region_page_size);

        output->verbose(CALL_INFO, 16, 0, "-> page-size:            %" PRIu64 "\n", region_page_size);
        output->verbose(CALL_INFO, 16, 0, "-> requested page count: %" PRIu64 "\n", allocation_page_count);
        output->verbose(CALL_INFO, 16, 0, "-> free-pages:           %" PRIu64 "\n", (uint64_t)free_pages.size());
        output->verbose(CALL_INFO, 16, 0, "-> used-pages:           %" PRIu64 "\n", region_pages_in_use_count);

        for (uint64_t page_start = region_start; page_start < (region_end - allocation_page_count);
             page_start += region_page_size) {

            bool is_contiguous = true;

            for (uint64_t i = 0; i < allocation_page_count; ++i) {
                // if the page is found in the set of free pages we can use it
                if (free_pages.find(page_start + (i * region_page_size)) == free_pages.end()) {
                    is_contiguous = false;
                    break;
                }
            }

            if (is_contiguous) {
                found_allocation = true;
                allocation_start = page_start;
                break;
            }
        }

        if (found_allocation) {
            output->verbose(CALL_INFO, 8, 0,
                            "alloc matched: size: %" PRIu64 " / page-count: %" PRIu64 " / alloc-start: %" PRIu64 "\n",
                            allocation_size, allocation_page_count, allocation_start);

            for (uint64_t i = 0; i < allocation_page_count; ++i) {
                free_pages.erase(allocation_start + (i * region_page_size));
            }

            region_pages_in_use_count += allocation_page_count;
            (*allocation_address) = allocation_start;

            return 0;
        } else {
            output->verbose(CALL_INFO, 8, 0,
                            "alloc match-failed for size: %" PRIu64 " / page-count: %" PRIu64
                            " / pages-in-use: %" PRIu64 "\n",
                            allocation_size, allocation_page_count, region_pages_in_use_count);
            return 1;
        }
    }

protected:
    SST::Output* output;
    std::unordered_set<uint64_t> free_pages;
    uint64_t region_pages_in_use_count;
    const uint64_t region_start;
    const uint64_t region_end;
    const uint64_t region_page_size;
};

} // namespace Vanadis
} // namespace SST

#endif
