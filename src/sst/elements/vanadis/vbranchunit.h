
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

	void push( const uint64_t ins_addr, const uint64_t pred_addr ) {
		if( predict.find( ins_addr ) != predict.end() ) {
			predict[ ins_addr ] = pred_addr;
		} else {
			if( lru_keeper.size() >= max_entries ) {
				const uint64_t lru_victim = lru_expire();
				predict.erase( predict.find( lru_victim ) );
			}

			lru_keeper.push_front( ins_addr );
			predict.insert( std::pair<uint64_t, uint64_t>( ins_addr, pred_addr ) );
		}
	}

	uint64_t predictAddress( const uint64_t addr ) {
		if( predict.find( addr ) != predict.end() ) {
			return predict[ addr ] ;
		} else {
			return 0;
		}
	}

	bool contains( const uint64_t addr ) {
		return predict.find( addr ) != predict.end();
	}

protected:
	void lru_reorder( const uint64_t addr ) {
		for( auto lru_itr = lru_keeper.begin(); lru_itr != lru_keeper.end(); ) {
			if( addr == (*lru_itr) ) {
				lru_keeper.erase( lru_itr );
				lru_keeper.push_front( addr );
				break;
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
	std::map<uint64_t, uint64_t> predict;

};

}
}

#endif
