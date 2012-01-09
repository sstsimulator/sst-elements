/* prefetch-context.cpp: Context-based prefetcher, similar to markov/correlating prefetchers [Joseph and Grunwald, ISCA 1997] */
/*
 * __COPYRIGHT__ GT
 */
#define COMPONENT_NAME "context"


#ifdef PREFETCH_PARSE_ARGS
if(!strcasecmp(COMPONENT_NAME,type))
{
  int num_entries;
  int tag_bits;
  int next_bits;
  int conf_bits;
  int u_bits;
  if(sscanf(opt_string,"%*[^:]:%d:%d:%d:%d:%d",&num_entries,&tag_bits,&next_bits,&conf_bits,&u_bits) != 5)
    fatal("bad %s prefetcher options string %s (should be \"context:num_entries:tag-bits:next-bits:conf-bits:u-bits\")",cp->name,opt_string);
  return new prefetch_context_t(cp,num_entries,tag_bits,next_bits,conf_bits,u_bits);
}
#else

class prefetch_context_t:public prefetch_t
{
  protected:
  int num_entries;
  int mask;

  int tag_bits;
  int next_bits;
  int conf_bits;
  int u_bits;

  md_paddr_t tag_mask;
  md_paddr_t next_mask;
  int conf_max;
  int u_max;

  struct prefetch_context_table_t {
    md_paddr_t tag;
    md_paddr_t next_paddr;
    int conf;
    int u;
  } * table;

  md_paddr_t last_paddr;

  public:

  /* CREATE */
  prefetch_context_t(struct cache_t * arg_cp,
                    int arg_num_entries,
                    int arg_tag_bits,
                    int arg_next_bits,
                    int arg_conf_bits,
                    int arg_u_bits)
  {
    init();

    last_paddr = 0;

    char name[256]; /* just used for error checking macros */
    type = strdup("context");
    if(!type)
      fatal("couldn't calloc context-prefetch type (strdup)");

    cp = arg_cp;
    sprintf(name,"%s.%s",cp->name,type);

    CHECK_PPOW2(arg_num_entries);
    num_entries = arg_num_entries;
    mask = num_entries - 1;

    tag_bits = arg_tag_bits;
    next_bits = arg_next_bits;
    conf_bits = arg_conf_bits;
    u_bits = arg_u_bits;

    tag_mask = ((1<<tag_bits)-1) << log_base2(num_entries);
    next_mask = (1<<next_bits)-1;
    conf_max = (1<<conf_bits)-1;
    u_max = (1<<u_bits)-1;

    table = (struct prefetch_context_t::prefetch_context_table_t*) calloc(num_entries,sizeof(*table));
    if(!table)
      fatal("couldn't calloc context-prefetch table");

    bits = num_entries * (tag_bits + next_bits + conf_bits + u_bits);
  }

  /* DESTROY */
  ~prefetch_context_t()
  {
    free(table);
    table = NULL;
  }

  /* LOOKUP */
  PREFETCH_LOOKUP_HEADER
  {
    lookups++;

    int index = paddr & mask;
    md_paddr_t pf_paddr = 0;

    /* do current lookup first */
    if((paddr & tag_mask) == table[index].tag)
    {
      /* hit: check if confident */
      if(table[index].conf >= conf_max)
        pf_paddr = (paddr&~next_mask) | (table[index].conf&next_mask);
    }

    /* train */
    index = last_paddr & mask;

    /* is it a hit? */
    if((last_paddr&tag_mask) == table[index].tag) /* hit */
    {
      /* each hit increments the useful counter */
      if(table[index].u < u_max)
        table[index].u++;

      /* increase confidence if we see the same "next" address */
      if((paddr & next_mask) == table[index].next_paddr)
      {
        if(table[index].conf < conf_max)
          table[index].conf ++;
      }
      else
      {
        table[index].next_paddr = paddr & next_mask;
        table[index].conf = 0;
        if(table[index].u > 0) /* also decrease usefulness */
          table[index].u --;
      }
    }
    else /* miss */
    {
      if(table[index].u > 0)
      {
        table[index].u --;
      }
      else /* this entry hasn't been useful lately, replace it */
      {
        table[index].u = u_max;
        table[index].conf = 0;
        table[index].tag = last_paddr & tag_mask;
        table[index].next_paddr = paddr;
      }
    }

    last_paddr = paddr;

    return pf_paddr;
  }
};



#endif /* PREFETCH_PARSE_ARGS */
#undef COMPONENT_NAME
