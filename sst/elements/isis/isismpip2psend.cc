
#include "isismpip2psend.h"

IsisMPIPt2PtSendEvent::IsisMPIPt2PtSendEvent(IsisMPIOperationType the_mpi_function,
		void* data_ptr,
		int count_, IsisMPIDataType type_, int dest,
		int tag_, IsisMPICommGroup group_) {
				
	IsisMPIPt2PtEvent(IsisMPIOperationType the_mpi_function,
				int count_, IsisMPIDataType type_, int tag_,
				IsisMPICommGroup group_);
				
	payload_ptr = data_ptr;
	destination = dest;
}

void* IsisMPIPt2PtSendEvent::getPayload() {
	return payload_ptr;
}

int IsisMPIPt2PtSendEvent::getDestination() {
	return destination;
}
