/* bpred-skewed.cpp: skewed predictor [Michaud et al., ISCA 1997] */
/*
 * __COPYRIGHT__ GT
 */

#include <math.h>
#define COMPONENT_NAME "skewed"

#ifdef BPRED_PARSE_ARGS
if(!strcasecmp(COMPONENT_NAME,type))
{
  int l1_size;
  int l2_size;
  int his_width;
  int partial_update;
  int enhanced;
  int double_bim; /* make first table 2x the size */

  if(sscanf(opt_string,"%*[^:]:%[^:]:%d:%d:%d:%d:%d:%d",name,&l1_size,&l2_size,&his_width,&partial_update,&enhanced,&double_bim) != 7)
    fatal("bad bpred options string %s (should be \"skewed:name:l1_size:l2_size:his_width:partial_update:enhanced:double-bim\")",opt_string);
  return new bpred_skewed_t(name,l1_size,l2_size,his_width,partial_update,enhanced,double_bim);
}
#else

class bpred_skewed_t:public bpred_dir_t
{
  class bpred_skewed_sc_t:public bpred_sc_t
  {
    public:
    my2bc_t * pht_ctr[3];
    int * bhr;
    int pred;
    int lookup_bhr;
  };

  protected:

  bool partial_update;
  bool enhanced;
  bool double_bim;
  int bht_size;
  int bht_mask;
  int * bht;
  int pht_size;
  int pht_mask;
  my2bc_t ** pht;
  int history_length;
  int history_mask;
  int half_length;
  int half_mask;
  int high_mask;

  public:

  /* CREATE */
  bpred_skewed_t(char * const arg_name,
                 const int arg_bht_size,
                 const int arg_pht_size,
                 const int arg_history_length,
                 const int arg_partial_update,
                 const int arg_enhanced,
                 const int arg_double_bim
                )
  {
    init();

    /* verify arguments are valid */
    CHECK_PPOW2(arg_bht_size);
    CHECK_PPOW2(arg_pht_size);
    CHECK_NNEG(arg_history_length);
    CHECK_BOOL(arg_enhanced);
    CHECK_BOOL(arg_double_bim);
    CHECK_BOOL(arg_partial_update);

    name = strdup(arg_name);
    if(!name)
      fatal("couldn't malloc skewed name (strdup)");
    type = strdup(COMPONENT_NAME);
    if(!type)
      fatal("couldn't malloc skewed type (strdup)");

    bht_size = arg_bht_size;
    bht_mask = arg_bht_size-1;
    pht_size = arg_pht_size;
    pht_mask = arg_pht_size-1;
    history_length = arg_history_length;
    history_mask = (1<<arg_history_length)-1;
    half_length = log_base2(arg_pht_size);
    half_mask = pht_mask;
    high_mask = arg_pht_size>>1;
    partial_update = arg_partial_update;
    enhanced = arg_enhanced;
    double_bim = arg_double_bim;

    bht = (int*) calloc(bht_size,sizeof(int));
    if(!bht)
      fatal("couldn't malloc skewed BHT");
    pht = (my2bc_t**) calloc(3,sizeof(my2bc_t*));
    if(!pht)
      fatal("couldn't malloc skewed pht");
    if(double_bim)
      pht[0] = (my2bc_t*) calloc(2*pht_size,sizeof(my2bc_t));
    else
      pht[0] = (my2bc_t*) calloc(pht_size,sizeof(my2bc_t));
    pht[1] = (my2bc_t*) calloc(pht_size,sizeof(my2bc_t));
    pht[2] = (my2bc_t*) calloc(pht_size,sizeof(my2bc_t));
    if(!pht[0] || !pht[1] || !pht[2])
      fatal("couldn't malloc skewed PHT");
    for(int i=0;i<pht_size;i++)
    {
      pht[0][i] = (i&1)?MY2BC_WEAKLY_TAKEN:MY2BC_WEAKLY_NT;
      if(double_bim)
      pht[0][pht_size+i] = (i&1)?MY2BC_WEAKLY_TAKEN:MY2BC_WEAKLY_NT;
      pht[1][i] = (!(i&1))?MY2BC_WEAKLY_TAKEN:MY2BC_WEAKLY_NT;
      pht[2][i] = (i&1)?MY2BC_WEAKLY_TAKEN:MY2BC_WEAKLY_NT;
    }

    if(double_bim)
      bits = bht_size*history_length + pht_size*8;
    else
      bits = bht_size*history_length + pht_size*6;
  }

  /* DESTROY */
  ~bpred_skewed_t()
  {
    free(pht[0]); pht[0] = NULL;
    free(pht[1]); pht[1] = NULL;
    free(pht[2]); pht[2] = NULL;
    free(pht); pht = NULL;
    free(bht); bht = NULL;
  }

