
#ifndef _H_VANADIS_INST_LOADER
#define _H_VANADIS_INST_LOADER

#include <sst/core/subcomponent.h>
#include <sst/core/interfaces/simpleMem.h>

#include <vector>
#include <cinttypes>
#include <cstdint>

#include "vinsbundle.h"
#include "datastruct/vcache.h"

namespace SST {
namespace Vanadis {

class VanadisInstructionLoader {
public:
	VanadisInstructionLoader(
		const size_t uop_cache_size,
		const size_t predecode_cache_entries,
		const uint64_t cachelinewidth ) {

		cache_line_width = cachelinewidth;
		uop_cache = new VanadisCache< uint64_t, VanadisInstructionBundle* >( uop_cache_size );
		predecode_cache = new VanadisCache< uint64_t, std::vector<uint8_t>* >( predecode_cache_entries );

		mem_if = nullptr;
	}

	~VanadisInstructionLoader() {
		delete uop_cache;
		delete predecode_cache;
	}

	uint64_t getCacheLineWidth() {
		return cache_line_width;
	}

	void setCacheLineWidth( const uint64_t new_line_width ) {
		// if the line width is changed then we have to flush our cache
		if( cache_line_width != new_line_width ) {
			predecode_cache->clear();
		}

		cache_line_width = new_line_width;
	}

	void setMemoryInterface( SST::Interfaces::SimpleMem* new_if ) {
		mem_if = new_if;
	}

	bool acceptResponse( SST::Output* output, SST::Interfaces::SimpleMem::Request* req ) {
		// Looks like we created this request, so we should accept and process it
		auto check_hit_local = pending_loads.find( req->id );

		if( check_hit_local != pending_loads.end() ) {
			if( req->data.size() != cache_line_width ) {
				output->fatal(CALL_INFO, -1, "Error: request to the instruction cache was not for a full line, req=%d, line-width=%d\n",
					(int) req->data.size(), (int) cache_line_width);
			}

			std::vector<uint8_t>* new_line = new std::vector<uint8_t>();
			new_line->reserve( cache_line_width );

			for( uint64_t i = 0; i < cache_line_width; ++i ) {
				new_line->push_back( req->data[i] );
			}

			output->verbose(CALL_INFO, 16, 0, "[ins-loader] ---> response has: %" PRIu64 " bytes in payload\n",
				(uint64_t) req->data.size());

			if( output->getVerboseLevel() >= 16 ) {
				char* print_line = new char[2048];
				char* temp_line  = new char[2048];
				print_line[0] = '\0';

				for( uint64_t i = 0; i < cache_line_width; ++i ) {
					for( uint64_t j = 0; j < 2048; ++j ) {
						temp_line[j] = print_line[j];
					}

					snprintf( print_line, 2048, "%s %" PRIu8 "", temp_line, req->data[i] );
				}

				output->verbose(CALL_INFO, 16, 0, "[ins-loader] ---> cache-line: %s\n", print_line);

				delete[] print_line;
				delete[] temp_line;
			}

			output->verbose(CALL_INFO, 16, 0, "[ins-loader] ---> hit (addr=%p), caching line in predecoder.\n",
				(void*) req->addr);

			predecode_cache->store( req->addr, new_line );

			// Remove from pending load stores.
			pending_loads.erase(check_hit_local);

			return true;
		} else {
			return false;
		}
	}

	bool getPredecodeBytes( SST::Output* output, const uint64_t addr,
		uint8_t* buffer, const size_t buffer_req ) {

		if( buffer_req > cache_line_width ) {
			output->fatal(CALL_INFO, -1, "[inst-predecode-bytes-check]: Requested a decoded bytes fill of longer than a cache line, req=%" PRIu64 ", line-width=%" PRIu64 "\n",
				(uint64_t) buffer_req, cache_line_width);
		}

		// calculate offset within the cache line
		const uint64_t inst_line_offset = (addr % cache_line_width);

		// get the start of the cache line containing the request
		const uint64_t cache_line_start = addr - inst_line_offset;

		bool filled = true;

		output->verbose(CALL_INFO, 16, 0, "[fill-decode]: ins-addr: 0x%llx line-offset: %" PRIu64 " line-start=%" PRIu64 " / 0x%llx\n",
			addr, inst_line_offset, cache_line_start, cache_line_start);

		if( predecode_cache->contains( cache_line_start ) ) {
			std::vector<uint8_t>* cached_bytes = predecode_cache->find( cache_line_start );

			for( uint64_t i = 0; i < buffer_req; ++i ) {
				buffer[i] = cached_bytes->at( inst_line_offset + i );
			}
		} else {
			filled = false;
		}

		output->verbose(CALL_INFO, 16, 0, "[fill-decode]: line-filled: %s\n", (filled) ? "yes" : "no");

		return filled;
	}

