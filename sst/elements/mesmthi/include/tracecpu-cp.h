#include <fstream>
#include <stdint.h>

#include "cont_proc.h"
#include "cache.h"

namespace Slide {
  class tracecpu_t : 
    public Slide::MemSuper<uint64_t, simple_line_t<8> >
  {
  private:
    bool          stalled;
    int           cache_ports;
    std::ifstream tracefile;
    typedef Slide::MemSub <uint64_t, simple_line_t<8> > sub_t;
    sub_t *sub;
    
    class process_cpt : public cont_proc_t { public:
      tracecpu_t *parent; delay_cpt delay; 
      unsigned i; uint64_t addr; uint8_t size; int type;
      process_cpt(tracecpu_t *p): parent(p), delay(1) {}
    cont_proc_t *main() {
      CONT_BEGIN();
      std::cout << "Created a new tracecpu process.\n";
    
      while (parent->tracefile) {
        if (!parent->stalled) for (i = 0; i < parent->cache_ports; i++) {
          parent->tracefile >> std::hex >> addr >> std::dec >> size >> type;

	  addr &= ~uint64_t(3);

	  if (type) {
	    parent->sub->write_rq(addr, simple_line_t<8>());
	  } else {
	    parent->sub->read_rq(addr);
	  }
        }

	CONT_CALL(&delay); // Wait one cycle
      }

      CONT_END();
    }};

  public:
    void stall  () { stalled = true;  }
    void unstall() { stalled = false; }

    void connect_sub(sub_t &sub_) { sub = &sub_; }

    tracecpu_t(const char* tracefile, unsigned cache_ports): 
      stalled(false), cache_ports(cache_ports), tracefile(tracefile)
    {
      if (!tracefile) throw std::domain_error("Could not open trace file.");
    }

    void run()
    {
      Slide::spawn_cont_proc(new process_cpt(this));
      Slide::run_ready_procs();
    }

  };
};
