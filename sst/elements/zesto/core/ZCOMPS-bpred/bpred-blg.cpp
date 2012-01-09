/* bpred-blg.cpp: BLG predictor based on USPTO patent US
   6,374,349 B2, Scott McFarling, "Branch Predictor with
   Serially Connected Predictor Stages for Improving Branch
   Prediction Accuracy"                                    */
/*
 * __COPYRIGHT__ GT
 */

#include <math.h>
#define COMPONENT_NAME "blg"

#ifdef BPRED_PARSE_ARGS
if(!strcasecmp(COMPONENT_NAME,type))
{
  int B_size;
  int L_size;
  int L_hist_len;
  int L_hist_size;
  int L_tag_size;
  int G_size;
  int G_tag_size;

  if(sscanf(opt_string,"%*[^:]:%[^:]:%d:%d:%d:%d:%d:%d:%d",name,&B_size,&L_size,&L_hist_len,&L_hist_size,&L_tag_size,&G_size,&G_tag_size) != 8)
    fatal("bad bpred options string %s (should be \"blg:name:B_size:L_size:L_hist_len:L_hist_size:L_tag_size:G_size:G_tag_size\")",opt_string);
  return new bpred_blg_t(name,B_size,L_size,L_hist_len,L_hist_size,L_tag_size,G_size,G_tag_size);
}
#else

class bpred_blg_t:public bpred_dir_t
{
  class bpred_blg_sc_t:public bpred_sc_t
  {
    public:
    md_addr_t PC;
    int lookup_stew;
    int lookup_L_hist;

    int B_index;
    int L_index;
    int L_hist_index;
    int G_index;

    int B_pred;
    int L_pred;
    int G_pred;

    int G_hit;
    int L_hit;

    int G_tag;
  };

  protected:

  int B_size;
  int B_mask;
  int L_size;
  int L_mask;
  int L_hist_len;
  int L_hist_size;
  int L_hist_mask;
  int L_tag_size;
  int G_size;
  int G_mask;
  int G_tag_size;

  int L_tag_shamt;
  int L_tag_mask;

  int G_tag_shamt;
  int G_tag_mask;

  my2bc_t * B;
  int * L_hist;
  my2bc_t * L;
  int * L_tag;
  my2bc_t * G;
  int * G_tag;

  int stew;

  public:

  /* CREATE */
  bpred_blg_t(char * const arg_name,
              const int arg_B_size,
              const int arg_L_size,
              const int arg_L_hist_len,
              const int arg_L_hist_size,
              const int arg_L_tag_size,
              const int arg_G_size,
              const int arg_G_tag_size
             )
  {
    init();

    /* verify arguments are valid */
    CHECK_PPOW2(arg_B_size);
    CHECK_PPOW2(arg_L_size);
    CHECK_PPOW2(arg_L_hist_size);
    CHECK_POS(arg_L_hist_len);
    CHECK_POS(arg_L_tag_size);
    CHECK_PPOW2(arg_G_size);
    CHECK_POS(arg_G_tag_size);

    name = strdup(arg_name);
    if(!name)
      fatal("couldn't malloc blg name (strdup)");
    type = strdup(COMPONENT_NAME);
    if(!type)
      fatal("couldn't malloc blg type (strdup)");

    B_size = arg_B_size;
    B_mask = arg_B_size-1;
    L_size = arg_L_size;
    L_mask = arg_L_size-1;
    L_hist_len = arg_L_hist_len;
    L_hist_size = arg_L_hist_size;
    L_hist_mask = arg_L_hist_size-1;
    L_tag_size = arg_L_tag_size;
    G_size = arg_G_size;
    G_mask = arg_G_size-1;
    G_tag_size = arg_G_tag_size;

    L_tag_shamt = log_base2(arg_L_size);
    L_tag_mask = (1<<arg_L_tag_size)-1;
    G_tag_shamt = log_base2(arg_G_size);
    G_tag_mask = (1<<arg_G_tag_size)-1;

    B = (my2bc_t*) calloc(B_size,sizeof(*B));
    if(!B)
      fatal("failed to calloc blg B-array");
    for(int i=0;i<B_size;i++)
      B[i] = MY2BC_WEAKLY_TAKEN;

    L_hist = (int*) calloc(L_hist_size,sizeof(*L_hist));
    L_tag = (int*) calloc(L_hist_size,sizeof(*L_tag));
    L = (my2bc_t*) calloc(L_size,sizeof(*L));
    if(!L_hist || !L || !L_tag)
      fatal("failed to calloc blg L-arrays");
    for(int i=0;i<L_size;i++)
    {
      if((i>>(L_hist_len-1)) & 1) /* shift mode */
      {
        if(i&1)
          L[i] = MY2BC_WEAKLY_TAKEN;
        else
          L[i] = MY2BC_WEAKLY_NT;
      }
      else /* count mode */
      {
        if((i>>(L_hist_len-2)) & 1)
          L[i] = MY2BC_WEAKLY_TAKEN;
        else
          L[i] = MY2BC_WEAKLY_NT;
      }
    }

    G = (my2bc_t*) calloc(G_size,sizeof(*G));
    G_tag = (int*) calloc(G_size,sizeof(*G_tag));
    if(!G || !G_tag)
      fatal("failed to calloc blg G-arrays");
    for(int i=0;i<G_size;i++)
      G[i] = MY2BC_WEAKLY_TAKEN;

    bits = B_size*2 + L_size*2 + L_hist_size*(L_hist_len+L_tag_size)
              + G_size*(2+G_tag_size);
  }

