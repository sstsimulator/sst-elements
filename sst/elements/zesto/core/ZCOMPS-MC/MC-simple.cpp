/* MC-simple.cpp: Simple memory controller */
/*
 * __COPYRIGHT__ GT
 */

#define COMPONENT_NAME "simple"

#ifdef MC_PARSE_ARGS
if(!strcasecmp(COMPONENT_NAME,type))
{
  int RQ_size;
  int FIFO;
           
  if(sscanf(opt_string,"%*[^:]:%d:%d",&RQ_size,&FIFO) != 2)
    fatal("bad memory controller options string %s (should be \"simple:RQ-size:FIFO\")",opt_string);
/* ----- SoM ----- */
// uncore_t added in the argument for counter access
  return new MC_simple_t(RQ_size,FIFO,arg_uncore);
/* ----- EoM ----- */
}
#else

/* This implements a very simple queue of memory requests.
   Requests are enqueued and dequeued in FIFO order, but the
   MC scans the requests to try to batch requests to the
   same page together to avoid frequent opening/closing of
   pages (or set the flag to force FIFO request scheduling). */
class MC_simple_t:public MC_t
{
  protected:
  int RQ_size;        /* size of request queue, i.e. maximum number of outstanding requests */
  int RQ_num;         /* current number of requests */
  int RQ_head;        /* next transaction to send back to the cpu(s) */
  int RQ_req_head;    /* next transaction to send to DRAM */
  int RQ_tail;
  struct MC_action_t * RQ;

  bool FIFO_scheduling;
  md_paddr_t last_request;
  tick_t next_available_time;

  public:

/* ----- SoM ----- */
// uncore_t added in the argument for counter access
  MC_simple_t(const int arg_RQ_size, const int arg_FIFO, struct uncore_t * const arg_uncore):
              RQ_num(0), RQ_head(0), RQ_req_head(0), RQ_tail(0), last_request(0), next_available_time(0)
/* ----- EoM ----- */
  {
    init();

/* ----- SoM ----- */
    uncore = arg_uncore;
/* ----- EoM ----- */
    RQ_size = arg_RQ_size;
    FIFO_scheduling = arg_FIFO;
    RQ = (struct MC_action_t*) calloc(RQ_size,sizeof(*RQ));
    if(!RQ)
      fatal("failed to calloc memory controller request queue");
  }

  ~MC_simple_t()
  {
    free(RQ); RQ = NULL;
  }

  MC_ENQUEUABLE_HEADER
  {
    return RQ_num < RQ_size;
  }

  /* Enqueue a memory command (read/write) to the memory controller. */
  MC_ENQUEUE_HEADER
  {
    MC_assert(RQ_num < RQ_size,(void)0);

    struct MC_action_t * req = &RQ[RQ_tail];

    MC_assert(req->valid == false,(void)0);
    req->valid = true;
    req->prev_cp = prev_cp;
    req->cmd = cmd;
    req->addr = addr;
    req->linesize = linesize;
    req->op = op;
    req->action_id = action_id;
    req->MSHR_bank = MSHR_bank;
    req->MSHR_index = MSHR_index;
    req->cb = cb;
    req->get_action_id = get_action_id;
    req->when_enqueued = sim_cycle;
    req->when_started = TICK_T_MAX;
    req->when_finished = TICK_T_MAX;
    req->when_returned = TICK_T_MAX;

    RQ_num++;
    uncore->counters.mc.write++;
    RQ_tail = modinc(RQ_tail,RQ_size); //(RQ_tail + 1) % RQ_size;
    total_accesses++;
  }

  /* This is called each cycle to process the requests in the memory controller queue. */
  MC_STEP_HEADER
  {
    struct MC_action_t * req;

    /* MC runs at FSB speed */
    if(sim_cycle % uncore->fsb->ratio)
      return;

    if(RQ_num > 0) /* are there any requests at all? */
    {
      /* see if there's a new request to send to DRAM */
      if(next_available_time <= sim_cycle)
      {
        int req_index = -1;
        md_paddr_t req_page = (md_paddr_t)-1;

        /* try to find a request to the same page as we're currently accessing */
        int idx = RQ_head;
        for(int i=0;i<RQ_num;i++)
        {
          req = &RQ[idx];
          MC_assert(req->valid,(void)0);

          if(req->when_started == TICK_T_MAX)
          {
            if(req_index == -1) /* don't have a request yet */
            {
              req_index = idx;
              req_page = req->addr >> PAGE_SHIFT;
              if(FIFO_scheduling || (req_page == last_request)) /* using FIFO, or this is access to same page as current access */
                break;
            }
            if((req->addr >> PAGE_SHIFT) == last_request) /* found an access to same page as current access */
            {
              req_index = idx;
              req_page = req->addr >> PAGE_SHIFT;
              break;
            }
          }
          idx = modinc(idx,RQ_size);
        }

        if(req_index != -1)
        {
          req = &RQ[req_index];
          req->when_started = sim_cycle;
          req->when_finished = sim_cycle + dram->access(req->cmd, req->addr, req->linesize);
          last_request = req_page;
          next_available_time = sim_cycle + uncore->fsb->ratio;
          total_dram_cycles += req->when_finished - req->when_started;
        }
      }

      /* walk request queue and process requests that have completed. */
      int idx = RQ_head;
      for(int i=0;i<RQ_num;i++)
      {
        req = &RQ[idx];

        if((req->when_finished <= sim_cycle) && (req->when_returned == TICK_T_MAX) && (!req->prev_cp || bus_free(uncore->fsb)))
        {
          req->when_returned = sim_cycle;

          total_service_cycles += sim_cycle - req->when_enqueued;

          /* fill previous level as appropriate */
          if(req->prev_cp)
          {
            fill_arrived(req->prev_cp,req->MSHR_bank,req->MSHR_index);
            bus_use(uncore->fsb,req->linesize>>uncore->fsb_DDR,req->cmd==CACHE_PREFETCH);
            break; /* might as well break, since only one request can writeback per cycle */
          }
        }
        idx = modinc(idx,RQ_size);
      }

      /* attempt to "commit" the head */
      req = &RQ[RQ_head];
      if(req->valid)
      {
        if(req->when_returned <= sim_cycle)
        {
          req->valid = false;
          RQ_num--;
          uncore->counters.mc.read++;
          MC_assert(RQ_num >= 0,(void)0);
          RQ_head = modinc(RQ_head,RQ_size); //(RQ_head + 1) % RQ_size;
        }
      }
    }
  }

  MC_PRINT_HEADER
  {
    fprintf(stderr,"<<<<< MC >>>>>\n");
    for(int i=0;i<RQ_size;i++)
    {
      fprintf(stderr,"MC[%d]: ",i);
      if(RQ[i].valid)
      {
        if(RQ[i].op)
          fprintf(stderr,"%p(%lld)",RQ[i].op,((struct uop_t*)RQ[i].op)->decode.uop_seq);
        fprintf(stderr," --> %s",RQ[i].prev_cp->name);
        fprintf(stderr," MSHR[%d][%d]",RQ[i].MSHR_bank,RQ[i].MSHR_index);
      }
      else
        fprintf(stderr,"---");
      fprintf(stderr,"\n");
    }
  }

};


#endif /* MC_PARSE_ARGS */
