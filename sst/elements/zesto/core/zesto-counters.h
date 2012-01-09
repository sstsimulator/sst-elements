#ifndef ZESTO_COUNTERS_H
#define ZESTO_COUNTERS_H
/*#define ZESTO_COUNTERS*/
#ifdef ZESTO_COUNTERS

#include "../../energy_introspector/energy_introspector.h"

#define EXEC_PORTS 8

class core_counters_t {
 public:
  core_counters_t() { time_tick = 0.0; period = 0.0; reset(); }
  ~core_counters_t() {}

  double time_tick, period;

  // branch prediction (fetch)
  counters_t fusion, bpreds, RAS, dirjmpBTB, indirjmpBTB;

  // fetch
  counters_t byteQ, instQ;

  // decode
  class : public counters_t {
   public:
    counters_t predecoder, instruction_decoder, operand_decoder, uop_sequencer;
  }decoder;
  counters_t uopQ;

  // renaming (alloc)
  counters_t freelist, RAT, operand_dependency;

  // memory
  counters_t loadQ, storeQ, memory_dependency;

  // exec
  counters_t issue_select, RS, payload, FU[EXEC_PORTS];
    
  // commit
  counters_t ROB, commit_select;

  // bypass
  counters_t exec_bypass, load_bypass;

  // architectural registers
  class : public counters_t {
   public:
    counters_t integer, floating, segment, control, flag;
  }registers;    

  // pipeline latches (D-flipflops) -- inbetween components
  class {
   public:
    counters_t PC; // program counter
    counters_t BQ2PD; // byteQ to predecoder
    counters_t PD2IQ; // predecoder to instQ
    counters_t IQ2ID; // instQ to decoder;
    counters_t ID2uQ; // decoder to uopQ
    counters_t uQ2RR; // uopQ to register renaming (RAT)
    counters_t RR2RS; // register renaming (RAT) to RS - a whole instruction
    counters_t ARF2RS; // architectural registers to RS (committed operands)
    counters_t ROB2RS; // ROB to RS (in-flight operands)
    counters_t RS2PR; // RS to payload RAM (instruction issue)
    counters_t PR2FU; // payload RAM output (including snatch backs)
    counters_t FU2ROB; // FU to ROB
    counters_t ROB2CM; // ROB to commit
    counters_t LQ2ROB; // load writeback to ROB
    counters_t SQ2LQ; // data forwarding from store queue load queue
    counters_t LQ2L1; // load queue to L1 D$ - address
    counters_t L12LQ; // L1 D$ to load queue - data
    counters_t SQ2L1; // store queue to L1 D$ - data
  }latch;      

  class : public counters_t {
   public:
     counters_t missbuf, prefetch, linefill, writeback;
  }IL1, DL1, DL2, ITLB, DTLB, DTLB2;

  void reset(void) {
    fusion.reset(); bpreds.reset(); RAS.reset(); dirjmpBTB.reset(); indirjmpBTB.reset();
    byteQ.reset(); instQ.reset();
    decoder.reset(); decoder.predecoder.reset(); decoder.instruction_decoder.reset();
    decoder.operand_decoder.reset(); decoder.uop_sequencer.reset();
    uopQ.reset();
    freelist.reset(); RAT.reset(); operand_dependency.reset();
    loadQ.reset(); storeQ.reset(); memory_dependency.reset();
    issue_select.reset(); RS.reset(); payload.reset();
    ROB.reset(); commit_select.reset();
    exec_bypass.reset(); load_bypass.reset();
    for(int i = 0; i < EXEC_PORTS; i++)
      FU[i].reset();
    registers.reset(); registers.integer.reset(); registers.floating.reset();
    registers.segment.reset(); registers.control.reset(); registers.flag.reset();
    latch.PC.reset(); latch.BQ2PD.reset(); latch.PD2IQ.reset(); latch.IQ2ID.reset();
    latch.ID2uQ.reset(); latch.uQ2RR.reset(); latch.RR2RS.reset(); latch.ARF2RS.reset(); 
    latch.ROB2RS.reset(); latch.RS2PR.reset(); latch.PR2FU.reset(); latch.FU2ROB.reset(); 
    latch.ROB2CM.reset(); latch.LQ2ROB.reset(); latch.SQ2LQ.reset(); latch.LQ2L1.reset(); 
    latch.L12LQ.reset(); latch.SQ2L1.reset();

    IL1.reset(); IL1.missbuf.reset(); IL1.prefetch.reset(); IL1.linefill.reset(); IL1.writeback.reset();
    DL1.reset(); DL1.missbuf.reset(); DL1.prefetch.reset(); DL1.linefill.reset(); DL1.writeback.reset();
    DL2.reset(); DL2.missbuf.reset(); DL2.prefetch.reset(); DL2.linefill.reset(); DL2.writeback.reset();
    ITLB.reset(); ITLB.missbuf.reset(); ITLB.prefetch.reset(); ITLB.linefill.reset(); ITLB.writeback.reset();
    DTLB.reset(); DTLB.missbuf.reset(); DTLB.prefetch.reset(); DTLB.linefill.reset(); DTLB.writeback.reset();
    DTLB2.reset(); DTLB2.missbuf.reset(); DTLB2.prefetch.reset(); DTLB2.linefill.reset(); DTLB2.writeback.reset();
  }
};
#endif
#endif
