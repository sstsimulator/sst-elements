#include <assert.h>
#include "simple_cache.h"

using namespace std;


simple_cache::simple_cache (SST::ComponentId_t id, SST::Component::Params_t& params):
    SST::Component(id), send_st_complete(true), n_loads(0), n_load_hits(0)
{
  
  if(params.find("nodeId") == params.end())
    _abort(zesto_simple_cache, "couldn't find node id, use \"nodeId\" to specify!\n");
  node_id = params.find_integer("nodeId");  


  if(params.find("size") == params.end())
    _abort(zesto_simple_cache, "couldn't find cache size, use \"size\" to specify!\n");
  settings.size = params.find_integer("size");

  if(params.find("assoc") == params.end())
    _abort(zesto_simple_cache, "couldn't find cache association, use \"assoc\" to specify!\n");
  settings.assoc = params.find_integer("assoc");

  if(params.find("block_size") == params.end())
    _abort(zesto_simple_cache, "couldn't find cache block size, use \"block_size\" to specify!\n");
  settings.block_size = params.find_integer("block_size");

  if(params.find("hit_time") == params.end())
    _abort(zesto_simple_cache, "couldn't find cache hit_time, use \"hit_time\" to specify!\n");
  settings.hit_time = params.find_integer("hit_time");

  if(params.find("lookup_time") == params.end())
    _abort(zesto_simple_cache, "couldn't find cache lookup time, use \"lookup_time\" to specify!\n");
  settings.lookup_time = params.find_integer("lookup_time");

  settings.replacement_policy = RP_LRU;

  if(params.find("mc_node_ids") == params.end())
    _abort(zesto_simple_cache, "couldn't find memory controller ids, use \"mc_node_ids\" to specify, all mc node ids should be separated with comma!\n, semicolon or space");
  char * mcNodeIds_str = new char [params["mc_node_ids"].size()+1];
  std::vector<int> mcNodeIds;
  strcpy(mcNodeIds_str, params["mc_node_ids"].c_str());
  char * p = strtok(mcNodeIds_str, ",; ");
  while (p!=NULL) {
    mcNodeIds.push_back(strtol(p, NULL, 0));
    p = strtok(NULL, ",; ");
  }
  delete mcNodeIds_str;
  mc_map = new SimpleMcMap(mcNodeIds, settings.block_size);
  
  my_table = new hash_table(settings); 

  registerExit();

  link_upper = configureLink( "link_upper", 
         new SST::Event::Handler<simple_cache>(this,
               &simple_cache::handle_request) );
  link_lower = configureLink( "link_lower", 
         new SST::Event::Handler<simple_cache>(this,
               &simple_cache::handle_response) );

  if(params.find("clockFreq") == params.end())
    _abort(zesto_simple_cache, "clock frequency not specified\n");
  registerTimeBase(params["clockFreq"]);

}

simple_cache::~simple_cache (void)
{
  delete my_table;
  delete mc_map;
}


void simple_cache::handle_request (SST::Event *ev)
{

  cache_req *request = dynamic_cast<cache_req *>(ev);

  mem_req *mreq;

  assert ((request->op_type == OpMemLd) || (request->op_type == OpMemSt));

  request->msg = my_table->process_request (request);

  if (request->op_type == OpMemSt)
  {
    mreq = new mem_req (request, INITIAL);
    mreq->source_id = node_id;
    mreq->dest_id = mc_map->lookup (request->addr);
    delete request;
    //Even though we need to check if the data is in cache and update it
    //if it is, the message to the downstream can be sent right away.
    link_lower->Send(mreq);
  }
  else
  {
    n_loads++;
    if (request->msg == CACHE_HIT)
    {
       n_load_hits++;
       //Here the cache_req is reused, so we do not delete.
       link_upper->Send (my_table->get_hit_time(), request);
    }
    else if (request->msg == CACHE_MISS)
    {
       mreq = new mem_req (request, INITIAL);
       mreq->source_id = node_id;
       mreq->dest_id = mc_map->lookup (request->addr);
       delete request;
       link_lower->Send (my_table->get_lookup_time(), mreq);
    }
    else
    {
       assert(request->msg == CACHE_MISS_DUPLICATE);
       //Another load for the same address already missed and is waiting for response.
       //Therefore, do nothing.
       delete request;
    }
  }
}

//! Handle response from memory controller.
void simple_cache::handle_response (SST::Event *ev)
{
  mem_req *request = dynamic_cast<mem_req *> (ev);
   
  if(send_st_complete) {
      assert(request->msg == LD_RESPONSE || request->msg == ST_COMPLETE);
  }
  else {
      assert(request->msg == LD_RESPONSE);
  }


  if(request->msg == LD_RESPONSE) {
      list<cache_req*> reqlist; //the missed cache_req and its duplicates, i.e., requests for the
                                //same cache line.
      cache_req creq(request, request->msg);
      my_table->process_response (&creq, reqlist);

      for(list<cache_req*>::iterator it=reqlist.begin(); it != reqlist.end(); ++it) {
          (*it)->msg = LD_RESPONSE;
        link_upper->Send(my_table->get_hit_time(), *it);
      }
  }
  else {
      // On a ST_COMPLETE we can send the response back to the processor immediately
      cache_req* creq = new cache_req (request, request->msg);
      link_upper->Send(creq);
  }
  delete request; //delete the mem_req

}


void simple_cache :: print_stats(ostream& out)
{
    out << "********** simple-cache " << node_id << " stats **********" << endl;
    out << "    Total loads: " << n_loads << endl;
    out << "    Load hits: " << n_load_hits << " (" << (double)n_load_hits/n_loads*100 << "%)" << endl;  
}

