
#ifndef OPTERONDEFS_H
#define OPTERONDEFS_H

#include <string>
using std::string;
#ifndef EXTERN
#define EXTERN extern
#endif

#ifdef MEMDEBUG
#include <duma.h>
#endif
namespace McOpteron{
/// High level instruction categories
enum class Category:uint8_t { 
  UNKNOWN = 0, 
  GENERICINT = 1, 
  SPECIALINT = 2, 
  MULTINT = 4, 
  FLOAT = 8 
};

/// Functional Unit Designators
enum class FunctionalUnitType:uint8_t { 
  UNKNOWN= 0, 
  AGU = 1, 
  ALU0 = 2, 
  ALU1 = 4, 
  ALU2 = 8,
  FADD = 16, 
  FMUL = 32, 
  FSTORE = 64 };

#define ENUMOR(x,t) \
constexpr x operator|(x A,x B){return static_cast<x>(static_cast<t>(A) | static_cast<t>(B));}\
constexpr x operator&(x A,x B){return static_cast<x>(static_cast<t>(A) & static_cast<t>(B));}\
constexpr x operator~(x A){return static_cast<x>(~static_cast<t>(A));}\
inline bool operator&&(x A,x B){return (A&B) != x::UNKNOWN;}
//inline bool operator&(x A,x B){return (A&B)!=x::UNKOWN;}

ENUMOR(Category,uint8_t);
ENUMOR(FunctionalUnitType,uint8_t);

// Access byte bitmask for up to four instruction steps
#define STEP1(v) ((v)&0xff)
#define STEP2(v) (((v)>>8)&0xff)
#define STEP3(v) (((v)>>16)&0xff)
#define STEP4(v) (((v)>>24)&0xff)

#define HISTOGRAMSIZE 128
#define MAXCANASSIGN 3  ///< Opteron decode path allows 3 insns/cycle
#define AGU_LATENCY 1   ///< # of cycles to generate address in AGU

typedef unsigned long long InstructionCount;
typedef unsigned long long CycleCount;
typedef unsigned long long Address;

/// Records of inter-instruction data dependencies
/*
   struct Dependency
   {
   InstructionCount consumer;
   unsigned int numProducers;
   unsigned int numReady;
   bool consumed;
   struct Dependency *next;
   };


/// Records of inter-instruction data dependencies
struct Dependency
{
Token *tkn;				// token to which this record belongs
unsigned int numInputs; // # of input registers for token
unsigned int numReady;  // # of input registers that have become ready
InstructionCount *producers; // instructionCount of instructions that produce each input register
struct Dependency *next;
};

*/


typedef struct 
{
  double brMiss; 
  double iCacheMiss; 
  double iTLBMiss; 
  double fetchBuffEmpty; 
  double fetchBuff;  // fetch buffer is not empty but does not contain max number of instructions
  double decode;     // cycles where decoder does not decode max # of instructions (decodeWidth)
  // this could be due to types of instructions and/or ROB full 
  double decodeStall;    // cycles where decoded isntructions can not all dispatch  into instr queues (causing a stall at decode)

  double dispatch;   // cycles where decoded isntructions can not all dispatch  into instr queues
  // this could be due to IQ's full or IQ's restrictions
  double totalFrontend;   // all cycles where a maximum number of instructions are not scheduled
  // this should include all the above events
  double totalBackend; // cycles lost due to backend event (anything after scheduling stage)
  // this should include everything below
  double insLatency; // cycles lost because an instr is still executing (due to its long latency)
  double dataDep;    // cycles lost because an instr is still waiting for an earlier instruction to provide data
  double dTLBMiss;   // cycles lost due to a TLB miss
  double L1;         // cycles lost accessing L1 cache
  double L2;         // cycles lost accessing L2 cache (L1 miss)
  double L3;         // cycles lost accessing L3 cache (L1 and L2 misses)
  double mem;        // cycles lost accessing memory   (L1, L2, and L3 misses)	
  double ROBEmpty;  	
  double ROBNotFull;  	
  double ROBNotReady;  	

} CPIStack;


EXTERN unsigned int Debug;


inline string tokenize(string *str,string& aFormat);

//double genRandomProbability();
//void seedRandom(unsigned long seed);
}//end namespace McOpteron
#endif
