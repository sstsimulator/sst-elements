/* bpred-alloyedperceptron.cpp: alloyed-history perceptron predictor
   [Jimenez and Lin, ACM TOCS 2002]     */
/*
 * __COPYRIGHT__ GT
 */

#include <math.h>
#define COMPONENT_NAME "alloyedperceptron"

#ifdef BPRED_PARSE_ARGS
if(!strcasecmp(COMPONENT_NAME,type))
{
  int l1_size;
  int his_width;
  int gbhr_width;
  int top_size;

  if(sscanf(opt_string,"%*[^:]:%[^:]:%d:%d:%d:%d",name,&gbhr_width,&l1_size,&his_width,&top_size) != 5)
    fatal("bad bpred options string %s (should be \"alloyedperceptron:name:gbhr_width:l1_size:lhis_width:top_size\")",opt_string);
  return new bpred_alloyedperceptron_t(name,gbhr_width,l1_size,his_width,top_size);
}
#else


class bpred_alloyedperceptron_t:public bpred_dir_t
{
#define BIPOLAR(x) (((x)<<1)-1)
//#define BIPOLAR(x) (2*(x)-1)

  class bpred_alloyedperceptron_sc_t:public bpred_sc_t
  {
    public:
    short* top_entry;
    short* ltop_entry;
    qword_t * lbhr;
    int sum;
    qword_t lookup_bhr;
  };

  protected:

  /* local hist */
  int bht_size;
  int bht_mask;
  qword_t * bht;
  /* global hist */
  int bhr_size;
  qword_t bhr;

  int top_size;
  short **top;  /* table of perceptrons */
  short **ltop; /* table of perceptrons */
  int weight_width;
  int weight_max;
  int weight_min;
  int threshold;

  int ghistory_length;
  qword_t ghistory_mask;
  int lhistory_length;
  qword_t lhistory_mask;

  public:

  /* CREATE */
  bpred_alloyedperceptron_t(char * const arg_name,
                            const int arg_ghistory_length,
                            const int arg_bht_size,
                            const int arg_lhistory_length,
                            const int arg_top_size
                           )
  {
    init();

    /* verify arguments are valid */
    CHECK_PPOW2(arg_bht_size);
    CHECK_NNEG(arg_ghistory_length);
    CHECK_NNEG(arg_lhistory_length);
    CHECK_NNEG(arg_top_size);

    name = strdup(arg_name);
    if(!name)
      fatal("couldn't malloc alloyedperceptron name (strdup)");
    type = strdup(COMPONENT_NAME);
    if(!type)
      fatal("couldn't malloc alloyedperceptron type (strdup)");

    bht_size = arg_bht_size;
    bht_mask = arg_bht_size-1;
    lhistory_length = arg_lhistory_length;
    lhistory_mask = (((qword_t)1)<<arg_lhistory_length)-1;
    ghistory_length = arg_ghistory_length;
    ghistory_mask = (((qword_t)1)<<arg_ghistory_length)-1;

    top_size = arg_top_size;

    /* see Dan Jimenez's HPCA paper for these magic formulae */
    threshold = (int)rint(floor(1.93*(ghistory_length+lhistory_length+1) + 14.0));

    /* if power of two, then actually need an extra bit to represent.
       also add one bit for the sign. */
    if(threshold == (1<<log_base2(threshold)))
      weight_width = log_base2(threshold)+2;
    else
      weight_width = log_base2(threshold)+1;

    weight_max = (1<<(weight_width-1))-1;
    weight_min = -(1<<(weight_width-1));

    bht = (qword_t*) calloc(bht_size,sizeof(*bht));
    if(!bht)
      fatal("couldn't malloc alloyedperceptron BHT");
    top = (short**) calloc(top_size,sizeof(*top));
    ltop = (short**) calloc(top_size,sizeof(*ltop));
    if(!top || !ltop)
      fatal("couldn't malloc alloyedperceptron ToP");
    for(int i=0;i<top_size;i++)
    {
      top[i] = (short*) calloc(ghistory_length,sizeof(*top[i]));
      if(!top[i])
        fatal("couldn't malloc alloyedperceptron ToP entry");
      ltop[i] = (short*) calloc(lhistory_length+1,sizeof(*ltop[i]));
      if(!ltop[i])
        fatal("couldn't malloc alloyedperceptron ToP entry");
    }

    bits = ghistory_length + bht_size*(lhistory_length)
         + top_size*(ghistory_length+lhistory_length+1)*weight_width;
  }

  /* DESTROY */
  ~bpred_alloyedperceptron_t()
  {
    for(int i=0;i<top_size;i++)
    {
      free(top[i]); top[i] = NULL;
      free(ltop[i]); ltop[i] = NULL;
    }
    free(ltop); ltop = NULL;
    free(top); top = NULL;
    free(bht); bht = NULL;
  }

