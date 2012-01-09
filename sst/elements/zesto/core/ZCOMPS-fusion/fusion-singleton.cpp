/* fusion-singleton.cpp: No-op meta-predictor used when there is only a single component predictor */
/*
 * __COPYRIGHT__ GT
 */

#define COMPONENT_NAME "none"

#ifdef BPRED_PARSE_ARGS
if(!strcasecmp("none",type))
{
  return new fusion_singleton_t();
}
#else

class fusion_singleton_t:public fusion_t
{
  public:

  fusion_singleton_t(void)
  {
    init();

    num_pred = 1;
    name = strdup(COMPONENT_NAME);
    if(!name)
      fatal("couldn't malloc fusion singleton name (strdup)");
    type = strdup("none");
    if(!type)
      fatal("couldn't malloc fusion singleton type (strdup)");

    bits = 0;
  }

  /* LOOKUP */
  FUSION_LOOKUP_HEADER
  {
    BPRED_STAT(lookups++;)
    scvp->updated = false;
    return preds[0];
  }

  /* REG_STATS */
  FUSION_REG_STATS_HEADER
  {
    /* nada: don't bother registering any stats when there's only one
       component predictors since the stats will be identical. */
  }
};

#endif /* BPRED_PARSE_ARGS */
#undef COMPONENT_NAME