  /* DESTROY */
  ~bpred_blg_t()
  {
    free(B); B = NULL;
    free(L); L = NULL;
    free(L_hist); L_hist = NULL;
    free(L_tag); L_tag = NULL;
    free(G); G = NULL;
    free(G_tag); G_tag = NULL;
  }

  /* LOOKUP */
  BPRED_LOOKUP_HEADER
  {
    class bpred_blg_sc_t * sc = (class bpred_blg_sc_t*) scvp;

    sc->PC = PC;
    sc->B_index = PC&B_mask;
    sc->B_pred = MY2BC_DIR(B[sc->B_index]);

    sc->L_hist_index = (PC<<1) & L_hist_mask;
    sc->L_index = ((((PC << L_hist_len) | L_hist[sc->L_hist_index]) << 1) | sc->B_pred) & L_mask;
    sc->lookup_L_hist = L_hist[sc->L_hist_index];
    sc->L_hit = ((PC>>L_tag_shamt)&L_tag_mask) == (unsigned)L_tag[sc->L_hist_index];
    sc->L_pred = sc->L_hit?MY2BC_DIR(L[sc->L_index]):sc->B_pred;

    int temp = ((PC ^ stew)<<1) | sc->L_pred;
    sc->G_tag = temp & G_tag_mask;
    sc->G_index = ((temp >> G_tag_size) ^ sc->G_tag) & G_mask;
    sc->G_pred = MY2BC_DIR(G[sc->G_index]);
    sc->G_hit = sc->G_tag == G_tag[sc->G_index];

    sc->lookup_stew = stew;

    sc->updated = false;
    BPRED_STAT(lookups++;)


    if     (sc->G_hit) return sc->G_pred;
    else if(sc->L_hit) return sc->L_pred;
    else               return sc->B_pred;
  }

  /* UPDATE  partial update rules somewhat modeled after Michaud's CBP PPM-like predictor */
  BPRED_UPDATE_HEADER
  {
    class bpred_blg_sc_t * sc = (class bpred_blg_sc_t*) scvp;

    if(!sc->updated)
    {
      int L_hit = sc->L_hit;
      int G_hit = sc->G_hit;

      /* update counter that provided the prediction */
      if(G_hit)
      {
        MY2BC_UPDATE(G[sc->G_index],outcome);
      }
      else if(L_hit)
      {
        MY2BC_UPDATE(L[sc->L_index],outcome);
      }
      else
      {
        MY2BC_UPDATE(B[sc->B_index],outcome);
      }

      if(our_pred != outcome) /* we were wrong, try stealing a counter */
      {

        if(!L_hit && !G_hit)
        {
          /* allocate in L */
          L_tag[sc->L_hist_index] = ((PC>>L_tag_shamt)&L_tag_mask);

          /* history initialization */
          if((B[sc->B_index] == MY2BC_STRONG_NT) && outcome)
            L_hist[sc->L_hist_index] = 0xfffffff1;
          else if((B[sc->B_index] == MY2BC_WEAKLY_NT) && outcome)
            L_hist[sc->L_hist_index] = 0xffffffe3;
          else if((B[sc->B_index] == MY2BC_WEAKLY_TAKEN) && !outcome)
            L_hist[sc->L_hist_index] = 0x0000001c;
          else if((B[sc->B_index] == MY2BC_STRONG_TAKEN) && !outcome)
            L_hist[sc->L_hist_index] = 0x0000000e;

          L_hist[sc->L_hist_index] &= (1<<(L_hist_len-1))-1;
          L_hist[sc->L_hist_index] |= 1<<(L_hist_len-1); /* set to shift mode */

          //int new_L_index = ((PC << L_hist_len) | L_hist[sc->L_hist_index]) & L_mask;
          int new_L_index = ((((PC << L_hist_len) | L_hist[sc->L_hist_index]) << 1) | sc->B_pred) & L_mask;

          L[new_L_index] = outcome?MY2BC_WEAKLY_TAKEN:MY2BC_WEAKLY_NT;

          /* allocate in G */
          G_tag[sc->G_index] = sc->G_tag;
          G[sc->G_index] = outcome?MY2BC_WEAKLY_TAKEN:MY2BC_WEAKLY_NT;
        }

        if(L_hit && !G_hit)
        {
          /* allocate in G */
          G_tag[sc->G_index] = sc->G_tag;
          G[sc->G_index] = outcome?MY2BC_WEAKLY_TAKEN:MY2BC_WEAKLY_NT;
        }
      }

      BPRED_STAT(updates++;)
      BPRED_STAT(num_hits += our_pred == outcome;)
      sc->updated = true;
    }
  }


