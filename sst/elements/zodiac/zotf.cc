
#include "sst_config.h"
#include "sst/core/serialization/element.h"
#include <assert.h>

#include "sst/core/element.h"

#include "zotf.h"

using namespace std;
using namespace SST;
using namespace SST::Zodiac;

ZodiacOTFTraceReader::ZodiacOTFTraceReader(ComponentId_t id, Params_t& params) :
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

    string trace_file = params.find_string("trace");
    if("" == trace_file) {
	std::cerr << "Error: could not find a file contain a trace to simulate!" << std::endl;
	exit(-1);
    }

    uint32_t rank = params.find_integer("rank", 0);

    // Create a new reader and set it so that we only process one record per call
    reader = new OTFReader(trace_file, rank);

    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();

    registerClock( "1GHz",
                 new Clock::Handler<ZodiacOTFTraceReader>(this,
                                                     &ZodiacOTFTraceReader::clockTic ) );
}

ZodiacOTFTraceReader::~ZodiacOTFTraceReader() {
    // Close the trace file so nothing bad happens to the system/file
    //undumpi_close(trace);
}

ZodiacOTFTraceReader::ZodiacOTFTraceReader() :
    Component(-1)
{
    // for serialization only
}

void ZodiacOTFTraceReader::handleEvent(Event *ev) {

}

bool ZodiacOTFTraceReader::clockTic( Cycle_t ) {
  uint64_t eventCount = 0;
  if(0 != (eventCount = reader->generateNextEvent())) {
	std::cout << "generated: " << eventCount << " events." << std::endl;
  	return false;
  } else {
	std::cout << "Generating next event caused a stop." << std::endl;
	return true;
  }
}

BOOST_CLASS_EXPORT(ZodiacOTFTraceReader)
