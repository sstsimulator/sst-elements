/* fusion-colt.cpp: Combined Output Lookup Table [Loh, PACT 2002] */
/*
 * __COPYRIGHT__ GT
 */

#define COMPONENT_NAME "colt"

#ifdef BPRED_PARSE_ARGS
if(!strcasecmp("colt",type))
{
  int num_entries;
  int counter_width;
  int history_size;
  int bhr_width;
  if(sscanf(opt_string,"%*[^:]:%[^:]:%d:%d:%d:%d",name,&num_entries,&counter_width,&history_size,&bhr_width) != 5)
    fatal("bad fusion options string %s (should be \"colt:name:num-entries:counter_width:history-size:bhr-width\")",opt_string);
  return new fusion_colt_t(name,num_pred,num_entries,counter_width,history_size,bhr_width);
}
#else

class fusion_colt_t:public fusion_t
{
  class fusion_colt_sc_t:public fusion_sc_t
  {
    public:
    char * current_ctr;
    int * bhr;
    int lookup_bhr;
  };

  protected:

  int meta_size;
  int meta_mask;

  unsigned int counter_width;
  unsigned int counter_shift;
  unsigned int counter_max;

  int history_size;
  int history_mask;
  int bhr_width;
  int bhr_mask;
  int xorshift;
  int xormask;
  int xor2shift;

  int * history;
  char * ftable;

  public:

  /* CREATE */
  fusion_colt_t(char * const arg_name,
                const int arg_num_pred,
                const int arg_meta_size,
                const int arg_counter_width,
                const int arg_history_size,
                const int arg_bhr_width
               )
  {
    init();

    /* verify arguments are valid */
    CHECK_PPOW2(arg_meta_size);
    CHECK_PPOW2(arg_history_size);
    CHECK_NNEG(arg_bhr_width);
    CHECK_POS_LEQ(arg_counter_width,8);

    name = strdup(arg_name);
    if(!name)
      fatal("couldn't malloc fusion colt name (strdup)");
    type = strdup("fusion Table");
    if(!type)
      fatal("couldn't malloc fusion colt type (strdup)");

    num_pred = arg_num_pred;
    meta_size = arg_meta_size;
    meta_mask = arg_meta_size-1;
    counter_width = arg_counter_width;
    counter_shift = arg_counter_width-1;
    counter_max = (1<<arg_counter_width)-1;
    history_size = arg_history_size;
    history_mask = arg_history_size-1;
    bhr_width = arg_bhr_width;
    bhr_mask = (1<<arg_bhr_width)-1;

    xormask = (meta_size-1)>>num_pred;
    xor2shift = log_base2(meta_size)-num_pred;

    history = (int*) calloc(history_size,sizeof(*history));
    if(!history)
      fatal("couldn't malloc colt history table");

    ftable = (char*) calloc(meta_size,sizeof(*ftable));
    if(!ftable)
      fatal("couldn't malloc colt meta table");

    /* initialize to very weakly taken/not-taken: 2^(n-1) and 2^(n-1)-1*/
    for(int i=0;i<meta_size;i++)
      ftable[i] = (i&1)?((1<<(counter_width-1))-1):(1<<(counter_width-1));

    bits = meta_size*counter_width + history_size*bhr_width;
  }

  /* DESTROY */
  ~fusion_colt_t()
  {
    if(history) free(history); history = NULL;
    if(ftable) free(ftable); ftable = NULL;
  }

  FUSION_LOOKUP_HEADER
  {
    class fusion_colt_sc_t * sc = (class fusion_colt_sc_t*) scvp;
    int pred_cat = 0; /* concatenation of predictions */
    bool pred;
    int index;
    sc->bhr = &history[PC&history_mask];

    for(int i=0;i<num_pred;i++)
      pred_cat |= preds[i]<<i;

    /* p = bit for pred_cat; b = bit from bhr;
       0-9 = bits from PC; - = zero bits/padding */
            /* ppppbbb--- ^ ----543210 */
    index = ((PC^(*sc->bhr<<xorshift))&xormask) | (pred_cat<<xor2shift);

    sc->current_ctr = &ftable[index&meta_mask];

    pred = (*sc->current_ctr>>counter_shift)&1;

    BPRED_STAT(lookups++;)
    sc->lookup_bhr = *sc->bhr;
    sc->updated = false;

    return pred;
  }

  /* UPDATE */
  FUSION_UPDATE_HEADER
  {
    class fusion_colt_sc_t * sc = (class fusion_colt_sc_t*) scvp;

    if(!sc->updated)
    {
        BPRED_STAT(updates++;)
        BPRED_STAT(num_hits += our_pred == outcome;)
        sc->updated = true;
    }

    if(outcome)
    {
      if(*sc->current_ctr < (int)counter_max)
        ++ *sc->current_ctr;
    }
    else
    {
      if(*sc->current_ctr > 0)
        -- *sc->current_ctr;
    }
  }

  /* SPEC_UPDATE */
  FUSION_SPEC_UPDATE_HEADER
  {
    class fusion_colt_sc_t * sc = (class fusion_colt_sc_t*) scvp;

    BPRED_STAT(spec_updates++;)

    *sc->bhr = ((*sc->bhr<<1)|our_pred)&bhr_mask;
  }

  /* RECOVER */
  FUSION_RECOVER_HEADER
  {
    class fusion_colt_sc_t * sc = (class fusion_colt_sc_t*) scvp;
    if(history_size == 1)
        history[0] = (sc->lookup_bhr<<1)|outcome;
  }

  /* FLUSH */
  FUSION_FLUSH_HEADER
  {
    class fusion_colt_sc_t * sc = (class fusion_colt_sc_t*) scvp;
    if(history_size == 1)
      history[0] = sc->lookup_bhr;
  }

  /* GET_CACHE */
  FUSION_GET_CACHE_HEADER
  {
    class fusion_colt_sc_t * sc = new fusion_colt_sc_t();
    if(!sc)
      fatal("couldn't malloc colt State Cache");
    return sc;
  }

};

#endif /* BPRED_PARSE_ARGS */
#undef COMPONENT_NAME
