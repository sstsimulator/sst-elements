
#include "sst_config.h"
#include "sst/core/serialization.h"
#include <assert.h>

#include "sst/core/element.h"
#include "sst/core/params.h"

#include "ztrace.h"

using namespace SST;
using namespace SST::Zodiac;

ZodiacTraceReader::ZodiacTraceReader(ComponentId_t id, Params& params) :
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

ZodiacTraceReader::ZodiacTraceReader() :
    Component(-1)
{
    // for serialization only
}

void ZodiacTraceReader::handleEvent(Event *ev) {

}

bool ZodiacTraceReader::clockTic( Cycle_t ) {
  // return false so we keep going
  return false;
}

BOOST_CLASS_EXPORT(ZodiacTraceReader)
