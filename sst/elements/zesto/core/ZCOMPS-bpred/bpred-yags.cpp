/* bpred-yags.cpp: YAGS (Yet Another Global Scheme) Predictor [Eden and Mudge, MICRO 1998] */
/*
 * __COPYRIGHT__ GT
 */

#include <math.h>
#define COMPONENT_NAME "yags"

#ifdef BPRED_PARSE_ARGS
if(!strcasecmp(COMPONENT_NAME,type))
{
  int l1_size;
  int l2_size;
  int his_width;
  int Xor;
  int cache_size;
  int tag_width;

  if(sscanf(opt_string,"%*[^:]:%[^:]:%d:%d:%d:%d:%d:%d",name,&l1_size,&l2_size,&his_width,&Xor,&cache_size,&tag_width) != 7)
    fatal("bad bpred options string %s (should be \"yags:name:l1_size:l2_size:his_width:xor:cache_size:tag_width\")",opt_string);
  return new bpred_yags_t(name,l1_size,l2_size,his_width,Xor,cache_size,tag_width);
}
#else

class bpred_yags_t:public bpred_dir_t
{
  struct yags_cache_t{
    unsigned char tag;
    my2bc_t ctr;
  };

  class bpred_yags_sc_t:public bpred_sc_t
  {
    public:
    my2bc_t * pht_ctr;
    struct yags_cache_t * cache_entry;
    int * bhr;
    char tag_match;
    bool pred;
    bool choice;
    int lookup_bhr;
  };

  protected:

  int bht_size;
  int bht_mask;
  int * bht;
  int pht_size;
  int pht_mask;
  int tag_width;
  int tag_mask;
  int cache_size;
  int cache_mask;
  my2bc_t * pht;
  struct yags_cache_t * T_cache;
  struct yags_cache_t * NT_cache;

  int history_length;
  int history_mask;
  int Xor;
  int xorshift;

  public:

  /* CREATE */
  bpred_yags_t(char * const arg_name,
               const int arg_bht_size,
               const int arg_pht_size,
               const int arg_history_length,
               const int arg_Xor,
               const int arg_cache_size,
               const int arg_tag_width
              )
  {
    init();

    /* verify arguments are valid */
    CHECK_PPOW2(arg_bht_size);
    CHECK_PPOW2(arg_pht_size);
    CHECK_PPOW2(arg_cache_size);
    CHECK_NNEG(arg_history_length);
    CHECK_BOOL(arg_Xor);

    name = strdup(arg_name);
    if(!name)
      fatal("couldn't malloc yags name (strdup)");
    type = strdup(COMPONENT_NAME);
    if(!type)
      fatal("couldn't malloc yags type (strdup)");

    bht_size = arg_bht_size;
    bht_mask = arg_bht_size-1;
    pht_size = arg_pht_size;
    pht_mask = arg_pht_size-1;
    tag_width = arg_tag_width;
    tag_mask = (1<<arg_tag_width)-1;
    cache_size = arg_cache_size;
    cache_mask = arg_cache_size-1;
    history_length = arg_history_length;
    history_mask = (1<<arg_history_length)-1;
    Xor = arg_Xor;
    if(arg_Xor)
    {
      xorshift = log_base2(arg_cache_size)-arg_history_length;
      if(xorshift < 0)
        xorshift = 0;
    }

    bht = (int*) calloc(bht_size,sizeof(int));
    if(!bht)
      fatal("couldn't malloc yags BHT");
    pht = (my2bc_t*) calloc(pht_size,sizeof(my2bc_t));
    if(!pht)
      fatal("couldn't malloc yags PHT");
    for(int i=0;i<pht_size;i++)
      pht[i] = (i&1)?MY2BC_WEAKLY_TAKEN:MY2BC_WEAKLY_NT;
    T_cache = (struct yags_cache_t*) calloc(cache_size,sizeof(struct yags_cache_t));
    NT_cache = (struct yags_cache_t*) calloc(cache_size,sizeof(struct yags_cache_t));
    if(!T_cache || !NT_cache)
      fatal("couldn't malloc yags cache");
    for(int i=0;i<cache_size;i++)
    {
      T_cache[i].ctr = MY2BC_WEAKLY_TAKEN;
      NT_cache[i].ctr = MY2BC_WEAKLY_NT;
    }

    bits =  bht_size*history_length + pht_size*2
           + (tag_width+2)*cache_size * 2;
  }

