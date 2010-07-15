#ifndef _CP_CPSP_H
#define _CP_CPSP_H

#include "cprops/config.h"
#include "cprops/common.h"
#include "cprops/http.h"

__BEGIN_DECLS

void cpsp_init(int separate, char *bin_path, char *doc_root);
void cpsp_shutdown(void *dummy);

void *cpsp_add_shutdown_callback(void (*cb)(void *), void *prm);

typedef struct _cpsp_rec
{
	void *lib;
	int (*svc)(cp_http_request *, cp_http_response *);
	char *path;
} cpsp_rec;

cpsp_rec *cpsp_rec_new(char *path);

void cpsp_rec_dispose(cpsp_rec *rec);

int cpsp_gen(cp_http_request *request, cp_http_response *response);

__END_DECLS

#endif

