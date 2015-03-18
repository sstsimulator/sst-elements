// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

// By:Waleed Al-Kohlani

#include "perf_cnt.data"           // Performance Counters data!
#include "inst_prob.data"          // Instruction Special Probabilities
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <math.h>
#include "mersenne.h"           // The code for the MERSENNE PRNG

// defining histogram lengths
#define  LD_LD_HIST_LENGTH    513
#define  ST_ST_HIST_LENGTH    513
#define  FP_FP_HIST_LENGTH    513
#define  INT_INT_HIST_LENGTH  513
#define  BR_BR_HIST_LENGTH    513
#define  ST_LD_HIST_LENGTH    513
#define  LD_USE_HIST_LENGTH   513       // load-to-use histogram
#define  INT_USE_HIST_LENGTH  513       // int_prodcucer-to-use histogram
#define  FP_USE_HIST_LENGTH   513       // fp-to-use histogram

#define  MAX_STB_ENTRIES     8

//--------- Simulation TERMINATION parameters
#define MAX_INST      (2*TOTAL_INSTS)   /*  Twice the number of instructions. Of course,
                                           CPI will converge before getting there
                                           But this is just in case */
#define THRESHOLD      1.0E-3   /* Threshold on CPI variation tolerated */

//--------- Define latencies in cycles

#define FGU_LATENCY   6
#define BRANCH_MISS_PENALTY  7  // Documentation states 6-cycle, but micro-benchmarks
                                    // tell us that it's 7 cycles

/************************
 Info on the T2 Processor
 each core has a 16KB 8-way icache 32 byte lines
 8KB 4-way 16byte line L1 cache
 64 entry iTLB & 128entry DTLB
 All 8 cores share a unified 4MB 16-way L2 cache with 64 byte lines

 ************************/

// L1 access latency is 3
#define T1          3           // This is the basic latency, but it could vary depending on the
                        // distance to the next dep instruction/load

// L2 access latency.
#define T2          23          // This is the basic latency, but it could vary depending on the
                        // distance to the next dep instruction/load

// Main Memory Latency
#define MM         176          // Obtained from Microbenchmark

// ITLB/DTLB miss latency.
#define TT         190          // This has not been affirmed yet, but this is a close estimate

// --------- GLOBAL variables

/* This enumerates the reasons for STALLS */
enum stall_reason { I_CACHE, L1_CACHE, L2_CACHE, MEMORY, INT_DEP, FGU_DEP,
   BRANCH, P_FLUSH, STB_FULL, INT_USE_DEP, INT_DSU_DEP
};

enum token_type { IS_LOAD, IS_STORE };

void handle_delay_slot(unsigned long long i,
      enum stall_reason *last_ld_reason, double *last_ld_satisfied,
      double *fdiv_allowed);

struct dependency {
   unsigned long long which_inst;       // Instriction number of the DEPENDENT instruction
   double when_satisfied;       // Cycle number when the load produces
   enum stall_reason reason;
   struct dependency *next;     // Pointer to the next node
};
struct dependency *dc_head = NULL;

struct memory_queue {
   double when_satisfied;       // the cycle at which the mem op will be satisfied
   enum token_type whoami;      // Load or STORE
   struct memory_queue *next;
};
struct memory_queue *mq_head = NULL;
struct memory_queue *last_store = NULL;

// How many loads/stores are currently in memory queue
int ld_in_q = 0,
      st_in_q = 0;

// distance histograms for distances btween loads, stores, fp, int and branch instructions
double ld_ld_hist[LD_LD_HIST_LENGTH],
       st_st_hist[ST_ST_HIST_LENGTH],
       fp_fp_hist[FP_FP_HIST_LENGTH],
       int_int_hist[INT_INT_HIST_LENGTH];
double br_br_hist[BR_BR_HIST_LENGTH],
       st_ld_hist[ST_LD_HIST_LENGTH];

// distance to use histograms ==> for dependencey checking
double ld_use_hist[LD_USE_HIST_LENGTH],
       int_use_hist[INT_USE_HIST_LENGTH],
       fp_use_hist[FP_USE_HIST_LENGTH];

double P1,
       P2,
       PM,
       PT,
       PG,
       PF,
       PBM,
       P_SP,
       PBR,
       PLD,
       PST;

// Probabibilities in the Model Diagaram
double P_1,
       P_2,
       P_3,
       P_4,
       P_5,
       P_6,
       P_7,
       P_8,
       P_9,
       P_10,
       P_11,
       P_12,
       P_13;

// Delay Slot Probabilities
double PLD_D,
       PST_D,
       PF_D,
       PG_D;                    // probabilities of what type of instruction in delay slot

// Global variables
double cycles = 0;              // Global number of current cycle
double effective_t[50];         // This will hold the effective stall time casued by each STALL_REASON

double p_FDIV_FSQRT_S,
       p_FDIV_FSQRT_D;

// More variables: number of events, loads, stores, branches, ....etc
unsigned long long n_loads = 0,
      n_stores = 0,
      n_memops = 0,
      n_branches = 0,
      n_miss_branches = 0;
unsigned long long n_l1 = 0,
      n_l2 = 0,
      n_mem = 0,
      n_tlb = 0,
      n_gr_produced = 0,
      n_fr_produced = 0;
unsigned long long n_pipe_flushes = 0,
      n_icache_misses = 0,
      n_stb_full = 0,
      n_stb_reads = 0,
      next_ld = 0,
      last_fdiv = 0;

double total_stores,
       total_loads,
       total_gr_producers,
       total_fr_producers,
       total_int,
       total_fp,
       total_br,
       total_instructions;

// R.N.G. Generates a uniform random number in [0 1] This is A mersene twister randome number generator, state-of-the-art PRNG
double my_rand()
{
   double r = (double) genrand_real2(); // this function is defined in mersene.h

   return r;
}

