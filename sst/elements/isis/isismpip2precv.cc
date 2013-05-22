
#include "isismpip2precv.h"

IsisMPIPt2PtRecvEvent::IsisMPIPt2PtRecvEvent(int count_, IsisMPIDataType type_, 
		int source_, int tag_, IsisMPICommGroup group_) {
				
	source = source_;
	tag = tag_;
	type = type_;
	count = count_;
	group = group_;
	recv_type = recv;	
}

IsisMPIPt2PtRecvEvent::IsisMPIPt2PtRecvEvent(IsisMPIRecvOperationType recvtype,
		int count_, IsisMPIDataType type_, 
		int source_, int tag_, IsisMPICommGroup group_) {
				
	source = source_;
	tag = tag_;
	type = type_;
	count = count_;
	group = group_;
	recv_type = recvtype;	
}

int IsisMPIPt2PtRecvEvent::getSource() {
	return source;
}

IsisMPIRecvOperationType IsisMPIPt2PtRecvEvent::getRecvType() {
	return recv_type;
}
