
#ifndef _H_VANADIS_INST_LOADER
#define _H_VANADIS_INST_LOADER

#include <unordered_map>
#include <list>

#include <sst/core/subcomponent.h>
#include <sst/core/interfaces/simpleMem.h>

namespace SST {
namespace Vanadis {

class VanadisInstructionBundle {
public:
	VanadisInstructionBundle( const uint64_t addr ) : ins_addr(addr) {
		inst_bundle.reserve(1);
	}

	~VanadisInstructionBundle() {
		inst_bundle.clear();
	}

	uint32_t getInstructionCount() {
		inst_bundle.size();
	}

	void addInstruction( VanadisInstruction& newIns ) {
		inst_bundle.push_back(newIns);
	}

	VanadisInstruction* getInstructionByIndex( const uint32_t index ) {
		return inst_bundle[index]->clone();
	}

	uint64_t getInstructionAddress() const {
		return ins_addr;
	}

private:
	const uint64_t ins_addr;
	std::vector<VanadisInstruction> inst_bundle;
};

class VanadisInstructionLoader {
public:
	SST_ELI_REGISTER_SUBCOMPONENT_API(SST::Vanadis::VanadisInstructionDecoder)

	VanadisInstructionLoader( ComponentId_t& id, Params& params ) :
		SubComponent(id) {
	
		mem_if = nullptr;
		max_uop_cache_entries = params.find<uint32_t>("uop_cache_entries", 128);
		max_predecode_cache_entries = params.find<uint32_t>("predecode_buffer_entries", 64);
		cacheLineWidth = params.find<uint32_t>("cache_line_width", 64);
	}

	void setMemoryInterface( SimpleMem* new_if ) { mem_if = mem_if; }

	bool acceptResponse( SimpleMem::Request* req ) {
		// Looks like we created this request, so we should accept and process it
		if( pending_loads.find( req->id ) != pending_loads.end() ) {
			const uint64_t base_addr = req->addr;
			uint8_t* data_payload    = &req->data[0];

			for( int i = 0; i < req->size; i += 4 ) {
				uint32_t* read_4b = (uint32_t*) data_payload[i];
				cachePredecodedIns( base_addr + i, (*read_4b) );
			}

			return true;
		} else {
			return false;
		}
	}

	bool hasUopCacheForAddress( const uint64_t addr ) {
		return uop_cache.contains( addr );
	}

	bool getPredecodeBytes( SST::Output* output, const uint64_t addr,
		uint8_t* buffer, const size_t buffer_req ) {

		// calculate offset within the cache line
		const uint64_t inst_line_offset = (addr % cacheLineWidth);

		// get the start of the cache line containing the request
		const uint64_t cache_line_start = addr - inst_line_offset;

		// Most likely to NOT have this in our cache, so process the false
		// path first
		if( ! predecode_cache.contains( cache_line_start ) ) {
			// We don't have this instruction in our cache and are
			// going to have to request it

			bool req_pending = false;
			for( auto load_itr = pending_loads.begin(); load_itr != pending_loads.end();
				load_itr++) {

				// Already have a request which is in-flight to the cache so
				// we don't want to issue any more
				if( cache_line_start == load_itr->second->addr ) {
					req_pending = true;
					break;
				}
			}

			if( req_pending ) {
				SimpleMem::Request* req_line = new SimpleMem::Request(SimpleMem::Request::Read,
					cache_line_start, cacheLineWidth));
				pending_loads->insert( std::pair<SimpleMem::Request::id_t, SimpleMem::Request?(
					req_line.id, req_line); 			
			}
			return false;
		} else {
			// We DO have this in our cache, so now let's process it

			// do we only need a single line or do we need two?
			if( (cacheLineWidth - inst_line_offset) < buffer_req ) {
				std::vector<uint8_t>* line_bytes = (*predecode_cache.find( cache_line_start ));

				// Copy over the bytes requested from the line
				for( int i = 0; i < buffer_req; ++i ) {
					buffer[i] = line_bytes[ cache_line_start + i ];
				}

				// FIX UP THE LRU

				return true;
			} else {
				// We don't have enough data in this line and we have to check TWO 
				// cache lines as this is split
			}
		}

	}

	VanadisInstructionBundle* getUopBundleByInstructionAddress( const uint64_t addr,
		const uint64_t& id_seq_start ) {

		VanadisInstructionBundle* bundle = new VanadisInstructionBundle( addr );
		VanadisInstructionBundle* uop_cache_bundle = (*uop_cache.find( addr ));

		// make a copy of the cache instruction and then resequence the ID scheme
		// since ordering of instructions matters for retirement
		for( int i = 0; i < uop_cache_bundle->getInstructionCount(); ++i ) {
			VanadisInstruction* new_ins = uop_cache_bundle[i]->clone();
			new_ins->setID( id_seq_start++ );
			bundle.push_back( new_ins );
		}

		return bundle;
	}

	void cacheDecodedBundle( VanadisInstructionBundle* bundle ) {
		free_entry();

		uop_cache.insert( std::pair< uint64_t, VanadisInstructionBundle*>(
			bundle->getInstructionAddress(), bundle );
		uop_lru_q.push_front( bundle->getInstructionAddress() );
	}

	void clearCache() {
		uop_cache.clear();
		uop_lru_q.clear();

		predecode_cache.clear();
		predecode_lru_q.clear();
	}

	void cachePredecodedIns( const uint64_t addr, const uint32_t ins ) {
		free_predecode_entry();

		predecode_cache->insert( std::pair<uint64_t, uin32_t>( addr, ins ) );
		predecode_lru_q->push_front(addr);
	}

protected:
	void free_uop_entry() {
		if( max_cache_entries >= uop_lru_q.size() ) {
			// Get the last address and clear it out
			const uint64_t lru_uop_entry = uop_lru_q.back();
			uop_lru_q.pop_back();
			uop_cache.erase(lru_uop_entry);
		}
	}

	void free_predecode_entry() {
		if( max_predecode_cache_entries >= predecode_lrq_q.size() ) {
			const uint64_t lru_predecode_entry = predecode_lru_q.back();
			predecode_lru_q.pop_back();
			predecode_cache.erase(lru_predecode_entry);
		}
	}
	
	uint32_t max_uop_cache_entries;
	uint32_t max_predecode_cache_entries;
	uint16_t cacheLineWidth;

	SimpleMem* mem_if;
	std::unordered_map< uint64_t, VanadisInstructionBundle*> > uop_cache;
	std::list<uint64_t> uop_lru_q;

	std::unordered_map< uint64_t, std::vector<uint8_t> cache_line > predecode_cache;
	std::list<uint64_t> predecode_lru_q;

	std::unordered_map< SimpleMem::Request::id_t, SimpleMem::Request* > pending_loads;
};

}
}

#endif
