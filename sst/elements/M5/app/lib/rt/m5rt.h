#ifndef _m5rt_h
#define _m5rt_h

typedef struct {
    int nid;
    int pid;
} cnos_nidpid_map_t;

extern "C" {
extern int cnos_get_rank( void );
extern int cnos_get_size( void );
extern int cnos_barrier( void );
extern int cnos_get_nidpid_map( cnos_nidpid_map_t** map );
}
 
#endif
