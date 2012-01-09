/* dram-simplescalar.cpp:
   Based on SimpleScalar memory model (constant latency DRAM):
   Given latency in cpu cycles, or in ns which
   we'll convert to cpu cycles for you.
   Per-chunk latency computed based on cpu-speed,
   fsb-speed, and whether fsb uses DDR.

   This isn't precisely the same as the traditional SimpleScalar
   mode, but it's pretty close.

   usage:
     -dram simplescalar:LAT:UNITS (UNITS: 0=cycles, 1=ns)

   examples:
     -dram simplescalar:160   (160 sim_cycle's per access)
     -dram simplescalar:160:0 (ditto)
     -dram simplescalar:120:1 (120 ns per access) */
/*
 * __COPYRIGHT__ GT
 */


#define COMPONENT_NAME "simplescalar"

#ifdef DRAM_PARSE_ARGS
if(!strcasecmp(COMPONENT_NAME,type))
{
  int latency;
  int units = 0; /* 0=cycle, 1=ns */
           
  if(sscanf(opt_string,"%*[^:]:%d:%d",&latency,&units) != 2)
  {
    if(sscanf(opt_string,"%*[^:]:%d",&latency) != 1)
      fatal("bad dram options string %s (should be \"simplescalar:latency[:units]\")",opt_string);
  }
  return new dram_simplescalar_t(latency,units);
}
#else

class dram_simplescalar_t:public dram_t
{
  protected:
  int latency;
  int chunk_latency;

  public:

  /* CREATE */
  dram_simplescalar_t(int arg_latency,
                      int arg_units)
  {
    init();

    if(arg_units < 0 || arg_units > 1)
      fatal("simplescalar dram model's optional units arguments should be 0 (cycles) or 1 (ns)");

    if(arg_units)
      latency = (int)ceil(arg_latency * uncore->cpu_speed/1000.0);
    else
      latency = arg_latency;

    chunk_latency = uncore->cpu_ratio;
    best_latency = INT_MAX;
  }

  /* ACCESS */
  DRAM_ACCESS_HEADER
  {
    int chunks = (bsize + (uncore->fsb_width-1)) >> uncore->fsb_bits; /* burst length, rounded up */
    dram_assert(chunks > 0,latency);
    int lat = latency + ((chunk_latency * chunks)>>uncore->fsb_DDR);

    total_accesses++;
    total_latency += lat;
    total_burst += chunks;
    if(lat < best_latency)
      best_latency = lat;
    if(lat > worst_latency)
      worst_latency = lat;

    return lat;
  }
};

#endif /* DRAM_PARSE_ARGS */
#undef COMPONENT_NAME
