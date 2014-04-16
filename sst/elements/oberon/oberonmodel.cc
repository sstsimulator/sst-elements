// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include <sst_config.h>
#include "oberonmodel.h"

using namespace std;
using namespace SST;
using namespace SST::Oberon;

OberonModel::OberonModel(int32_t mSize, char* dumpPfx, Output* out) :
	SIZE_FP64(8), SIZE_INT64(8), SIZE_INT32(4) {
	memorySize = mSize;
	memory = (char*) malloc(sizeof(char) * mSize);

	dumpNumber = 0;
	dumpPrefix = (char*) malloc(sizeof(char) * (strlen(dumpPfx) + 1));
	strcpy(dumpPrefix, dumpPfx);

	output = out;
}

void OberonModel::copyToMemory(int32_t index, const char* source, int32_t length) {
	output->verbose(CALL_INFO, 9, 0, "Memory byte copier - copy to memory at index: %" PRIu32 " bytecount=%" PRIu32 "\n", index, length);
	for(int32_t i = 0; i < length; i++) {
		memory[index + i] = source[i];
	}
}

void OberonModel::copyToMemory(int32_t index, double* source, int32_t length) {
	copyToMemory(index, (const char*) source, length);
}

void OberonModel::copyToMemory(int32_t index, int64_t* source, int32_t length) {
	copyToMemory(index, (const char*) source, length);
}

void OberonModel::copyToMemory(int32_t index, int32_t* source, int32_t length) {
	copyToMemory(index, (const char*) source, length);
}

int32_t OberonModel::getInt32At(int32_t index) {
	output->verbose(CALL_INFO, 8, 0, "Memory read Int32 from index %" PRIu32 "\n", index);
	assert(index <= (memorySize - (int32_t) sizeof(int32_t)));

	int32_t val = 0;
	memcpy(&val, &memory[index], sizeof(int32_t));

	return val;
}

int64_t OberonModel::getInt64At(int32_t index) {
	output->verbose(CALL_INFO, 8, 0, "Memory read Int64 from index %" PRIu32 "\n", index);
	assert(index <= (memorySize - (int32_t) sizeof(int64_t)));

	int64_t val = 0;
	memcpy(&val, &memory[index], sizeof(int64_t));

	return val;
}

double OberonModel::getFP64At(int32_t index) {
	output->verbose(CALL_INFO, 8, 0, "Memory read FP64 from index %" PRIu32 "\n", index);
	assert(index <= (memorySize - (int32_t) sizeof(double)));

	double val = 0;
	memcpy(&val, &memory[index], sizeof(double));

	return val;
}

void OberonModel::setInt64At(int32_t index, int64_t value) {
	assert(index <= (memorySize - SIZE_INT64));
	output->verbose(CALL_INFO, 8, 0, "Memory write Int64 index %" PRIu32 ", Value=%" PRIu64 "\n", index, value);
	copyToMemory(index, &value, SIZE_INT64);
}

void OberonModel::setFP64At(int32_t index, double value) {
	assert(index <= (memorySize - SIZE_FP64));
	output->verbose(CALL_INFO, 8, 0, "Memory write FP64 index %" PRIu32 ", Value=%f\n", index, value);
	copyToMemory(index, &value, SIZE_FP64);
}

void OberonModel::setInt32At(int32_t index, int32_t value) {
	assert(index <= (memorySize - SIZE_INT32));
	copyToMemory(index, &value, SIZE_INT32);
}

int32_t OberonModel::getInstructionAt(int32_t index) {
	// Make sure we do not access an index which is out of bounds
	assert(index <= (memorySize - (int32_t) sizeof(index)));

	// Return the instruciton at the index we want
	// Instructions are regarded as unsigned 32-bit integers
	int32_t instructionCopy = 0;
	memcpy(&instructionCopy, &memory[index], sizeof(int32_t));

	return instructionCopy;
}

void OberonModel::setMemoryContents(char* copyFrom, int32_t length) {
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

int32_t OberonModel::getMemorySize() {
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

	for(int32_t i = 0; i < memorySize; i = i + 8) {
		fprintf(dump_file, "%20" PRIu32 " | %20.10f | %20" PRId64 " | %20" PRIu32 " / %20" PRIu32 "\n",
			i, getFP64At(i), getInt64At(i),
			getInt32At(i), getInt32At(i+4));
	}

	fclose(dump_file);
}
