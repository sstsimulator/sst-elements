
#ifndef _H_VANADIS_BRANCH_UNIT_BASIC
#define _H_VANADIS_BRANCH_UNIT_BASIC

#include "vbranch/vbranchunit.h"

#include <unordered_map>
#include <list>

namespace SST {
namespace Vanadis {

class VanadisBasicBranchUnit : public VanadisBranchUnit {

public:
	SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
		VanadisBasicBranchUnit,
		"vanadis", "VanadisBasicBranchUnit",
		SST_ELI_ELEMENT_VERSION(1,0,0),
		"Implements basic branch prediction capability that stores the last branch direction in a cache",
		SST::Vanadis::VanadisBranchUnit
	)

	SST_ELI_DOCUMENT_PARAMS(
		{ "branch_entries", 		"Sets the number of entries in the underlying cache of branch directions"	}
	)

	SST_ELI_DOCUMENT_STATISTICS(
		{ "branch_cache_hit",		"Counts the number of times a speculated address is found in the branch cache", "hits", 1 },
		{ "branch_cache_miss",	"Counts the number of times a speculated address is not found in the branch cache", "misses", 1 }
	)

	VanadisBasicBranchUnit( ComponentId_t id, Params& params ) : VanadisBranchUnit(id, params) {
		max_entries = params.find<uint32_t>("branch_entries", 64);

		stat_branch_hits   = registerStatistic<uint64_t>("branch_cache_hit",  "1");
		stat_branch_misses = registerStatistic<uint64_t>("branch_cache_miss", "1");
	}

	virtual ~VanadisBasicBranchUnit() {
		clear();
	}

	virtual void push( const uint64_t ins_addr, const uint64_t pred_addr ) {
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

	virtual uint64_t predictAddress( const uint64_t addr ) {
                if( predict.find( addr ) != predict.end() ) {
                        return predict[ addr ] ;
                } else {
                        return 0;
                }
        }

	virtual bool contains( const uint64_t addr ) {
		const bool found = predict.find( addr ) != predict.end();

		if( found ) {
			stat_branch_hits->addData(1);
		} else {
			stat_branch_misses->addData(1);
		}

		return found;
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

	void clear() {
		lru_keeper.clear();
		predict.clear();
	}

	uint32_t max_entries;
        std::list<uint64_t> lru_keeper;
        std::unordered_map<uint64_t, uint64_t> predict;

	Statistic<uint64_t>* stat_branch_hits;
	Statistic<uint64_t>* stat_branch_misses;

};

}
}

#endif
