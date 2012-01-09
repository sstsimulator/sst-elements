/* bpred-bimode.cpp: Bi-Mode predictor [Lee et al. MICRO 1997]
   NOTE: Bi-Mode is *not* the same as "bimodal" (2bC) */
/*
 * __COPYRIGHT__ GT
 */

#include <math.h>
#define COMPONENT_NAME "bimode"

#ifdef BPRED_PARSE_ARGS
if(!strcasecmp(COMPONENT_NAME,type))
{
  int l1_size;
  int l2_size;
  int his_width;
  int Xor;

  if(sscanf(opt_string,"%*[^:]:%[^:]:%d:%d:%d:%d",name,&l1_size,&l2_size,&his_width,&Xor) != 5)
    fatal("bad bpred options string %s (should be \"bimode:name:l1_size:l2_size:his_width:xor\")",opt_string);
  return new bpred_bimode_t(name,l1_size,l2_size,his_width,Xor);
}
#else

class bpred_bimode_t:public bpred_dir_t
{

  class bpred_bimode_sc_t:public bpred_sc_t
  {
    public:
    int *bhr;
    my2bc_t * select_ctr;
    my2bc_t * pred_ctr;
    int updated;
    int lookup_bhr;
  };

  protected:

  int bht_size;
  int bht_mask;
  int * bht;
  int pht_size;
  int pht_mask;
  my2bc_t ** pht;
  my2bc_t * pht_meta;
  int history_length;
  int history_mask;
  int Xor;
  int xorshift;

  public:

  /* CREATE */
  bpred_bimode_t(char * const arg_name,
                 const int arg_bht_size,
                 const int arg_pht_size,
                 const int arg_history_length,
                 const int arg_Xor
                )
  {
    init();

    /* verify arguments are valid */
    CHECK_PPOW2(arg_bht_size);
    CHECK_PPOW2(arg_pht_size);
    CHECK_NNEG(arg_history_length);
    CHECK_BOOL(arg_Xor);

    name = strdup(arg_name);
    if(!name)
      fatal("couldn't malloc bimode name (strdup)");
    type = strdup(COMPONENT_NAME);
    if(!type)
      fatal("couldn't malloc bimode type (strdup)");

    bht_size = arg_bht_size;
    bht_mask = arg_bht_size-1;
    pht_size = arg_pht_size;
    pht_mask = arg_pht_size-1;
    history_length = arg_history_length;
    history_mask = (1<<arg_history_length)-1;
    Xor = arg_Xor;
    if(Xor)
    {
      xorshift = log_base2(pht_size)-history_length;
      if(xorshift < 0)
        xorshift = 0;
    }

    bht = (int*) calloc(bht_size,sizeof(int));
    if(!bht)
      fatal("couldn't malloc bimode BHT");
    pht = (my2bc_t**) calloc(2,sizeof(my2bc_t*));
    if(!pht)
      fatal("couldn't malloc bimode pht");
    pht[0] = (my2bc_t*) calloc(pht_size,sizeof(my2bc_t));
    pht[1] = (my2bc_t*) calloc(pht_size,sizeof(my2bc_t));
    pht_meta = (my2bc_t*) calloc(pht_size,sizeof(my2bc_t));
    if(!pht[0] || !pht[1] || !pht_meta)
      fatal("couldn't malloc bimode PHT");
    for(int i=0;i<pht_size;i++)
    {
      pht[0][i] = MY2BC_WEAKLY_NT;
      pht[1][i] = MY2BC_WEAKLY_TAKEN;
      pht_meta[i] = (i&1)?MY2BC_WEAKLY_TAKEN:MY2BC_WEAKLY_NT;
    }

    bits = bht_size*history_length + pht_size*6;
  }

  /* DESTROY */
  ~bpred_bimode_t()
  {
    free(pht[0]); pht[0] = NULL;
    free(pht[1]); pht[1] = NULL;
    free(pht); pht = NULL;
    free(pht_meta); pht_meta = NULL;
    free(bht); bht = NULL;
  }


  /* LOOKUP */
  BPRED_LOOKUP_HEADER
  {
    class bpred_bimode_sc_t * sc = (class bpred_bimode_sc_t*) scvp;
    int l1index = PC&bht_mask;
    int l2index;
    int select;
    bool pred;

    sc->bhr = &bht[l1index];

    if(Xor)
    {
      l2index = PC^(*sc->bhr<<xorshift);
    }
    else
    {
      l2index = (PC<<history_length)|*sc->bhr;
    }

    l2index &= pht_mask;

    sc->select_ctr = &pht_meta[PC&pht_mask];
    select = MY2BC_DIR(*sc->select_ctr);
    sc->pred_ctr = &pht[select][l2index];
    pred = MY2BC_DIR(*sc->pred_ctr);

    BPRED_STAT(lookups++;)
    sc->updated = false;
    sc->lookup_bhr = *sc->bhr;

    return pred;
  }

  /* UPDATE */
  BPRED_UPDATE_HEADER
  {
    class bpred_bimode_sc_t * sc = (class bpred_bimode_sc_t*) scvp;
    bool select;
    bool pred;

    select = MY2BC_DIR(*sc->select_ctr);
    pred = MY2BC_DIR(*sc->pred_ctr);

    MY2BC_UPDATE(*sc->pred_ctr,outcome);

    if(!(pred == outcome && select != outcome))
      MY2BC_UPDATE(*sc->select_ctr,outcome);

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
    class bpred_bimode_sc_t * sc = (class bpred_bimode_sc_t*) scvp;
    BPRED_STAT(spec_updates++;)

    *sc->bhr = (((*sc->bhr)<<1)|our_pred)&history_mask;
  }

  /* RECOVER */
  BPRED_RECOVER_HEADER
  {
    class bpred_bimode_sc_t * sc = (class bpred_bimode_sc_t*) scvp;

    if(bht_size == 1)
        bht[0] = (sc->lookup_bhr<<1)|outcome;
  }

  /* FLUSH */
  BPRED_FLUSH_HEADER
  {
    class bpred_bimode_sc_t * sc = (class bpred_bimode_sc_t*) scvp;

    if(bht_size == 1)
      bht[0] = sc->lookup_bhr;
  }

  /* GET_CACHE */
  BPRED_GET_CACHE_HEADER
  {
    class bpred_bimode_sc_t * sc = new bpred_bimode_sc_t();
    if(!sc)
      fatal("couldn't malloc bimode State Cache");
    return sc;
  }

};

#endif /* BPRED_PARSE_ARGS */
#undef COMPONENT_NAME
