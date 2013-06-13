
#include "sst_config.h"
#include "sst/core/serialization/element.h"
#include <assert.h>

#include "sst/core/element.h"

#include "zdumpi.h"

using namespace std;
using namespace SST;
using namespace SST::Zodiac;

ZodiacDUMPITraceReader::ZodiacDUMPITraceReader(ComponentId_t id, Params_t& params) :
  Component(id) {

    string msgiface = params.find_string("msgapi");

    if ( msgiface == "" ) {
        msgapi = new MessageInterface();
    } else {
	msgapi = dynamic_cast<MessageInterface*>(loadModule(msgiface, params));

        if(NULL == msgapi) {
		std::cerr << "Message API: " << msgiface << " could not be loaded." << std::endl;
		exit(-1);
        }
    }

    string trace_file = params.find_string("tracefile");
    if("" == trace_file) {
	std::cerr << "Error: could not find a file contain a trace to simulate!" << std::endl;
	exit(-1);
    }

    // Open the DUMPI trace file so we can begin generating events
    /*trace = undumpi_open(trace_file.c_str());

    if(NULL == trace) {
	std::cerr << "Error: could not load trace from: " << trace_file << std::endl;
	exit(-1);
    }

    dumpi_header* trace_header = undumpi_read_header(trace);
    std::cout << "DUMPI Trace Information:" << std::endl;
    std::cout << "- Hostname:      " << trace_header->hostname << std::endl;
    std::cout << "- Username:      " << trace_header->username << std::endl;
    dumpi_free_header(trace_header);

    libundumpi_clear_callbacks(&callbacks);
    callbacks.on_send = *(void)(const dumpi_send*, uint16_t, const dumpi_time*, const dumpi_time*, const dumpi_perfinfo*, void*) process_send;
    */

    trace = new DUMPIReader(trace_file);

}

ZodiacDUMPITraceReader::~ZodiacDUMPITraceReader() {
    // Close the trace file so nothing bad happens to the system/file
    //undumpi_close(trace);
}

ZodiacDUMPITraceReader::ZodiacDUMPITraceReader() :
    Component(-1)
{
    // for serialization only
}

void ZodiacDUMPITraceReader::handleEvent(Event *ev) {

}

bool ZodiacDUMPITraceReader::clockTic( Cycle_t ) {
  // return false so we keep going
  return false;
}

BOOST_CLASS_EXPORT(ZodiacDUMPITraceReader)
