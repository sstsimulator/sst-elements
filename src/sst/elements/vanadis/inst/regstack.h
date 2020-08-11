
#ifndef _H_VANADIS_REG_STACK
#define _H_VANADIS_REG_STACK

namespace SST {
namespace Vanadis {

class VanadisRegisterStack {
public:	
	VanadisRegisterStack(const size_t count) :
		reg_count(count), max_capacity(count) {

		regs = new uint16_t[count];
		for( size_t i = count - 1; i > 0; --i ) {
			regs[i] = (uint16_t) i;
		}

		index = count - 1;
	}

	~VanadisRegisterStack() {
		delete[] regs;
	}

	uint16_t pop() {
		uint16_t temp = regs[index];
		index--;
		return temp;
	}

	void push(const uint16_t v) {
		index++;
		
		regs[index] = v;
	}

	size_t capacity() const {
		return max_capacity;
	}

	size_t unused() const {
		return index;	
	}

	size_t size() const {
		return index;
	}

	bool full()  { return index == reg_count; }
	bool empty() { return index == -1; }
	
private:
	size_t max_capacity;
	size_t reg_count;
	size_t index;
	uint16_t* regs;
};

}
}

#endif
