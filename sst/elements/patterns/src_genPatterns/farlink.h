#ifndef _FARLINK_H_
#define _FARLINK_H_


int read_farlink_file(FILE *fp_farlink, int verbose);
int farlink_in_use(void);
void disp_farlink_params(void);
int numFarPorts(int node);
int numFarLinks(void);
int farlink_dest_port(int src_node, int dest_node);
int farlink_src_port(int src_node, int dest_node);
int farlink_exists(int src_node, int dest_node);

void farlink_params(FILE *out, int port_offset);

#endif /* _FARLINK_H_ */
