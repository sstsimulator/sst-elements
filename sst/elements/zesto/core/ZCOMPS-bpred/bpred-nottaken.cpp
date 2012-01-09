/* bpred-nottaken.cpp: Static always not-taken predictor */
/*
 * __COPYRIGHT__ GT
 */

#define COMPONENT_NAME "nottaken"

#ifdef BPRED_PARSE_ARGS
if(!strcasecmp(COMPONENT_NAME,type))
{
  return new bpred_nottaken_t();
}
#else

class bpred_nottaken_t:public bpred_dir_t
{
  public:

  /* CREATE */
  bpred_nottaken_t(void)
  {
    init();

    name = strdup(COMPONENT_NAME);
    if(!name)
      fatal("couldn't malloc nottaken name (strdup)");

    type = strdup("static not-taken");
    if(!type)
      fatal("couldn't malloc nottaken type (strdup)");

    bits = 0;
  }

  /* LOOKUP */
  BPRED_LOOKUP_HEADER
  {
    BPRED_STAT(lookups++;)
    scvp->updated = false;
    return 0;
  }
};
#endif /* BPRED_PARSE_ARGS */
#undef COMPONENT_NAME
