/* fusion-oracle.cpp: oracle selection; if the correct prediction
   exists among *any* of the component predictors, then this
   meta-predictor will select the correct one.  This is extremely
   optimistic as the only time this will mispredict is when *all*
   component predictions are wrong.  Obviously using a static
   taken predictor and static NT predictor with the fusion_oracle
   will result in perfect direction prediction. */
/*
 * __COPYRIGHT__ GT
 */

#define COMPONENT_NAME "oracle"

#ifdef BPRED_PARSE_ARGS
if(!strcasecmp("oracle",type) || !strcmp("perfect",type))
{
  return new fusion_oracle_t(num_pred);
}
#else

class fusion_oracle_t:public fusion_t
{
  public:

  /* CREATE */
  fusion_oracle_t(const int arg_num_pred)
  {
    init();

    name = strdup(COMPONENT_NAME);
    if(!name)
      fatal("couldn't malloc fusion oracle name (strdup)");

    type = strdup("oracle");
    if(!type)
      fatal("couldn't malloc fusion oracle type (strdup)");

    num_pred = arg_num_pred;

    bits = 0;
  }

  /* LOOKUP */
  FUSION_LOOKUP_HEADER
  {
    bool pred = !outcome;

    for(int i=0;i<num_pred;i++)
    {
      if(preds[i]==outcome)
      {
        pred = outcome;
        break;
      }
    }

    BPRED_STAT(lookups++;)
    scvp->updated = false;

    return pred;
  }

  /* UPDATE */

  /* generic n-component version */
  FUSION_UPDATE_HEADER
  {
    if(!scvp->updated)
    {
      BPRED_STAT(updates++;)
      BPRED_STAT(num_hits += our_pred == outcome;)
      scvp->updated = true;
    }
  }

};

#endif /* BPRED_PARSE_ARGS */
#undef COMPONENT_NAME
