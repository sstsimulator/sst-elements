/* bpred-perceptron.cpp: The original perceptron predictor [Jimenez and Lin, ACM TOCS 2002] */
/*
 * __COPYRIGHT__ GT
 */

#include <math.h>
#define COMPONENT_NAME "perceptron"

#ifdef BPRED_PARSE_ARGS
if(!strcasecmp(COMPONENT_NAME,type))
{
  int his_width; /* bhr history length */
  int top_size; /* top = Table of Perceptrons, i.e., total number of perceptrons */

  if(sscanf(opt_string,"%*[^:]:%[^:]:%d:%d",name,&his_width,&top_size) != 3)
    fatal("bad bpred options string %s (should be \"perceptron:name:his_width:top_size\")",opt_string);
  return new bpred_perceptron_t(name,his_width,top_size);
}
#else

#define BIPOLAR(x) (((x)<<1)-1)
//#define BIPOLAR(x) (2*(x)-1)

class bpred_perceptron_t:public bpred_dir_t
{

  class bpred_perceptron_sc_t:public bpred_sc_t
  {
    public:
    short* top_entry;
    int sum;
    /* contents of bhr at lookup (used for update and recovery) */
    qword_t lookup_bhr;
    qword_t lookup_bhr_old;
  };

  protected:

  qword_t bhr;
  qword_t bhr_old;

  int top_size;
  int top_mask;
  short **top; /* table of perceptrons */
  int weight_width;
  int weight_max;
  int weight_min;
  int theta;

  int history_length;

  public:


  /* CREATE */
  bpred_perceptron_t(char * const arg_name,
                     const int arg_history_length,
                     const int arg_top_size
                    )
  {
    init();

    /* verify arguments are valid */
    CHECK_NNEG(arg_history_length);
    CHECK_PPOW2(arg_top_size);

    name = strdup(arg_name);
    if(!name)
      fatal("couldn't malloc perceptron name (strdup)");
    type = strdup(COMPONENT_NAME);
    if(!type)
      fatal("couldn't malloc perceptron type (strdup)");

    history_length = arg_history_length;

    top_size = arg_top_size;
    top_mask = arg_top_size-1;

    /* see Dan Jimenez's HPCA paper for these magic formulae */
    theta = (int) rint(floor(1.93*history_length + 14.0));

    /* if power of two, then actually need an extra bit to represent.
       also add one bit for the sign. */
    if(theta == (1<<log_base2(theta)))
      weight_width = log_base2(theta)+2;
    else
      weight_width = log_base2(theta)+1;

    weight_max = (1<<(weight_width-1))-1;
    weight_min = -(1<<(weight_width-1));

    bhr = 0;
    bhr_old = 0;
    top = (short**) calloc(top_size,sizeof(*top));
    if(!top)
      fatal("couldn't malloc perceptron ToP");
    for(int i=0;i<top_size;i++)
    {
      top[i] = (short*) calloc(1+history_length,sizeof(*top[i]));
      if(!top[i])
        fatal("couldn't malloc perceptron ToP entry");
    }

    bits =  history_length
         + top_size*(history_length+1)*weight_width;
  }

  /* DESTROY */
  ~bpred_perceptron_t()
  {
    for(int i=0;i<top_size;i++)
    {
      free(top[i]);
      top[i] = NULL;
    }
    free(top); top = NULL;
  }

  /* LOOKUP */
  BPRED_LOOKUP_HEADER
  {
    class bpred_perceptron_sc_t * sc = (class bpred_perceptron_sc_t*) scvp;
    bool pred;
    int top_index = PC&top_mask;

    sc->top_entry = top[top_index];
    sc->sum = sc->top_entry[0];

    for(int i=0;i<history_length;i++)
    {
      int hist_dir;
      if(i<64) hist_dir = BIPOLAR((bhr>>i)&1);
      else     hist_dir = BIPOLAR((bhr_old>>(i-64))&1);
      sc->sum += sc->top_entry[i+1]*hist_dir;
    }

    pred = sc->sum >= 0;
    sc->lookup_bhr_old = bhr_old;
    sc->lookup_bhr = bhr;

    BPRED_STAT(lookups++;)
    sc->updated = false;

    return pred;
  }

  /* UPDATE */
  BPRED_UPDATE_HEADER
  {
    class bpred_perceptron_sc_t * sc = (class bpred_perceptron_sc_t*) scvp;

    if((((sc->sum >=0)?1:0) != outcome) ||
       ((sc->sum < theta) && (sc->sum > -theta)))
    {
      if(outcome) {
          if(sc->top_entry[0] < weight_max) sc->top_entry[0]++;
      } else {
          if(sc->top_entry[0] > weight_min) sc->top_entry[0]--;
      }
      for(int i=0;i<history_length;i++)
      {
        int hist_bit;
        if(i<64) hist_bit = (sc->lookup_bhr>>i)&1;
        else     hist_bit = (sc->lookup_bhr_old>>(i-64))&1;
        if(hist_bit == outcome) {
          if(sc->top_entry[i+1] < weight_max) sc->top_entry[i+1]++;
        } else {
          if(sc->top_entry[i+1] > weight_min) sc->top_entry[i+1]--;
        }
      }
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
    BPRED_STAT(spec_updates++;)

    bhr_old = (bhr_old<<1)|((bhr>>63)&1);
    bhr = (bhr<<1)|our_pred;
  }

  /* RECOVER */
  BPRED_RECOVER_HEADER
  {
    class bpred_perceptron_sc_t * sc = (class bpred_perceptron_sc_t*) scvp;
    bhr_old = (sc->lookup_bhr_old<<1)|(sc->lookup_bhr<1);
    bhr = (sc->lookup_bhr<<1)|outcome;
  }

  /* FLUSH */
  BPRED_FLUSH_HEADER
  {
    class bpred_perceptron_sc_t * sc = (class bpred_perceptron_sc_t*) scvp;
    bhr_old = sc->lookup_bhr_old;
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
    stat_reg_int(sdb, true, buf, buf2, &theta, theta, NULL);
    sprintf(buf,"c%d.%s.weight_width",id,name);
    sprintf(buf2,"%s weight/counter width in bits",type);
    stat_reg_int(sdb, true, buf, buf2, &weight_width, weight_width, NULL);
  }

  /* GET_CACHE */
  BPRED_GET_CACHE_HEADER
  {
    class bpred_perceptron_sc_t * sc = new bpred_perceptron_sc_t();
    if(!sc)
      fatal("couldn't malloc perceptron State Cache");
    return sc;
  }

};

#undef BIPOLAR
#endif /* BPRED_PARSE_ARGS */
#undef COMPONENT_NAME
