
#include "isismpievent.h"
		
int IsisMPIPt2PtEvent::getCount() {
	return count;
}

int IsisMPIPt2PtEvent::getTag() {
	return tag;
}

IsisMPIDataType IsisMPIPt2PtEvent::getDataType() {
	return type;
}

IsisMPICommGroup IsisMPIPt2PtEvent::getCommunicatorGroup() {
	return group;
}