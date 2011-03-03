// Copyright 2009-2010 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2010, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include <sst_config.h>
#include <sst/core/serialization/element.h>
#include <sst/core/cpunicEvent.h>
#include <assert.h>
#include <qsim.h>
#include <sst/elements/include/memoryDev.h>
#include <sst/elements/include/memMap.h>

#include "mesmthi.h"
#include "mesmthi_model.h"
#include "include/des.h"
#include "include/cont_proc.h"

using namespace std;
using namespace Slide;
using namespace Qsim;
using namespace SST;

// TODO: Arun's going to make this variable disappear.
int badCheat = 1;

// A simple hack to make debugging a little easier.
uint64_t insts__ = 0;
bool debug = false;
#define _STRINGIFY_(x) #x
#define _STRINGIFY(x) _STRINGIFY_(x)
#define DBG if (debug) \
    cout << "Debug ("/*"thd=" << _running << */\
      /*", l=" _STRINGIFY(__LINE__) ", "*/ << "t=" << dec << _now << "): "
#define DBGC if (debug) cout

#define DBGT DBG << dec << '(' << tile->here_x << ", " << tile->here_y << "): "
#define DBGTR DBGT << "rid=" << dec << rid << "; "

// A way to keep track of all of our counters
map <string, map<string, uint64_t *> > counter_registry;

MesmthiModel *mm;

void print_counters() {
  typedef map <string, map<string, uint64_t *> >::iterator set_it;
  typedef map <string, uint64_t *>::iterator counter_it;

  for (set_it i = counter_registry.begin(); i != counter_registry.end(); i++) {
    for (counter_it j = i->second.begin(); j != i->second.end(); j++) {
      cout << i->first << ';' << j->first << " = " << dec << *j->second <<'\n';
    }
  }
}

void clear_counters() {
  typedef map <string, map<string, uint64_t *> >::iterator set_it;
  typedef map <string, uint64_t *>::iterator counter_it;

  for (set_it i = counter_registry.begin(); i != counter_registry.end(); i++) {
    for (counter_it j = i->second.begin(); j != i->second.end(); j++) {
      *j->second = 0;
    }
  }
}

inline void register_counter(string set, string name, uint64_t &counter) {
  counter_registry[set][name] = &counter;
}

// Debugging tool: keep track of requests so we can always query the oldest
// pending request that locked up.
uint64_t last_rid = 0;

struct req_t {
  uint64_t addr;
  unsigned x, y;
  bool write;
  bool inst;
  uint64_t rid;
  string history;

  req_t(uint64_t a, unsigned x, unsigned y, bool w, 
        bool i, uint64_t r) : 
    addr(a), x(x), y(y), write(w), inst(i), rid(r), history() {}
};

ostream &operator<<(ostream &os, const req_t &r) {
    os << dec << "rid = " << r.rid << "; (" << r.x << ", " << r.y << "); 0x" 
       << hex << r.addr << ";\n" << "    " << r.history;
    return os;
}

multimap<uint64_t, req_t> outstanding_reqs;
typedef multimap<uint64_t, req_t>::iterator or_it;

or_it start_req(uint64_t addr, unsigned x, unsigned y, 
                bool write, bool inst, uint64_t i) 
{
  return outstanding_reqs.insert(pair<uint64_t, req_t>
				 (_now, req_t(addr, x, y, write, inst, i)));
}

void end_req(or_it i) {
  outstanding_reqs.erase(i);
}

void show_oldest_reqs() {
  cout << "10 oldest outstanding requests:\n";
  unsigned i; or_it it;

  for (i = 0, it = outstanding_reqs.begin(); 
       i < 10 && it != outstanding_reqs.end(); 
       i++, it++)
    cout << "  " << dec << it->first << ": " << it->second << '\n';
}


// Timings and other parameters; converting the names of the old const values
// to the ones declared in mesmthi.h
#define ROW_SIZE_L2  log2_row_size
#define ROW_SIZE     (1<<ROW_SIZE_L2)
#define THREADS      num_threads
#define FETCH_BYTES  fetch_bytes
#define BPB_LOG2     log2_bytes_per_block
#define I_SETS_LOG2  log2_sets
#define D_SETS_LOG2  log2_sets
#define L2_SETS_LOG2 log2_sets
#define I_WAYS       i_ways
#define D_WAYS       d_ways
#define L2_WAYS      l2_ways
#define L2_L1_SWAP_T l2_l1_swap_t
#define D_I_SWAP_T   d_i_swap_t
#define CYC_PER_TICK cyc_per_tick
#define PADDR_BITS   paddr_bits
#define NET_LATENCY  net_latency
#define NET_CPP      net_cpp
#define NET_DELAY    (NET_LATENCY - NET_CPP)

