
#include "isismpievent.h"
		
IsisMPIPt2PtEvent::IsisMPIPt2PtEvent(IsisMPIOperationType the_mpi_function,
		int count_, IsisMPIDataType type_, int tag_,
		IsisMPICommGroup group_) {
	
	IsisMPIEvent(IsisMPIOperationType the_mpi_function);
	count = count_;
	type = type_;
	tag = tag_;
	group = group_;
}
		
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