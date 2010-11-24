#ifndef __SLIDE_DES_H
#define __SLIDE_DES_H

#include <map>

//   schedule(cycles, S* c, void(*f)(T*), T* arg)
//
// This is the single function that must be implemented in order to port the
// Slide libraries to your simulation infrastructure of choice. A very simple
// serial discrete event simulator is included for both testing and simple
// standalone simulations. The only thing it can do is call single-argument
// class member functions at arbitrary times in the future.
//
// The following define the control mechanism for the built-in DES. Other DES
// systems will have their own main loops to control advancement.
//
// bool _tick();
//
// Advance the builtin DES to the next event and execute it. Returns true if
// more events remain to process.
//
// bool _advance(uint64_t n);
//
// Advance the builtin DES by n cycles. Returns true if more events remain to
// process.

namespace Slide {
#ifdef MANIFOLD
  template <typename T> static inline void schedule(unsigned cycles,
						    void(*f)(T*),
						    T* arg)
  {
#error schedule() not implemented for Manifold.
  }
#else 
#ifdef SLIDE_SST
#include <vector>
  SST::Component* _sstComponent;

  class PrintStuff { public:
    virtual ~PrintStuff() {}
    virtual void printStuff() = 0;
  };
  std::vector<PrintStuff*> printstuffs;

  uint64_t _nEvents = 0;
  SST::Link *_selfLink;

  class SlideEventBase : public SST::Event {
  public: 
    virtual ~SlideEventBase() {}
    virtual void go() = 0;
  };

  template <typename S, typename T> class SlideEvent : public SlideEventBase {
  public:
    SlideEvent(S* s, void (S::*f)(T*), T* a) : s(s), f(f), a(a) {}
    ~SlideEvent() {}
    void go()     { ((s)->*(f))(a); }
  private:
      S *s;             // Class on which the function is to be called.
      void (S::*f)(T*); // Function pointer to event "handler"
      T *a;             // Argument pointer.

  };

  template <typename S, typename T> 
    static inline void schedule(unsigned cycles, 
				S* s, void (S::*f)(T*),
				T* a)
  {
    _nEvents++;
    _selfLink->Send(cycles, new SlideEvent<S, T>(s, f, a));
  }

  void  run_ready_procs();

  void register_ps(PrintStuff* p) { printstuffs.push_back(p); }
  void do_ps() {
    std::vector<PrintStuff*>::iterator i;
    for (i = printstuffs.begin(); i != printstuffs.end(); i++) {
      (*i)->printStuff();
    }
  }

  class _HandleEvent { public:
    void handle( SST::Event* e ) {
      SlideEventBase *s = static_cast<SlideEventBase*>(e);
      s->go();
      delete s;
      run_ready_procs();
      if (--_nEvents == 0) {
	std::cout << "No more events to process!\n";
	do_ps();
	exit(0);
      }
    }
  } _handleEvent;

  // The timebase should be changeable.
  void des_init(SST::Component *c) {
    _sstComponent = c;
    SST::TimeConverter *tc = c->registerTimeBase("500ps");
    _selfLink = c->configureSelfLink("slide_des_link", 
				     new SST::Event::Handler<_HandleEvent>
				     (&_handleEvent, &_HandleEvent::handle)
				     );
    _selfLink->setDefaultTimeBase(tc);
    run_ready_procs();
  }

#define _now _sstComponent->getCurrentSimTime()

#else /* Standalone serial DES : */

  uint64_t _now = 0;

  struct _event_t { virtual ~_event_t() {} };
  template <typename S, typename T> struct _event_spec_t: public _event_t { 
    _event_spec_t(S* s, void (S::*f)(T*), T* a): s(s), f(f), a(a) {}
    virtual ~_event_spec_t() { ((s)->*(f))(a); }
    S* s; void (S::*f)(T*); T* a;
  };

  std::multimap<uint64_t, _event_t*> _event_q;
  static inline bool _tick() {
    if (_event_q.empty()) return false;
    _now = _event_q.begin()->first;
    delete _event_q.begin()->second; 
    _event_q.erase(_event_q.begin()); 
    return true; 
  }

  static inline bool _advance(uint64_t cycles) {
    _now += cycles;
    while (!_event_q.empty() && _event_q.begin()->first <= _now && _tick());
    return !_event_q.empty();
  }

  template <typename S, typename T> 
    static inline void schedule(unsigned cycles, 
				S* s, 
				void (S::*f)(T*), 
				T* arg)
  {
    _event_q.insert( std::pair<uint64_t,_event_t*>(_now+cycles, 
						   new _event_spec_t
						   <S, T>(s, f, arg)));
  }
#endif 
#endif
};
#endif /*__SLIDE_DES_H*/
