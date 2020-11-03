
#ifndef _H_VANADIS_CIRC_Q
#define _H_VANADIS_CIRC_Q

#include <deque>
#include <cassert>

namespace SST {
namespace Vanadis {

template<class T>
class VanadisCircularQueue {
public:
	VanadisCircularQueue( const size_t size ) :
		max_capacity(size) {
	}

	~VanadisCircularQueue() {

	}

	bool empty() {
		return 0 == data.size();
	}

	bool full() {
		return max_capacity == data.size();
	}

	void push(T item) {
		data.push_back(item);
	}

	T peek() {
		return data.front();
	}

	T peekAt( const size_t index ) {
		return data.at( index );
	}

	T pop() {
		T tmp = data.front();
		data.pop_front();
		return tmp;
	}

	size_t size() const {
		return data.size();
	}

	size_t capacity() const {
		return max_capacity;
	}

	void clear() {
		data.clear();
	}

	void removeAt( const size_t index ) {
		auto remove_itr = data.begin();

		for( size_t i = 0; i < index; ++i, remove_itr++ ) {}

		data.erase(remove_itr);
	}

private:
	const size_t max_capacity;
	std::deque<T> data;

};

}
}

#endif
