
#ifndef _H_SST_ARIEL_EVENT
#define _H_SST_ARIEL_EVENT

#include <sst_config.h>
#include <sst/core/serialization.h>


namespace SST {
namespace ArielComponent {

enum ArielEventType {
	READ_ADDRESS,
	WRITE_ADDRESS,
	START_DMA_TRANSFER,
	WAIT_ON_DMA_TRANSFER,
	CORE_EXIT,
	NOOP,
	MALLOC,
	FREE
};

class ArielEvent {

	public:
		ArielEvent();
		~ArielEvent();
		virtual ArielEventType getEventType() = 0;

};

}
}

#endif
