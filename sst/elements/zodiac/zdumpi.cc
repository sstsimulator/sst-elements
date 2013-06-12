
#include "sst_config.h"
#include "sst/core/serialization/element.h"
#include <assert.h>

#include "sst/core/element.h"

#include "zdumpi.h"

using namespace SST;
using namespace SST::Zodiac;

ZodiacDUMPITraceReader::ZodiacDUMPITraceReader(ComponentId_t id, Params_t& params) :
  Component(id) {

    std::string msgiface = params.find_string("msgapi");

    if ( msgiface == "" ) {
        msgapi = new MessageInterface();
    } else {
	msgapi = dynamic_cast<MessageInterface*>(loadModule(msgiface, params));

        if(NULL == msgapi) {
		std::cerr << "Message API: " << msgiface << " could not be loaded." << std::endl;
		exit(-1);
        }
    }
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
