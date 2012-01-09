/* fusion-random.cpp: randomly choose an outcome from the component predictors */
/*
 * __COPYRIGHT__ GT
 */

#define COMPONENT_NAME "random"

#ifdef BPRED_PARSE_ARGS
if(!strcasecmp("random",type))
{
  return new fusion_random_t(num_pred);
}
#else

class fusion_random_t:public fusion_t
{
  public:

  /* CREATE */
  fusion_random_t(const int arg_num_pred)
  {
    init();

    name = strdup(COMPONENT_NAME);
    if(!name)
      fatal("couldn't malloc fusion random name (strdup)");
    type = strdup("random Selection");
    if(!type)
      fatal("couldn't malloc fusion random type (strdup)");

    num_pred = arg_num_pred;

    bits = 0;
  }

  /* LOOKUP */
  FUSION_LOOKUP_HEADER
  {
    bool pred = preds[random()%num_pred];
    BPRED_STAT(lookups++;)
    scvp->updated = false;
    return pred;
  }
};

#endif /* BPRED_PARSE_ARGS */
#undef COMPONENT_NAME