void display_constants(void)
{
   printf("\n");
   printf("Latency ::\n");
   printf(" L1    = %3d cycles\n", T1);
   printf(" L2    = %3d cycles\n", T2);
   printf(" Mem   = %3.0f cycles\n", (double) MM);
   printf(" TLB   = %3d cycles\n\n", TT);

   printf("Model Diagaram Probabilities ::\n");
   printf(" P1(Instr Not Retire Immed.)  = %f (%4.2f%% of instructions)\n",
         P_1, 100.0 * P_1);
   printf(" P2(Instr Retires Immed.)     = %f (%4.2f%% of instructions)\n",
         P_2, 100.0 * P_2);
   printf(" P3(Branch Instr)             = %f (%4.2f%% of instructions)\n",
         P_3, 100.0 * P_3);
   printf(" P4(FP instr)                 = %f (%4.2f%% of instructions)\n",
         P_4, 100.0 * P_4);
   printf(" P5(Fdiv/FSQRT Instr)         = %f (%4.2f%% of instructions)\n",
         P_5, 100.0 * P_5);
   printf(" P6(All Other FGU Instr)      = %f (%4.2f%% of instructions)\n",
         P_6, 100.0 * P_6);
   printf(" P7(SP. INT)                  = %f (%4.2f%% of instructions)\n",
         P_7, 100.0 * P_7);
   printf(" P8(LOAD Instr)               = %f (%4.2f%% of instructions)\n",
         P_8, 100.0 * P_8);
   printf(" P9(LOAD &  hit DTLB)         = %f (%4.2f%% of instructions)\n",
         P_9, 100.0 * P_9);
   printf(" P10(LOAD & miss DTLB         = %f (%4.2f%% of instructions)\n",
         P_10, 100.0 * P_10);
   printf(" P11(LOAD & hit L1)           = %f (%4.2f%% of instructions)\n",
         P_11, 100.0 * P_11);
   printf(" P12(LOAD & hit L2)           = %f (%4.2f%% of instructions)\n",
         P_12, 100.0 * P_12);
   printf(" P13(LOAD & hit MEM)          = %f (%4.2f%% of instructions)\n",
         P_13, 100.0 * P_13);

   printf("\n\nOther Useful Probabilities used internally ::\n");
   printf(" P(Branch mispredict)                   =  %f (%4.2f%%)\n", PBM,
         100.0 * PBM);
   printf(" P(inst in delay slot executed)         =  %f (%4.2f%%)\n", P_DS,
         100.0 * P_DS);
   printf(" P(LD)                                  =  %f (%4.2f%%)\n", PLD,
         100.0 * PLD);
   printf(" P(ST| no load)                         =  %f (%4.2f%%)\n", PST,
         100.0 * PST);
   printf(" P(BR| no load/st)                      =  %f (%4.2f%%)\n", PBR,
         100.0 * PBR);
   printf(" P(INT|no load/st/br)                   =  %f (%4.2f%%)\n", PG,
         100.0 * PG);
   printf(" P(FP| no load/st/br/int)               =  %f (%4.2f%%)\n", PF,
         100.0 * PF);
   printf(" P(LD in delay slot)                    =  %f (%4.2f%%)\n", PLD_D,
         100.0 * PLD_D);
   printf(" P(ST| no load in delay slot)           =  %f (%4.2f%%)\n", PST_D,
         100.0 * PST_D);
   printf(" P(INT|no load/st/br in delay slot)     =  %f (%4.2f%%)\n", PG_D,
         100.0 * PG_D);
   printf(" P(FP| no load/st/br/int in delay slot) =  %f (%4.2f%%)\n", PF_D,
         100.0 * PF_D);

}

// the following function will read histogram files (given as input in the commandline)
// and make a CDF array
double make_cdf(double *buf, int length, int ignore_last_n)
{
   unsigned long long val;
   double cumsum;
   int i,
       err_chk;

   for (cumsum = 0.0, i = 0; i < length; i++) {
      err_chk = scanf("%llu", &val);
      fflush(stdin);
      assert(err_chk != 0 && err_chk != EOF);
      if (i < (length - ignore_last_n))
         cumsum += (double) val;
      buf[i] = cumsum;
   }

   // Make a CDF
   for (i = 0; i < length; i++) {
      buf[i] = buf[i] / cumsum;
   }

   return cumsum;               // the total number of occurences of the instruction is returned!
}

// the following will actually return a number(instruction) probablistically/randomlly
// which would tell us
// when the next load,st,fp,int,br will occour; or when the dependent instruction on
// the current one will be at.
// This function utilizes the CDF array made earlier
int sample_hist(double *hist, int hist_length)
{
   register int i;
   double r;

   r = my_rand();

   for (i = 0; i < hist_length; i++) {
      if (hist[i] >= r)
         break;
   }

   return i;
}

// The following function returns 0 if the difference between performance counts and
// collected Shade counts
// is okay (<2% for each category of instructions & <5% for total_instruction count)
// It returns 1 otherwise.
int diff(double pred, unsigned long long real, int flag)
{
   // 2% difference is max allowed difference for each type of instructions
   // This is crucial so that we get probabilities as close as possible to the real mix of instructions
   double b = 2.0;

   double diff = (double) 100 * (pred - real) / (real);

   if (diff < 0)
      diff = diff * -1;         // resetting the negative

   // This is the special case of FGU instructions in INT programs
   // Performance Counts in purely INT programs count few FGU instructions but shade counts 0 FGU instructions
   // Therefore, the difference would be 100% but is in fact negligible so we take care of that here
   if (diff == 100 && flag == 2)
      return 1;

   // If difference between total instructions is > 20% then there's definately a big problem
   if (flag == 1 && diff >= 20.0) {
      printf("Error: The difference %2.4f is too big.\nPlease make sure the input data are reasonable\n.", diff);
      exit(1);
   }
   // If difference between total instructions is > 5% and < 20% ==> attemp to fix it
   if (flag == 1)
      b = 5.0;

   if (diff <= b)
      return 0;
   else
      return 1;

}

// The following function will check Shade-collected counts against performance-counters-collected counts
// If the difference is small enough (see diff() above ), it will try to fix the count
// If the difference is too big, the program will exit with an error
void sanity_check()
{

   // Checking numbers
   printf("Difference between Instrumentation and counters ::\n");
   printf("If you see \"Fixing....\", then you might have a problem in the collected data\n");
   printf("An attempt will be made to fix this, but if the result error is too big, then\n");
   printf("You should revisit this and check why Shade-collected numbers differ from performance-counters numbers\n");

   printf("Sanity Checks ..... \n");

   if (diff(total_instructions, TOTAL_INSTS, 1) == 0)
      printf("Total Instructions ==> Okay\n");
   else {
      printf("Total Instructions ==> Fixing....\n");
      total_instructions = TOTAL_INSTS;
   }

   if (diff(total_loads, TOTAL_LDS, 0) == 0)
      printf("..");
   else {
      total_loads = TOTAL_LDS;
   }

   if (diff(total_stores, TOTAL_STS, 0) == 0)
      printf("..");

   else {
      total_stores = TOTAL_STS;
   }

   if (diff(total_fp, TOTAL_FPS, 2) == 0)
      printf("..");
   else {
      total_fp = TOTAL_FPS;
   }

   if (diff(total_br, TOTAL_BRS, 0) == 0)
      printf("..");
   else {
      total_br = TOTAL_BRS;
   }

   total_int =
         total_instructions - total_br - total_loads - total_stores -
         total_fp;
   total_instructions =
         total_loads + total_stores + total_fp + total_int + total_br;
   if (diff(total_instructions, TOTAL_INSTS, 1) != 0) {
      printf("Total Instructions ==> Fixing ...\n");
      sanity_check();           // keep doing this utnil the error is fixed or we abort.
   } else {
      printf("\nTotal Instructions ==> All Okay\n");

   }

}

