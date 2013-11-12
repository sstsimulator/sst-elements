
#include <vcpu.h>

VanadisProcessor::VanadisProcessor(ComponentId_t id, Params& params) :
	Component(id) {

	int verbosity = params.find_integer("verbose", 0);
	processorID = (uint64_t) params.find_integer("cpuid", id);

	char* prefix_buffer = (char*) malloc(sizeof(char) * 128);
	sprintf(prefix_buffer, "VProc[%" PRIu64 "|@f:@l:@p]", processorID);

	output = new SST::Output(prefix_buffer, verbosity, 0, SST::Output::STDOUT);

	const uint32_t hwContextCount = (uint32_t) params.find_integer("hwcontexts", 4);
	const uint32_t regCountPerContext = (uint32_t) params.find_integer("regcount", 64);
	const uint32_t regWidthBytes = (uint32_t) params.find_integer("regwidth", 8);

	core_count = (uint32_t) params.find_integer("corecount", 1);
	cores = (VanadisCore*) malloc(sizeof(VanadisCore) * core_count);

	for(uint32_t i = 0; i < core_count; ++i) {
		output->verbose(CALL_INFO, 1, 0, "Creating processor %" PRIu64 ", core: %" PRIu32 "...\n", processorID, i);
		cores[i] = new VanadisCore(hwContextCount, verbosity, processorID, regCountPerContext, regWidthBytes);
	}

	std::string cpu_clock = params.find_string("clock", "1GHz");
        output->verbose(CALL_INFO, 1, 0, "Registering processor %" PRIu64 " at %s\n", cpu_clock.c_str());
        registerClock( cpu_clock, new Clock::Handler<VanadisProcessor>(this, &AVanadisProcessor::tick ) );

	isHalt = false;

	output->verbose(CALL_INFO, 1, 0, "Configuration of processor %" PRIu64 " complete.\n");
}

void VanadisProcessor::finish() {

}

void VanadisProcessor::setup() {

}

void VanadisProcessor::tick() {
	if(isHalt) return;

	for(uint32_t i = 0; i < core_count; ++i) {
		cores[i].tick();
	}
}

uint64_t VanadisProcessor::getProcessorID() {
	return processorID;
}

bool VanadisProcessor::isInHalt() {
	return isHalt;
}
