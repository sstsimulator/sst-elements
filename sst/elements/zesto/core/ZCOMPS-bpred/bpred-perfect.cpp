/* bpred-perfect.cpp: Oracle/perfect predictor 
   NOTE: for branch direction only (not target) */
/*
 * __COPYRIGHT__ GT
 */

#define COMPONENT_NAME "perfect"

#ifdef BPRED_PARSE_ARGS
if(!strcasecmp(COMPONENT_NAME,type))
{
  return new bpred_perfect_t();
}
#else

class bpred_perfect_t:public bpred_dir_t
{
  public:

  bpred_perfect_t()
  {
    init();

    name = strdup(COMPONENT_NAME);
    if(!name)
      fatal("couldn't malloc perfect name (strdup)");
    type = strdup("perfect/oracle");
    if(!type)
      fatal("couldn't malloc perfect name (strdup)");

    bits = 0;
  }

  /* LOOKUP */
  BPRED_LOOKUP_HEADER
  {
    BPRED_STAT(lookups++;)
    scvp->updated = false;
    return outcome;
  }

};

#endif /* BPRED_PARSE_ARGS */
#undef COMPONENT_NAME
