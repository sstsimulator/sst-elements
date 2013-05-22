
#ifndef _H_ISIS_MPI_P2P_SEND_EVENT
#define _H_ISIS_MPI_P2P_SEND_EVENT

#include "isismpievent.h"
#include "isismpip2pev.h"

namespace SST {
namespace IsisComponent {
namespace IsisMPI {

	enum IsisMPISendOperationType {
		send,
		isend,
		bsend,
		ssend
	}
	
	IsisMPIOperationType convertIsisSendToIsisMPIOperation(IsisMPISendOperationType s);

	class IsisMPIPt2PtSendEvent : IsisMPIPt2PtEvent {
		public:
			IsisMPIPt2PtSendEvent(void* data_ptr,
				int count, IsisMPIDataType type, int destination,
				int tag, IsisMPICommGroup group);
			IsisMPIPt2PtSendEvent(IsisMPISendOperationType send_type,
				void* data_ptr, int count, IsisMPIDataType type, int destination,
				int tag, IsisMPICommGroup group);

			void* getPayload();
			int   getDestination();
			IsisMPISendOperationType getSendType();
			
		protected:
			void* 	payload_ptr;
			int		destination;
			IsisMPISendOperationType send_type;
	}

}
}
}

#endif