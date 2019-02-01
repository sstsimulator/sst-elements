
#ifndef _H_SHOGUN_EVENT
#define _H_SHOGUN_EVENT

#include <sst/core/event.h>

namespace SST {
namespace Shogun {

class ShogunEvent : public SST::Event {

public:
	ShogunEvent(int dst) :
		dest(dst) {}
	ShogunEvent(int source, int dst) :
		dest(dst) {}
	~ShogunEvent() {}

	int getDestination() const {
		return dest;
	}

	int getSource() const {
		return src;
	}

	void setSource(int source) {
		src = source;
	}

private:
	int src;
	int dest;

};

}
}

#endif
