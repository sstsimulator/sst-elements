// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include <sst_config.h>
#include "dumpireader.h"

using namespace std;
using namespace SST::Zodiac;

int handleDUMPISend(const dumpi_send *prm, uint16_t thread, const dumpi_time *cpu, const dumpi_time *wall, const dumpi_perfinfo *perf, void *userarg) {

}

int handleDUMPINullFunction(const void *prm, uint16_t thread, const dumpi_time *cpu, const dumpi_time *wall, const dumpi_perfinfo *perf, void *userarg) {
	// Don't do anything
}

int handleDUMPIInit(const void* prm, uint16_t thread, const dumpi_time* cpu, const dumpi_time *wall,
                            const dumpi_perfinfo* perf, void* userarg) {
	std::cout << "Called: DUMPI Init" << std::endl;
}

DUMPIReader::DUMPIReader(string file, uint32_t focusOnRank, uint32_t maxQLen, std::queue<ZodiacEvent*>* evQ) {
	rank = focusOnRank;
	eventQ = evQ;
	qLimit = maxQLen;
	foundFinalize = false;

	trace = undumpi_open(file.c_str());
	if(NULL == trace) {
		std::cerr << "Error: unable to open DUMPI trace: " << file << std::endl;
		exit(-1);
	}

	dumpi_header* trace_header = undumpi_read_header(trace);
	dumpi_free_header(trace_header);

	/*
	callbacks = (libundumpi_callbacks*) malloc(sizeof(libundumpi_callbacks));
	libundumpi_clear_callbacks(callbacks);
	callbacks->on_send = (dumpi_send_call) handleDUMPISend;
	*/

	callbacks = new libundumpi_callbacks;
	libundumpi_clear_callbacks(callbacks);

	callbacks->on_init = (dumpi_init_call) handleDUMPIInit;
	callbacks->on_send = handleDUMPISend;

}

void DUMPIReader::close() {
	undumpi_close(trace);
}

uint32_t DUMPIReader::generateNextEvents() {
	int finalized_reached = 0;

/*	while((finalized_reached == 0) && (eventQ->size() < qLimit)) {
		int active = undumpi_read_single_call(
			trace,
			callbacks,
			this,
			&finalized_reached);

		if(active != 1) {
			// the stream for reading has become
			// inactive, so we need to exit
			break;
		}
	}*/
	undumpi_read_stream(trace, callbacks, this);

	return (uint32_t) eventQ->size();
}

uint32_t DUMPIReader::getQueueLimit() {
	return qLimit;
}

uint32_t DUMPIReader::getCurrentQueueSize() {
	return eventQ->size();
}

void DUMPIReader::enqueueEvent(ZodiacEvent* ev) {
	assert(eventQ->size() < qLimit);
	eventQ->push(ev);
}
