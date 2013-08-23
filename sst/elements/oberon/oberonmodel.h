
#ifndef _SST_OBERON_MODEL
#define _SST_OBERON_MODEL

#include <sst/sst_config.h>
#include <sst/core/output.h>

#include <iostream>

#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

namespace SST {
namespace Oberon {

class OberonModel {

	public:
		OberonModel(uint32_t memorySize, char* dumpPrefix, Output* out);
		uint32_t getInstructionAt(uint32_t index);

		int64_t getInt64At(uint32_t index);
		double getFP64At(uint32_t index);
		uint32_t getUInt32At(uint32_t index);

		void setInt64At(uint32_t index, int64_t value);
		void setFP64At(uint32_t index, double value);
		void setUInt32At(uint32_t index, uint32_t value);

		void setMemoryContents(char* copyFrom, uint32_t length);
		void setMemoryContentsFromFile(const char* filePath);

		void zeroMemory();
		uint32_t getMemorySize();

		void dumpMemory();
		void copyToMemory(uint32_t index, const char* source, uint32_t length);
		void copyToMemory(uint32_t index, double* source, uint32_t length);
		void copyToMemory(uint32_t index, int64_t* source, uint32_t length);
		void copyToMemory(uint32_t index, uint32_t* source, uint32_t length);

	private:
		char* memory;
		uint32_t memorySize;
		uint32_t dumpNumber;
		char* dumpPrefix;
		Output* output;

		const uint32_t SIZE_FP64;
		const uint32_t SIZE_INT64;
		const uint32_t SIZE_UINT32;

};

}
}

#endif