void show_dir_ent(uint64_t addr, 
                  set<pair<unsigned, unsigned> > &present)
{
  DBG << "Dir. ent. for 0x" << hex << addr << ": ";

  set<pair<unsigned, unsigned> >::iterator i;
  for (i = present.begin(); i != present.end(); i++) {
    DBGC << '(' << dec << i->first << ", " << i->second << ')';
  }
  DBGC << '\n';
}

struct memop_t { 
  uint64_t addr; int type;
  memop_t(uint64_t a, int t) : addr(a), type(t) {}
};

struct MemoryController;
MemoryController *mc;

struct MemoryController {
  MemoryDev <uint64_t, cont_cond_t*> *mem_dev;

  MemoryController() {
    register_counter("Memory Controller", "Reads",  reads);
    register_counter("Memory Controller", "Writes", writes);


    // Hardcoding these things for now. Don't do this in the rewrite.
    Params memParams;
    memParams["mem.initialCredit"]   = "8";
    memParams["mem.readRespCredit"]  = "1";
    memParams["mem.writeRespCredit"] = "1";
    mem_dev = new MemoryDev<uint64_t, cont_cond_t*>(*_sstComponent, 
                                                    memParams, 
                                                    "bus");

    // Spawn the cookie polling process.
    spawn_cont_proc(new cookie_poll_cp());
  }

  ~MemoryController() {
    delete mem_dev;
  }

  // Poll for memory responses every cycle. There has to be a better way to do
  // this.
  class cookie_poll_cp : public cont_proc_t {public:
    delay_cpt delay;
    cont_cond_t *c;

    cookie_poll_cp() : delay(1) {}

    cont_proc_t *main() {
      CONT_BEGIN();
      while (1) {
        while (mc->mem_dev->popCookie(c)) { 
          DBG << "Popped cookie c=" << c << '\n';
          *c = true;
        }
        CONT_CALL(&delay);
      }
      CONT_END();
    }
  };

  class mem_req_cp : public cont_proc_t {public:
    // Arguments
    uint64_t addr;
    unsigned y;
    bool write;
    
    // Local variables.
    cont_cond_t *c;

    mem_req_cp() {}

    cont_proc_t *main() {
      CONT_BEGIN();
      DBG << (write?"W_REQ(0x":"R_REQ(0x") << hex << addr << ")\n";
      (write?mc->writes:mc->reads)++;
	

      c = new cont_cond_t(false);
      DBG << "Created cookie c=" << c << '\n';

      if (write) mc->mem_dev->write(addr, c);
      else       mc->mem_dev->read (addr, c);

      CONT_WAIT(*c, true);
      delete c;

      DBG << "Finished " << (write?"W_REQ(0x":"R_REQ(0x") << hex << addr
          << ")\n";
      CONT_END();
    }
  };

  uint64_t reads, writes;
};

const char *packet_type_s[] = {
  "WRITE_S", "WRITE_I", "READ_I", "FAIL_RESP", "INV", "RESP", "RESP_E",
  "R_REQ", "W_REQ", "CLEAR_P", "C_REQ"
};

class Tile {
public:
  // Data delivered by callbacks from QSim.
  uint64_t inst_addr;
  uint8_t inst_len;
  

