
#ifndef _H_DUMPI_READER
#define _H_DUMPI_READER

namespace SST {
namespace IsisComponent {

class DUMPIReader : IsisEventGenerator {
	public:
		IsisEvent generateNextEvent();
		DUMPIReader(string trace_path);
		~DUMPIReader();

	private:
		FILE* dumpi_trace_file;
}

}
}

#endif
