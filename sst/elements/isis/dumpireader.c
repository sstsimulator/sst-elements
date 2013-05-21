
#include "dumpireader.h"
#include <cstdio>


DUMPIReader::DUMPIReader(string trace_path) {
	dumpi_trace_file = fopen(trace_path.c_str(), "rb");
}

DUMPIReader::~DUMPIReader() {
	fclose( dumpi_trace_file );
}
