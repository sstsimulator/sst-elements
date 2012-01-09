/* btb-btac.cpp: Regular PC-indexed, set-associative BTB */
/*
 * __COPYRIGHT__ GT
 */

#define COMPONENT_NAME "btac"

#ifdef BPRED_PARSE_ARGS
if(!strcasecmp(COMPONENT_NAME,type))
{
  int num_ents;
  int num_ways;
  int tag_width;
  char replace_policy;
  if(sscanf(opt_string,"%*[^:]:%[^:]:%d:%d:%d:%c",name,&num_ents,&num_ways,&tag_width,&replace_policy) != 5)
    fatal("bad BTB options string %s (should be \"btac:name:entries:ways:tag-width:replacement_policy\")",opt_string);
  return new BTB_btac_t(name,num_ents,num_ways,tag_width,replace_policy);
}
#else


class BTB_btac_t:public BTB_t
{
  /* struct definitions */
  struct BTB_btac_Entry_t
  {
    md_addr_t PC;
    md_addr_t target;
    struct BTB_btac_Entry_t * prev;
    struct BTB_btac_Entry_t * next;
  };

  class BTB_btac_sc_t:public BTB_sc_t
  {
    public:
    struct BTB_btac_Entry_t * p;
    struct BTB_btac_Entry_t ** set;
  };

  protected:
  int num_entries;
  int num_ways;
  int tag_width;
  int tag_mask;
  int tag_shift;
  char replacement_policy;
  struct BTB_btac_Entry_t ** set;

  int btac_mask;

  public:
  /* CREATE */
  BTB_btac_t(char * const arg_name,
             const int arg_num_entries,
             const int arg_num_ways,
             const int arg_tag_width,
             const char arg_replacement_policy
            )
  {
    init();

    /* TODO: error check parameters */
    CHECK_PPOW2(arg_num_entries);
    CHECK_POS(arg_num_ways);
    if((mytolower(arg_replacement_policy) != 'l') && (mytolower(arg_replacement_policy) != 'r'))
      fatal("%s(%s) %s must be 'l' (LRU) or 'r' (random).",name,COMPONENT_NAME,"replacement_policy");

    name = strdup(arg_name);
    if(!name)
      fatal("couldn't malloc btac name (strdup)");

    num_entries = arg_num_entries;
    num_ways = arg_num_ways;
    replacement_policy = arg_replacement_policy;
    tag_width = arg_tag_width;

    btac_mask = num_entries-1;
    tag_shift = log_base2(num_entries);
    tag_mask = (1<<tag_width)-1;

    set = (struct BTB_btac_Entry_t**) calloc(num_entries,sizeof(*set));
    if(!set)
      fatal("couldn't malloc btac set array");
    for(int i=0;i<num_entries;i++)
    {
      struct BTB_btac_Entry_t * p;
      for(int j=0;j<num_ways;j++)
      {
        p = (struct BTB_btac_Entry_t*) calloc(1,sizeof(**set));
        if(!p)
          fatal("couldn't malloc btac entry");
        if(set[i])
        {
          p->next = set[i];
          set[i]->prev = p;
          set[i] = p;
        }
        else
          set[i] = p;
      }
    }

    int tagsize = 8*sizeof(md_addr_t) - log_base2(num_entries);
    int lrusize = log_base2(num_ways);
    int entrysize = sizeof(md_addr_t) + tagsize + lrusize + 1; /* +1 for valid bit */
    bits =  num_entries*num_ways*entrysize;
    type = strdup(COMPONENT_NAME);
  }

  /* DESTROY */
  ~BTB_btac_t()
  {
    free(name); name = NULL;
    free(type); type = NULL;
    for(int i=0;i<num_entries;i++)
    {
      struct BTB_btac_Entry_t * p;
      struct BTB_btac_Entry_t * next = NULL;
      for(p=set[i];p;p = next)
      {
        next = p->next;
        free(p);
      }
    }
    free(set); set = NULL;
  }

  /* LOOKUP */
  BTB_LOOKUP_HEADER
  {
    class BTB_btac_sc_t * sc = (class BTB_btac_sc_t*) scvp;
    int index = PC & btac_mask;
    struct BTB_btac_Entry_t * p = set[index];

    BPRED_STAT(lookups++;)
    sc->updated = false;

    sc->set = &set[index];
    sc->p = NULL;

    while(p)
    {
      if(p->PC == ((PC>>tag_shift)&tag_mask))
      {
        sc->p = p;
        return p->target;
      }
      p = p->next;
    }

    return 0;
  }

  /* UPDATE */
  BTB_UPDATE_HEADER
  {
    class BTB_btac_sc_t * sc = (class BTB_btac_sc_t*) scvp;

    if(!outcome) /* don't store not-taken targets */
    {
      BPRED_STAT(num_nt++;)
      return;
    }

    if(!sc->updated)
    {
      BPRED_STAT(updates++;)
      BPRED_STAT(num_hits += oPC == our_target;)
      sc->updated = true;
    }

    if(!our_pred)
    {
      int index = PC & btac_mask;
      struct BTB_btac_Entry_t * p = set[index];

      sc->set = &set[index];
      sc->p = NULL;

      while(p)
      {
        if(p->PC == ((PC>>tag_shift)&tag_mask))
        {
          sc->p = p;
          break;
        }
        p = p->next;
      }
    }

    if(sc->p)
    {
      /* had a BTB hit; update the target, put to front of list */
      sc->p->target = oPC;
      
      if(sc->p->prev == NULL) /* already at head */
        return;
      sc->p->prev->next = sc->p->next;
      if(sc->p->next)
        sc->p->next->prev = sc->p->prev;

      (*sc->set)->prev = sc->p;
      sc->p->prev = NULL;
      sc->p->next = (*sc->set);
      *sc->set = sc->p;
      return;
    }
    else
    {
      /* was a BTB miss; evict entry depending on replacement policy,
         update, and put to front of list */
      struct BTB_btac_Entry_t * p;
      int pos=0;
      int randpos=0;
      p = *sc->set;
      if(replacement_policy == 'r')
        randpos = random()%num_ways;

      while(p)
      {
        if(replacement_policy == 'l' && !p->next)
          break;
        else if(replacement_policy == 'r' && pos==randpos)
          break;
        else
          pos++;
        p = p->next;
      }

      assert(p != NULL);

      p->target = oPC;
      p->PC = (PC>>tag_shift)&tag_mask;
      
      if(p->prev == NULL) /* already at head */
        return;

      p->prev->next = p->next;
      if(p->next)
        p->next->prev = p->prev;

      (*sc->set)->prev = p;
      p->prev = NULL;
      p->next = *sc->set;
      *sc->set = p;
      return;
    }
  }

  /* GET_CACHE */
  BTB_GET_CACHE_HEADER
  {
    class BTB_btac_sc_t * sc = new BTB_btac_sc_t();
    if(!sc)
      fatal("couldn't malloc btac state cache");
    return sc;
  }

};


#endif /* BTB_PARSE_ARGS */
#undef COMPONENT_NAME
