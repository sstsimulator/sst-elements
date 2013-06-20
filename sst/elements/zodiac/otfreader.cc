
#include "otfreader.h"

using namespace std;
using namespace SST::Zodiac;

int handleOTFDefineProcess(void *userData, uint32_t stream, uint32_t process, const char *name, uint32_t parent) {
	std::cout << "OTF: Define Process: " << process << " process name: " << name << std::endl;

	return OTF_RETURN_OK;
}

int handleOTFSendMsg(void *userData, uint64_t time, uint32_t sender, uint32_t receiver, uint32_t group, uint32_t type, uint32_t length, uint32_t source, OTF_KeyValueList *list) {
	std::cout << "OTF: Send message from " << sender << " to " << receiver << std::endl;

	return OTF_RETURN_OK;
}

int handleOTFRecvMsg(void *userData, uint64_t time, uint32_t recvProc, uint32_t sendProc, uint32_t group, uint32_t type, uint32_t length, uint32_t source, OTF_KeyValueList *list) {
	std::cout << "OTF: Recv message from " << sendProc << " (recv'd by: " << recvProc << ")" << std::endl;

	return OTF_RETURN_OK;
}

int handleOTFEnter(void* data, uint64_t time, uint32_t func, uint32_t proc, uint32_t src) {
	std::cout << "OTF: Entered function: " << func << " on process: " << proc << std::endl;

	return OTF_RETURN_OK;
}

int handleOTFExit(void* data, uint64_t time, uint32_t func, uint32_t proc, uint32_t src) {
	std::cout << "OTF: Exited function: " << func << " on process: " << proc << std::endl;

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
	OTF_HandlerArray_setHandler(handlers, (OTF_FunctionPointer*) handleOTFDefineProcess, OTF_DEFPROCESS_RECORD);
	OTF_HandlerArray_setFirstHandlerArg(handlers, this, OTF_DEFPROCESS_RECORD);

	// Read in the definitions from the trace(s)
	OTF_Reader_readDefinitions(reader, handlers);

	// Focus the OTF to a specific rank (disable all ranks, then enable the focus)
	std::cout << "Disabling all ranks, then re-enabling rank: " << rank << std::endl;
	OTF_Reader_setProcessStatusAll(reader, 0);
	OTF_Reader_setProcessStatus(reader, rank, 1);

	// These initialize the reading logic
	OTF_Reader_setRecordLimit(reader, 0);
	OTF_Reader_readEvents(reader, handlers);

	std::cout << "Constructing an OTF Reader, setting event count to 1..." << std::endl;
        OTF_Reader_setRecordLimit(reader, 1);
}

void OTFReader::close() {
	OTF_HandlerArray_close(handlers);
	OTF_FileManager_close(fileMgr);
	OTF_Reader_close(reader);
}

uint64_t OTFReader::generateNextEvent() {
	std::cout << "Generating next event..." << std::endl;
	return OTF_Reader_readEvents(reader, handlers);
}