  /* SPEC UPDATE */
  BPRED_SPEC_UPDATE_HEADER
  {
    class bpred_blg_sc_t * sc = (class bpred_blg_sc_t*) scvp;

    BPRED_STAT(spec_updates++;)

    stew = ((stew ^ PC)<<1)|our_pred;

    /* shift or count update */
    if(((PC>>L_tag_shamt)&L_tag_mask) == (unsigned)(L_tag[sc->L_hist_index])) /* L_hist hit */
    {
      int hist = L_hist[sc->L_hist_index];

      if((hist>>(L_hist_len-1)) & 1) /* shift mode */
      {
        hist <<= 1;
        hist |= our_pred;

        int tmp = hist & ((1<<(L_hist_len-1))-1);
        if((tmp == 0) || ((~tmp)==0)) /* same bit repeated for full hist, switch to count mode */
        {
          if(our_pred)
            hist = 1<<(L_hist_len-2);
          else
            hist = 0;
        }
        else
        {
          hist |= 1<<(L_hist_len-1);
          hist &= (1<<(L_hist_len))-1;
        }
      }
      else /* count mode */
      {
        int dir = (hist>>(L_hist_len-2)) & 1;
        if(dir == our_pred)
        {
          int count = hist & ((1<<(L_hist_len-2))-1);
          count++;
          if(count >> (L_hist_len-2)) /* saturated */
            count--;

          hist = (dir<<(L_hist_len-2)) | count;
        }
        else /* direction change */
        {
          hist = 1;   /* 000...0001 */
          if(!our_pred)
            hist = ~hist; /* 111...1110 */
          hist |= 1<<(L_hist_len-1); /* set to shift mode */
        }
      }

      L_hist[sc->L_hist_index] = hist;
    }
  }

  BPRED_RECOVER_HEADER
  {
    class bpred_blg_sc_t * sc = (class bpred_blg_sc_t*) scvp;

    stew = ((sc->lookup_stew ^ sc->PC)<<1)|outcome;

    /* if local hist still matches */
    if(((sc->PC>>L_tag_shamt)&L_tag_mask) == (unsigned)(L_tag[sc->L_hist_index]))
    {
      int hist = sc->lookup_L_hist;

      if((hist>>(L_hist_len-1)) & 1) /* shift mode */
      {
        hist <<= 1;
        hist |= outcome;

        int tmp = hist & ((1<<(L_hist_len-1))-1);
        if((tmp == 0) || ((~tmp)==0)) /* same bit repeated for full hist, switch to count mode */
        {
          if(outcome)
            hist = 1<<(L_hist_len-2);
          else
            hist = 0;
        }
        else
        {
          hist |= 1<<(L_hist_len-1);
          hist &= (1<<(L_hist_len))-1;
        }
      }
      else /* count mode */
      {
        int dir = (hist>>(L_hist_len-2)) & 1;
        if(dir == outcome)
        {
          int count = hist & ((1<<(L_hist_len-2))-1);
          count++;
          if(count >> (L_hist_len-2)) /* saturated */
            count--;

          hist = (dir<<(L_hist_len-2)) | count;
        }
        else /* direction change */
        {
          hist = 1;   /* 000...0001 */
          if(!outcome)
            hist = ~hist; /* 111...1110 */
          hist |= 1<<(L_hist_len-1); /* set to shift mode */
        }
      }

      L_hist[sc->L_hist_index] = hist;
    }
  }

  BPRED_FLUSH_HEADER
  {
    class bpred_blg_sc_t * sc = (class bpred_blg_sc_t*) scvp;

    stew = sc->lookup_stew ^ sc->PC;

    /* if local hist still matches */
    if(((sc->PC>>L_tag_shamt)&L_tag_mask) == (unsigned)(L_tag[sc->L_hist_index]))
      L_hist[sc->L_hist_index] = sc->lookup_L_hist;
  }

  /* GET_CACHE */
  BPRED_GET_CACHE_HEADER
  {
    class bpred_blg_sc_t * sc = new bpred_blg_sc_t();
    if(!sc)
      fatal("couldn't malloc blg State Cache");
    return sc;
  }

};

#endif /* BPRED_PARSE_ARGS */
#undef COMPONENT_NAME
