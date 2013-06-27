
#include "sst_config.h"
#include "sst/core/serialization/element.h"
#include <assert.h>

#include "sst/core/element.h"

#include "zsirius.h"

using namespace std;
using namespace SST;
using namespace SST::Zodiac;

ZodiacSiriusTraceReader::ZodiacSiriusTraceReader(ComponentId_t id, Params_t& params) :
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

    uint32_t rank = (uint32_t) params.find_integer("rank", 0);
    eventQ = new std::queue<ZodiacEvent*>();

    trace = new SiriusReader(trace_file, rank, 64, eventQ);
    trace->generateNextEvents();
}

ZodiacSiriusTraceReader::~ZodiacSiriusTraceReader() {
	trace->close();
}

ZodiacSiriusTraceReader::ZodiacSiriusTraceReader() :
    Component(-1)
{
    // for serialization only
}

void ZodiacSiriusTraceReader::handleEvent(Event *ev) {

}

bool ZodiacSiriusTraceReader::clockTic( Cycle_t ) {
  std::cout << "Generated: " << eventQ->size() << " events." << std::endl;

  // return false so we keep going
  return false;
}

BOOST_CLASS_EXPORT(ZodiacSiriusTraceReader)
