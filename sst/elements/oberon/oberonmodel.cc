
#include "oberonmodel.h"

using namespace std;
using namespace SST;
using namespace SST::Oberon;

OberonModel::OberonModel(uint32_t mSize, char* dumpPfx, Output* out) :
	SIZE_FP64(8), SIZE_INT64(8), SIZE_UINT32(4) {
	memorySize = mSize;
	memory = (char*) malloc(sizeof(char) * mSize);

	dumpNumber = 0;
	dumpPrefix = (char*) malloc(sizeof(char) * (strlen(dumpPfx) + 1));
	strcpy(dumpPrefix, dumpPfx);

	output = out;
}

void OberonModel::copyToMemory(uint32_t index, const char* source, uint32_t length) {
	output->verbose(CALL_INFO, 2, 0, "Memory byte copier - copy to memory at index: %" PRIu32 " bytecount=%" PRIu32 "\n", index, length);
	for(uint32_t i = 0; i < length; i++) {
		memory[index + i] = source[i];
	}
}

void OberonModel::copyToMemory(uint32_t index, double* source, uint32_t length) {
	copyToMemory(index, (const char*) source, length);
}

void OberonModel::copyToMemory(uint32_t index, int64_t* source, uint32_t length) {
	copyToMemory(index, (const char*) source, length);
}

void OberonModel::copyToMemory(uint32_t index, uint32_t* source, uint32_t length) {
	copyToMemory(index, (const char*) source, length);
}

uint32_t OberonModel::getUInt32At(uint32_t index) {
	output->verbose(CALL_INFO, 2, 0, "Memory read UInt32 from index %" PRIu32 "\n", index);
	assert(index <= (memorySize - sizeof(uint32_t)));

	uint32_t val = 0;
	memcpy(&val, &memory[index], sizeof(uint32_t));

	return val;
}

int64_t OberonModel::getInt64At(uint32_t index) {
	output->verbose(CALL_INFO, 2, 0, "Memory read Int64 from index %" PRIu32 "\n", index);
	assert(index <= (memorySize - sizeof(int64_t)));

	int64_t val = 0;
	memcpy(&val, &memory[index], sizeof(int64_t));

	return val;
}

double OberonModel::getFP64At(uint32_t index) {
	output->verbose(CALL_INFO, 2, 0, "Memory read FP64 from index %" PRIu32 "\n", index);
	assert(index <= (memorySize - sizeof(double)));

	double val = 0;
	memcpy(&val, &memory[index], sizeof(double));

	return val;
}

void OberonModel::setInt64At(uint32_t index, int64_t value) {
	assert(index <= (memorySize - SIZE_INT64));
	output->verbose(CALL_INFO, 2, 0, "Memory write Int64 index %" PRIu32 ", Value=%" PRIu64 "\n", index, value);
	copyToMemory(index, &value, SIZE_INT64);
}

void OberonModel::setFP64At(uint32_t index, double value) {
	assert(index <= (memorySize - SIZE_FP64));
	output->verbose(CALL_INFO, 2, 0, "Memory write FP64 index %" PRIu32 ", Value=%f\n", index, value);
	copyToMemory(index, &value, SIZE_FP64);
}

void OberonModel::setUInt32At(uint32_t index, uint32_t value) {
	assert(index <= (memorySize - SIZE_UINT32));
	copyToMemory(index, &value, SIZE_UINT32);
}

uint32_t OberonModel::getInstructionAt(uint32_t index) {
	// Make sure we do not access an index which is out of bounds
	assert(index <= (memorySize - sizeof(index)));

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

void OberonModel::setMemoryContentsFromFile(const char* file) {
        FILE* model_file = fopen(file, "rb");
        if(model_file == NULL) {
        	std::cerr << "Oberon: unable to open model file." << std::endl;
		exit(-1);
        }

        fseek(model_file, 0, SEEK_END);
        long int model_exe_size = ftell(model_file);

	if(model_exe_size >= memorySize) {
		std::cerr << "Oberon: executable size exceeds memory." << std::endl;
		exit(-1);
	}

	fseek(model_file, 0, SEEK_SET);

	fread(memory, (size_t) model_exe_size, 1, model_file);
	fclose(model_file);
}

uint32_t OberonModel::getMemorySize() {
	return memorySize;
}

void OberonModel::zeroMemory() {
	memset(memory, 0, (size_t) memorySize);
}

void OberonModel::dumpMemory() {
	char* dump_file_name = (char*) malloc(sizeof(char) * 1024);
	sprintf(dump_file_name, "%s-%010" PRIu32 ".dump",
		dumpPrefix, (dumpNumber++));

	FILE* dump_file = fopen(dump_file_name, "wt");
	free(dump_file_name);

	for(uint32_t i = 0; i < memorySize; i = i + 8) {
		fprintf(dump_file, "%20" PRIu32 " | %10.5f | %20" PRIu64 "\n",
			i, getFP64At(i), getInt64At(i));
	}

	fclose(dump_file);
}
