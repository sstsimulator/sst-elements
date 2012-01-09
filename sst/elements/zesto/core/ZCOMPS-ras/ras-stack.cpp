/* ras-stack.cpp: Conventional fixed capacity Return Address Stack predictor */
/*
 * __COPYRIGHT__ GT
 */

#define COMPONENT_NAME "stack"

#ifdef BPRED_PARSE_ARGS
if(!strcasecmp(COMPONENT_NAME,type))
{
  int size;
  if(sscanf(opt_string,"%*[^:]:%[^:]:%d",name,&size) != 2)
    fatal("bad ras options string %s (should be \"stack:name:size\")",opt_string);
  return new RAS_stack_t(name,size);
}
#else

class RAS_stack_t:public RAS_t
{
  class RAS_stack_chkpt_t:public RAS_chkpt_t
  {
    public:
    int pos;
  };

  protected:

  md_addr_t *stack;
  int size;
  int head;

  public:

  /* CREATE */
  RAS_stack_t(char * const arg_name,
              const int arg_num_entries
             )
  {
    init();

    size = arg_num_entries;
    stack = (md_addr_t*) calloc(arg_num_entries,sizeof(md_addr_t));
    if(!stack)
      fatal("couldn't malloc stack stack");

    head = 0;

    name = strdup(arg_name);
    if(!name)
      fatal("couldn't malloc stack name (strdup)");

    bits = size*8*sizeof(md_addr_t);
    type = strdup("finite RAS");
  }

  /* DESTROY */
  ~RAS_stack_t()
  {
    free(stack); stack = NULL;
    free(name); name = NULL;
    free(type); type = NULL;
  }

  /* PUSH */
  RAS_PUSH_HEADER
  {
    head = modinc(head,size); //(head + 1) % size;
    stack[head] = ftPC;
  }

  /* POP */
  RAS_POP_HEADER
  {
    md_addr_t predPC = stack[head];
    head = moddec(head,size); //(head - 1 + size) % size;

    return predPC;
  }

  /* RECOVER */
  RAS_RECOVER_HEADER
  {
    class RAS_stack_chkpt_t * cp = (class RAS_stack_chkpt_t*) cpvp;
    BPRED_STAT(num_recovers++;)

    head = cp->pos;
  }

  /* RECOVER INDEX */
  RAS_RECOVER_STATE_HEADER
  {
    class RAS_stack_chkpt_t * cp = (class RAS_stack_chkpt_t*) cpvp;
    cp->pos = head;
  }

  /* GET_STATE */
  RAS_GET_STATE_HEADER
  {
    class RAS_stack_chkpt_t * cp = new RAS_stack_chkpt_t();
    if(!cp)
      fatal("couldn't malloc stack recovery state");
    return cp;
  }

  /* RET_STATE */
  RAS_RET_STATE_HEADER
  {
    delete(cpvp);
  }

};

#endif /* RAS_PARSE_ARGS */
#undef COMPONENT_NAME