  // Constructor; pre wire-up
  Tile(unsigned base_cpuid, unsigned x, unsigned y) : 
    tile_n(NULL), tile_e(NULL), tile_s(NULL), tile_w(NULL),
    here_x(x), here_y(y),
    instructions(0), bus_transfers(0), bus_requests(0), 
    dir_reads(0), dir_writes(0)
  {
    DBG << "Constructing Tile.\n";

    unsigned idx = x + (here_y << log2_row_size);

    // Assign counters to the registry.
    ostringstream setname;
    setname << "T(" << here_x << ", " << here_y << ")";
    register_counter(setname.str(), "Instructions",  instructions);
    register_counter(setname.str(), "Idle Instructions", idle_instructions);

    register_counter(setname.str(), "L1I Hits",     mm->l1i_hits[idx]);
    register_counter(setname.str(), "L1I Misses",   mm->l1i_misses[idx]);

    register_counter(setname.str(), "L1D R Misses", mm->l1d_read_misses[idx]);
    register_counter(setname.str(), "L1D W Misses", mm->l1d_write_misses[idx]);
    register_counter(setname.str(), "L1D W Hits",   mm->l1d_write_hits[idx]);
    register_counter(setname.str(), "L1D R Hits",   mm->l1d_read_hits[idx]);

    register_counter(setname.str(), "L2 R Misses",  mm->l2_read_misses[idx]);
    register_counter(setname.str(), "L2 W Misses",  mm->l2_write_misses[idx]);
    register_counter(setname.str(), "L2 R Hits",    mm->l2_read_hits[idx]);
    register_counter(setname.str(), "L2 W Hits",    mm->l2_write_hits[idx]);

    register_counter(setname.str(), "Net Trans",mm->network_transactions[idx]);
    register_counter(setname.str(), "Total Pkts", mm->total_packets[idx]);

    // Create THREADS thread processes for this tile and put them in the queue
    for (unsigned i = 0; i < THREADS; i++) {
      // Create the new thread process.
      hw_thread_cp *hwt = new hw_thread_cp(this, base_cpuid + i);
      // Put it in this tile's thread queue.
      thread_q.push(hwt);
      // Make it runnable in cont_proc.
      spawn_cont_proc(hwt);
    }
    thread_q.front()->my_turn = true;
  }

  // Functions for performing wire-up
  void connect_n(Tile &n) { tile_n = &n; }
  void connect_e(Tile &e) { tile_e = &e; }
  void connect_s(Tile &s) { tile_s = &s; }
  void connect_w(Tile &w) { tile_w = &w; }

  // Reset; post wire-up initialization
  void reset() {
  }

  // Counters
  uint64_t instructions;      // # Instructions executed.
  uint64_t idle_instructions; // # Instructions executed w/ no task scheduled.
  uint64_t bus_transfers;     // # Lines transferred over the bus.
  uint64_t bus_requests;      // # requests (just address and req. type)

  uint64_t dir_reads;
  uint64_t dir_writes;

  // Prevent copies and assignment of this simulation component.
  Tile(Tile &t) : e_busy(false), n_busy(false), w_busy(false), s_busy(false) {}
  Tile &operator=(Tile &t) {}

  cont_cond_t e_busy, n_busy, w_busy, s_busy;

  // A. The Thread Process.

  // Forward declarations of cont_proc functions to be called.
  class generic_ac_cp;

  class hw_thread_cp : public cont_proc_t { public:
    Tile*       tile;     // Pointer to the parent class.
    cont_cond_t my_turn;  // True when first in this tile's thread queue
    unsigned    qsim_cpu; // CPU ID in qsim that corresponds to this thread

    // cont_proc_t functions to be called.
    generic_ac_cp     *i_rd;
    generic_ac_cp     *d_rd;
    generic_ac_cp     *d_wr;
    delay_cpt          delay;
    queue<memop_t>     mem_ops;

    // local vars
    or_it cur_req;
    
    hw_thread_cp(Tile *t, unsigned c) 
      : my_turn(false), qsim_cpu(c), tile(t), delay(1)
    {
      DBG << "Constructing HW Thread " << c << '\n';

      i_rd = new generic_ac_cp(this, true, false);
      d_rd = new generic_ac_cp(this, false, false);
      d_wr = new generic_ac_cp(this, false, true);
      i_rd->tile = d_rd->tile = d_wr->tile = tile;
    }

    ~hw_thread_cp() {
      delete i_rd;
      delete d_rd;
      delete d_wr;
    }

