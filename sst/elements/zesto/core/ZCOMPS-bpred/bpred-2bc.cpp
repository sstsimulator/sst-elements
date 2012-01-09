/* bpred-2bC.cpp: 2-bit saturating counter [Smith, ISCA 1981] */
/*
 * __COPYRIGHT__ GT
 */

#define COMPONENT_NAME "2bC"

#ifdef BPRED_PARSE_ARGS
if(!strcasecmp(COMPONENT_NAME,type))
{
  int num_entries;
           
  if(sscanf(opt_string,"%*[^:]:%[^:]:%d",name,&num_entries) != 2)
    fatal("bad bpred options string %s (should be \"2bC:name:num_entries\")",opt_string);
  return new bpred_2bC_t(name,num_entries);
}
#else

class bpred_2bC_t:public bpred_dir_t
{
  class bpred_2bC_sc_t:public bpred_sc_t
  {
    public:
    my2bc_t * current_ctr;
  };

  protected:

  int num_entries;
  int entry_mask;
  my2bc_t * table;

  public:

  /* CREATE */
  bpred_2bC_t(char * const arg_name,
              const int arg_num_entries
             )
  {
    init();

    /* verify arguments are valid */
    CHECK_PPOW2(arg_num_entries);

    name = strdup(arg_name);
    if(!name)
      fatal("couldn't malloc 2bC name (strdup)");
    type = strdup("2bC");
    if(!type)
      fatal("couldn't malloc 2bC type (strdup)");

    num_entries = arg_num_entries;
    entry_mask = arg_num_entries-1;
    table = (my2bc_t*) calloc(arg_num_entries,sizeof(my2bc_t));
    if(!table)
      fatal("couldn't malloc 2bC table");
    for(int i=0;i<num_entries;i++)
      table[i] = (i&1)?MY2BC_WEAKLY_TAKEN:MY2BC_WEAKLY_NT;

    bits = num_entries * 2;
  }

  /* DESTROY */
  ~bpred_2bC_t()
  {
    if(table) free(table); table = NULL;
  }

  /* LOOKUP */
  BPRED_LOOKUP_HEADER
  {
    class bpred_2bC_sc_t * sc = (class bpred_2bC_sc_t*) scvp;
    sc->current_ctr = &table[PC&entry_mask];
    sc->updated = false;
    BPRED_STAT(lookups++;)
    return MY2BC_DIR(*sc->current_ctr);
  }

  /* UPDATE */
  BPRED_UPDATE_HEADER
  {
    class bpred_2bC_sc_t * sc = (class bpred_2bC_sc_t*) scvp;
    if(!sc->updated)
    {
      BPRED_STAT(num_hits += our_pred == outcome;)
      BPRED_STAT(updates++;)
      sc->updated = true;
    }
    MY2BC_UPDATE(*sc->current_ctr,outcome);
  }

  /* GET_CACHE */
  BPRED_GET_CACHE_HEADER
  {
    class bpred_2bC_sc_t * sc = new bpred_2bC_sc_t();
    if(!sc)
      fatal("couldn't malloc 2bC State Cache");
    return sc;
  }

};

#endif /* BPRED_PARSE_ARGS */
#undef COMPONENT_NAME
