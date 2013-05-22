
#include "isismpip2psend.h"

IsisMPIOperationType convertIsisSendToIsisMPIOperation(IsisMPISendOperationType s) {
	switch(s) {
		case send:
			return send;
		case isend:
			return isend;
		case bsend:
			return bsend;
		case ssend:
			return ssend;
	}
}

IsisMPIPt2PtSendEvent::IsisMPIPt2PtSendEvent(void* data_ptr,
				int count_, IsisMPIDataType type_, int dest,
				int tag_, IsisMPICommGroup grp) {
				
	count = count_;
	tag = tag_
	payload_ptr = data_ptr;
	destintation = dest;
	group = grp;
	send_type = send;
}

IsisMPIPt2PtSendEvent::IsisMPIPt2PtSendEvent(IsisMPISendOperationType sendtype,
				void* data_ptr, int count_, IsisMPIDataType type_, int dest,
				int tag_, IsisMPICommGroup grp) {
				
	count = count_;
	tag = tag_;
	payload_ptr = data_ptr;
	destination = dest;
	group = grp;
	send_type = sendtype;
}

void* IsisMPIPt2PtSendEvent::getPayload() {
	return payload_ptr;
}

int IsisMPIPt2PtSendEvent::getDestination() {
	return destination;
}

IsisMPISendOperationType IsisMPIPt2PtSendEvent::getSendType() {
	return send_type;
}