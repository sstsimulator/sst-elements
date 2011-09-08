#ifndef _MACHINE_H_
#define _MACHINE_H_


int read_machine_file(FILE *fp_machine);
int num_nodes(void);
int Net_x_dim(void);
int Net_y_dim(void);
int NoC_x_dim(void);
int NoC_y_dim(void);
int num_cores(void);
int num_router_nodes(void);
int envelope_size(void);


#endif /* _MACHINE_H_ */
