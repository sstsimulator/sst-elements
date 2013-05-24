
#include "obhaltev.h"

using namespace SST;
using namespace Oberon;

OberonHaltEvent::OberonHaltEvent() {

}

OberonEventType OberonHaltEvent::getEventType() {
	return HALT;
}