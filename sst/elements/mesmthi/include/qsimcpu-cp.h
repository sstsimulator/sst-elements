#include <set>
#include <stdint.h>
#include <qsim.h>

#include "cont_proc.h"
#include "cache.h"

struct mem_ac_t {
  mem_ac_t(uint64_t a, int t): addr(a), type(t) {}
  bool operator<(const mem_ac_t &a) const { return a.addr < addr; }
  uint64_t addr;
  int type;
};

std::multiset<mem_ac_t> pending_requests;
bool running = true;

typedef Slide::MemSub <uint64_t, Slide::simple_line_t<8> > sub_t;
sub_t *sub;

void app_end(int) {
  running = false;
}

void inst_cb(int, uint64_t va, uint64_t pa, uint8_t size, const uint8_t* b) {
  sub->read_rq(pa);
  pending_requests.insert(mem_ac_t(pa, 0));
}

void mem_cb(int, uint64_t va, uint64_t pa, uint8_t size, int type) {
  // Make the request.
  if (type) {
    sub->write_rq(pa, Slide::simple_line_t<8>());
  } else {
    sub->read_rq(pa);
  }

  // Add it to the list of pending requests.
  pending_requests.insert(mem_ac_t(pa, type));
}

const unsigned IPC = 4;
const unsigned INSTBLOCK = 10000;
const unsigned BLOCKSPERINT = 100;

namespace Slide {
  class qsimcpu_t : 
    public Slide::MemSuper<uint64_t, simple_line_t<8> > 
  {
   private:
    bool          stalled;
    unsigned      cache_ports;

    Qsim::OSDomain cd;
    
    class process : public cont_proc_t { public:
      qsimcpu_t *p; unsigned i; delay_cpt delay;
      process(qsimcpu_t *p) : p(p), delay(INSTBLOCK) {}

    cont_proc_t *main() {
      CONT_BEGIN();
      while (running) {
	for (i = 0; i < BLOCKSPERINT; i++) if (running) {
	
          if (!p->stalled && pending_requests.empty()) 
	    p->cd.run(0, INSTBLOCK*IPC);

          CONT_CALL(&delay);
	}
	p->cd.timer_interrupt();
	std::cout << "Timer interrupt\n";
      }
      CONT_END();
    }};

   public:
    void stall()   { stalled = true;  }
    void unstall() { stalled = false; }

    void connect_sub(sub_t &sub_) { sub = &sub_; }

    qsimcpu_t(unsigned cache_ports): 
      stalled(false), cache_ports(cache_ports),
      cd(1, "bzImage")
    {
      cd.connect_console(std::cout);
      cd.set_app_end_cb(app_end);
      cd.set_mem_cb(mem_cb);
      cd.set_inst_cb(inst_cb);
    }

    void run()
    {
      Slide::spawn_cont_proc(new process(this));
      Slide::run_ready_procs();
    }

    void read_ack(uint64_t addr) {
      if (pending_requests.find(mem_ac_t(addr, 0)) == pending_requests.end())
	std::cout << "Received read_ack() for something funky.\n";
      pending_requests.erase(pending_requests.find(mem_ac_t(addr, 0)));
    }

    void write_ack(uint64_t addr) {
      if (pending_requests.find(mem_ac_t(addr, 1)) == pending_requests.end())
	std::cout << "Received write_ack() for something funky.\n";
      pending_requests.erase(pending_requests.find(mem_ac_t(addr, 1)));
    }

  };
};
