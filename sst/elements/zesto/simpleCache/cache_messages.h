#ifndef SIMPLE_CACHE_CACHE_MESSAGES_H_
#define SIMPLE_CACHE_CACHE_MESSAGES_H_


#include <iostream>
#include <sst_config.h>
#include <sst/core/serialization.h>
#include <sst/core/element.h>
#include <sst/core/event.h>
#include <sst/core/sst_types.h>

#include "cache_types.h"


typedef uint64_t paddr_t;

typedef enum {
   OpMemNoOp = 0,
   OpMemLd,
   OpMemSt
} mem_optype_t;

typedef enum {
   INITIAL = 0,
   CACHE_HIT,
   CACHE_MISS,
   CACHE_MISS_DUPLICATE,
   LD_RESPONSE,
   ST_COMPLETE  //response fro STOREs
} cache_msg_t;

class mem_req;

class cache_req : public SST::Event{
   public:
      int req_id; //request id; may be useful for processor
      int source_id;
      paddr_t addr;
      mem_optype_t op_type; 
      cache_msg_t msg;
      int resolve_time;

      cache_req();  //for serialization
      cache_req (const mem_req *request, cache_msg_t msg);
      cache_req (int rid, int sid, paddr_t addr, mem_optype_t op_type, cache_msg_t msg);
      ~cache_req (void);
      friend std::ostream& operator<<(std::ostream& stream, const cache_req& creq);

    private:
        friend class boost::serialization::access;
        template<class Archive>
        void
        serialize(Archive & ar, const unsigned int version)
        {
            ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(SST::Event);
            ar & BOOST_SERIALIZATION_NVP(req_id);
            ar & BOOST_SERIALIZATION_NVP(source_id);
            ar & BOOST_SERIALIZATION_NVP(addr);
            ar & BOOST_SERIALIZATION_NVP(op_type);
            ar & BOOST_SERIALIZATION_NVP(msg);
            ar & BOOST_SERIALIZATION_NVP(resolve_time);
        }
};

class mem_req  : public SST::Event{
   public:
      int req_id; //cache_req's req_id
      int originator_id;
      int source_id;
      int source_port;
      int dest_id;
      int dest_port;
      paddr_t addr;
      mem_optype_t op_type;
      cache_msg_t msg;
      int resolve_time;

      mem_req() {}  //for serialization
      mem_req (const cache_req *request, cache_msg_t msg);
      mem_req (int rid, int sid, paddr_t addr, mem_optype_t op_type, cache_msg_t msg);
      ~mem_req (void);

      int get_src() { return source_id; }
      int get_src_port() { return source_port; }
      int get_dst() { return dest_id; }
      int get_dst_port() { return dest_port; }

      uint8_t * get_pack_data();
      int get_pack_size();
      void unpack_data(uint8_t *);
      friend std::ostream& operator<<(std::ostream& stream, const mem_req& mreq);

    private:
        friend class boost::serialization::access;
        template<class Archive>
        void
        serialize(Archive & ar, const unsigned int version)
        {
            ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(SST::Event);
            ar & BOOST_SERIALIZATION_NVP(req_id);
            ar & BOOST_SERIALIZATION_NVP(originator_id);
            ar & BOOST_SERIALIZATION_NVP(source_id);
            ar & BOOST_SERIALIZATION_NVP(source_port);
            ar & BOOST_SERIALIZATION_NVP(dest_id);
            ar & BOOST_SERIALIZATION_NVP(dest_port);
            ar & BOOST_SERIALIZATION_NVP(addr);
            ar & BOOST_SERIALIZATION_NVP(op_type);
            ar & BOOST_SERIALIZATION_NVP(msg);
            ar & BOOST_SERIALIZATION_NVP(resolve_time);
        }

};


#endif // SIMPLE_CACHE_CACHE_MESSAGES_H_