    int fetched_bytes;
    bool first_memop;
    cont_proc_t *main() {
      uint64_t rid;

      CONT_BEGIN();
      DBGT << "Beginning hw_thread_cp " << dec << qsim_cpu << " main().\n";
      while (simulation_running) {
	CONT_WAIT(my_turn, true);

	// If the thread has not been started yet, don't do anything.
	if (cd->booted(qsim_cpu)) {

	  //DBG << "It's thread " << qsim_cpu << "'s turn.\n";
	  // Run this CPU 1 instruction.
	  tile->cur_thread = this;
	  cd->run(qsim_cpu, 1);
          if (cd->idle(qsim_cpu)) tile->idle_instructions++;
	  tile->instructions++; insts__++;

	  // Perform instruction fetch in L1 i-cache.
	  //DBG << "Doing fetch for latest inst at 0x" << hex 
          //    << tile->inst_addr << ".\n";
	  fetched_bytes = 0;
	  while (fetched_bytes < tile->inst_len) {
	    // Let fetches beyond the first FETCH_BYTES incur 1-cycle penalty.
	    if (fetched_bytes > 0) CONT_CALL(&delay);

            rid = last_rid++;
	    //DBGTR << "READ from L1 I$ @0x" << hex 
	    //      << tile->inst_addr + fetched_bytes << '\n';
	    i_rd->addr = tile->inst_addr + fetched_bytes;
            i_rd->rid = rid;
            cur_req = start_req(tile->inst_addr + fetched_bytes, 
                                tile->here_x, tile->here_y, false, true, 
                                rid);
            i_rd->req = &cur_req->second;
            cur_req->second.history += "I-READ";
	    CONT_CALL(i_rd);
            end_req(cur_req);

	    fetched_bytes += FETCH_BYTES;
	  }

          // Perform all outstanding L1 d-cache accesseses.
	  first_memop = true;
	  while (!mem_ops.empty()) {
	    if (first_memop == false) CONT_CALL(&delay);

            rid = last_rid++;
	    if (mem_ops.front().type) {
	      //DBGTR << "WRITE to L1 D$ @0x" << hex 
	      //      << mem_ops.front().addr << '\n';
	      d_wr->addr = mem_ops.front().addr;
              d_wr->rid = rid;
	      cur_req = start_req(d_wr->addr, 
                                  tile->here_x, tile->here_y, 
                                  true, false, rid);
              d_wr->req = &cur_req->second;
              cur_req->second.history += "D-WRITE";
	      CONT_CALL(d_wr);
              end_req(cur_req);
	    } else {
	      //DBGTR << "READ from L1 D$ @0x" << hex
	      //      << mem_ops.front().addr << '\n';
	      d_rd->addr = mem_ops.front().addr;
              d_rd->rid = rid;
              cur_req = start_req(d_rd->addr, 
                                  tile->here_x, tile->here_y, 
                                  false, false, rid);
              cur_req->second.history += "D-READ";
              d_rd->req = &cur_req->second;
	      CONT_CALL(d_rd);
              end_req(cur_req);
	    }
	    mem_ops.pop();

	    // Let memory operations beyond the first incur 1-cycle penalty.
	    first_memop = false;
	  }

        }
        // Wait for the clock to tick before we go to the next instruction.
	// This implies that we want even as-yet unbooted threads to still
	// require cycles.
	CONT_CALL(&delay);
	
	// Pass the torch to the next thread.
	my_turn = false;
	tile->thread_q.pop();
	tile->thread_q.push(this);
	tile->thread_q.front()->my_turn = true;
      }
      CONT_END();
    }
  private:
    
  };

  hw_thread_cp        *cur_thread;
  queue<hw_thread_cp*> thread_q;


  // The caches within a tile are fully exclusive in order to simplify
  // coherence between tiles. The inter-tile coherency protocol is a full
  // directory MESI protocol.

  // We use a real LRU replacement policy, which actually doesn't show up in
  // hardware much beyond 2-way caches. Intel and AMD use tree-based pseudo-LRU
  // policies, which are harder to implement in software but much easier to
  // implement in hardware than actual LRU policies. Examples of pseudo-LRU and
  // random implementations can be seen in the prototype cache models.

  // The exclusive policy complicates handling misses by making all misses that
  // hit in another cache swap operations. This adds another bus transfer for
  // these operations, and may cause unnecessary thrashing when a line could be
  // shared. This is only apparent when threads on the same core are executing
  // and reading/writing within the same line at the same time (as in self-
  // modifying code) which never happens in any of the considered benchmarks.

  // Also, note that for convenience we are compltely ignoring the concept of a
  // tag and using the base address of the cache block. The behavior is the
  // same with no performance hit, and the line_t object looks the same no 
  // matter which cache it is in.

