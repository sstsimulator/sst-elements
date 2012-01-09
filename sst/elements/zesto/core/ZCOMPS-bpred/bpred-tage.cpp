/* bpred-tage.cpp: TAgged GEometric-history length predictor [Seznec and Michaud, JILP 2006] */
/*
 * __COPYRIGHT__ GT
 */

#define COMPONENT_NAME "tage"

#ifdef BPRED_PARSE_ARGS
if(!strcasecmp(COMPONENT_NAME,type))
{
  int num_tables;
  int bim_size;
  int table_size;
  int tag_width;
  int first_length;
  int last_length;

  if(sscanf(opt_string,"%*[^:]:%[^:]:%d:%d:%d:%d:%d:%d",name,&num_tables,&bim_size,&table_size,&tag_width,&first_length,&last_length) != 7)
    fatal("bad bpred options string %s (should be \"tage:name:num-tables:bim-size:table-size:tag-width:first-hist-length:last-hist-length\")",opt_string);
  return new bpred_tage_t(name,num_tables,bim_size,table_size,tag_width,first_length,last_length);
}
#else

#include <math.h>


class bpred_tage_t:public bpred_dir_t
{
#define TAGE_MAX_HIST 512
#define TAGE_MAX_TABLES 16

  typedef qword_t bpred_tage_hist_t[8]; /* Max length: 8 * 64 = 512 */

  struct bpred_tage_ent_t
  {
    int tag;
    char ctr;
    char u;
  };

  class bpred_tage_sc_t:public bpred_sc_t
  {
    public:
    my2bc_t * current_ctr;
    bpred_tage_hist_t lookup_bhr;
    int index[TAGE_MAX_TABLES];
    int provider;
    int provpred;
    int alt;
    int altpred;
    int used_alt;
    int provnew;
  };

  private:

  inline void bpred_tage_hist_update(bpred_tage_hist_t H, int outcome)
  {
    H[7] <<= 1; H[7] |= (H[6]>>63)&1;
    H[6] <<= 1; H[6] |= (H[5]>>63)&1;
    H[5] <<= 1; H[5] |= (H[4]>>63)&1;
    H[4] <<= 1; H[4] |= (H[3]>>63)&1;
    H[3] <<= 1; H[3] |= (H[2]>>63)&1;
    H[2] <<= 1; H[2] |= (H[1]>>63)&1;
    H[1] <<= 1; H[1] |= (H[0]>>63)&1;
    H[0] <<= 1; H[0] |= outcome&1;
  }

  inline void bpred_tage_hist_copy(bpred_tage_hist_t D /*dest*/, bpred_tage_hist_t S /*src*/)
  {
    D[0] = S[0];
    D[1] = S[1];
    D[2] = S[2];
    D[3] = S[3];
    D[4] = S[4];
    D[5] = S[5];
    D[6] = S[6];
    D[7] = S[7];
  }

  int bpred_tage_hist_hash(bpred_tage_hist_t H, int hist_length, int hash_length)
  {
    int result = 0;
    int i;
    int mask = (1<<hash_length)-1;
    int row=0;
    int pos=0;
    for(i=0;i<hist_length/hash_length;i++)
    {
      result ^= (H[row]>>pos) & mask;
      if(pos+hash_length > 64) /* wrap past end of current qword_t */
      {
        row++;
        pos = (pos+hash_length)&63;
        result ^= (H[row]<<(hash_length-pos))&mask;
      }
      else /* includes when pos+HL is exactly 64 */
      {
        pos = pos+hash_length;
        if(pos > 63)
        {
          pos &=63;
          row++;
        }
      }
    }
    return result;
  }

  protected:

  int num_tables;
  int table_size;
  int log_size;
  int table_mask;
  int tag_width;
  int tag_mask;

  int bim_size;
  int bim_mask;

  int alpha;
  int * Hlen; /* array of history lengths */
  struct bpred_tage_ent_t **T;
  int pwin;

  zcounter_t * Tuses;

  bpred_tage_hist_t bhr;

  public:

