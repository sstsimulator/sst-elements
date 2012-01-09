#ifndef ZESTO_QSIM_INCLUDED
#define ZESTO_QSIM_INCLUDED
#ifndef USE_PIN_TRACES
#include "qsim.h"

#include "qsim-net.h"

using std::string; using std::cout;

using Qsim::QueueItem;

class QsimClient {
public:
  QsimClient(int fd) : sbs(fd) { wait_for_dot(); }
  ~QsimClient() { sbs << 'x'; }

  unsigned run(unsigned short cpu, unsigned insts) {
    uint16_t i(cpu);
    uint32_t n(insts);
    sbs << 'r' << i << n;
    process_callbacks();
    sbs >> n;
    return n;
  }

  void timer_interrupt() {
    sbs << 't';
    wait_for_dot();
  }

  void interrupt(int cpu, uint8_t vec) {
    uint16_t i(cpu);
    sbs << 'i' << i << vec;
    wait_for_dot();
  }

  bool booted(unsigned cpu) {
    uint16_t i(cpu);
    sbs << 'b' << i;
    return get_bool_rval();
  }

  bool idle(unsigned cpu) {
    uint16_t i(cpu);
    sbs << 'd' << i;
    return get_bool_rval();
  }

  unsigned get_n() {
    uint16_t n;
    sbs << 'n';
    sbs >> n;
    return n; 
  }

  template <typename T> 
    void set_atomic_cb
      (T* p, typename Qsim::OSDomain::atomic_cb_obj<T>::atomic_cb_t f)
  {
    sbs << 's' << 'a'; wait_for_dot();
    atomic_cbs.push_back(new Qsim::OSDomain::atomic_cb_obj<T>(p, f));
  }

  template <typename T>
    void set_inst_cb
      (T* p, typename Qsim::OSDomain::inst_cb_obj<T>::inst_cb_t f)
  {
    sbs << 's' << 'i'; wait_for_dot();
    inst_cbs.push_back(new Qsim::OSDomain::inst_cb_obj<T>(p, f));
  }

  template <typename T>
    void set_int_cb
      (T *p, typename Qsim::OSDomain::int_cb_obj<T>::int_cb_t f)
  {
    sbs << 's' << 'v'; wait_for_dot();
    int_cbs.push_back(new Qsim::OSDomain::int_cb_obj<T>(p, f));
  }

  template <typename T>
    void set_mem_cb
      (T *p, typename Qsim::OSDomain::mem_cb_obj<T>::mem_cb_t f)
  {
    sbs << 's' << 'm'; wait_for_dot();
    mem_cbs.push_back(new Qsim::OSDomain::mem_cb_obj<T>(p, f));
  }

  template <typename T>
    void set_magic_cb
      (T *p, typename Qsim::OSDomain::magic_cb_obj<T>::magic_cb_t f)
  {
    sbs << 's' << 'g'; wait_for_dot();
    magic_cbs.push_back(new Qsim::OSDomain::magic_cb_obj<T>(p, f));
  }
 
  template <typename T>
    void set_io_cb
      (T *p, typename Qsim::OSDomain::io_cb_obj<T>::io_cb_t f)
  {
    sbs << 's' << 'o'; wait_for_dot();
    io_cbs.push_back(new Qsim::OSDomain::io_cb_obj<T>(p, f));
  }

private:
  bool get_bool_rval() {
    char c;
    sbs >> c;
    return (c == 'T');
  }

  void wait_for_dot() {
    char c;
    sbs >> c;
    if (c != '.') {
      std::cout << "Unexpected response from server.\n";
      exit(1);
    }
  }

  void process_callbacks();

  uint8_t inst_bytes[16];

  std::vector<Qsim::OSDomain::atomic_cb_obj_base*> atomic_cbs;
  std::vector<Qsim::OSDomain::inst_cb_obj_base*>   inst_cbs;
  std::vector<Qsim::OSDomain::int_cb_obj_base*>    int_cbs;
  std::vector<Qsim::OSDomain::mem_cb_obj_base*>    mem_cbs;
  std::vector<Qsim::OSDomain::magic_cb_obj_base*>  magic_cbs;
  std::vector<Qsim::OSDomain::io_cb_obj_base*>     io_cbs;

  QsimNet::SockBinStream sbs;
};

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