  // Our mechanism for avoiding race conditions is simply keeping track of
  // which cache each line lives in with a directory. In actual hardware,
  // the caches themselves keep track of their own lines and mutual exclusion
  // is a property of the bus protocol.

   unsigned here_x, here_y;

   // F, H. Directory and Router

   struct dir_ent_t {
     // Tile addresses on which line is present;
     set<pair<unsigned, unsigned> > present;  
   };
   map <uint64_t, dir_ent_t> directory;

   Tile *tile_n, *tile_e, *tile_s, *tile_w;

   // Circumvent the inherent bias from using the modulo operator. Only use 
   // thisfor numbers much smaller than RAND_MAX.
   unsigned unbiased_rand (unsigned max) {
     int r = rand();
     double u = r/((double)RAND_MAX+1);
     return (unsigned)(u*max);
   }

   template <typename T> const T &rand_element(set<T> &s) {
     if (s.size() == 0) { *((int*)0) = 1234; }
     unsigned n = unbiased_rand(s.size());
     typename set<T>::iterator it;
     unsigned i;
     for (it = s.begin(), i = 0; it != s.end(); i++, it++) {
       if (i == n) return *it;
     }
     cout << "rand_element() is broken. This should never happen.\n";
     exit(1);
   }

   // Access memory system through i-cache or d-cache.
   class generic_ac_cp 
     : public cont_proc_t 
   { public:
     // Instance Parameters
     bool inst;
     bool write;

     // Per-call Parameters
     Tile *tile;
     uint64_t addr;
     uint64_t rid;
     req_t *req;

     // Local variables
     delay_cpt delay;
     MemoryController::mem_req_cp mem_req;

     bool miss, mshr_wait, evict_mem_access, main_mem_access;
     uint64_t evict_mem_addr, main_mem_addr;
     unsigned pre_evict_delay, post_evict_delay, post_main_delay;
     hw_thread_cp *my_thread;
     bool swapped_thread;

     generic_ac_cp(hw_thread_cp *my_thread, bool inst, bool write) 
       : inst(inst), write(write), my_thread(my_thread), swapped_thread(false) 
     {}

     void thread_swap() {
       // Don't let the same access cause multiple thread-swaps.
       if (swapped_thread == true) return;
       swapped_thread = true;
       if (!tile->thread_q.empty()) tile->thread_q.pop();
       if (!tile->thread_q.empty()) tile->thread_q.front()->my_turn = true;
     }

     void thread_putback() {
       if (swapped_thread) {
	 my_thread->my_turn = false;
	 tile->thread_q.push(my_thread);
	 tile->thread_q.front()->my_turn = true;
       }
     }

     void do_access() {
       // Perform cache access regardless of timing and compute delays.
       mm->do_access((tile->here_y << log2_row_size)+tile->here_x, 
                     addr, inst, write);
       mm->get_results(pre_evict_delay, post_evict_delay, post_main_delay,
                       miss, mshr_wait, evict_mem_access, evict_mem_addr,
                       main_mem_access, main_mem_addr);
     }

     cont_proc_t *main() {
       CONT_BEGIN();
       do_access();

       if (miss) {
         thread_swap();
         delay.t = pre_evict_delay; CONT_CALL(&delay);
         if (evict_mem_access) {
           mem_req.write = true;
           mem_req.y = 0; // TODO: Sane value for y.
           mem_req.addr = evict_mem_addr;
           CONT_CALL(&mem_req);
         }
         delay.t = post_evict_delay; CONT_CALL(&delay);
         if (main_mem_access) {
           mem_req.write = false;
           mem_req.y = 0; // TODO: Sane value for y.
           mem_req.addr = main_mem_addr;
           CONT_CALL(&mem_req);
         }
         delay.t = post_main_delay; CONT_CALL(&delay);
         thread_putback(); CONT_WAIT(my_thread->my_turn, true);
       } else if (mshr_wait) { 
         thread_swap();
         delay.t = pre_evict_delay; CONT_CALL(&delay);
         thread_putback(); CONT_WAIT(my_thread->my_turn, true);
       }
       CONT_END();
    }
  };
};

void inst_cb(int hwt, uint64_t va, uint64_t pa, uint8_t l, const uint8_t *) 
{
  // Figure out the corresponding tile.
  Tile *t = tiles[hwt/THREADS];

  // Set that tile's instruction.
  t->inst_addr = pa;
  t->inst_len = l;
}