// This function initializes the variables(probabilities)
// This function will also read the distance histograms(see the run_model script)
void init(void)
{
   int seed;
   double p1,
          p2,
          pm;

   memset(effective_t, 0, sizeof(effective_t));

   // Reading Histogram Files Below and Making CDF arrays

   // Read load-to-use histogram
   make_cdf(ld_use_hist, LD_USE_HIST_LENGTH, 0);

   // Read int-to-use histogram
   total_gr_producers = make_cdf(int_use_hist, INT_USE_HIST_LENGTH, 0);

   // Read fp-to-use histogram
   total_fr_producers = make_cdf(fp_use_hist, FP_USE_HIST_LENGTH, 0);

//-----------------------------------------------------------------//
   // read ld-ld histogram
   total_loads = make_cdf(ld_ld_hist, LD_LD_HIST_LENGTH, 0);

   // read st-st histogram
   total_stores = make_cdf(st_st_hist, ST_ST_HIST_LENGTH, 0);

   // read fp-fp histogram
   total_fp = make_cdf(fp_fp_hist, FP_FP_HIST_LENGTH, 0);

   // read int-int histogram
   total_int = make_cdf(int_int_hist, INT_INT_HIST_LENGTH, 0);

   // read br-br histogram
   total_br = make_cdf(br_br_hist, BR_BR_HIST_LENGTH, 0);

   // Read store-to-load histogram
   make_cdf(st_ld_hist, ST_LD_HIST_LENGTH, 0);

   // Now, we add CTI (Control Transfer Instructions ) to regular branch instructions
   // This is crucial so as to correctly consider branch instructions.
   // CTI instructions are counted as branch instructions and they act as branch instructions too
   total_br += PB_6_CTI_N;
   total_int = total_int - PB_6_CTI_N;  // This is done for the same reason
   total_instructions =
         total_loads + total_stores + total_fp + total_int + total_br;

   sanity_check();              // check if our numbers are correct
   total_instructions =
         total_loads + total_stores + total_fp + total_int + total_br;

/************
      Model Chart Probabilities

P_1 == Probability of an instruction that does NOT retire immediately
P_2 == Probability of an instruction that retires immediately
P_3 == Probability of an instruction being  a branch instruction
P_4 == Probability of an instruction being  a floating point instruction
   P_5 == Probability of an instruction being  a fp fdiv/fsqrt instruction
   P_6 == Probability of an instruction being  a fp instruction NOT fdiv/fsqrt
    P_4 == P_5 + P_6

P_7 == Probability of an instruction being an INT instruction that has a latency of more than 1 cycle
P_8 == Probability of an instruction being a load instruction
   P_9  == Probability of an instruction being a load that does not miss the DTLB
   P_10 ==   Probability of an instruction being a load that misses the DTLB
   P_8 = P_9 + P_10

   P_11 == Probability of an instruction being a load that hits in the L1 cache
   P_12 == Probability of an instruction being a load that hits in the L2 cache
   P_13 == Probability of an instruction being a load that hist in main memory

P_1 = P_3 + P_4 + P_7 + P_8 ;
P_2 = 1 - P_1 ;

**************/

   // ----- Memory Probabilities
   // Probability of load being satisfied in L2
   p2 = (L1_MISSES - L2_MISSES) / total_loads;  // need # here and possibly correct it

   // Probability of load being satisfied in memory
   pm = (L2_MISSES) / total_loads;      // or just L2_MISSES/total_loads;

   // Probability of load being satisfied in L1
   p1 = 1.0 - (p2 + pm);
   // Probability of load being satisfied in L1
   P1 = p1;

   // Given L1 miss, Probability of load being satisfied in L2
   P2 = p2 / (1 - p1);

   // Given L1, L2  miss, Probability of load being satisfied in Memory (must = 1)
   PM = pm / (1 - p1 - p2);
   // Probability of TLB miss for
   PT = ((double) 1.0 * TLB_MISSES / (TOTAL_LDS));

   P_9 = (double) 1.0 *(total_loads - TLB_MISSES) / total_instructions;
   P_10 = (double) 1.0 *TLB_MISSES / total_instructions;
   P_11 = (double) 1.0 *(total_loads - L1_MISSES) / total_instructions;
   P_12 = (double) 1.0 *(L1_MISSES - L2_MISSES) / total_instructions;
   P_13 = (double) 1.0 *(L2_MISSES) / total_instructions;

   double den;

   // Probablility of a load
   den = total_instructions - DELAY_SLOT_N;
   PLD = ((double) 1.0 * (total_loads - D_LOADS) / den);
   P_8 = ((double) 1.0 * (total_loads) / (total_instructions));

   // Probability of store if no load
   den = den - (total_loads - D_LOADS);
   PST = ((double) 1.0 * (total_stores - D_STORES) / den);

   // Probability of a branch GIVEN no load/store
   den = den - (total_stores - D_STORES);
   PBR = ((double) 1.0 * (total_br) / den);
   P_3 = ((double) 1.0 * (total_br) / (total_instructions));

   // If not a load/store/branch token, what is the probability of a GR producer
   den = den - total_br;
   PG = ((double) 1.0 * (total_int - D_INTS) / den);
   P_7 = ((double) 1.0 * (PB_6_FGU_N + PB_30_FGU_N + PB_6_INT_N +
         PB_25_INT_N + PB_6_FGU_D_N + PB_30_FGU_D_N + PB_6_INT_D_N +
         PB_25_INT_D_N) / (total_instructions));

   // If not a load/store/branch/GR-producing token, what is the probability of a FR producer
   den = den - (total_int - D_INTS);
   PF = ((double) 1.0 * (total_fp - D_FLOATS) / den);
   P_4 = ((double) 1.0 * total_fp / (total_instructions));
   P_5 = (double) 1.0 *(P_FDIV_FSQRT_S_N + P_FDIV_FSQRT_D_N +
         P_FDIV_FSQRT_S_D_N + P_FDIV_FSQRT_D_D_N) / (total_instructions);
   P_6 = (double) 1.0 *(total_fp - (P_FDIV_FSQRT_S_N + P_FDIV_FSQRT_D_N +
         P_FDIV_FSQRT_S_D_N + P_FDIV_FSQRT_D_D_N)) / (total_instructions);

   // Other Probabilities
   // Probability of a branch miss GIVEN A BRANCH INST --
   PBM = ((double) 1.0 * (TAKEN_BRS) / TOTAL_BRS);      // From Performance Counters
   // Probability of an instruction that retires immediately/not retirre immediately
   P_1 = P_3 + P_4 + P_7 + P_8;
   P_2 = 1 - P_1;

   // Delay Slot Probabilities
   double a = 0.00000001;

   PLD_D = (double) D_LOADS / (a + DELAY_SLOT_N);
   PST_D = (double) D_STORES / (a + DELAY_SLOT_N - D_LOADS);
   PG_D = (double) D_INTS / (a + DELAY_SLOT_N - D_STORES - D_LOADS);
   PF_D = (double) D_FLOATS / (a + DELAY_SLOT_N - D_STORES - D_LOADS -
         D_INTS);

   display_constants();

   // Initalize Random number genrator
   seed = time(NULL) % 1000;
   init_genrand(seed);          // this function is defined in mersene.h as part of the mersene twister PRNG
   printf("\nRandom Number Generator SEED initialized to %d\n", seed);

}

// This function scans memory queue and removes completed requests
double scan_memq(void)
{
   double ans = 0;
   struct memory_queue *temp,
               *next,
               *prev;

   // Scan the chain of memory queue tokens and remove completed ones
   for (prev = NULL, temp = mq_head; temp;) {
      // if completed....
      if (cycles >= temp->when_satisfied) {
         if (temp->whoami == IS_LOAD)
            ld_in_q--;
         else
            st_in_q--;

         if (prev)
            prev->next = temp->next;
         else
            mq_head = temp->next;

         next = temp->next;
         free(temp);
         temp = next;
      } else {
         prev = temp;
         temp = temp->next;
      }
   }

   // ans is always zero
   return ans;

}

