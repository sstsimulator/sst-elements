/* prefetch-IP.cpp: IP-based (PC) prefetcher [http://download.intel.com/technology/architecture/sma.pdf] */
/*
 * __COPYRIGHT__ GT
 */
#define COMPONENT_NAME "IP"


#ifdef PREFETCH_PARSE_ARGS
if(!strcasecmp(COMPONENT_NAME,type))
{
  int num_entries;
  int last_addr_bits;
  int last_stride_bits;
  int last_prefetch_bits;
  if(sscanf(opt_string,"%*[^:]:%d:%d:%d:%d",&num_entries,&last_addr_bits,&last_stride_bits,&last_prefetch_bits) != 4)
    fatal("bad %s prefetcher options string %s (should be \"IP:num_entries:addr-bits:stride-bits:last-PF-bits\")",cp->name,opt_string);
  return new prefetch_IP_t(cp,num_entries,last_addr_bits,last_stride_bits,last_prefetch_bits);
}
#else

class prefetch_IP_t:public prefetch_t
{
  protected:
  int num_entries;
  int mask;

  int last_addr_bits;
  int last_stride_bits;
  int last_prefetch_bits;

  int last_addr_mask;
  int last_stride_mask;
  int last_prefetch_mask;

  struct prefetch_IP_table_t {
    md_paddr_t last_paddr;
    int last_stride;
    my2bc_t conf;
    md_paddr_t last_prefetch;
  } * table;

  public:
  /* CREATE */
  prefetch_IP_t(struct cache_t * arg_cp,
                int arg_num_entries,
                int arg_last_addr_bits,
                int arg_last_stride_bits,
                int arg_last_prefetch_bits)
  {
    init();

    char name[256]; /* just used for error checking macros */

    type = strdup("IP");
    if(!type)
      fatal("couldn't calloc IP-prefetch type (strdup)");

    cp = arg_cp;
    sprintf(name,"%s.%s",cp->name,type);

    CHECK_PPOW2(arg_num_entries);
    num_entries = arg_num_entries;
    mask = num_entries - 1;

    last_addr_bits = arg_last_addr_bits;
    last_stride_bits = arg_last_stride_bits;
    last_prefetch_bits = arg_last_prefetch_bits;

    last_addr_mask = (1<<last_addr_bits)-1;
    last_stride_mask = (1<<last_stride_bits)-1;
    last_prefetch_mask = (1<<last_prefetch_bits)-1;

    table = (struct prefetch_IP_t::prefetch_IP_table_t*) calloc(num_entries,sizeof(*table));
    if(!table)
      fatal("couldn't calloc IP-prefetch table");

    bits = num_entries * (last_addr_bits + last_stride_bits + last_prefetch_bits + 2);
    assert(arg_cp);
  }

  /* DESTROY */
  ~prefetch_IP_t()
  {
    free(table);
    table = NULL;
  }

  /* LOOKUP */
  PREFETCH_LOOKUP_HEADER
  {
    lookups++;

    int index = PC & mask;
    md_paddr_t pf_paddr = 0;

    /* train entry first */
    md_paddr_t last_paddr = (paddr&~last_addr_mask) | table[index].last_paddr;
    int this_stride = (paddr - last_paddr) & last_stride_mask;

    if(this_stride == table[index].last_stride)
      MY2BC_UPDATE(table[index].conf,true);
    else
      table[index].conf = MY2BC_STRONG_NT;

    table[index].last_paddr = paddr&last_addr_mask;
    table[index].last_stride = this_stride;

    /* only make a prefetch if confident */
    if(table[index].conf == MY2BC_STRONG_TAKEN)
    {
      pf_paddr = paddr + this_stride;
      if((pf_paddr & last_prefetch_mask) == table[index].last_prefetch)
        pf_paddr = 0; /* don't keep prefetching the same thing */
      table[index].last_prefetch = pf_paddr & last_prefetch_mask;
    }

    return pf_paddr;
  }
};


#endif /* PREFETCH_PARSE_ARGS */
#undef COMPONENT_NAME