// Simulating full/empty bits by converting readFE, writeEF operations to
// writes; operations which would have identical timing effects.
struct feb_support_t {
  enum thread_state_t {NORMAL = 0, ATOMIC_PREREAD, ATOMIC_PREWRITE};
  vector <thread_state_t> thread_state;
  vector <uint64_t>       op_count;
  map <uint64_t, bool>    empty;        // Actual simulated FEBs.

  feb_support_t(unsigned n_threads) : 
    thread_state(n_threads), op_count(n_threads) 
  {
    for (unsigned i = 0; i < n_threads; i++) {
      ostringstream oss;
      oss << dec << setw(2) << setfill('0') << i;
      register_counter("F/E Bit Support", "F/E Bit Ops " + oss.str(), 
                       op_count[i]);
    }
    
  }

  void return_fail(int hwt) { cd->set_reg(hwt, QSIM_RCX, 0); }
  void return_succ(int hwt) { cd->set_reg(hwt, QSIM_RCX, 1); }

  void feb_op(unsigned hwt, uint64_t pa) {
    op_count[hwt]++;
  }

  void mem_cb(int hwt, uint64_t va, uint64_t pa, uint8_t s, int type) {
    switch (thread_state[hwt]) {
    case ATOMIC_PREREAD:  type = 1; thread_state[hwt] = ATOMIC_PREWRITE; 
                          feb_op(hwt, pa); break;
    case ATOMIC_PREWRITE: type = 1; thread_state[hwt] = NORMAL; 
                          feb_op(hwt, pa); break;
    default: /* NORMAL; do nothing. */ ;
    }

    Tile *t = tiles[hwt/THREADS];
    t->cur_thread->mem_ops.push(memop_t(pa, type));
 }

 int magic_cb(int hwt, uint64_t rax) {
    // Only handle F/E bit related calls.
    if ( (rax & 0xffffff00) != 0xcd16fe00 ) return 0;

    Tile *t = tiles[hwt/THREADS];

    uint64_t rcx = cd->get_reg(hwt, QSIM_RCX);
    uint64_t rdx = cd->get_reg(hwt, QSIM_RDX);
    bool write;

    feb_op(hwt, rcx);

    switch (rax & 0xff) {
    case 0x00: /* status */
      rax = empty[rcx];
      cd->set_reg(hwt, QSIM_RAX, rax);
      return_succ(hwt);
      write = false;
      break;

    case 0x01: /* empty */
      empty[rcx] = true;
      return_succ(hwt);
      write = true;
      break;

    case 0x02: /* fill */
      empty.erase(rcx);
      return_succ(hwt);
      write = true;
      break;

    case 0x03: /* writeEF */
      if (empty[rcx] == true) {
	// If I can, do the write and set full.
	cd->mem_wr_virt(hwt, rdx, rcx);
	empty.erase(rcx);
	return_succ(hwt);
      } else {
	return_fail(hwt);
      }
      write = true;
      break;

    case 0x05: /* writeF */
      cd->mem_wr_virt(hwt, rdx, rcx);
      empty.erase(rcx);
      return_succ(hwt);
      write = true;
      break;

    case 0x07: /* readFF */
      if (empty[rcx] == false) {
	// If I can, do the read.
	uint64_t new_rax;
	cd->mem_rd_virt(hwt, new_rax, rcx);
	cd->set_reg(hwt, QSIM_RAX, new_rax);
	return_succ(hwt);
      } else {
	return_fail(hwt);
      }
      write = true;
      break;

    case 0x09: /* readFE */
      if (empty[rcx] == false) {
	// If I can, do the read and set the empty bit.
	uint64_t new_rax;
	cd->mem_rd_virt(hwt, new_rax, rcx);
	cd->set_reg(hwt, QSIM_RAX, new_rax);
	empty[rcx] = true;
	return_succ(hwt);
      } else {
	return_fail(hwt);
      }
      write = true;
      break;

    default: 
      write = false;
      cout << "Invalid F/E call.\n";
      exit(1);
    }

    t->cur_thread->mem_ops.push(memop_t(rcx, write));

    return 0;
  }

