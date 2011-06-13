#ifndef _m5rt_h
#define _m5rt_h

typedef struct {
    int nid;
    int pid;
} m5_nidpid_map_t;

extern int m5_get_rank( void );
extern int m5_get_size( void );
extern int m5_barrier( void );
extern int m5_get_nidpid_map( m5_nidpid_map_t** map );
 
#endif
