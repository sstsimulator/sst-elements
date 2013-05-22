
#ifndef _H_ISIS_MPI_P2P_EVENT
#define _H_ISIS_MPI_P2P_EVENT

namespace SST {
namespace IsisComponent {
namespace IsisMPI {

	class IsisMPIPt2PtEvent : IsisMPIEvent {
		public:
			IsisMPIPt2PtEvent(IsisMPIOperationType the_mpi_function,
				int count_, IsisMPIDataType type_, int tag_,
				IsisMPICommGroup group_);

			int 				getCount();
			IsisMPIDataType 	getDataType();
			int 				getTag();
			IsisMPICommGroup 	getCommunicatorGroup();
			
		protected:
			int		count;
			int		tag;
			IsisMPIDataType type;
			IsisMPICommGroup group;
	}

}
}
}


#endif