	void cacheDecodedBundle( VanadisInstructionBundle* bundle ) {
		uop_cache->store( bundle->getInstructionAddress(), bundle );
	}

	void clearCache() {
		uop_cache->clear();
		predecode_cache->clear();
	}

	bool hasBundleAt( const uint64_t addr ) const {
		return uop_cache->contains( addr );
	}

	bool hasPredecodeAt( const uint64_t addr ) const {
		const uint64_t line_start = addr - (addr % (uint64_t) cache_line_width);
		return predecode_cache->contains( line_start );
	}

	VanadisInstructionBundle* getBundleAt( const uint64_t addr ) {
		return uop_cache->find( addr );
	}

	void requestLoadAt( SST::Output* output, const uint64_t addr, const uint64_t len ) {
		if( len > cache_line_width ) {
			output->fatal(CALL_INFO, -1, "Error: requested an instruction load which is longer than a cache line, req=%" PRIu64 ", line=%" PRIu64 "\n",
				len, cache_line_width);
		}

		output->verbose(CALL_INFO, 8, 0, "[ins-loader] ---> processing a requested load ip=%p, icache-line: %" PRIu64 " bytes.\n",
			(void*) addr, cache_line_width);

		const uint64_t line_start_offset = (addr % cache_line_width);
		uint64_t line_start = addr - line_start_offset;

		do {
			output->verbose(CALL_INFO, 8, 0, "[ins-loader] ---> issue ins-load line-start: %p (offset=%" PRIu64 "), line-len: %" PRIu64 " read-len=%" PRIu64 " \n",
				(void*) line_start, line_start_offset, cache_line_width, cache_line_width);

			bool found_pending_load = false;

			for( auto pending_load_itr : pending_loads ) {
				if( pending_load_itr.second->addr == line_start ) {
					found_pending_load = true;
					break;
				}
			}

			if( ! found_pending_load ) {
				SST::Interfaces::SimpleMem::Request* req_line = new SST::Interfaces::SimpleMem::Request(
       	                	 	SST::Interfaces::SimpleMem::Request::Read, line_start, cache_line_width );

				mem_if->sendRequest( req_line );

				pending_loads.insert( std::pair<SST::Interfaces::SimpleMem::Request::id_t,
                        		SST::Interfaces::SimpleMem::Request*> ( req_line->id, req_line) );
			} else {
				output->verbose(CALL_INFO, 8, 0, "[ins-loader] -----> load is already in progress, will not issue another load.\n");
			}

			line_start += cache_line_width;

		} while( line_start < (addr + len) );
	}

	void printStatus( SST::Output* output ) {
		output->verbose(CALL_INFO, 8, 0, "Instruction Loader - Internal State Report:\n");
		output->verbose(CALL_INFO, 8, 0, "--> Cache Line Width:          %" PRIu64 "\n", cache_line_width );
		output->verbose(CALL_INFO, 8, 0, "--> Pending Loads:             %" PRIu32 "\n", (uint32_t) pending_loads.size() );

		for( auto pl_itr : pending_loads ) {
			output->verbose(CALL_INFO, 16, 0, "-----> Address:       %p\n", (void*) pl_itr.second->addr);
		}

		output->verbose(CALL_INFO, 8, 0, "--> uop Cache Entries:         %" PRIu32 " / %" PRIu32 "\n", (uint32_t) uop_cache->size(),
			(uint32_t) uop_cache->capacity());
		output->verbose(CALL_INFO, 8, 0, "--> Predecode Cache Entries:   %" PRIu32 " / %" PRIu32 "\n", (uint32_t) predecode_cache->size(),
			(uint32_t) predecode_cache->capacity());
	}

private:
	uint64_t cache_line_width;
	SST::Interfaces::SimpleMem* mem_if;

	VanadisCache< uint64_t, VanadisInstructionBundle* >* uop_cache;
	VanadisCache< uint64_t, std::vector<uint8_t>* >* predecode_cache;

	std::unordered_map<SST::Interfaces::SimpleMem::Request::id_t,SST::Interfaces::SimpleMem::Request* > pending_loads;
};

}
}

#endif
