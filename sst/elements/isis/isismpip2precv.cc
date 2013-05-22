
#include "isismpip2precv.h"

IsisMPIPt2PtRecvEvent::IsisMPIPt2PtRecvEvent(IsisMPIOperationType the_mpi_function,
		int count_, IsisMPIDataType type_, 
		int source_, int tag_, IsisMPICommGroup group_) {
		
	IsisMPIPt2PtEvent(IsisMPIOperationType the_mpi_function,
				int count_, IsisMPIDataType type_, int tag_,
				IsisMPICommGroup group_);		
}

int IsisMPIPt2PtRecvEvent::getSource() {
	return source;
}
