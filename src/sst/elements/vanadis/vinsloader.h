
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

	void setMemoryInterface( SST::Interfaces::SimpleMem* new_if ) {
		mem_if = new_if;
	}

	bool acceptResponse( SST::Interfaces::SimpleMem::Request* req ) {
		// Looks like we created this request, so we should accept and process it
		if( pending_loads.find( req->id ) != pending_loads.end() ) {
			std::vector<uint8_t>* new_line = new std::vector<uint8_t>( cache_line_width );

			for( uint64_t i = 0; i < cache_line_width; ++i ) {
				new_line->push_back( req->data[i] );
			}

			predecode_cache->store( req->id, new_line );

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

		if( predecode_cache->contains( cache_line_start ) ) {
			if( (addr + buffer_req) > (cache_line_start + cache_line_width) ) {
				if( predecode_cache->contains( cache_line_start + cache_line_width ) ) {
					// Needs two cache lines
					std::vector<uint8_t>* cached_bytes = predecode_cache->find( cache_line_start );

					for( uint64_t i = 0; i < (cache_line_width - inst_line_offset); ++i ) {
						buffer[i] = cached_bytes->at( inst_line_offset + i );
					}

					cached_bytes = predecode_cache->find( cache_line_start + cache_line_width );

					for( uint64_t i = 0; i < (buffer_req - (cache_line_width - inst_line_offset)); ++i ) {
						buffer[(cache_line_width - inst_line_offset) + i] = cached_bytes->at(i);
					}
				} else {
					filled = false;
				}
			} else {
				std::vector<uint8_t>* cached_bytes = predecode_cache->find( cache_line_start );

				for( uint64_t i = 0; i < buffer_req; ++i ) {
					buffer[i] = cached_bytes->at( inst_line_offset + i );
				}
			}
		} else {
			filled = false;
		}

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

		uint64_t line_start = addr - (addr % cache_line_width);

		do {
			output->verbose(CALL_INFO, 8, 0, "(ins-loader) ---> issue ins-load line-start: %" PRIu64 ", len=%" PRIu64 " \n",
				line_start, cache_line_width);

			SST::Interfaces::SimpleMem::Request* req_line = new SST::Interfaces::SimpleMem::Request(
                                        SST::Interfaces::SimpleMem::Request::Read,
                                        line_start, cache_line_width );

			mem_if->sendRequest( req_line );

			pending_loads.insert( std::pair<SST::Interfaces::SimpleMem::Request::id_t,
                                        SST::Interfaces::SimpleMem::Request*> (
                                        req_line->id, req_line) );

			line_start += cache_line_width;

		} while( line_start < (addr + len) );
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
