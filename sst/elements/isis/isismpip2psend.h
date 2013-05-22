
#ifndef _H_ISIS_MPI_P2P_SEND_EVENT
#define _H_ISIS_MPI_P2P_SEND_EVENT

#include "isismpievent.h"
#include "isismpip2pev.h"

namespace SST {
namespace IsisComponent {
namespace IsisMPI {

	class IsisMPIPt2PtSendEvent : IsisMPIPt2PtEvent {
		public:
			IsisMPIPt2PtSendEvent(IsisMPIOperationType the_mpi_function,
				void* data_ptr,
				int count_, IsisMPIDataType type_, int dest,
				int tag_, IsisMPICommGroup group_);

			void* getPayload();
			int   getDestination();
			
		protected:
			void* 	payload_ptr;
			int		destination;
	}

}
}
}

#endif