
#ifndef _H_VANADIS_UTIL_CACHE_LINE_SPLIT
#define _H_VANADIS_UTIL_CACHE_LINE_SPLIT


namespace SST {
namespace Vanadis {

uint64_t vanadis_line_remainder( const uint64_t start, const uint64_t line_length ) {
	if( 64 == line_length ) {
		return line_length - ( start & (uint64_t) 63 );
	} else {
		return line_length - ( start % line_length );
	}
};

}
}

#endif
