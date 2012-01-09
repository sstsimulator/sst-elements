/* bpred-taken.cpp: Static always taken predictor */
/*
 * __COPYRIGHT__ GT
 */

#define COMPONENT_NAME "taken"

#ifdef BPRED_PARSE_ARGS
if(!strcasecmp(COMPONENT_NAME,type))
{
  return new bpred_taken_t();
}
#else

class bpred_taken_t:public bpred_dir_t
{
  public:

  /* CREATE */
  bpred_taken_t()
  {
    init();

    name = strdup(COMPONENT_NAME);
    if(!name)
      fatal("couldn't malloc taken name (strdup)");

    type = strdup("static taken");
    if(!type)
      fatal("couldn't malloc taken type (strdup)");

    bits = 0;
  }

  /* LOOKUP */
  BPRED_LOOKUP_HEADER
  {
    BPRED_STAT(lookups++;)
    scvp->updated = false;
    return 1;
  }

};

#endif /* BPRED_PARSE_ARGS */
#undef COMPONENT_NAME
