#ifndef SIMPLE_CACHE_CACHE_TYPES_H_
#define SIMPLE_CACHE_CACHE_TYPES_H_

#include <stdint.h>


typedef enum {
   CACHE_DATA = 1,
   CACHE_TAG
} cache_type_t;

typedef enum {
   RP_LRU = 1
} replacement_policy_t;

typedef struct {
//   int comp_id;
//   const char *name;
//   cache_type_t type;
   int size;
   int assoc;
   int block_size;
   int hit_time;
   int lookup_time;
   replacement_policy_t replacement_policy;
//   bool send_st_response; //whether or not the cache send response for stores.
} simple_cache_settings;

#endif //SIMPLE_CACHE_CACHE_TYPES_H_
