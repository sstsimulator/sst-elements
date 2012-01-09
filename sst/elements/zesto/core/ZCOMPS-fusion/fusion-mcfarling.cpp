/* fusion-mcfarling.cpp: McFarling selection table [McFarling, DEC-WRL TN36]
   NOTE: number of component predictors must equal two */
/*
 * __COPYRIGHT__ GT
 */

#define COMPONENT_NAME "mcfarling"

#ifdef BPRED_PARSE_ARGS
if(!strcasecmp("mcfarling",type))
{
  int num_entries;

  if(sscanf(opt_string,"%*[^:]:%[^:]:%d",name,&num_entries) != 2)
    fatal("bad fusion options string %s (should be \"mcfarling:name:num_entries\")",opt_string);
  return new fusion_mcfarling_t(name,num_pred,num_entries);
}
#else

class fusion_mcfarling_t:public fusion_t
{
  protected:
  int meta_size;
  int meta_mask;
  my2bc_t * meta;

  public:

  /* CREATE */
  fusion_mcfarling_t(char * const arg_name,
                     const int arg_num_pred,
                     const int arg_meta_size
                    )
  {
    init();

    /* verify arguments are valid */
    CHECK_PPOW2(arg_meta_size);
    if(arg_num_pred != 2)
      fatal("mcfarling fusion only valid for two component/direction predictors");

    name = strdup(arg_name);
    if(!name)
      fatal("couldn't malloc fusion mcfarling name (strdup)");
    type = strdup("McFarling tournament selection");
    if(!type)
      fatal("couldn't malloc fusion mcfarling type (strdup)");

    num_pred = arg_num_pred;
    meta_size = arg_meta_size;
    meta_mask = meta_size-1;
    meta = (my2bc_t*) calloc(meta_size,sizeof(my2bc_t));
    if(!meta)
      fatal("couldn't malloc mcfarling meta-table");
    bits =  meta_size*2;
  }

  /* DESTROY */
  ~fusion_mcfarling_t()
  {
    free(meta); meta = NULL;
  }

  /* LOOKUP */
  FUSION_LOOKUP_HEADER
  {
    int index = PC&meta_mask;
    int meta_direction = MY2BC_DIR(meta[index]);
    bool pred = preds[meta_direction];
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

    /* meta table only updated when predictors disagree. If both
       agree, then either both are right, or both are wrong: in
       either case the meta table is not updated. */
    if(preds[0]^preds[1])
    {
      MY2BC_UPDATE(meta[index],(preds[1]==outcome));
    }
  }

};

#endif /* BPRED_PARSE_ARGS */
#undef COMPONENT_NAME