// Add main memory request to memory queue
// This adds stores and loads to the memory queue
// for loads, when_satisfied is the cycle count in the future when the load will be done
// for store, when_satisfied is how much long (in cycles) the store will stay in the store buffer (STB)
void add_memq(double when_satisfied, enum token_type whoami)
{
   struct memory_queue *temp,
               *temp2;

   if (whoami == IS_LOAD)
      ld_in_q++;
   else
      st_in_q++;

   temp = (struct memory_queue *) malloc(sizeof(struct memory_queue));
   temp2 = (struct memory_queue *) malloc(sizeof(struct memory_queue));

   assert(temp);
   if (whoami == IS_STORE) {
      // if there has been a store before
      if (last_store) {
         if (cycles >= last_store->when_satisfied)
            when_satisfied += cycles;
         else
            when_satisfied += last_store->when_satisfied;
      } else
         // first store in the instruction stream
      {
         when_satisfied += cycles;
         temp2->when_satisfied = when_satisfied;
         temp2->whoami = whoami;
         temp2->next = NULL;
         last_store = temp2;

      }
      // now we add the mem request to the mem q
      temp->when_satisfied = when_satisfied;
      temp->whoami = whoami;
      temp->next = mq_head;
      mq_head = temp;

   }
   // if load
   else {
      temp->when_satisfied = when_satisfied;
      temp->whoami = whoami;
      temp->next = mq_head;
      mq_head = temp;
   }

}

// How long to staisfy this load?
double cycles_to_serve_load(enum stall_reason *where, int flag, int d_dist,
      int l_dist)
{
   enum stall_reason reason;
   double c = 0.0;

   // L1 hit?
   if (my_rand() <= P1) {
      // TLB miss?
      if (my_rand() <= PT) {
         n_tlb++;
         c += TT;
      }
      reason = L1_CACHE;
      n_l1++;
      if (d_dist == 0 || (d_dist + 1) % 4 == 0 || (l_dist + 1) % 4 == 0) {
         //stall for 3 cycles
         c += T1;
      } else if (d_dist == 1) {
         //stall for 2 cycles
         c += (T1 - 1);
      } else {
         //don't stall
         c += 0;
      }

   }
   // L2 hit?
   else if (my_rand() <= P2) {

      if (my_rand() <= PT) {
         n_tlb++;
         c = TT;
      }
      reason = L2_CACHE;
      n_l2++;
      if (d_dist % 4 == 0 || l_dist == 2 || (l_dist - 2) % 4 == 0) {
         //stall for 24 cycles
         c += (T2 + 1);

      } else if ((d_dist + 1) % 2 || l_dist == 1 || (l_dist - 1) % 4 == 0) {
         //stall for 22 cycles only
         c += (T2 - 1);

      } else {
         //stall for 23 cycles
         c += (T2 - 1);
      }

   }
   // Memory hit? better be!
   else if (my_rand() <= PM) {
      // TLB miss?
      if (my_rand() <= PT) {
         n_tlb++;
         c = TT;
      }
      reason = MEMORY;
      n_mem++;
      c += MM;

      c += scan_memq();         // this scans mem q and removes compleled tokens
   }

   else {
      assert(0);
   }

   *where = reason;
   return c;
}

// Add a dependency to the chain
void add_dependency(unsigned long long which_inst, double when_satisfied,
      enum stall_reason reason)
{
   struct dependency *temp;

   // Scan the chain for the dependent instruction to see if it's already there waiting
   for (temp = dc_head; temp && temp->which_inst != which_inst;
         temp = temp->next);

   // Found. This dependent instruction exists in chain
   if (temp) {
      // New dependency is bigger (longer latency) than existing one
      if (when_satisfied > temp->when_satisfied) {
         temp->when_satisfied = when_satisfied;
         temp->reason = reason;
      }
      // Existing dependence is bigger than new one
      else {
      }
   }
   // Not found. Add this to the chain
   else {
      temp = (struct dependency *) malloc(sizeof(struct dependency));
      assert(temp);

      temp->which_inst = which_inst;
      temp->when_satisfied = when_satisfied;
      temp->reason = reason;
      temp->next = dc_head;
      dc_head = temp;
   }
}

// whenever we flush the pipe we need to adjust the dependence chain
// For the most part this adjustment is actually counted for in the latency of instructions such as loads that miss L1 and mispredicted branches
// But, it's done mostly for i-cache misses
void adjust_dependence_chain(double c)
{
   struct dependency *temp,
             *next;

   for (temp = dc_head; temp;) {
      temp->when_satisfied = temp->when_satisfied + c;
      next = temp->next;
      temp = next;
   }                            // end for

   //free(temp);
   //free(next);

}

// Will return the cycle number at which the depndency for the current instruction is satisfied
double is_dependent(unsigned long long which_inst, enum stall_reason *where)
{
   struct dependency *temp,
             *prev;
   double ret_val = 0;

   // Scan the chain for the dependent instruction
   for (prev = NULL, temp = dc_head; temp && temp->which_inst != which_inst;
         prev = temp, temp = temp->next);

   // Found. Unlink node and return when_satisfied
   if (temp) {
      if (prev) {
         prev->next = temp->next;
      } else {
         dc_head = temp->next;
      }

      ret_val = temp->when_satisfied;
      *where = temp->reason;
      free(temp);
   }

   return ret_val;
}

void serve_store(void)
{
   int c = 0;

   c += (int) scan_memq();      // this scans mem q and removes compleled tokens and return extra latency if needed
   c = (int) (my_rand() * 100) % 8;     // this will give me a random number between 0 and 8
   add_memq(c, IS_STORE);       // when_satisfied is will be how much longer the store will stay in the STB

}

void un_init(void)
{
   struct dependency *temp_d;
   struct memory_queue *temp_m;
   void *next = NULL;

   // Scan the chain for the dependend instruction
   for (temp_d = dc_head; temp_d != NULL;
         temp_d = (struct dependency *) next);
   {
      if (temp_d) {
         next = temp_d->next;
         free(temp_d);
      }
   }

   // Scan the memory queue and unlink
   for (temp_m = mq_head; temp_m != NULL;
         temp_m = (struct memory_queue *) next);
   {
      if (temp_m) {
         next = temp_m->next;
         free(temp_m);
      }
   }
}

