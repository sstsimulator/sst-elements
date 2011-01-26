#ifndef __CONT_PROC
#define __CONT_PROC

#include <deque>
#include <queue>
#include <stdint.h>

namespace Slide {
  class cont_proc_t;
  cont_proc_t               *_running = NULL;
  std::deque<cont_proc_t *>  _runnable;
  std::queue<cont_proc_t *>  _finished;

  class cont_proc_t {
  public:
    cont_proc_t(): _l(1), _p(0) {}
    virtual ~cont_proc_t() {}

    bool done()    { return _l == 0; }
    virtual cont_proc_t *main() = 0;

    uint16_t     _l; // Current line
    cont_proc_t *_p; // Previous stack frame
  };

  void spawn_cont_proc(cont_proc_t *p) {
    _runnable.push_back(p);
  }

  void run_ready_procs() {
    while (!_runnable.empty()) {
      _running = _runnable.front(); _runnable.pop_front();
      cont_proc_t *next_running = _running->main();
      bool call_made = (next_running && next_running != _running);
      _running = next_running;
      if (_running) {
        if (_running->done()) { delete _running; }
        else if (call_made)   _runnable.push_front(_running);
	else                  _runnable.push_back (_running);
      }
    }
    _running = NULL;
  }

  class cont_cond_t {
  public:
    cont_cond_t(bool b) : b(b) { }
    cont_cond_t() : b(false) { }
    virtual ~cont_cond_t()             { }
    operator bool()     { return b; }

    cont_cond_t &operator=(bool c) { 
      if (c != b) { 
	b = c; 
	while (!waitQ.empty()) { 
          _runnable.push_back(waitQ.front()); waitQ.pop(); 
        } 
      }

      return *this;
    }

    bool b;
    std::queue<cont_proc_t *> waitQ;
  };

#define CONT_BEGIN() switch (_l) { case 1:

#define CONT_YIELD() do {	       \
                       _l = __LINE__;  \
                       return this;    \
                       case __LINE__:; \
                     } while(0)

#define CONT_CALL(f) do {              \
                       _l = __LINE__;  \
                       (f)->_p = this; \
                       return (f);     \
                       case __LINE__:; \
		       (f)->_l = 1;    \
                     } while(0)

#define CONT_WAIT(cond, val) do {			 \
                               while ((cond) != (val)) { \
                                 (cond).waitQ.push(this);\
				 _l = __LINE__;		 \
				 return 0;               \
  			         case __LINE__:;         \
                               }                         \
                             } while(0)

#define CONT_RET() do{ if (_p) return _p; else {_l = 0; return this;} }while(0)
#define CONT_END() CONT_RET(); } return 0l;

// Wait a given number of cycles.
  class delay_cpt : public cont_proc_t { public:
    class alarm_t : public cont_cond_t { public:
      void trigger(void *) { *static_cast<cont_cond_t*>(this) = true; }
    };

    delay_cpt(unsigned t = 0) : t(t) {}

    unsigned t; alarm_t a;

    cont_proc_t *main() {
      CONT_BEGIN();
      schedule(t, &a, &alarm_t::trigger, (void*)0);
      CONT_WAIT(a, true);
      (*static_cast<cont_cond_t*>(&a)) = false;
      CONT_END();
    }
  };
};

#endif

