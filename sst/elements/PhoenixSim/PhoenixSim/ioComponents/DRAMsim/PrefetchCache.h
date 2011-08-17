
#include "constants.h"
#include <stdint.h>

typedef struct RECENT_ACCESS_HISTORY{
	uint64_t    address[RECENT_ACCESS_HISTORY_DEPTH];      /* 0 is most recently accessed, MAX is oldest */
} RECENT_ACCESS_HISTORY;

typedef struct GLOBAL_ACCESS_HISTORY_ENTRY {
	uint64_t    address[MAX_BUDDY_COUNT];
} GLOBAL_ACCESS_HISTORY_ENTRY;

typedef struct GLOBAL_ACCESS_HISTORY {
	GLOBAL_ACCESS_HISTORY_ENTRY      table_entry[MAX_ACCESS_HISTORY_TABLE_SIZE];
} GLOBAL_ACCESS_HISTORY;




class PrefetchCache {

public:

	PrefetchCache();



	int		mechanism;		/* No prefetch, or streaming prefetch, or markov */

	int		debug_flag;
	int		depth;			/* prefetch ahead 1,2,3, or 4? */
	int		cache_hit_count;	/* How many were eventually used? */
	int		biu_hit_count;		/* How many were eventually used? */
	int		prefetch_attempt_count;	/* how many prefetches were done? */
	int		prefetch_entry_count;	/* how many prefetches were done? */
	int		dirty_evict;		/* special dirty evict mode */
	int		hit_latency;		/* # of CPU cycles for a hit */

	uint64_t	address[MAX_PREFETCH_CACHE_SIZE];
	RECENT_ACCESS_HISTORY	recent_access_history[MEMORY_ACCESS_TYPES_COUNT];
	GLOBAL_ACCESS_HISTORY	*global_access_history_ptr;



/*
void init_prefetch_cache(int);
int  get_prefetch_mechanism();
int  active_prefetch();
void set_prefetch_debug(int);
int  prefetch_debug();
void set_prefetch_depth(int);
int  get_prefetch_depth();
void set_dirty_evict();
int  dirty_evict();

void init_global_access_history();
void init_recent_access_history();
void update_recent_access_history(int, uint64_t);
int  get_prefetch_attempt_count();
int  get_prefetch_hit_count();

void check_prefetch_algorithm(int, uint64_t, tick_t);
void update_prefetch_cache(uint64_t);
void issue_prefetch(uint64_t, int, tick_t);

int  pfcache_hit_check(uint64_t);	// given address, see if there's a hit in the pfetch cache hit
int  pfcache_hit_latency();
void remove_pfcache_entry(uint64_t);
*/

/***** implemented ***********/


/****************************/



};