  /* LOOKUP - loops unrolled by factor of 2.  This code takes a fair amount of
     time, and so we'll do what we can to speed it up. */
  BPRED_LOOKUP_HEADER
  {
    class bpred_alloyedperceptron_sc_t * sc = (class bpred_alloyedperceptron_sc_t*) scvp;
    int l1index = PC&bht_mask;
    bool pred;
    int top_index = PC%top_size;
    int x1; /* temporary */
    int y1; /* temporary */

    sc->lbhr = &bht[l1index];
    sc->top_entry = top[top_index];
    sc->ltop_entry = ltop[top_index];
    sc->sum = 0;
    sc->updated = false;

    for(int i=0;i<ghistory_length;i++)
    {
      /* add top_entry[i] if bit i in bhr is a one, subtract if zero */
      /* This is what one iteration used to do:
         if((*sc->bhr>>i)&1)
           sc->sum += sc->top_entry[i];
         else
           sc->sum -= sc->top_entry[i];
       */
      x1 = ((bhr>>i)&1);
      y1 = ((x1-1)^sc->top_entry[i])+(1-x1);  /* cond. negative: gives 2's complement if x1==0 */
      sc->sum += y1;
    }
    for(int i=0;i<lhistory_length;i++)
    {
      /* add top_entry[i] if bit i in bhr is a one, subtract if zero */
      /* This is what one iteration used to do:
         if((*sc->bhr>>i)&1)
           sc->sum += sc->top_entry[i];
         else
           sc->sum -= sc->top_entry[i];
       */
      x1 = ((*sc->lbhr>>i)&1);
      y1 = ((x1-1)^sc->ltop_entry[i])+(1-x1);  /* cond. negative: gives 2's complement if x1==0 */
      sc->sum += y1;
    }
    sc->sum += sc->ltop_entry[lhistory_length]; /* last entry corresponds to w_0, i.e. the bias */

    pred = sc->sum >= 0;
    sc->lookup_bhr = bhr;

    BPRED_STAT(lookups++;)

    return pred;
  }


  /* UPDATE */
  BPRED_UPDATE_HEADER
  {
    class bpred_alloyedperceptron_sc_t * sc = (class bpred_alloyedperceptron_sc_t*) scvp;
    int y_out;
    int t = BIPOLAR(outcome); /* make bipolar */
    int x; /* temporary */
    int diff;

    if(!sc->updated)
    {
      BPRED_STAT(updates++;)
      BPRED_STAT(num_hits += our_pred == outcome;)
      sc->updated = true;
    }

    /*
    if(sc->sum > threshold)
      y_out = 1;
    else if(sc->sum<-threshold)
      y_out = -1;
    else
      y_out = 0;
    */
    y_out = (sc->sum > threshold) - (sc->sum<-threshold);

    /* don't need to optimize this too much, since this code
       doesn't get called after the perceptron has ``finished''
       training */
    if(y_out != t)
    {
      for(int i=0;i<ghistory_length;i++)
      {
        //diff = t*BIPOLAR((*sc->bhr>>i)&1);
        x = ((sc->lookup_bhr>>i)&1);
        diff = ((x-1)^t)+(1-x);
        if(diff > 0)
        {
          if(sc->top_entry[i] < weight_max)
            sc->top_entry[i] ++;
        }
        else
        {
          if(sc->top_entry[i] > weight_min)
            sc->top_entry[i] --;
        }
      }
      for(int i=0;i<lhistory_length+1;i++)
      {
        if(i < lhistory_length)
        {
          //diff = t*BIPOLAR((*sc->bhr>>i)&1);
          x = ((*sc->lbhr>>i)&1);
          diff = ((x-1)^t)+(1-x);
        }
        else /* w_0 */
          diff = t;
        if(diff > 0)
        {
          if(sc->ltop_entry[i] < weight_max)
            sc->ltop_entry[i] ++;
        }
        else
        {
          if(sc->ltop_entry[i] > weight_min)
            sc->ltop_entry[i] --;
        }
      }
    }

    *sc->lbhr = ((*sc->lbhr<<1)|outcome)&lhistory_mask;
    bhr = ((bhr<<1)|outcome)&ghistory_mask;
  }

  /* SPEC_UPDATE */
  BPRED_SPEC_UPDATE_HEADER
  {
    BPRED_STAT(spec_updates++;)
    bhr = (bhr<<1)|our_pred;
  }

  /* RECOVER */
  BPRED_RECOVER_HEADER
  {
    class bpred_alloyedperceptron_sc_t * sc = (class bpred_alloyedperceptron_sc_t*) scvp;
    bhr = (sc->lookup_bhr<<1)|outcome;
  }

  /* FLUSH */
  BPRED_FLUSH_HEADER
  {
    class bpred_alloyedperceptron_sc_t * sc = (class bpred_alloyedperceptron_sc_t*) scvp;
    bhr = sc->lookup_bhr;
  }

  /* REG_STATS */
  BPRED_REG_STATS_HEADER
  {
    bpred_dir_t::reg_stats(sdb,core);

    int id = core?core->current_thread->id:0;
    char buf[256];
    char buf2[256];

    sprintf(buf,"c%d.%s.threshold",id,name);
    sprintf(buf2,"%s training threshold",type);
    stat_reg_int(sdb, true, buf, buf2, &threshold, threshold, NULL);
    sprintf(buf,"c%d.%s.weight_width",id,name);
    sprintf(buf2,"%s weight/counter width in bits",type);
    stat_reg_int(sdb, true, buf, buf2, &weight_width, weight_width, NULL);
  }

  /* GETCACHE */
  BPRED_GET_CACHE_HEADER
  {
    class bpred_alloyedperceptron_sc_t * sc = new bpred_alloyedperceptron_sc_t();
    if(!sc)
      fatal("couldn't malloc alloyedperceptron State Cache");
    return sc;
  }

};

#undef BIPOLAR
#endif /* BPRED_PARSE_ARGS */
#undef COMPONENT_NAME