int main(void)
{
   double when_satisfied,
          den;
   double cur_cpi = 0,
         last_cpi = 0,
         p_cpi,
         latency,
         fdiv_allowed,
         lat;
   unsigned long long i,
        i2 = 0,
         next_load,
         next_st,
         next_fp;
   int dep_distance;
   enum stall_reason where;
   enum stall_reason reason,
                last_ld_reason;
   double tempb,
          last_ld_satisfied = 0.0,
         cycles2;
   double ic_p2 = 0,
         ic_pm = 0,
         i_miss = 0,
         ic_p1 = 0,
         ITLB_P = 0,
         istall = 0;

   // one instruction per cycle is the max issue frequency for a single-thread single-core model
   double CPIi = 1.0;

   // initialize variables and read-in distance-histograms
   init();

   // calculating IC miss rates
   i_miss = (double) 1.0 *(IC_MISSES) / total_instructions;
   ic_p2 = (double) 1.0 *(IC_MISSES - L2_I_MISSES) / IC_MISSES;
   ic_pm = (double) 1.0 *(L2_I_MISSES) / IC_MISSES;

   ic_p1 = 1.0 - (ic_p2 + ic_pm);
   ITLB_P = ((double) 1.0 * ITLB_MISSES / IC_MISSES);

   // main simulator loop
   printf("Processing");
   for (i = 0; i < MAX_INST; i++) {

      cycles += CPIi;

      // --------------------------------------------------------------- STALL check: start
      // Deos this instruction miss the I$
      // We assume that the Instruction Cache does not pose any stalls unless it's missed

      if (my_rand() <= i_miss) {
         if (my_rand() <= ic_p1) {
            // TLB miss?
            if (my_rand() <= ITLB_P)
               istall += TT;
            where = I_CACHE;
         } else if (my_rand() <= ic_p2) {
            // TLB miss?
            if (my_rand() <= ITLB_P)
               istall += TT;
            where = I_CACHE;
            istall += T2;
         } else if (my_rand() <= ic_pm) {
            // TLB miss?
            if (my_rand() <= ITLB_P)
               istall += TT;
            where = I_CACHE;
            istall += MM;
         }
      }
      // not a miss
      else {
         istall = 0;            // resetting the value
         // JEC: adding in possibility of ITLB miss; this adds almost 1%
         // to CPI
         if (my_rand() <= ITLB_P)
            istall += TT;
         where = I_CACHE;
      }

      if (istall != 0) {
         cycles += istall;
         effective_t[where] += istall;
         n_icache_misses++;

         // Whenever the I$ is missed, the pipe is flushed for that thread
         // the next instruction will need at least 4 cycles to get to the EXE stage
         n_pipe_flushes++;
         cycles += 4;
         effective_t[P_FLUSH] += 4;
         // JEC: do not adjust dependencies because the producers are
         // not delayed, only the consumers
         //adjust_dependence_chain(4);
         istall = 0;            // reset istall value to zero
      }
      // Is this instruction depndent on a previous instruction
      when_satisfied = is_dependent(i, &where);

      // Yes. Not only is it dependent, we also need to STALL
      if (when_satisfied > cycles) {
         effective_t[where] += (when_satisfied - cycles);
         cycles = when_satisfied;
      }
      // --------------------------------------------------------------- STALL check: done

      // is this a load?
      if (my_rand() <= PLD) {

         n_memops++;
         n_loads++;
         cycles2 = cycles;

         // First, check if there's another load still there in the q
         // If there's ,then stall until it's done
         scan_memq();
         if (ld_in_q > 0) {
            effective_t[last_ld_reason] += (last_ld_satisfied - cycles);
            cycles = last_ld_satisfied; // stall
         }
         // The following call, will re-organize the mem q appropriately
         scan_memq();

         // How far is the consumer?
         dep_distance = sample_hist(ld_use_hist, LD_USE_HIST_LENGTH);
         // How far is the next load?
         next_load = sample_hist(ld_ld_hist, LD_LD_HIST_LENGTH);
         // How long to satisfy this load?
         latency =
               cycles_to_serve_load(&where, 0, dep_distance - 1,
               next_load - 1);

         // is this load getting its data from the store buffer?
         if (i == next_ld) {
            n_stb_reads++;
            latency = 2;
         }
         // Check Special Memory instructions that stall the pipe
         // Whenever encounted at fetch/decode stage, these instructions will stall the pipeline for the duration of their latency
         den = total_loads - (double) D_LOADS;
         if (my_rand() <= (double) PB_6_MEM_N / den) {
            latency = 6;
            reason = where;
            cycles += latency;  // stall
            effective_t[where] += latency;      // Accumulates STALL cycles
         } else if (my_rand() <= (double) PB_25_MEM_N / (den - PB_6_MEM_N)) {
            latency = 25;
            reason = where;
            cycles += latency;  // stall
            effective_t[where] += latency;      // Accumulates STALL cycles
         } else if (my_rand() <=
               (double) PB_3_LD_N / (den - PB_6_MEM_N - PB_25_MEM_N)) {
            latency = 3;
            reason = where;
            cycles += latency;  // stall
            effective_t[where] += latency;      // Accumulates STALL cycles
         }
         // if not a special memory instruction, stall appropriately as given by cycles_serve_load()/read from STB
         else {
            reason = where;
            cycles += latency;  // stall
            effective_t[where] += latency;      // Accumulates STALL cycles
         }

         //Add this load to mem queue
         add_memq(cycles2 + latency, IS_LOAD);

         // Add this to the dependency chain
         add_dependency(i + dep_distance, cycles2 + latency, where);
         last_ld_satisfied = cycles2 + latency;
         last_ld_reason = where;

         // if this load misses the L1, this will cause a pipeline flush
         if (latency > T1) {
            n_pipe_flushes++;
         }

      } else if (my_rand() <= PST) {

         n_memops++;
         n_stores++;

         next_st = sample_hist(st_st_hist, ST_ST_HIST_LENGTH);
         next_ld = sample_hist(st_ld_hist, ST_LD_HIST_LENGTH);  // the load that depends on this store
         if (next_ld < 8)       // only consider stores that have been in the STB up to 8
            next_ld += i;

         // check if STB is full ==> See Page 75
         if (st_in_q >= MAX_STB_ENTRIES || (next_st == 1 &&
               st_in_q >= MAX_STB_ENTRIES - 1)) {
            n_stb_full++;
            cycles += 2;
            effective_t[STB_FULL] += 2;
         }
         // Add to mem_q if necessary
         serve_store();
      }
      // branch token?
      else if (my_rand() <= PBR) {

         n_branches++;
         cycles2 = cycles + BRANCH_MISS_PENALTY;

         // Is the instruction in the delay slot executed?
         if (my_rand() <= (double) P_DS) {
            cycles += 1;        // Dispaching a delay slot requires a cycle
            i2++;               // Keep track of the number of instructions executed in the delay slot
            // JEC: should force delay slot instruction to consume
            // an i value -> change (i+1) to (++i)
            handle_delay_slot((i + 1), &last_ld_reason, &last_ld_satisfied,
                  &fdiv_allowed);
         }                      // end if  a delay slot instruction

         // Is this branch Mis-predicted?
         // Note: Whenever a branch is mis-predicted a pipeline flush occurs
         if (my_rand() <= PBM) {
            n_miss_branches++;
            // Here we stall more if all the 7-cycle branch mispredict penalty is not
            // completely used by the Delay Slot instruction
            if ((cycles - cycles2) < 0) {
               effective_t[BRANCH] += cycles2 - cycles; // Accumulates PRED BRANCH CYCLES
               cycles = cycles2;
            }
            n_pipe_flushes++;
            // JEC: do not adjust dependencies because the producers are
            // not delayed, only the consumers
            //adjust_dependence_chain(4);

         }
         // See Page 74, if another DCTI (branch) is detected following this one?
         if (sample_hist(br_br_hist, BR_BR_HIST_LENGTH) <= 3) {
            //stall at least till the current branch finishes
            effective_t[BRANCH] += 1;
            cycles += 1;
         }

      }
      // GR producer
      else if (my_rand() <= PG) {

         n_gr_produced++;
         cycles2 = cycles;

         den = total_int - (double) D_INTS;

         // Checking to see if this is a Special int instruction that stall the pipe
         // If this is an integer instruction that is executed by the FGU
         // If this is an integer multiply or popc or  pdist instructions
         if (my_rand() <= (double) PB_6_FGU_N / den) {
            latency = 8;        // micro-benchmarks say it's 8-10 cycles
            reason = INT_DEP;
            cycles += latency;  // stall
            effective_t[reason] += latency;     // Accumulates STALL cycles

         }
         // If this is an integer divide instruction
         else if (my_rand() <= (double) PB_30_FGU_N / (den - PB_6_FGU_N)) {
            latency = 30;
            reason = INT_DEP;
            cycles += latency;  // stall
            effective_t[reason] += latency;     // Accumulates STALL cycles
         }
         // Now we check if this is a special INT instruction that stalls the pipe
         // These instructions are save, saved, restore, restored
         else if (my_rand() <=
               (double) PB_6_INT_N / (den - PB_6_FGU_N - PB_30_FGU_N)) {
            latency = 7;        // 6 in documentation 7 in microbenchmarks
            reason = INT_DEP;
            cycles += latency;  // stall
            effective_t[reason] += latency;     // Accumulates STALL cycles
         } else if (my_rand() <=
               (double) PB_25_INT_N / (den - PB_6_INT_N - PB_6_FGU_N -
               PB_30_FGU_N)) {
            latency = 25;
            reason = INT_DEP;
            cycles += latency;  // stall
            effective_t[reason] += latency;     // Accumulates STALL cycles
         } else {
            latency = 0;        // Latency of Simple INT instructions is reflected in CPIi
         }

         // how far is the consumer?
         dep_distance = sample_hist(int_use_hist, INT_USE_HIST_LENGTH);
         // Add this to the dependency chain
         //fprintf(stderr,"dependence distance: %lu\n", dep_distance);
         add_dependency(i + dep_distance, cycles2 + latency, INT_USE_DEP);

      }
      // Now it must be an FP instruction
      // NOTE: using micro-benchmarks, it turns out that fdiv and fsqrt are blocking
      // that is, they will use all of their latency before allowing any instruction
      // to go thru the pipe. This is unlike what the documentation says !

      else if (my_rand() <= PF) {

         n_fr_produced++;

         cycles2 = cycles;
         // how far is the consumer?
         dep_distance = sample_hist(fp_use_hist, FP_USE_HIST_LENGTH);
         // how far is the next fp instruction?
         next_fp = sample_hist(fp_fp_hist, FP_FP_HIST_LENGTH);

         den = total_fp - (double) D_FLOATS;

         if (my_rand() <= (double) P_FDIV_FSQRT_S_N / (den)) {
            if (last_fdiv == (i - 1))
               latency = 23;    // 23 fdiv_followed by fdiv
            else if (next_fp == 1 || dep_distance == 1)
               latency = 22;    // 22
            else
               latency = 21;    // 21
            cycles += latency;  // stall
            effective_t[FGU_DEP] += latency;
            last_fdiv = i;

         }

         else if (my_rand() <=
               (double) P_FDIV_FSQRT_D_N / (den - P_FDIV_FSQRT_S_N)) {

            if (last_fdiv == (i - 1))
               latency = 37;    // 37
            else if (dep_distance == 1)
               latency = 36;    //36
            else
               latency = 35;    // 35
            cycles += latency;  // stall
            effective_t[FGU_DEP] += latency;
            last_fdiv = i;

         }
         // All other FGU instructions
         else {
            if (dep_distance <= 4 && next_fp <= 4) {
               latency = FGU_LATENCY - 2;       // 5 cycles ==> forwarding
               cycles += latency;       // stall
               effective_t[FGU_DEP] += latency;
            } else
               latency = 0;     // will take 1 cycle which is reflected in CPIi

         }

         // Add this to the dependency chain
         add_dependency(i + dep_distance, cycles2 + latency, FGU_DEP);

      }                         // end else

      else {
         fprintf(stderr, "ERROR: no instruction type selected!!!\n");
      }

      // termination check
      if (i % 1000000 == 0) {

         printf(".");
         fflush(stdout);
         cur_cpi = cycles / (i + i2);

         // Is this an early finish?
         if (fabs(cur_cpi - last_cpi) < THRESHOLD) {
            break;
         }

         last_cpi = cur_cpi;
      }

   }

   printf("\n\nTotal Instructions Simulated: %llu (%llu,%llu)\n", i+i2, i, i2);
   
   //Let's correct i by adding in the number of instructions executed in the delay slot
   i += i2;

   printf("\n");
   printf("Mem Ops      = %10llu ( %4.2f%% of all tokens )\n", n_memops,
         100.0 * n_memops / i);
   printf("Loads        = %10llu ( %4.2f%% of all tokens )\n", n_loads,
         100.0 * n_loads / i);
   printf("Stores       = %10llu ( %4.2f%% of all tokens )\n", n_stores,
         100.0 * n_stores / i);
   printf("Branches     = %10llu ( %4.2f%% of all tokens )\n", n_branches,
         100.0 * n_branches / i);
   printf("FR producers = %10llu ( %4.2f%% of all tokens )\n", n_fr_produced,
         100.0 * n_fr_produced / i);
   printf("GR producers = %10llu ( %4.2f%% of all tokens )\n", n_gr_produced,
         100.0 * n_gr_produced / i);

   printf("\n\n");
   printf("Loads to L1  = %10llu (%4.2f%% of loads, %4.2f%% of all tokens)\n",
         n_l1, 100.0 * n_l1 / n_loads, 100.0 * n_l1 / i);
   printf("Loads to L2  = %10llu (%4.2f%% of loads, %4.2f%% of all tokens)\n",
         n_l2, 100.0 * n_l2 / n_loads, 100.0 * n_l2 / i);
   printf("Loads to Mem = %10llu (%4.2f%% of loads, %4.2f%% of all tokens)\n",
         n_mem, 100.0 * n_mem / n_loads, 100.0 * n_mem / i);
   printf("DTLB miss    = %10llu (%4.2f%% of loads )\n", n_tlb,
         100.0 * n_tlb / n_loads);
   printf("I$ misses    = %10llu (%4.2f%% of tokens)\n", n_icache_misses,
         100.0 * n_icache_misses / i);

   printf("\n\n");
   printf("Pipeline Flushes     = %10llu (%4.2f%% of tokens  )\n",
         n_pipe_flushes, 100.0 * n_pipe_flushes / i);
   printf("Loads from ST Buffer = %10llu (%4.2f%% of loads   )\n",
         n_stb_reads, 100.0 * n_stb_reads / n_loads);
   printf("ST Buffer Full Stalls= %10llu (%4.2f%% of stores  )\n", n_stb_full,
         100.0 * n_stb_full / (n_stores));

   double br_cp,
          int_cp,
          fp_cp,
          L1_cp,
          L2_cp,
          IC_cp,
          MEM_cp,
          flush_cp,
          stb_cp,
          int_use_cp,
          int_dsu_cp,
          tot_cp;

   br_cp = effective_t[BRANCH] / i;
   int_cp = effective_t[INT_DEP] / i;
   int_use_cp = effective_t[INT_USE_DEP] / i;
   int_dsu_cp = effective_t[INT_DSU_DEP] / i;
   fp_cp = effective_t[FGU_DEP] / i;
   L1_cp = effective_t[L1_CACHE] / i;
   L2_cp = effective_t[L2_CACHE] / i;
   MEM_cp = effective_t[MEMORY] / i;
   IC_cp = effective_t[I_CACHE] / i;
   stb_cp = effective_t[STB_FULL] / i;
   flush_cp = effective_t[P_FLUSH] / i;

   tot_cp =
         CPIi + br_cp + int_cp + fp_cp + L1_cp + L2_cp + MEM_cp + IC_cp +
         flush_cp + stb_cp + int_use_cp + int_dsu_cp;
   p_cpi = tot_cp;

   //printf("\nEffective L1 is %f \n", effective_t[L1_CACHE]);
   printf("\n\n");

   printf("CPI Components:\n");
   printf("CPIi    =\t%4.5f\t(%4.02f%% of CPI)\n", CPIi, 100 * CPIi / tot_cp);
   printf("P_Flush =\t%4.5f\t(%4.02f%% of CPI)\n", flush_cp,
         100 * flush_cp / tot_cp);
   printf("STB_FULL=\t%4.5f\t(%4.02f%% of CPI)\n", stb_cp,
         100 * stb_cp / tot_cp);
   printf("Branch  =\t%4.5f\t(%4.02f%% of CPI)\n", br_cp,
         100 * br_cp / tot_cp);
   printf("INT_DEP =\t%4.5f\t(%4.02f%% of CPI)\n", int_cp,
         100 * int_cp / tot_cp);
   printf("INT_USE_DEP =\t%8.9f\t(%4.02f%% of CPI)\n", int_use_cp,
         100 * int_use_cp / tot_cp);
   printf("INT_DSU_DEP =\t%4.5f\t(%4.02f%% of CPI)\n", int_dsu_cp,
         100 * int_dsu_cp / tot_cp);
   printf("FGU_DEP =\t%4.5f\t(%4.02f%% of CPI)\n", fp_cp,
         100 * fp_cp / tot_cp);
   printf("I_CACHE =\t%4.5f\t(%4.02f%% of CPI)\n", IC_cp,
         100 * IC_cp / tot_cp);
   printf("L1_CACHE=\t%4.5f\t(%4.02f%% of CPI)\n", L1_cp,
         100 * L1_cp / tot_cp);
   printf("L2_CACHE=\t%4.5f\t(%4.02f%% of CPI)\n", L2_cp,
         100 * L2_cp / tot_cp);
   printf("MEMORY  =\t%4.5f\t(%4.02f%% of CPI)\n", MEM_cp,
         100 * MEM_cp / tot_cp);
   printf("TOT CPI =\t%4.5f\t(%4.02f%% of CPI)\n", tot_cp,
         100 * tot_cp / tot_cp);

   printf("\n\nCompare To REAL MEASUREMENTS Below:");
   printf("\nLD  = %4.2f", (double) LD_PERC);
   printf("\nST  = %4.2f", (double) ST_PERC);
   printf("\nBR  = %4.2f", (double) BR_PERC);
   printf("\nFP  = %4.2f", (double) FP_PERC);
   printf("\nGR  = %4.2f", (double) GR_PERC);

   printf("\n\nResults Summary: ");
   printf("\nMeasured CPI  = %4.5f", MEASURED_CPI);
   printf("\nPredicted CPI = %4.5f", p_cpi);
   printf("\nDifference    = %4.5f%%\n\n",
         100 * (p_cpi - MEASURED_CPI) / MEASURED_CPI);

   un_init();
   return 0;
}

