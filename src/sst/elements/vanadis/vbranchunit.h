
#ifndef _H_VANADIS_BRANCH_UNIT
#define _H_VANADIS_BRANCH_UNIT

#include <unordered_map>
#include <list>
#include "inst/vspeculate.h"

namespace SST {
namespace Vanadis {

class VanadisBranchUnit {

public:
	VanadisBranchUnit( size_t entries ) :
		max_entries(entries) { }

	void push( const uint64_t ins_addr, VanadisBranchDirection dir ) {
		if( predict.find(ins_addr) != predict.end() ) {
			// Found
			predict[ ins_addr ] = dir;
		} else {
			// Not found, need to create a new entry
			if( lru_keeper.size() >= max_entries ) {
				uint64_t remove_addr = lru_expire();
				predict.erase( predict.find(remove_addr) );
			}

			lru_keeper.push_front( ins_addr );
			predict.insert( std::pair<uint64_t, VanadisBranchDirection>( ins_addr, dir ) );
		}
	}

	VanadisBranchDirection predictDirection( const uint64_t addr ) {
		if( predict.find(addr) != predict.end() ) {
			return predict[ addr ];
		} else {
			// Unknown, so we just say it will be taken as our default
			return BRANCH_TAKEN;
		}
	}
protected:
	void lru_reorder( const uint64_t addr ) {
		for( auto lru_itr = lru_keeper.begin(); lru_itr != lru_keeper.end(); ) {
			if( addr == (*lru_itr) ) {
				lru_keeper.erase( lru_itr );
				lru_keeper.push_front( addr );
			}
		}
	}

	uint64_t lru_expire() {
		const uint64_t back_entry = lru_keeper.back();
		lru_keeper.pop_back();

		return back_entry;
	}

	const uint32_t max_entries;
	std::list<uint64_t> lru_keeper;
	std::map<uint64_t, VanadisBranchDirection> predict;	

};

}
}

#endif
