
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
		OberonModel(int32_t memorySize, char* dumpPrefix, Output* out);
		int32_t getInstructionAt(int32_t index);

		int64_t getInt64At(int32_t index);
		double getFP64At(int32_t index);
		int32_t getInt32At(int32_t index);

		void setInt64At(int32_t index, int64_t value);
		void setFP64At(int32_t index, double value);
		void setInt32At(int32_t index, int32_t value);

		void setMemoryContents(char* copyFrom, int32_t length);
		void setMemoryContentsFromFile(const char* filePath);

		void zeroMemory();
		int32_t getMemorySize();

		void dumpMemory();
		void copyToMemory(int32_t index, const char* source, int32_t length);
		void copyToMemory(int32_t index, double* source, int32_t length);
		void copyToMemory(int32_t index, int64_t* source, int32_t length);
		void copyToMemory(int32_t index, int32_t* source, int32_t length);

	private:
		char* memory;
		int32_t memorySize;
		int32_t dumpNumber;
		char* dumpPrefix;
		Output* output;

		const int32_t SIZE_FP64;
		const int32_t SIZE_INT64;
		const int32_t SIZE_INT32;

};

}
}

#endif
