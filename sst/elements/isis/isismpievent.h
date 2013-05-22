
#ifndef _H_ISIS_MPI_EVENT
#define _H_ISIS_MPI_EVENT

#include "stdint.h"

namespace SST {
namespace IsisComponent {
namespace IsisMPI {

	enum IsisMPIOperationType {
		init,
		finalize,
		send,
		isend,
		bsend,
		ssend,
		recv,
		allreduce,
		reduce
	}
	
	enum IsisMPIDataType {
		isis_mpi_double,
		isis_mpi_integer
	}
	
	typedef uint32_t IsisMPICommGroup


	class IsisMPIEvent : IsisEvent {
		public:
			virtual IsisMPIOperationType getMPIOperationType();
			
		protected:
			IsisMPIOperationType the_mpi_function;
	}

}
}
}


#endif