  int int_cb(int hwt, uint8_t vector) {
    if (vector == 14 && 
	(thread_state[hwt] == ATOMIC_PREREAD || 
	 thread_state[hwt] == ATOMIC_PREWRITE)) 
    {
      // Current atomic operation cancelled by page fault.
      thread_state[hwt] = NORMAL;
    }
    return 0;
  }

  int atomic_cb(int hwt) {
    thread_state[hwt] = ATOMIC_PREREAD;
    return 0;
  }

} *feb_support;

void start_cb(int) { benchmark_running  = true;  }
void end_cb(int)   { simulation_running = false; 
                     _sstComponent->unregisterExit(); }

Mesmthi::Mesmthi(ComponentId_t id, Params_t& params) : 
  IntrospectedComponent(id) 
{
  std::string clock_pd;
  // The parameters:
  //   log2_row_size:        log_2 of N for an NxN mesh
  //   threads:              number of threads per tile
  //   fetch_bytes:          number of bytes that can be fetched in a cycle
  //   log2_bytes_per_block: log_2 # bytes per cache block
  //   log2_sets:            log_2 # sets in L1/L2 caches
  //   i_ways:               # ways in L1 I-cache
  //   d_ways:               # ways in L1 D-cache
  //   l2_ways:              # ways in L2 cache
  //   d_i_swap_t:           penlaty for swap between L1 D$ and I$
  //   l2_l1_swap_t:         penalty for swap between L1 and L2
  //   cyc_per_tick:         sim cycles per tick of the interval timer
  //   paddr_bits:           log_2 of physical address space size
  //   net_latency:          # of cycles to move by one tile on the network
  //   net_cpp:              cycles per packet, 1/throughput
  read_param(params, "log2_row_size",        log2_row_size,        2);
  read_param(params, "threads",              num_threads,          2);
  read_param(params, "fetch_bytes",          fetch_bytes,          8);
  read_param(params, "log2_bytes_per_block", log2_bytes_per_block, 6);
  read_param(params, "log2_sets",            log2_sets,            9);
  read_param(params, "i_ways",               i_ways,               2);
  read_param(params, "d_ways",               d_ways,               2);
  read_param(params, "l2_ways",              l2_ways,             16);
  read_param(params, "d_i_swap_t",           d_i_swap_t,          20);
  read_param(params, "l2_l1_swap_t",         l2_l1_swap_t,         5);
  read_param(params, "cyc_per_tick",         cyc_per_tick,   4000000);
  read_param(params, "samp_per_tick",        samp_per_tick,      400);
  read_param(params, "paddr_bits",           paddr_bits,          32);
  read_param(params, "net_latency",          net_latency,          5);
  read_param(params, "net_cpp",              net_cpp,              1);
  read_param(params, "clock_pd",             clock_pd,       "500ps");
  read_param(params, "kernel_img",        kernel_img,"linux/bzImage");
  read_param(params, "push_introspector", pushIntrospector,"CountIntrospector");
  
  // Initialize the MesmthiModel.
  mm = new MesmthiModel(log2_row_size, net_latency, l2_l1_swap_t,
                        i_ways, log2_sets, d_ways, log2_sets, 
                        l2_ways, log2_sets, log2_bytes_per_block);

  // The simulation won't end until we unregisterExit().
  registerExit();

  // Initialize the discrete event simulator.
  Slide::des_init(this, clock_pd);

  // Initialize our OSDomain
  cd = new Qsim::OSDomain((1<<log2_row_size)*(1<<log2_row_size)*num_threads, 
                         kernel_img,
                         (1<<(paddr_bits - 20)) - 512);
  cd->connect_console(std::cout);
  cd->set_app_start_cb(start_cb);

  // Fast forward to the beginning of the benchmark.
  while (!benchmark_running) {
    for (unsigned k = 0; k < 1000; k++) {
      for (unsigned j = 0; 
           j < (1<<log2_row_size)*(1<<log2_row_size)*num_threads; 
           j++) 
      {
        cd->run(j, 1000);
      }
    }
    cd->timer_interrupt();
  }

  // Set up full-empty bit support
  feb_support = new feb_support_t(num_threads*(1<<(log2_row_size*2)));

  // Set callbacks.
  cd->set_inst_cb(inst_cb);
  cd->set_mem_cb(feb_support, &feb_support_t::mem_cb);
  cd->set_app_end_cb(end_cb);
  cd->set_magic_cb(feb_support, &feb_support_t::magic_cb);
  cd->set_atomic_cb(feb_support, &feb_support_t::atomic_cb);
  cd->set_int_cb(feb_support, &feb_support_t::int_cb);

  // Instantiate the memory controller.
  mc = new MemoryController();

  // Instantiate the tiles.
  tiles.resize((1<<(log2_row_size*2)));
  for (unsigned i = 0; i < (1u<<(log2_row_size*2)); i++) 
    tiles[i] = new Tile(num_threads*i, i%(1<<log2_row_size), i>>log2_row_size);

  // Wire up our mesh
  for (unsigned i = 0; i < (1u<<(2*log2_row_size)); i++) {
    if (i >= (1<<log2_row_size))
      tiles[i]->connect_n(*tiles[i-(1<<log2_row_size)]);
    if (i%(1<<log2_row_size) != ((1<<log2_row_size)-1))
      tiles[i]->connect_e(*tiles[i+1]);
    if (i < (1<<log2_row_size)*((1<<log2_row_size)-1))
      tiles[i]->connect_s(*tiles[i+(1<<log2_row_size)]);
    if (i%(1<<log2_row_size) != 0)
      tiles[i]->connect_w(*tiles[i-1]);
  }

  // Do post wire-up initialization.
  for (unsigned i = 0; i < (1u<<(2*log2_row_size)); i++) tiles[i]->reset();

  // Create an interval timer to deliver interrupts.
  Slide::spawn_cont_proc(new timer_cp());

  clear_counters();

  //Introspection: register monitors (counters)
  ostringstream setname;
  registerMonitor("MCreads", new MonitorPointer<uint64_t>(&mc->reads));
  registerMonitor("MCwrites", new MonitorPointer<uint64_t>(&mc->writes));
  for (unsigned i = 0; i < (1u<<(2*log2_row_size)); i++){
    typedef MonitorPointer<uint64_t> mpc_t;
    setname << "Tile[" << i << "]L1Ireadmisses";
    registerMonitor(setname.str(), new mpc_t(&mm->l1i_misses[i]));
    setname << "Tile[" << i << "]L1Ireadhits";
    registerMonitor(setname.str(), new mpc_t(&mm->l1i_hits[i]));

    setname << "Tile[" << i << "]L1Dreadmisses";
    registerMonitor(setname.str(), new mpc_t(&mm->l1d_read_misses[i]));
    setname << "Tile[" << i << "]L1Dwritemisses";
    registerMonitor(setname.str(), new mpc_t(&mm->l1d_write_misses[i]));
    setname << "Tile[" << i << "]L1Dreadhits";
    registerMonitor(setname.str(), new mpc_t(&mm->l1d_read_hits[i]));
    setname << "Tile[" << i << "]L1Dwritehits";
    registerMonitor(setname.str(), new mpc_t(&mm->l1d_write_hits[i]));

    setname << "Tile[" << i << "]L2readmisses";
    registerMonitor(setname.str(), new mpc_t(&mm->l2_read_misses[i]));
    setname << "Tile[" << i << "]L2writemisses";
    registerMonitor(setname.str(), new mpc_t(&mm->l2_write_misses[i]));
    setname << "Tile[" << i << "]L2readhits";
    registerMonitor(setname.str(), new mpc_t(&mm->l2_read_hits[i]));
    setname << "Tile[" << i << "]L2writehits";
    registerMonitor(setname.str(), new mpc_t(&mm->l2_write_hits[i]));

    setname << "Tile[" << i << "]Instructions";
    registerMonitor(setname.str(), new mpc_t(&tiles[i]->instructions));
    setname << "Tile[" << i << "]IdleInstructions";
    registerMonitor(setname.str(), new mpc_t(&tiles[i]->idle_instructions));
    setname << "Tile[" << i << "]NetworkTransactions";
    registerMonitor(setname.str(), new mpc_t(&mm->network_transactions[i]));
    setname << "Tile[" << i << "]TotalPackets";
    registerMonitor(setname.str(), new mpc_t(&mm->total_packets[i]));
  }
}

extern "C" {
  Mesmthi *mesmthiAllocComponent(SST::ComponentId_t        id,
                                 SST::Component::Params_t& params) {
    return new Mesmthi(id, params);
  }
};

BOOST_CLASS_EXPORT(Mesmthi)
