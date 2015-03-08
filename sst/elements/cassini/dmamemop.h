#ifndef _H_SST_CASSINI_DMA_MEMORY_OPERATION
#define _H_SST_CASSINI_DMA_MEMORY_OPERATION

namespace SST {
namespace Cassini {

class DMAMemoryOperation {

public:
	DMAMemoryOperation(const uint64_t byteOffset,
		const uint64_t len,
		const bool isReadOperation) :
		offset(byteOffset), length(len), isRead(isReadOperation) {}
	DMAMemoryOperation(const uint64_t byteOffset,
		const uint64_t len) :
		offset(byteOffset), length(len), isRead(true) {}

	~DMAMemoryOperation() {}
	uint64_t getByteOffset() const { return offset; }
	uint64_t getLength() const { return length; }
	bool isRead() const { return isRead; }
	bool isWrite() const { return isWrite; }

private:
	const uint64_t offset;
	const uint64_t length;
	const bool isRead;

};

}
}

#endif
