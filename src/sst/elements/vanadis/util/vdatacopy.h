
#ifndef _H_VANADIS_UTIL_DATA_COPY
#define _H_VANADIS_UTIL_DATA_COPY

#include <vector>

namespace SST {
namespace Vanadis {

template<typename T>
void vanadis_vec_copy_in( std::vector<uint8_t>& vec, T value ) {
	uint8_t* value_ptr = (uint8_t*) &value;

	for( size_t i = 0; i < sizeof(T); ++i ) {
		vec.push_back( value_ptr[i] );
	}
};

}
}

#endif
