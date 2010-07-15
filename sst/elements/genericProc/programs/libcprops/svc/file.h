#ifndef _CP_SVC_FILE_H
#define _CP_SVC_FILE_H

#include "cprops/http.h"

/** initialize file service */
int init_file_service(char *mimetypes_filename, char *doc_path);
/** register service by file extension (ie .cpsp) */
void *register_ext_svc(char *ext, cp_http_service *svc);

/** file service function */
int file_service(cp_http_request *request, cp_http_response *response);

#endif /* _CP_SVC_FILE_H */
