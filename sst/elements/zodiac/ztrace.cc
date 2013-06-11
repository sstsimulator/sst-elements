
#include "sst_config.h"
#include "sst/core/serialization/element.h"
#include <assert.h>

#include "sst/core/element.h"

#include "ztrace.h"

using namespace SST;
using namespace SST::Zodiac;

ZodiacTraceReader::ZodiacTraceReader(ComponentId_t id, Params_t& params) :
  Component(id) {

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
