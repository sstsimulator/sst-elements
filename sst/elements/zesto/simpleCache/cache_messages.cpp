#include "sst_config.h"
#include "sst/core/serialization.h"
#include <assert.h>

#include "sst/core/element.h"


#include "cache_messages.h"

using namespace std;

#ifdef SIMPLE_CACHE_UTEST
int cache_req :: Cache_req_count = 0;
int mem_req :: Mem_req_count = 0;
#endif

cache_req::cache_req():SST::Event()
{
#ifdef SIMPLE_CACHE_UTEST
   Cache_req_count++;
#endif
}


#ifdef SIMPLE_CACHE_UTEST
cache_req::cache_req (const cache_req& request):SST::Event()
{
   this->req_id = request.req_id;
   this->source_id = request.source_id;
   this->addr = request.addr;
   this->op_type = request.op_type;
   this->msg = request.msg;
   this->resolve_time = request.resolve_time;
   Cache_req_count++;
}
#endif


cache_req::cache_req (const mem_req *request, cache_msg_t msg):SST::Event()
{
   this->req_id = request->req_id;
   this->source_id = request->source_id;
   this->addr = request->addr;
   this->op_type = request->op_type;
   this->msg = msg;
   this->resolve_time = 0;
#ifdef SIMPLE_CACHE_UTEST
   Cache_req_count++;
#endif
}

cache_req::cache_req (int rid, int sid, paddr_t addr, mem_optype_t op_type, cache_msg_t msg)
	:SST::Event()
{
   this->req_id = rid;
   this->source_id = sid;
   this->addr = addr;
   this->op_type = op_type;
   this->msg = msg;
   this->resolve_time = 0;
#ifdef SIMPLE_CACHE_UTEST
   Cache_req_count++;
#endif
}

cache_req::~cache_req (void)
{
#ifdef SIMPLE_CACHE_UTEST
   Cache_req_count--;
#endif
}

mem_req::mem_req (const cache_req *request, cache_msg_t msg):SST::Event()
{
   this->req_id = request->req_id;
   this->originator_id = request->source_id;
   this->source_id = request->source_id;
   this->addr = request->addr;
   this->op_type = request->op_type;
   this->msg = msg;
   this->resolve_time = 0;
#ifdef SIMPLE_CACHE_UTEST
   Mem_req_count++;
#endif
}

mem_req::mem_req (int rid, int sid, paddr_t addr, mem_optype_t op_type, cache_msg_t msg)
	:SST::Event()

{
   this->req_id = rid;
   this->originator_id = sid;
   this->source_id = sid;
   this->addr = addr;
   this->op_type = op_type;
   this->msg = msg;
   this->resolve_time = 0;
#ifdef SIMPLE_CACHE_UTEST
   Mem_req_count++;
#endif
}

mem_req::~mem_req (void)
{
#ifdef SIMPLE_CACHE_UTEST
   Mem_req_count--;
#endif
}

uint8_t * mem_req::get_pack_data()
{
    int pack_size = get_pack_size();
    uint8_t * packed_data = new uint8_t[pack_size];
    memcpy(packed_data, &req_id, pack_size);
    return packed_data;
}

int mem_req::get_pack_size()
{
    return (uint8_t*)&resolve_time - (uint8_t*)&req_id + sizeof(resolve_time);
}

void mem_req::unpack_data(uint8_t * packed_data)
{
    int pack_size = get_pack_size();
    memcpy(&req_id, packed_data, pack_size);
}

ostream& operator<<(ostream& stream, const cache_req& creq)
{
    const char* mstr = 0;
    switch (creq.msg) {
    case INITIAL:
        mstr = "INITIAL";
	break;
    case CACHE_HIT:
        mstr = "CACHE_HIT";
	break;
    case CACHE_MISS:
        mstr = "CACHE_MISS";
	break;
    case LD_RESPONSE:
        mstr = "LD_RESPONSE";
	break;
	  case ST_COMPLETE:
	  case CACHE_MISS_DUPLICATE:
	break;	
    }

    const char* pstr =0;
    switch (creq.op_type) {
    case OpMemNoOp:
        pstr = "OpMemNoOp";
	break;
    case OpMemLd:
        pstr = "OpMemLd";
	break;
    case OpMemSt:
        pstr = "OpMemSt";
	break;
    }

    stream << "cache_req: req_id= " << creq.req_id << " src_id= " << creq.source_id << " addr= 0x" <<hex<< creq.addr <<dec
           << " " << mstr << " " << pstr;
  
  return stream;
}

ostream& operator<<(ostream& stream, const mem_req& mreq)
{
    const char* mstr = 0;
    switch (mreq.msg) {
    case INITIAL:
        mstr = "INITIAL";
	break;
    case CACHE_HIT:
        mstr = "CACHE_HIT";
	break;
    case CACHE_MISS:
        mstr = "CACHE_MISS";
	break;
    case LD_RESPONSE:
        mstr = "LD_RESPONSE";
	break;
	  case ST_COMPLETE:
	  case CACHE_MISS_DUPLICATE:
	break;	
    }

    const char* pstr = 0;
    switch (mreq.op_type) {
    case OpMemNoOp:
        pstr = "OpMemNoOp";
	break;
    case OpMemLd:
        pstr = "OpMemLd";
	break;
    case OpMemSt:
        pstr = "OpMemSt";
	break;
    }

    stream << "mem_req: req_id= " << mreq.req_id << " org_id= " << mreq.originator_id << " src_id= " << mreq.source_id << " src_port= " << mreq.source_port
           << " dst_id= " << mreq.dest_id << " dst_port= " << mreq.dest_port<< " addr= 0x" <<hex<< mreq.addr <<dec
           << " " << mstr << " " << pstr;

  return stream;
}


