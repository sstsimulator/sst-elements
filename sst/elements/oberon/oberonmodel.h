
#ifndef _SST_OBERON_MODEL
#define _SST_OBERON_MODEL

namespace SST {
namespace Oberon {

class OberonModel {

	public:
		OberonModel(uint32_t memorySize);
		uint32_t getInstructionAt(uint32_t index);

		int64_t getInt64At(uint32_t index);
		double getFP64At(uint32_t index);
		uint32_t getUInt32At(uint32_t index);

		void setInt64At(uint32_t index, int64_t value);
		void setFP64At(uint32_t index, double value);
		void setUInt32At(uint32_t index, uint32_t value);

		void setMemoryContents(char* copyFrom, uint32_t length);
		void zeroMemory();
		uint32_t getMemorySize();

	private:
		char* memory;
		uint32_t memorySize;
}

}
}

#endif
