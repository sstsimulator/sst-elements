/* fusion-priority.cpp: ``Multi-Hybrid'' priority selection table [Evers et al., ISCA 1996] */
/*
 * __COPYRIGHT__ GT
 */

#define COMPONENT_NAME "priority"

#ifdef BPRED_PARSE_ARGS
if(!strcasecmp("priority",type))
{
  int num_entries;

  if(sscanf(opt_string,"%*[^:]:%[^:]:%d",name,&num_entries) != 2)
    fatal("bad fusion options string %s (should be \"priority:name:num_entries\")",opt_string);
  return new fusion_priority_t(name,num_pred,num_entries);
}
#else

class fusion_priority_t:public fusion_t
{
  protected:

  int meta_size;
  int meta_mask;
  my2bc_t ** meta;

  public:

  /* CREATE */
  fusion_priority_t(char * const arg_name,
                    const int arg_num_pred,
                    const int arg_meta_size
                   )
  {
    init();

    /* verify arguments are valid */
    CHECK_PPOW2(arg_meta_size);

    name = strdup(arg_name);
    if(!name)
      fatal("couldn't malloc fusion priority name (strdup)");
    type = strdup("priority Multi-Hybrid");
    if(!type)
      fatal("couldn't malloc fusion priority type (strdup)");


    num_pred = arg_num_pred;
    meta_size = arg_meta_size;
    meta_mask = arg_meta_size-1;
    meta = (my2bc_t**) calloc(arg_meta_size,sizeof(my2bc_t*));
    if(!meta)
      fatal("couldn't malloc priority meta-table");
    for(int i=0;i<meta_size;i++)
    {
      meta[i] = (my2bc_t*) calloc(num_pred,sizeof(my2bc_t));
      if(!meta[i])
        fatal("couldn't malloc priority meta-table[%d]",i);
      for(int j=0;j<num_pred;j++)
        meta[i][j] = 3; /* all counters initialized to 3 */
    }

    bits = meta_size*num_pred*2;
  }

  /* DESTROY */
  ~fusion_priority_t()
  {
    for(int i=0;i<meta_size;i++)
    {
      free(meta[i]); meta[i] = NULL;
    }
    free(meta); meta = NULL;
  }

  /* LOOKUP */
  /* we don't do any unrolled version of the lookup function;
     I haven't thought of any reasonable implementation that
     isn't basically the same as the generic version. -GL */
  FUSION_LOOKUP_HEADER
  {
    int index = PC&meta_mask;
    my2bc_t * meta_counters = meta[index];
    bool pred;
    int i;

    /* because all meta_counters are initialized to 3, and the update
       rules ensure that there will always be a three, this loop will
       always terminate through the break. */
    for(i=0;i<num_pred;i++)
    {
      if(meta_counters[i]==MY2BC_STRONG_TAKEN)
        break;
    }

    pred = preds[i];

    BPRED_STAT(lookups++;)
    scvp->updated = false;

    return pred;
  }

  /* UPDATE */

  FUSION_UPDATE_HEADER
  {
    int i;
    int a_3_was_correct = false;
    int index = PC&meta_mask;
    my2bc_t * meta_counters = meta[index];

    if(!scvp->updated)
    {
        BPRED_STAT(updates++;)
        BPRED_STAT(num_hits += our_pred == outcome;)
        scvp->updated = true;
    }

    /* update rule depends on whether one of the meta-counters with a
       value of 3 predicted correctly ("a 3 was correct"). */
    for(i=0;i<num_pred;i++)
      if(meta_counters[i]==MY2BC_STRONG_TAKEN && preds[i]==outcome)
      {
        a_3_was_correct = true;
        break;
      }

    /* if one of the components that had a meta-counter value of 3
       was correct, decrement the meta-counters of all components
       that mispredicted. */
    if(a_3_was_correct)
      for(i=0;i<num_pred;i++)
      {
        MY2BC_COND_DEC(meta_counters[i],(preds[i]!=outcome));
      }
    else /* otherwise increment those who were correct */
      for(i=0;i<num_pred;i++)
      {
        MY2BC_COND_INC(meta_counters[i],(preds[i]==outcome));
      }
  }

};

#endif /* BPRED_PARSE_ARGS */
#undef COMPONENT_NAME