  /* CREATE */
  bpred_tage_t(char * const arg_name,
               const int arg_num_tables,
               const int arg_bim_size,
               const int arg_table_size,
               const int arg_tag_width,
               const int arg_first_length,
               const int arg_last_length
              )
  {
    init();

    int i;
    /* verify arguments are valid */
    CHECK_PPOW2(arg_bim_size);
    CHECK_PPOW2(arg_table_size);
    CHECK_POS(arg_num_tables);
    CHECK_POS(arg_tag_width);
    CHECK_POS(arg_first_length);
    CHECK_POS(arg_last_length);
    CHECK_POS_GT(arg_last_length,arg_first_length);
    CHECK_POS_LT(arg_num_tables,TAGE_MAX_TABLES);
    CHECK_POS_LEQ(arg_last_length,TAGE_MAX_HIST);

    for(i=0;i<8;i++)
      bhr[i] = 0;

    name = strdup(arg_name);
    if(!name)
      fatal("couldn't malloc tage name (strdup)");
    type = strdup(COMPONENT_NAME);
    if(!type)
      fatal("couldn't malloc tage type (strdup)");

    num_tables = arg_num_tables;
    table_size = arg_table_size;
    table_mask = arg_table_size-1;
    log_size = log_base2(arg_table_size);

    bim_size = arg_bim_size;
    bim_mask = arg_bim_size-1;

    double logA = (log(arg_last_length) - log(arg_first_length)) / ((double)arg_num_tables - 2.0);
    alpha = (int) exp(logA);

    Hlen = (int*) calloc(arg_num_tables,sizeof(*Hlen));
    if(!Hlen)
      fatal("couldn't calloc Hlen array");

    /* Hlen[0] = 0; set by calloc, for 2bc */
    for(i=1;i<num_tables-1;i++)
      Hlen[i] = (int)floor(arg_first_length * pow(alpha,i-1) + 0.5);
    Hlen[i] = arg_last_length;

    T = (struct bpred_tage_ent_t**) calloc(num_tables,sizeof(*T));
    if(!T)
      fatal("couldn't calloc tage T array");

    tag_width = arg_tag_width;
    tag_mask = (1<<arg_tag_width)-1;

    for(i=0;i<num_tables;i++)
    {
      if(i>0)
      {
        T[i] = (struct bpred_tage_ent_t*) calloc(table_size,sizeof(**T));
        if(!T[i])
          fatal("couldn't calloc tage T[%d]",i);
        for(int j=0;j<table_size;j++)
        {
          T[i][j].tag = 0;
          T[i][j].ctr = 4; /* weakly taken */
        }
      }
      else /* T0 */
      {
        T[i] = (struct bpred_tage_ent_t*) calloc(bim_size,sizeof(**T));
        if(!T[i])
          fatal("couldn't calloc tage T[%d]",i);
        for(int j=0;j<bim_size;j++)
          T[i][j].ctr = MY2BC_WEAKLY_TAKEN; /* weakly taken */
      }
    }

    Tuses = (zcounter_t*) calloc(num_tables,sizeof(*Tuses));
    if(!Tuses)
      fatal("failed to calloc Tuses");

    pwin = 15;

    bits = bim_size*2 /*2bc*/ + (num_tables-1)*(2/*u*/+3/*ctr*/+tag_width)*table_size + arg_last_length + 4/*use altpred?*/;
  }

  /* DESTROY */
  ~bpred_tage_t()
  {
    free(Tuses); Tuses = NULL;
    for(int i=0;i<num_tables;i++)
    {
      free(T[i]); T[i] = NULL;
    }
    free(T); T = NULL;
    free(Hlen); Hlen = NULL;
  }

  /* LOOKUP */
  BPRED_LOOKUP_HEADER
  {
    class bpred_tage_sc_t * sc = (class bpred_tage_sc_t*) scvp;

    for(int i=0;i<num_tables;i++)
    {
      if(i==0)
        sc->index[i] = PC;
      else
        sc->index[i] = PC^bpred_tage_hist_hash(bhr,Hlen[i],log_size);
    }
    sc->provider = 0;
    sc->provpred = 0;
    sc->altpred = 0;
    sc->provnew = false;

    int hit = false;
    int pwin_hit = false;
    for(int i=num_tables-1;i>0;i--)
    {
      struct bpred_tage_ent_t * ent = &T[i][sc->index[i]&table_mask];
      if(((unsigned)ent->tag) == (PC&tag_mask))
      {
        sc->provnew = !ent->u && ((ent->ctr==4) || (ent->ctr==3));
        if((pwin < 8) || !sc->provnew)
          pwin_hit = true;
        sc->provpred = (ent->ctr >= 4);
        hit = true;
        sc->provider = i;
        break;
      }
    }

    sc->alt = -1;
    if(hit)
    {
      for(int i=sc->provider-1;i>0;i--)
      {
        struct bpred_tage_ent_t * ent = &T[i][sc->index[i]&table_mask];
        if((unsigned)ent->tag == (PC&tag_mask))
        {
          sc->altpred = (ent->ctr >= 4);
          sc->alt = i;
          break;
        }
      }
    }

    if(sc->alt == -1)
    {
      sc->altpred = MY2BC_DIR(T[0][sc->index[0]&bim_mask].ctr);
      sc->alt = 0;
    }

    sc->updated = false;

    BPRED_STAT(lookups++;)
    bpred_tage_hist_copy(sc->lookup_bhr,bhr);

    if(!pwin_hit)
    {
      sc->used_alt = true;
      return sc->altpred;
    }
    else
    {
      sc->used_alt = false;
      return sc->provpred;
    }
  }

