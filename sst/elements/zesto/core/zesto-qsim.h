#ifndef ZESTO_QSIM_INCLUDED
#define ZESTO_QSIM_INCLUDED
#ifndef USE_PIN_TRACES
#include <qsim.h>
#include <qsim-client.h>

using std::string; using std::cout;

using Qsim::QueueItem;

class Queue : public std::queue<Qsim::QueueItem> {
public:
  Queue(QsimClient &c, int cpu) : cpu(cpu) {
    c.set_inst_cb(this, &Queue::inst_cb);
    c.set_mem_cb(this, &Queue::mem_cb);
    c.set_int_cb(this, &Queue::int_cb);
  }
  ~Queue() {}

  void inst_cb(int, uint64_t, uint64_t, uint8_t, const uint8_t *);
  void mem_cb(int, uint64_t, uint64_t, uint8_t, int);
  int int_cb(int, uint8_t);

private:
  int cpu;
};

extern Queue *q[64];

#endif
#endif
