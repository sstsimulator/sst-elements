/* prefetch-stream.cpp: Stream-based prefetcher.  [See DPL/DCU description in the "Intel 64 and
   IA-32 Architecture Optimization Refernce Manual"] */
/*
 * __COPYRIGHT__ GT
 */
#define COMPONENT_NAME "stream"


#ifdef PREFETCH_PARSE_ARGS
if(!strcasecmp(COMPONENT_NAME,type))
{
  int num_upstream_entries;
  int num_downstream_entries;
  if(sscanf(opt_string,"%*[^:]:%d:%d",&num_upstream_entries,&num_downstream_entries) != 2)
    fatal("bad %s prefetcher options string %s (should be \"stream:num_upstream_entries:num_downstream_entries\")",cp->name,opt_string);
  return new prefetch_stream_t(cp,num_upstream_entries,num_downstream_entries);
}
#else

class prefetch_stream_t:public prefetch_t
{
  protected:
  int num_upstream_entries;
  int num_downstream_entries;

  struct prefetch_stream_table_t {
    md_paddr_t last_paddr;
    struct prefetch_stream_table_t * next;
  };

  struct prefetch_stream_table_t * upstream;
  struct prefetch_stream_table_t * downstream;
  struct prefetch_stream_table_t * upstream_head;
  struct prefetch_stream_table_t * downstream_head;

  public:
  /* CREATE */
  prefetch_stream_t(struct cache_t * arg_cp,
                int arg_num_upstream_entries,
                int arg_num_downstream_entries)
  {
    init();

    char name[256]; /* just used for error checking macros */

    type = strdup("stream");
    if(!type)
      fatal("couldn't calloc stream-prefetch type (strdup)");

    assert(arg_cp);
    cp = arg_cp;
    sprintf(name,"%s.%s",cp->name,type);

    num_upstream_entries = arg_num_upstream_entries;
    num_downstream_entries = arg_num_downstream_entries;

    upstream = (struct prefetch_stream_t::prefetch_stream_table_t*) calloc(num_upstream_entries,sizeof(*upstream));
    downstream = (struct prefetch_stream_t::prefetch_stream_table_t*) calloc(num_downstream_entries,sizeof(*downstream));
    if(!upstream || !downstream)
      fatal("couldn't calloc stream-prefetch table(s)");

    for(int i=0;i<num_upstream_entries-1;i++)
      upstream[i].next = &upstream[i+1];
    for(int i=0;i<num_downstream_entries-1;i++)
      downstream[i].next = &downstream[i+1];
    upstream_head = upstream;
    downstream_head = downstream;

    bits = (num_upstream_entries+num_downstream_entries) * (40-12); /* assume 40-bit physical addres, 4KB pages */
  }

  /* DESTROY */
  ~prefetch_stream_t()
  {
    free(upstream); upstream = upstream_head = NULL;
    free(downstream); downstream = downstream_head = NULL;
  }

  /* LOOKUP */
  PREFETCH_LOOKUP_HEADER
  {
    lookups++;

    md_paddr_t page_addr = paddr & ~(PAGE_SIZE-1);

    /* see if we can find a matching entry */
    struct prefetch_stream_table_t * p = upstream_head;
    struct prefetch_stream_table_t * prev = NULL;
    bool possible_up = false;
    bool possible_down = false;
    bool up_hit = false; /* page present */
    bool down_hit = false; /* page present */

    /* check upstreams */
    while(p)
    {
      if(page_addr == (p->last_paddr & ~(PAGE_SIZE-1))) /* hit */
      {
        up_hit = true;
        break;
      }
      prev = p;
      p = p->next;
    }

    if(p) /* hit */
    {
      if((paddr>>cp->addr_shift) == ((p->last_paddr>>cp->addr_shift)+1))
      {
        p->last_paddr = paddr; /* record this address */

        if(prev) /* entry to MRU position */
        {
          prev->next = p->next;
          p->next = upstream_head;
          upstream_head = p;
        }
        return paddr + (1<<cp->addr_shift);
      }
      else if((paddr>>cp->addr_shift) == ((p->last_paddr>>cp->addr_shift)-1))
      {
        /* possible downward-stream */
        possible_down = true;
      }
    }

    /* check downstreams */
    p = downstream_head;
    prev = NULL;
    while(p)
    {
      if(page_addr == (p->last_paddr & ~(PAGE_SIZE-1))) /* hit */
      {
        down_hit = true;
        break;
      }
      prev = p;
      p = p->next;
    }

    if(p) /* hit */
    {
      if((paddr>>cp->addr_shift) == ((p->last_paddr>>cp->addr_shift)-1))
      {
        p->last_paddr = paddr; /* record this address */

        if(prev) /* entry to MRU position */
        {
          prev->next = p->next;
          p->next = downstream_head;
          downstream_head = p;
        }
        return paddr - (1<<cp->addr_shift);
      }
      else if((paddr>>cp->addr_shift) == ((p->last_paddr>>cp->addr_shift)+1))
      {
        /* possible upward-stream */
        possible_up = true;
      }
    }

    if((!up_hit && !down_hit) || (!up_hit && possible_up))
    {
      /* we don't know about this page; just allocate an entry in the upstream
         table */
      p = upstream_head; prev = NULL;
      while(p->next)
      {
        prev = p;
        p = p->next;
      }

      assert(p);
      p->last_paddr = paddr;

      if(prev) /* entry to MRU position */
      {
        prev->next = p->next;
        p->next = upstream_head;
        upstream_head = p;
      }
    }
    else if(!down_hit && possible_down)
    {
      p = downstream_head; prev = NULL;
      while(p->next)
      {
        prev = p;
        p = p->next;
      }

      assert(p);
      p->last_paddr = paddr;

      if(prev) /* entry to MRU position */
      {
        prev->next = p->next;
        p->next = downstream_head;
        downstream_head = p;
      }
    }

    return 0; /* nothing to prefetch */
  }
};


#endif /* PREFETCH_PARSE_ARGS */
#undef COMPONENT_NAME