  /* UPDATE */
  BPRED_UPDATE_HEADER
  {
    class bpred_tage_sc_t * sc = (class bpred_tage_sc_t*) scvp;

    if(!sc->updated)
    {
      BPRED_STAT(updates++;)
      BPRED_STAT(num_hits += our_pred == outcome;)

      if(sc->used_alt)
        Tuses[sc->alt]++;
      else
      {
        assert(sc->provider);
        Tuses[sc->provider]++;
      }


      if(sc->provnew)
      {
        if(sc->altpred != sc->provpred)
        {
          if(sc->altpred == outcome)
          {
            if(pwin < 15)
              pwin++;
          }
          else
          {
            if(pwin > 0)
              pwin--;
          }
        }
      }

      /* Allocate new entry if needed */
      if(our_pred != outcome) /* mispred */
      {
        if(sc->provider < (num_tables-1)) /* not already using longest history */
        {
          int allocated = 0;
          int allocated2 = 0;
          for(int i=sc->provider+1;i<num_tables;i++)
          {
            if(T[i][sc->index[i]&table_mask].u == 0)
            {
              if(!allocated)
                allocated = i;
              else
              {
                allocated2 = i;
                break;
              }
            }
          }
          if(allocated)
          {
            if(allocated2) /* more than one choice */
            {
              if((random() & 0xffff) > 21845) /* choose allocated over allocated2 with 2:1 probability */
                allocated = allocated2;
            }
            struct bpred_tage_ent_t * ent = &T[allocated][sc->index[allocated]&table_mask];
            ent->u = 0;
            ent->ctr = 3+outcome;
            ent->tag = PC & tag_mask;
          }
          else
          {
            for(int i=sc->provider+1;i<num_tables;i++)
              if(T[i][sc->index[i]&table_mask].u > 0)
                T[i][sc->index[i]&table_mask].u --;
          }
        }
      }

      /* perioidic reset of ubits */
      if((updates & ((1<<18)-1)) == 0)
      {
        int mask = ~1;
        if(updates & (1<<18)) /* alternate reset msb and lsb */
          mask = ~2;
        for(int i=1;i<num_tables;i++)
        {
          for(int j=0;j<table_size;j++)
            T[i][j].u &= mask;
        }
      }

      /* update provider component */
      if((sc->provider == 0) || ((sc->alt == 0) && (sc->used_alt)))
        MY2BC_UPDATE(T[0][sc->index[0]&bim_mask].ctr,outcome);
      else
      {
        if(outcome)
        {
          if(T[sc->provider][sc->index[sc->provider]&table_mask].ctr < 7)
            T[sc->provider][sc->index[sc->provider]&table_mask].ctr ++;
        }
        else
        {
          if(T[sc->provider][sc->index[sc->provider]&table_mask].ctr > 0)
            T[sc->provider][sc->index[sc->provider]&table_mask].ctr --;
        }
      }

      /* update u counter: inc if correct and different than altpred */
      if(sc->provpred != sc->altpred)
      {
        if(sc->provpred == outcome)
        {
          if(T[sc->provider][sc->index[sc->provider]&table_mask].u < 3)
            T[sc->provider][sc->index[sc->provider]&table_mask].u ++;
        }
        else
        {
          if(T[sc->provider][sc->index[sc->provider]&table_mask].u > 0)
            T[sc->provider][sc->index[sc->provider]&table_mask].u --;
        }
      }

      sc->updated = true;
    }
  }

  /* SPEC UPDATE */
  BPRED_SPEC_UPDATE_HEADER
  {
    BPRED_STAT(spec_updates++;)
    bpred_tage_hist_update(bhr,our_pred);
  }

  BPRED_RECOVER_HEADER
  {
    class bpred_tage_sc_t * sc = (class bpred_tage_sc_t*) scvp;

    bpred_tage_hist_copy(bhr,sc->lookup_bhr);
    bpred_tage_hist_update(bhr,outcome);
  }

  BPRED_FLUSH_HEADER
  {
    class bpred_tage_sc_t * sc = (class bpred_tage_sc_t*) scvp;

    bpred_tage_hist_copy(bhr,sc->lookup_bhr);
  }

  /* REG_STATS */
  BPRED_REG_STATS_HEADER
  {
    bpred_dir_t::reg_stats(sdb,core);
    int id = core?core->current_thread->id:0;
    for(int i=0;i<num_tables;i++)
    {
      char buf[256];
      char buf2[256];

      sprintf(buf,"c%d.%s.uses%d",id,name,i);
      sprintf(buf2,"predictions made with %s's T[%d]",name,i);
      stat_reg_counter(sdb, true, buf, buf2, &Tuses[i], 0, NULL);
    }
  }

  /* GETCACHE */
  BPRED_GET_CACHE_HEADER
  {
    class bpred_tage_sc_t * sc = new bpred_tage_sc_t();
    if(!sc)
      fatal("couldn't malloc tage State Cache");
    return sc;
  }

};


#endif /* BPRED_PARSE_ARGS */
#undef COMPONENT_NAME
