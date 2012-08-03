#ifndef POWERCOUNTER__H
#define POWERCOUNTER__H



class PowerCounter {

public:

	PowerCounter(){};



  uint64_t             bnk_pre;    /* pecentage of time all banks are precharged */
  uint64_t             cke_hi_pre;
  uint64_t             cke_hi_act;
  tick_t          last_RAS_time; /* last time we issue RAS command  */
  tick_t          last_CAS_time; /* last time we issue CAS command  */
  tick_t          last_CAW_time; /* last time we issue CAW command  */
  uint64_t             RAS_count; /* How many times we isssue RAS, CAS, and CAW */
  uint64_t             RAS_sum;   /* summation of all the RAS command cycles */
  tick_t             current_CAS_cycle, current_CAW_cycle; /* cycles of current CAS/CAW command in current RAS period */
  tick_t           sum_per_RD, sum_per_WR;    /* summation of the CAS/CAW cycles in current print period  */
  uint64_t             dram_access;
  uint64_t             refresh_count;
  tick_t			  amb_busy;
  tick_t			  amb_busy_spill_over;
  tick_t			  amb_data_pass_through;
  tick_t			  amb_data_pass_through_spill_over;
  tick_t 		  busy_till;
  tick_t		  busy_last;



};

#endif

