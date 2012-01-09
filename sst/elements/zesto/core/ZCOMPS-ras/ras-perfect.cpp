/* ras-perfetch.cpp: Oracle return address predictor */
/*
 * __COPYRIGHT__ GT
 */

#define COMPONENT_NAME "perfect"

#ifdef BPRED_PARSE_ARGS
if(!strcasecmp(COMPONENT_NAME,type))
{
  return new RAS_perfect_t();
}
#else

class RAS_perfect_t:public RAS_t
{
  public:
  /* CREATE */
  RAS_perfect_t(void)
  {
    init();

    name = strdup("RAS");
    if(!name)
      fatal("couldn't malloc perfect name (strdup)");

    type = strdup(COMPONENT_NAME);
    if(!type)
      fatal("couldn't malloc perfect type (strdup)");
  }

  /* DESTROY */
  ~RAS_perfect_t()
  {
    if(name) free(name); name = NULL;
    if(type) free(type); type = NULL;
  }

  /* POP */
  RAS_POP_HEADER
  {
    return oPC;
  }

  /* RECOVER */
  RAS_RECOVER_HEADER
  {
    BPRED_STAT(num_recovers++;)
  }
};

#endif /* RAS_PARSE_ARGS */
#undef COMPONENT_NAME
