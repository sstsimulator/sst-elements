
#include "oberonmodel.h"

using namespace SST;
using namespace SST::Oberon;

OberonModel::OberonModel(uint32_t mSize) {
	memorySize = mSize;
	memory = (char*) malloc(sizeof(char) * mSize);
}

int64_t OberonModel::getInt64At(uint32_t index) {
	assert(index < (memorySize - sizeof(index)));

	int64_t val = 0;
	memcpy(&val, &memory[index], sizeof(int64_t));
}

uint32_t OberonModel::getInstructionAt(uint32_t index) {
	// Make sure we do not access an index which is out of bounds
	assert(index < (memorySize - sizeof(index)));

	// Return the instruciton at the index we want
	// Instructions are regarded as unsigned 32-bit integers
	uint32_t instructionCopy = 0;
	memcpy(&instructionCopy, &memory[index], sizeof(uint32_t));

	return instructionCopy;
}

void OberonModel::setMemoryContents(char* copyFrom, uint32_t length) {
	assert(memorySize >= length);

	// Copy over the instruction bytes from the input into
	// our own buffer
	memcpy(copyFrom, memory, (size_t) length);
}

uint32_t OberonModel::getMemorySize() {
	return memorySize;
}

void OberonModel::zeroMemory() {
	memset(memory, 0, (size_t) memorySize);
}
