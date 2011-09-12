#ifndef _MACHINE_H_
#define _MACHINE_H_


int read_machine_file(FILE *fp_machine, int verbose);
void disp_machine_params(void);

int num_nodes(void);
int Net_x_dim(void);
int Net_y_dim(void);
int NoC_x_dim(void);
int NoC_y_dim(void);
int num_cores(void);
int num_router_nodes(void);
int NetNICgap(void);
int NoCNICgap(void);

int NetNICinflections(void);
int NoCNICinflections(void);
int NetNICinflectionpoint(int index);
int NoCNICinflectionpoint(int index);
int64_t NetNIClatency(int index);
int64_t NoCNIClatency(int index);

int64_t NetLinkBandwidth(void);
int64_t NoCLinkBandwidth(void);
int64_t NetLinkLatency(void);
int64_t NoCLinkLatency(void);
int64_t IOLinkBandwidth(void);
int64_t IOLinkLatency(void);
int64_t NetIntraLatency(void);
int64_t NoCIntraLatency(void);

int NoCRouterLatency(void);
int NetRouterLatency(void);

void machine_params(FILE *out);

#endif /* _MACHINE_H_ */
