
#include "otfreader.h"

using namespace std;
using namespace SST::Zodiac;

int handleOTFEnter(void* data, uint64_t time, uint32_t func, uint32_t proc, uint32_t src) {
	OTFReader* reader = (OTFReader*) data;
	std::cout << "Entered function: " << func << std::endl;

	return OTF_RETURN_OK;
}

int handleOTFExit(void* data, uint64_t time, uint32_t func, uint32_t proc, uint32_t src) {
	OTFReader* reader = (OTFReader*) data;
	std::cout << "Exited function: " << func << std::endl;

	return OTF_RETURN_OK;
}

OTFReader::OTFReader(string file, uint32_t focusRank) {
	// Open a maximum of 5 files concurrently
	fileMgr = OTF_FileManager_open(5);

	// Set the rank we are going to focus on.
	rank = focusRank;

	if(NULL == fileMgr) {
		std::cerr << "Unable to successfully create an OFT file manager." << std::endl;
		exit(-1);
	}

	reader = OTF_Reader_open(file.c_str(), fileMgr);

	if(NULL == reader) {
		std::cerr << "Unable to successfully launch an OTF reader." << std::endl;
		exit(-1);
	}

	handlers = OTF_HandlerArray_open();

	if(NULL == handlers) {
		std::cerr << "Unable to create an OTF handler array." << std::endl;
		exit(-1);
	}

	OTF_HandlerArray_setHandler(handlers, (OTF_FunctionPointer*) handleOTFEnter, OTF_ENTER_RECORD);
	OTF_HandlerArray_setFirstHandlerArg(handlers, this, OTF_ENTER_RECORD);
	OTF_HandlerArray_setHandler(handlers, (OTF_FunctionPointer*) handleOTFExit, OTF_LEAVE_RECORD);
	OTF_HandlerArray_setFirstHandlerArg(handlers, this, OTF_LEAVE_RECORD);

	// Focus the OTF to a specific rank (disable all ranks, then enable the focus)
	OTF_Reader_setProcessStatusAll(reader, 0);
	OTF_Reader_setProcessStatus(reader, rank, 1);

	// These initialize the reading logic
	OTF_Reader_setRecordLimit(reader, 0);
	OTF_Reader_readEvents(reader, handlers);

#ifdef __SST_DEBUG_OUTPUT__
	#pragma message "COMPILED WITH ADDITIONAL DEBUG SUPPORT"
#endif
}

void OTFReader::close() {
	OTF_HandlerArray_close(handlers);
	OTF_FileManager_close(fileMgr);
	OTF_Reader_close(reader);
}

void OTFReader::generateNextEvent() {

}