  /* LOOKUP */
  BPRED_LOOKUP_HEADER
  {
    class bpred_skewed_sc_t * sc = (class bpred_skewed_sc_t*) scvp;
    int l1index = PC&bht_mask;
    int l2index_base;
    int v1, v2;
    int l2index[3];
    bool pred;

    sc->bhr = &bht[l1index];
    l2index_base = (PC<<history_length)|*sc->bhr;

    v2 = (l2index_base >> half_length) & half_mask;
    v1 = l2index_base & half_mask;

    /* f0 */
    if(!enhanced)
      l2index[0] = ((v1>>1)|((v1&high_mask) ^ ((v1&1) << (half_length-1)))) /* H(v1) */
               ^ ((v2<<1)|(((v2&high_mask) ^ ((v2&(high_mask>>1))<<1))>>(half_length-1))) /* Hinv(v2) */
               ^ v2;
    else
      l2index[0] = PC;

    /* f1 */
    l2index[1] = ((v1>>1)|((v1&high_mask) ^ ((v1&1) << (half_length-1)))) /* H(v1) */
               ^ ((v2<<1)|(((v2&high_mask) ^ ((v2&(high_mask>>1))<<1))>>(half_length-1))) /* Hinv(v2) */
               ^ v1;

    /* f2 */
    l2index[2] = ((v2>>1)|((v2&high_mask) ^ ((v2&1) << (half_length-1)))) /* H(v2) */
               ^ ((v1<<1)|(((v1&high_mask) ^ ((v1&(high_mask>>1))<<1))>>(half_length-1))) /* Hinv(v1) */
               ^ v2;

    if(double_bim)
      l2index[0] &= (pht_mask<<1)|1;
    else
      l2index[0] &= pht_mask;
    l2index[1] &= pht_mask;
    l2index[2] &= pht_mask;

    sc->pht_ctr[0] = &pht[0][l2index[0]];
    sc->pht_ctr[1] = &pht[1][l2index[1]];
    sc->pht_ctr[2] = &pht[2][l2index[2]];

    pred  = MY2BC_DIR(*sc->pht_ctr[0]);
    pred += MY2BC_DIR(*sc->pht_ctr[1]);
    pred += MY2BC_DIR(*sc->pht_ctr[2]);

    pred >>= 1;

    BPRED_STAT(lookups++;)

    sc->pred = pred;
    sc->updated = false;
    sc->lookup_bhr = *sc->bhr;

    return pred;
  }

  /* UPDATE */
  BPRED_UPDATE_HEADER
  {
    class bpred_skewed_sc_t * sc = (class bpred_skewed_sc_t*) scvp;

    if(sc->pred==outcome && partial_update)
    {
      if(MY2BC_DIR(*sc->pht_ctr[0]) == outcome)
        MY2BC_UPDATE(*sc->pht_ctr[0],outcome);
      if(MY2BC_DIR(*sc->pht_ctr[1]) == outcome)
        MY2BC_UPDATE(*sc->pht_ctr[1],outcome);
      if(MY2BC_DIR(*sc->pht_ctr[2]) == outcome)
        MY2BC_UPDATE(*sc->pht_ctr[2],outcome);
    }
    else
    {
      MY2BC_UPDATE(*sc->pht_ctr[0],outcome);
      MY2BC_UPDATE(*sc->pht_ctr[1],outcome);
      MY2BC_UPDATE(*sc->pht_ctr[2],outcome);
    }

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
    class bpred_skewed_sc_t * sc = (class bpred_skewed_sc_t*) scvp;
    BPRED_STAT(spec_updates++;)

    *sc->bhr = ((*sc->bhr<<1)|our_pred)&history_mask;
  }

  /* RECOVER */
  BPRED_RECOVER_HEADER
  {
    class bpred_skewed_sc_t * sc = (class bpred_skewed_sc_t*) scvp;
    if(bht_size == 1)
        bht[0] = (sc->lookup_bhr<<1)|outcome;
  }

  /* FLUSH */
  BPRED_FLUSH_HEADER
  {
    class bpred_skewed_sc_t * sc = (class bpred_skewed_sc_t*) scvp;
    if(bht_size == 1)
      bht[0] = sc->lookup_bhr;
  }

  /* GET_CACHE */
  BPRED_GET_CACHE_HEADER
  {
    class bpred_skewed_sc_t * sc = new bpred_skewed_sc_t();
    if(!sc)
      fatal("couldn't malloc skewed State Cache");
    return sc;
  }

};


#endif /* BPRED_PARSE_ARGS */
#undef COMPONENT_NAME
