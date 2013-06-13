
#include "dumpireader.h"

using namespace std;
using namespace SST::Zodiac;

DUMPIReader::DUMPIReader(string file) {
	trace = fopen(file.c_str(), "rb");

	if(NULL == trace) {
		std::cerr << "Error opening: " << file << std::endl;
		exit(-1);
	}
}

void DUMPIReader::close() {
	fclose(trace);
}

void DUMPIReader::readTrace(uint16_t* val) {
	fread(&val, sizeof(uint16_t), 1, trace);
}

void DUMPIReader::readTrace(uint32_t* val) {
	fread(&val, sizeof(uint32_t), 1, trace);
}

void DUMPIReader::readTrace(uint64_t* val) {
	fread(&val, sizeof(uint64_t), 1, trace);
}

void DUMPIReader::readArrayTrace(uint16_t* values, int count) {
	fread(values, sizeof(uint16_t), count, trace);
}

void DUMPIReader::readArrayTrace(uint32_t* values, int count) {
	fread(values, sizeof(uint32_t), count, trace);
}

void DUMPIReader::readArrayTrace(char** str, uint32_t* count) {
	readTrace(count);

	char* the_string = (char*) malloc(sizeof(char) * ((*count) + 1));
	fread(the_string, sizeof(char), *count, trace);
	the_string[(*count)] = '\0';

	*str = the_string;
}

DUMPIFunction DUMPIReader::readNextFunction() {
	uint16_t func = 0;
	readTrace(&func);

	return (DUMPIFunction) func;
}
