/* bpred-random.cpp: Random (50-50) predictor */
/*
 * __COPYRIGHT__ GT
 */

#define COMPONENT_NAME "random"

#ifdef BPRED_PARSE_ARGS
if(!strcasecmp(COMPONENT_NAME,type))
{
  return new bpred_random_t();
}
#else

struct bpred_random_t:public bpred_dir_t
{
  protected:

  zcounter_t num_taken;

  public:

  /* CREATE */
  bpred_random_t()
  {
    init();

    name = strdup(COMPONENT_NAME);
    if(!name)
      fatal("couldn't malloc random name (strdup)");
    type = strdup("completely random");
    if(!type)
      fatal("couldn't malloc random name (strdup)");

    bits = 0;
  }

  /* LOOKUP */
  BPRED_LOOKUP_HEADER
  {
    int pred = random()&1;
    BPRED_STAT(lookups++;)
    scvp->updated = false;
    return pred;
  }

  /* UPDATE */
  BPRED_UPDATE_HEADER
  {
    if(!scvp->updated)
    {
      if(outcome)
        BPRED_STAT(num_taken++;)
      BPRED_STAT(updates++;)
      BPRED_STAT(num_hits += our_pred == outcome;)
      scvp->updated = true;
    }
  }

  /* REG_STATS */
  BPRED_REG_STATS_HEADER
  {
    bpred_dir_t::reg_stats(sdb,core);

    int id = core?core->current_thread->id:0;
    char buf[256];
    char buf2[256];

    sprintf(buf,"c%d.%s.num_taken",id,name);
    sprintf(buf2,"number of taken branch predictions by %s",name);
    stat_reg_counter(sdb, true, buf, buf2, &num_taken, num_taken, NULL);
  }

  /* RESET_STATS */
  BPRED_RESET_STATS_HEADER
  {
    bpred_dir_t::reset_stats();
    num_taken = 0;
  }

};

#endif /* BPRED_PARSE_ARGS */
#undef COMPONENT_NAME
