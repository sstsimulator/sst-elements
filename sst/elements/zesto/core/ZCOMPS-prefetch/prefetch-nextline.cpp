/* prefetch-nextline.cpp: Simple prefetch the next cacheline based on the physical address.
   NOTE: this will cross page boundaries. */
/*
 * __COPYRIGHT__ GT
 */
#define COMPONENT_NAME "nextline"

#ifdef PREFETCH_PARSE_ARGS
if(!strcasecmp(COMPONENT_NAME,type))
{
  return new prefetch_nextline_t(cp);
}
#else

class prefetch_nextline_t:public prefetch_t
{
  public:
  /* CREATE */
  prefetch_nextline_t(struct cache_t * arg_cp)
  {
    init();

    type = strdup("nextline");
    if(!type)
      fatal("couldn't calloc nextline-prefetch type (strdup)");

    bits = 0;
    cp = arg_cp;
  }

  /* LOOKUP */
  PREFETCH_LOOKUP_HEADER
  {
    lookups++;

    /* just return next cache line */
    return (paddr + cp->linesize) & ~(cp->linesize-1);
  }
};


#endif /* PREFETCH_PARSE_ARGS */
#undef COMPONENT_NAME
