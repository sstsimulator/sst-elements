/* bpred-btfnt.cpp: Static BTFNT (backwards-taken, forward-not-taken)
   predictor [Ball and Larus, PPoPP 1993] */
/*
 * __COPYRIGHT__ GT
 */

#define COMPONENT_NAME "btfnt"

#ifdef BPRED_PARSE_ARGS
if(!strcasecmp(COMPONENT_NAME,type))
{
  return new bpred_btfnt_t();
}
#else

class bpred_btfnt_t:public bpred_dir_t
{
  public:

  /* CREATE */
  bpred_btfnt_t()
  {
    init();

    name = strdup(COMPONENT_NAME);
    if(!name)
      fatal("couldn't malloc BTFNT name (strdup)");
    type = strdup("static btfnt");
    if(!type)
      fatal("couldn't malloc BTFNT type (strdup)");

    bits = 0;
  }

  /* LOOKUP */
  BPRED_LOOKUP_HEADER
  {
    BPRED_STAT(lookups++;)
    scvp->updated = false;
    return (tPC < PC);
  }

};

#endif /* BPRED_PARSE_ARGS */
#undef COMPONENT_NAME