  /* DESTROY */
  ~bpred_yags_t()
  {
    free(T_cache); T_cache = NULL;
    free(NT_cache); NT_cache = NULL;
    free(pht); pht = NULL;
    free(bht); bht = NULL;
  }

  /* LOOKUP */
  BPRED_LOOKUP_HEADER
  {
    class bpred_yags_sc_t * sc = (class bpred_yags_sc_t*) scvp;
    int l1index = PC&bht_mask;
    int l2index;
    bool pred;

    sc->bhr = &bht[l1index];

    /* "choice PHT" */
    sc->pht_ctr = &pht[PC&pht_mask];
    pred = MY2BC_DIR(*sc->pht_ctr);
    sc->choice = pred;

    if(Xor)
      l2index = PC^(*sc->bhr<<xorshift);
    else
      l2index = (PC<<history_length)|*sc->bhr;
    l2index &= cache_mask;

    sc->cache_entry = (pred)
                     ? (&NT_cache[l2index])
                     : (&T_cache[l2index]);
    sc->tag_match = sc->cache_entry->tag == (PC & tag_mask);

    if(sc->tag_match)
      pred = MY2BC_DIR(sc->cache_entry->ctr);

    BPRED_STAT(lookups++;)

    sc->pred = pred;
    sc->updated = false;
    sc->lookup_bhr = *sc->bhr;

    return pred;
  }

  /* UPDATE */
  BPRED_UPDATE_HEADER
  {
    class bpred_yags_sc_t * sc = (class bpred_yags_sc_t*) scvp;
    bool pred = sc->pred;
    bool choice = sc->choice;

    /*
    choice = MY2BC_DIR(*sc->pht_ctr);
    pred = (sc->tag_match)
         ? MY2BC_DIR(sc->cache_entry->ctr)
         : MY2BC_DIR(*sc->pht_ctr);
     */

    if(sc->tag_match)
    {
      MY2BC_UPDATE(sc->cache_entry->ctr,outcome);
    }
    else if(choice ^ outcome)
    {
      MY2BC_UPDATE(sc->cache_entry->ctr,outcome);
      sc->cache_entry->tag = PC & tag_mask;
    }
    if(!(pred == outcome && choice != outcome))
      MY2BC_UPDATE(*sc->pht_ctr,outcome);

    if(!sc->updated)
    {
        BPRED_STAT(updates++;)
        BPRED_STAT(num_hits += our_pred == outcome;)
        sc->updated = true;
    }
  }

  /* SPEC_UPDATE */
  BPRED_SPEC_UPDATE_HEADER
  {
    class bpred_yags_sc_t * sc = (class bpred_yags_sc_t*) scvp;
    BPRED_STAT(spec_updates++;)

    *sc->bhr = ((*sc->bhr<<1)|our_pred)&history_mask;
  }

  /* RECOVER */
  BPRED_RECOVER_HEADER
  {
    class bpred_yags_sc_t * sc = (class bpred_yags_sc_t*) scvp;
    if(bht_size == 1)
        bht[0] = (sc->lookup_bhr<<1)|outcome;
  }

  /* FLUSH */
  BPRED_FLUSH_HEADER
  {
    class bpred_yags_sc_t * sc = (class bpred_yags_sc_t*) scvp;
    if(bht_size == 1)
      bht[0] = sc->lookup_bhr;
  }

  /* GET_CACHE */
  BPRED_GET_CACHE_HEADER
  {
    class bpred_yags_sc_t * sc = new bpred_yags_sc_t();
    if(!sc)
      fatal("couldn't malloc yags State Cache");
    return sc;
  }

};

#endif /* BPRED_PARSE_ARGS */
#undef COMPONENT_NAME
