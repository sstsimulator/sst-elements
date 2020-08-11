
#ifndef _H_VANADIS_CORE
#define _H_VANADIS_CORE

namespace SST {
namespace Vanadis {

class VanadisCore {
public:
	VanadisCore( const uint16_t id,
		const uint32_t hw_threads ) : core_id(id),
		thread_count(hw_threads) {
		
	}
	
private:
	const uint16_t core_id;
	const uint32_t thread_count;

};


}
}

#endif
