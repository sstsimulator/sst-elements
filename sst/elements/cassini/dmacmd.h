
#ifndef _H_SST_CASSINI_DMA_COMMAND
#define _H_SST_CASSINI_DMA_COMMAND

#include <sst/core/event.h>
#include <sst/core/component.h>

namespace SST {
namespace Cassini {

typedef std::pair<uint64_t, uint64_t> DMACommandID;

class DMACommand : public Event {

public:
	DMACommand(const Component* reqComp,
		const uint64_t dst,
		const uint64_t src,
		const uint64_t size) :
		Event(),
		destAddr(dst), srcAddr(src), length(size) {

		cmdID = std::make_pair(nextCmdID++, reqComp->getId());
		returnLinkID = 0;
	}

	uint64_t getSrcAddr() const { return srcAddr; }
	uint64_t getDestAddr() const { return destAddr; }
	uint64_t getLength() const { return length; }
	DMACommandID getCommandID() const { return cmdID; }
	void setCommandID(DMACommandID newID) { cmdID = newID; }

protected:
	static uint64_t nextCmdID;
	DMACommandID cmdID;
	const uint64_t destAddr;
	const uint64_t srcAddr;
	const uint64_t length;

	DMACommand() {} // For serialization
    	friend class boost::serialization::access;

	void serialize(Archive & ar, const unsigned int version)
    	{
        	ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Event);
        	ar & BOOST_SERIALIZATION_NVP(cmdID);
	        ar & BOOST_SERIALIZATION_NVP(destAddr);
	        ar & BOOST_SERIALIZATION_NVP(srcAddr);
	        ar & BOOST_SERIALIZATION_NVP(length);
    	}
};

}
}

#endif
