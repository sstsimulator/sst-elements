
#ifndef _H_EMBER_ENGINE
#define _H_EMBER_ENGINE

#include <vector>

namespace SST {
namespace Ember {

class EmberEngine {
public:
	EmberEngine();
	~EmberEngine();

	void refillQueue();
	int handleFinalize();
	int handleEvent();

protected:
	int thisRank;
	uint32_t eventCount;
	std::queue<EmberEvent*> evQueue;
	EmberGenerator* generator;
	SST::Link* selfEventLink;

};

}
}

#endif
