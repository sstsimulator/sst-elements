
#ifndef _H_VANADIS_MEMORY_MANAGER
#define _H_VANADIS_MEMORY_MANAGER

#include <cassert>
#include <cstdint>
#include <cinttypes>
#include <unordered_set>

namespace SST {
namespace Vanadis {

class VanadisMemoryManager {
public:
	VanadisMemoryManager( int verbosity, uint64_t start, uint64_t end, uint64_t pg_size ) :
		region_start(start), region_end(end), region_page_size(pg_size) {

		assert( region_page_size > 0 );
		assert( (region_start % region_page_size) == 0 );
		assert( (region_end   % region_page_size) == 0 );
		assert( region_start < region_end );

		output = new SST::Output("[os-memgmr] ", verbosity, 0, Output::STDOUT);

		for( uint64_t page_start = region_start; page_start < region_page_size; page_start += region_page_size ) {
			free_pages.insert( page_start );
		}
	}

	~VanadisMemoryManager() {
		free_pages.clear();
		delete output;
	}

	int allocateRange( uint64_t allocation_size, uint64_t* allocation_address ) {
		bool found_allocation = false;
		uint64_t allocation_start = 0;

		// if we evenly divide page size, then allocate only what we need
		// or else we need to round up
		const uint64_t allocation_page_count = ((allocation_size % region_page_size) > 0) ?
			(allocation_size / region_page_size) + 1 :
			(allocation_size / region_page_size);

		for( uint64_t page_start = region_start;
			page_start < (region_end - allocation_page_count);
			++page_start ) {

			bool is_contiguous = true;

			for( uint64_t i = 0; i < allocation_page_count; ++i ) {
				// if the page is found in the set of free pages we can use it
				if( free_pages.find( page_start + (i * region_page_size)) != free_pages.end() ) {
					is_contiguous = false;
					break;
				}
			}

			if( is_contiguous ) {
				found_allocation = true;
				allocation_start = page_start;
				break;
			}
		}

		if( found_allocation ) {
			output->verbose(CALL_INFO, 8, 0, "alloc matched: size: %" PRIu64 " / page-count: %" PRIu64 " / alloc-start: %" PRIu64 "\n",
				allocation_size, allocation_page_count, allocation_start);

			for( uint64_t i = 0; i < allocation_page_count; ++i ) {
				free_pages.erase( allocation_start + (i * region_page_size) );
			}

			region_pages_in_use_count += allocation_page_count;
			(*allocation_address) = allocation_start;

			return 0;
		} else {
			output->verbose(CALL_INFO, 8, 0, "alloc match-failed for size: %" PRIu64 " / page-count: %" PRIu64 " / pages-in-use: %" PRIu64 "\n",
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

}
}

#endif
