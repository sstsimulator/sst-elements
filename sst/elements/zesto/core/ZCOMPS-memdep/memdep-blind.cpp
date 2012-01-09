/* memdep-blind.cpp: Always predict that no memory dependencies/conflicts exist */
/*
 * __COPYRIGHT__ GT
 */
#define COMPONENT_NAME "blind"

#ifdef MEMDEP_PARSE_ARGS
if(!strcasecmp(COMPONENT_NAME,type))
{
  return new memdep_blind_t(arg_core);
}
#else

class memdep_blind_t: public memdep_t
{
  public:
  /* CONSTRUCTOR */
  memdep_blind_t(struct core_t * const arg_core)
  {
    init();
    name = strdup(COMPONENT_NAME); if(!name) fatal("failed to allocate memory for %s (strdup)",COMPONENT_NAME);
    type = strdup(COMPONENT_NAME); if(!type) fatal("failed to allocate memory for %s (strdup)",COMPONENT_NAME);

    core = arg_core;
  }
  /* LOOKUP */
  MEMDEP_LOOKUP_HEADER
  {
    #ifdef ZESTO_COUNTERS
    core->counters->memory_dependency.read++;
    #endif

    MEMDEP_STAT(lookups++;)
    return true;
  }
};

#endif /* MEMDEP_PARSE_ARGS */
#undef COMPONENT_NAME
