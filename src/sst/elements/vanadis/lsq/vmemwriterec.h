
#ifndef _H_VANADIS_MEMORY_WRITTEN_RECORD
#define _H_VANADIS_MEMORY_WRITTEN_RECORD

#include "sst/core/output.h"
#include <vector>

namespace SST {
namespace Vanadis {

class VanadisMemoryWrittenRecord {

public:
	VanadisMemoryWrittenRecord( int verbosity, uint64_t memory_size ) {
		// Round up the next highest number of lines unless we evenly divide
		uint64_t lines = (memory_size / 64) + ( (memory_size % 64) == 0 ? 0 : 1 );

		printf("creating a memory record with size: %" PRIu64 " = %" PRIu64 " lines.\n", memory_size, lines);

		// Zero out the memory - nothing written at start
		for( uint64_t i = 0; i < lines; ++i ) {
			memory_state.push_back(0);
		}
	}

	~VanadisMemoryWrittenRecord() {
		memory_state.clear();
	}

	void markByte( uint64_t byte_addr ) {
		uint64_t line        = byte_addr / 64;
		uint64_t line_offset = byte_addr % 64;

		uint64_t line_value  = memory_state[line];
		uint64_t bit_update  = 1;
		bit_update = bit_update << line_offset;

		line_value = (line_value | bit_update);

		//printf("mark-address: 0x%llx / line: %" PRIu64 " / offset: %" PRIu64 " / before 0x%llx / after 0x%llx\n", byte_addr,
		//	line, line_offset, memory_state[line], line_value);

		memory_state[line] = line_value;
	}

	bool isMarked( uint64_t byte_addr ) {
		uint64_t line        = byte_addr / 64;
		uint64_t line_offset = byte_addr % 64;

		uint64_t line_value  = memory_state[line];
		uint64_t bit_value   = 1;
		bit_value = bit_value << line_offset;

		const bool is_marked = (line_value & bit_value) != 0;

		printf("check-address: 0x%llx / line: %" PRIu64 " / offset: %" PRIu64 " / value: 0x%llx / bit_value: 0x%llx / marked: %3s\n",
			byte_addr, line, line_offset, line_value, bit_value,
			is_marked ? "yes" : "no");

		return is_marked;
	}

private:
	std::vector<uint64_t> memory_state;


};

}
}

#endif
