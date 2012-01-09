/* fusion-multihybrid.cpp: multihybrid selection table: two-level (four-way)
   tournament selection.  Given four predictors P0, P1, P2 and P3, meta-predictor
   M01 uses tournament prediction (see fusion_mcfarling.cpp) to select one
   prediction from P0 and P1, meta-predictor M23 selects from P2 and P3, and a
   final meta-meta-predictor selects between M01 and M23.
   NOTE: The number of predictors must equal four. */
/*
 * __COPYRIGHT__ GT
 */

#define COMPONENT_NAME "multihybrid"

#ifdef BPRED_PARSE_ARGS
if(!strcasecmp("multihybrid",type))
{
  int num_entries;

  if(sscanf(opt_string,"%*[^:]:%[^:]:%d",name,&num_entries) != 2)
    fatal("bad fusion options string %s (should be \"multihybrid:name:num_entries\")",opt_string);
  return new fusion_multihybrid_t(name,num_pred,num_entries);
}
#else

static const char meta_selection[2][2][2] =
{
  {
    {0, 0}, {1, 1}
  },
  
  {
    {2, 3}, {2, 3}
  }
};

class fusion_multihybrid_t:public fusion_t
{
  protected:
 
  int meta_size;
  int meta_mask;
  my2bc_t ** meta;

  int meta_direction;

  public:

  /* CREATE */
  fusion_multihybrid_t(char * const arg_name,
                       const int arg_num_pred,
                       const int arg_meta_size
                      )
  {
    init();

    /* verify arguments are valid */
    CHECK_PPOW2(arg_meta_size);
    if(arg_num_pred != 4)
      fatal("multihybrid must have exactly four components");

    name = strdup(arg_name);
    if(!name)
      fatal("couldn't malloc fusion multihybrid name (strdup)");
    type = strdup("multihybrid Selection");
    if(!type)
      fatal("couldn't malloc fusion multihybrid type (strdup)");

    num_pred = arg_num_pred;
    meta_size = arg_meta_size;
    meta_mask = arg_meta_size-1;
    meta = (my2bc_t**) calloc(3,sizeof(*meta));
    if(!meta)
      fatal("couldn't malloc multihybrid meta-table");
    meta[0] = (my2bc_t*) calloc(meta_size,sizeof(my2bc_t));
    meta[1] = (my2bc_t*) calloc(meta_size,sizeof(my2bc_t));
    meta[2] = (my2bc_t*) calloc(meta_size,sizeof(my2bc_t));
    if(!meta[0] || !meta[1] || !meta[2])
      fatal("couldn't malloc multihybrid meta-table entries");

    for(int i=0;i<meta_size;i++)
    {
      meta[0][i] = 3;
      meta[1][i] = (i&1)?MY2BC_WEAKLY_NT:MY2BC_WEAKLY_TAKEN;
      meta[2][i] = (i&1)?MY2BC_WEAKLY_TAKEN:MY2BC_WEAKLY_NT;
    }
    
    bits =  meta_size*7;
  }

  /* DESTROY */
  ~fusion_multihybrid_t()
  {
    if(meta) free(meta); meta = NULL;
  }

  /* LOOKUP */
  FUSION_LOOKUP_HEADER
  {
    int index = PC&meta_mask;
    bool pred;

    meta_direction = meta_selection[(meta[0][index]>>2)]
                                   [MY2BC_DIR(meta[1][index])]
                                   [MY2BC_DIR(meta[2][index])];
    pred = preds[meta_direction];

    BPRED_STAT(lookups++;)
    scvp->updated = false;
    return pred;
  }

  /* UPDATE */
  FUSION_UPDATE_HEADER
  {
    int index = PC&meta_mask;

    if(!scvp->updated)
    {
      BPRED_STAT(updates++;)
      BPRED_STAT(num_hits += our_pred == outcome;)
      scvp->updated = true;
    }

    /* update the top level meta-predictor when the outcomes of the two selected
       components disagree */
    if(preds[MY2BC_DIR(meta[1][index])] ^ preds[2+MY2BC_DIR(meta[2][index])])
    {
      if((preds[2+MY2BC_DIR(meta[2][index])]==outcome))
      {
        if(meta[0][index] < 7)
          ++ meta[0][index];
      }
      else
      {
        if(meta[0][index] > 0)
          -- meta[0][index];
      }
    }

    /* meta table for each set of two components only updated when
       those predictors disagree. If both agree, then either both
       are right, or both are wrong: in either case the meta table
       is not updated. */
    if(preds[0]^preds[1])
    {
      MY2BC_UPDATE(meta[1][index],(preds[1]==outcome));
    }
    if(preds[2]^preds[3])
    {
      MY2BC_UPDATE(meta[2][index],(preds[3]==outcome));
    }
  }
};

#endif /* BPRED_PARSE_ARGS */
#undef COMPONENT_NAME