void handle_delay_slot(unsigned long long i,
      enum stall_reason *last_ld_reason_ptr, double *last_ld_satisfied_ptr,
      double *fdiv_allowed_ptr)
{

   double when_satisfied;
   double latency;
   double cycles2,
          den;
   int dep_distance;
   enum stall_reason where;
   enum stall_reason reason;
   enum stall_reason last_ld_reason = *last_ld_reason_ptr;
   double last_ld_satisfied = *last_ld_satisfied_ptr;
   double fdiv_allowed = *fdiv_allowed_ptr;
   unsigned long long next_load,
        next_st,
        next_fp;
   double ic_p2 = 0,
         ic_pm = 0,
         ic_p1 = 0,
         i_miss = 0,
         ITLB_P = 0,
         istall = 0;

   // calculating IC miss rates
   i_miss = (double) 1.0 *(IC_MISSES) / total_instructions;
   ic_p2 = (double) 1.0 *(IC_MISSES - L2_I_MISSES) / IC_MISSES;
   ic_pm = (double) 1.0 *(L2_I_MISSES) / IC_MISSES;

   ic_p1 = 1.0 - (ic_p2 + ic_pm);
   ITLB_P = ((double) 1.0 * ITLB_MISSES / IC_MISSES);

   // --------------------------------------------------------------- STALL check: start

   // Deos this instruction miss the I$
   // We assume that the Instruction Cache does not pose any stalls unless it's missed
   if (my_rand() <= i_miss) {
      if (my_rand() <= ic_p1) {
         // TLB miss?
         if (my_rand() <= ITLB_P)
            istall += TT;
         where = I_CACHE;
      } else if (my_rand() <= ic_p2) {
         // TLB miss?
         if (my_rand() <= ITLB_P)
            istall += TT;
         where = I_CACHE;
         istall += T2;
      } else if (my_rand() <= ic_pm) {
         // TLB miss?
         if (my_rand() <= ITLB_P)
            istall += TT;
         where = I_CACHE;
         istall += MM;
      }
   }
   // not a miss
   else {
      istall = 0;               // resetting the value
   }

   if (istall != 0) {
      cycles += istall;
      effective_t[where] += istall;
      n_icache_misses++;
      // Whenever the I$ is missed, the pipe is flushed for that thread
      n_pipe_flushes++;
      cycles += 4;
      effective_t[P_FLUSH] += 4;
      // JEC: do not adjust dependencies because the producers are
      // not delayed, only the consumers
      //adjust_dependence_chain(4);
      istall = 0;               // reset istall value to zero
   }
   // Is this instruction depndent on a previous instruction
   when_satisfied = is_dependent(i, &where);

   // Yes. Not only is it dependent, we also need to STALL
   if (when_satisfied > cycles) {
      effective_t[where] += (when_satisfied - cycles);
      cycles = when_satisfied;
   }
   // --------------------------------------------------------------- STALL check: done

   if (my_rand() <= PLD_D) {

      n_memops++;
      n_loads++;
      cycles2 = cycles;

      // First, check if there's another load still there in the q
      // If there's ,then stall until it's done
      scan_memq();
      if (ld_in_q > 0) {
         effective_t[last_ld_reason] += (last_ld_satisfied - cycles);
         cycles = last_ld_satisfied;    // stall
      }
      // The following call, will re-organize the mem q appropriately
      scan_memq();

      // How far is the consumer?
      dep_distance = sample_hist(ld_use_hist, LD_USE_HIST_LENGTH);
      // How far is the next load?
      next_load = sample_hist(ld_ld_hist, LD_LD_HIST_LENGTH);
      // How long to satisfy this load?
      latency =
            cycles_to_serve_load(&where, 0, dep_distance - 1, next_load - 1);

      // is this load getting its data from the store buffer?
      if (i == next_ld) {
         n_stb_reads++;
         latency = 2;
      }
      // Check Special Memory instructions that stall the pipe
      den = 0.00001 + (double) D_LOADS; // all int instructions in the delay slot

      if (my_rand() <= (double) PB_6_MEM_D_N / den) {
         latency = 6;
         reason = where;
         cycles += latency;     // stall
         effective_t[where] += latency; // Accumulates STALL cycles

      } else if (my_rand() <= (double) PB_25_MEM_D_N / (den - PB_6_MEM_D_N)) {
         latency = 25;
         reason = where;
         cycles += latency;     // stall
         effective_t[where] += latency; // Accumulates STALL cycles
      }

      else if (my_rand() <=
            (double) PB_3_LD_D_N / (den - PB_6_MEM_D_N - PB_25_MEM_D_N)) {
         latency = 3;
         reason = where;
         cycles += latency;     // stall
         effective_t[where] += latency; // Accumulates STALL cycles

      } else {
         reason = where;
         cycles += latency;     // stall
         effective_t[where] += latency; // Accumulates STALL cycles
      }

      //Add this load to mem queue
      add_memq(cycles2 + latency, IS_LOAD);

      // Add this to the dependency chain
      add_dependency(i + dep_distance, cycles2 + latency, where);
      last_ld_satisfied = cycles2 + latency;
      last_ld_reason = where;

      // pipeline flush
      if (latency > T1) {
         n_pipe_flushes++;
      }

   }

   else if (my_rand() <= PST_D) {

      n_memops++;
      n_stores++;

      next_st = sample_hist(st_st_hist, ST_ST_HIST_LENGTH);
      next_ld = sample_hist(st_ld_hist, ST_LD_HIST_LENGTH);

      if (next_ld < 8)          // only consider stores that have been in the STB
         next_ld += i;

      // check if STB is full ==> See Page 75
      if (st_in_q >= MAX_STB_ENTRIES || (next_st == 1 &&
            st_in_q >= MAX_STB_ENTRIES - 1)) {
         n_stb_full++;
         cycles += 2;
         effective_t[STB_FULL] += 2;

      }
      // Add to mem_q if necessary
      serve_store();
   }
   // GR producer
   else if (my_rand() <= PG_D) {

      n_gr_produced++;
      cycles2 = cycles;
      den = 0.00001 + (double) D_INTS;  // all int instructions in the delay slot

      // Checking to see if this is a Special int instruction that stall the pipe
      // If this is an integer instruction that is executed by the FGU
      // If this is an integer multiply or popc or  pdist instructions
      if (my_rand() <= (double) PB_6_FGU_D_N / den) {
         latency = 8;           // 6 in documentation and 8-10 in microbenchmarks
         reason = INT_DEP;
         cycles += latency;     // stall
         effective_t[reason] += latency;        // Accumulates STALL cycles

      }
      // If this is an integer divide instruction
      else if (my_rand() <= (double) PB_30_FGU_D_N / (den - PB_6_FGU_D_N)) {
         latency = 30;
         reason = INT_DEP;
         cycles += latency;     // stall
         effective_t[reason] += latency;        // Accumulates STALL cycles
      }
      // Now we check if this is a special INT instruction that stalls the pipe
      // These instructions are save, saved, restore, restored
      else if (my_rand() <=
            (double) PB_6_INT_D_N / (den - PB_6_FGU_D_N - PB_30_FGU_D_N)) {
         latency = 7;           // 6 in documentation 7 in microbenchmarks
         reason = INT_DEP;
         cycles += latency;     // stall
         effective_t[reason] += latency;        // Accumulates STALL cycles
      } else if (my_rand() <=
            (double) PB_25_INT_D_N / (den - PB_6_INT_D_N - PB_6_FGU_D_N -
            PB_30_FGU_D_N)) {
         latency = 25;
         reason = INT_DEP;
         cycles += latency;     // stall
         effective_t[reason] += latency;        // Accumulates STALL cycles
      } else {
         latency = 0;
      }

      // how far is the consumer?
      dep_distance = sample_hist(int_use_hist, INT_USE_HIST_LENGTH);
      // Add this to the dependency chain
      add_dependency(i + dep_distance, cycles2 + latency, INT_DSU_DEP);

   }
   // Now it must be an FP instruction
   else if (my_rand() <= PF_D) {

      n_fr_produced++;

      cycles2 = cycles;
      // how far is the consumer?
      dep_distance = sample_hist(fp_use_hist, FP_USE_HIST_LENGTH);
      // how far is the next fp instruction?
      next_fp = sample_hist(fp_fp_hist, FP_FP_HIST_LENGTH);

      den = 0.00001 + (double) D_FLOATS;

      if (my_rand() <= (double) P_FDIV_FSQRT_S_D_N / (den)) {
         if (last_fdiv == (i - 1))
            latency = 23;       // 23 fdiv_followed by fdiv
         else if (next_fp == 1 || dep_distance == 1)
            latency = 22;       // 22
         else
            latency = 21;       // 21
         cycles += latency;     // stall
         effective_t[FGU_DEP] += latency;
         last_fdiv = i;

      } else if (my_rand() <=
            (double) P_FDIV_FSQRT_D_D_N / (den - P_FDIV_FSQRT_S_D_N)) {
         if (last_fdiv == (i - 1))
            latency = 37;       // 37
         else if (dep_distance == 1)
            latency = 36;       //36
         else
            latency = 35;       // 35
         cycles += latency;     // stall
         effective_t[FGU_DEP] += latency;
         last_fdiv = i;

      }
      // All other FGU instructions
      else {
         if (dep_distance <= 4 && next_fp <= 4) {
            latency = FGU_LATENCY - 2;  // 5 cycles ==> forwarding
            cycles += latency;  // stall
            effective_t[FGU_DEP] += latency;
         } else
            latency = 0;        // will take 1 cycle which is reflected in CPIi

      }
      // Add this to the dependency chain
      add_dependency(i + dep_distance, cycles2 + latency, FGU_DEP);

   }                            // end else

   else {
      fprintf(stderr, "ERROR: no instruction selected in delay slot!!!\n");
   }

   // update stuff
   *last_ld_reason_ptr = last_ld_reason;
   *last_ld_satisfied_ptr = last_ld_satisfied;
   *fdiv_allowed_ptr = fdiv_allowed;

   return;

}                               // end function handle_delay_slot
