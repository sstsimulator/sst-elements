/*
** Work stealing benchmark
*/

#ifndef _WS_H_
#define _WS_H_


#undef TRUE
#define TRUE            	(1)
#undef FALSE
#define FALSE           	(0)


extern int my_rank;
extern int num_ranks;


int ws_init(int argc, char *argv[]);
double ws_work(void);
void ws_print(void);


#endif  /* _WS_H_ */
