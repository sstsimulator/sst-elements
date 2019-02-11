
#ifndef _H_SHOGUN_Q
#define _H_SHOGUN_Q

namespace SST {
namespace Shogun {

template<typename T>
class ShogunQueue {

public:
	ShogunQueue(const int buffSize) :
		buffMax(buffSize) {

		printf("Create a queue with buffer size %d\n", buffSize);
		queue = new T[buffSize];
		clear();
	}

	~ShogunQueue() {
		delete[] queue;
	}

	bool empty() const {
		return head == tail;
	}

	void clear() {
		head = 0;
		tail = 0;
	}

	bool full() const {
		return ( (tail == buffMax && head == 0) ||
			 ( tail == head - 1 ) );
	}

	bool hasNext() const {
		return ! empty();
	}

	T peek() const {
		T item = queue[head];
		return item;
	}

	T pop() {
		T item = queue[head];
		incHead();

		return item;
	}

	void push(T newItem) {
		queue[tail] = newItem;
		incTail();
	}

	int capacity() const {
		return buffMax;
	}

	int count() const {
		int entries = 0;

		int head_copy = head;
		while( head_copy != tail ) {
			entries++;
			head_copy = (head_copy + 1) % buffMax;
		}

		return entries;
	}

private:
	int head;
	int tail;
	T* queue;
	const int buffMax;

	void incHead() {
		head = nextHead();
	}

	void incTail() {
		tail = nextTail();
	}

	int nextHead() {
		return (head + 1) % buffMax;
	}

	int nextTail() const {
		return (tail + 1) % buffMax;
	}

};

}
}

#endif
