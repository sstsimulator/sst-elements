
#ifndef _CP_SVC_CGI_H
#define _CP_SVC_CGI_H

#include "cprops/common.h"
#include "cprops/http.h"

__BEGIN_DECLS

/** initialized cgi service */
int init_cgi(char *path, char *uri_prefix);
/** stop cgi service */
void shutdown_cgi(void *dummy);

/** cgi service function */
int cgi(cp_http_request *request, cp_http_response *response);

__END_DECLS

#endif

