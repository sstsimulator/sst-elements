
#ifndef _H_VANADIS_CIRC_Q
#define _H_VANADIS_CIRC_Q

#include <cassert>

namespace SST {
namespace Vanadis {

template<class T>
class VanadisCircularQueue {
public:
	VanadisCircularQueue(
		const size_t size ) :
		max_capacity(size) {

		front = 0;
		back = 0;

		count = 0;

		data = new T[size];
	}

	~VanadisCircularQueue() {
		delete[] data;
	}

	bool empty() {
		return (front == back);
	}

	bool full() {
		return ( safe_inc(back) == front );
	}

	void push(T item) {
		if( nullptr == item ) {
			fprintf(stderr, "QUEUE INSERT GIVEN A NULL\n");
			assert(0);
		}

		data[back] = item;
		back = safe_inc(back);
		count++;
	}

	T peek() {
		return data[front];
	}

	T peekAt( const size_t index ) {
		return data[ (front+index) % max_capacity ];
	}

	T pop() {
		T temp = data[front];
		data[front] = nullptr;

		front = safe_inc(front);
		count--;
		return temp;
	}

	size_t size() const {
		return count;
	}

	size_t capacity() const {
		return max_capacity;
	}

	void clear() {
		for( size_t i = 0; i < max_capacity; ++i ) {
			data[i] = nullptr;
		}

		front = 0;
		back  = 0;
		count = 0;
	}

private:
	size_t safe_inc(size_t v) {
		return (v+1) % max_capacity;
	}

	size_t front;
	size_t back;
	size_t count;
	const size_t max_capacity;
	T* data;

};

}
}

#endif
