/* btb-perfect.cpp: Perfect target prediction (not including RAS) */
/*
 * __COPYRIGHT__ GT
 */

#define COMPONENT_NAME "perfect"

#ifdef BPRED_PARSE_ARGS
if(!strcasecmp(COMPONENT_NAME,type))
{
  return new BTB_perfect_t();
}
#else

class BTB_perfect_t:public BTB_t
{
  public:
  /* CREATE */
  BTB_perfect_t(void)
  {
    init();

    name = strdup(COMPONENT_NAME);
    if(!name)
      fatal("couldn't malloc perfect name (strdup)");

    type = strdup(COMPONENT_NAME);
  }

  /* DESTROY */
  ~BTB_perfect_t()
  {
    free(name); name = NULL;
    free(type); type = NULL;
  }

  /* LOOKUP */
  BTB_LOOKUP_HEADER
  {
    BPRED_STAT(lookups++;)
    scvp->updated = false;
    if(our_pred && outcome) /* if pred taken, and actually taken */
      return oPC;
    else if(our_pred)
      return tPC;
    else
      return 0;
  }

  /* UPDATE */
  BTB_UPDATE_HEADER
  {
    if(!scvp->updated)
    {
      BPRED_STAT(updates++;)
      BPRED_STAT(num_hits += oPC == our_target;)
      scvp->updated = true;
    }
  }

};

#endif /* BTB_PARSE_ARGS */
#undef COMPONENT_NAME
