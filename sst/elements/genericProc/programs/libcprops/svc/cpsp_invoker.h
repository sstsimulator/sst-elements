#ifndef _CP_SVC_CPSP_INVOKER_H
#define _CP_SVC_CPSP_INVOKER_H

#include <sys/types.h>
#include "cprops/common.h"
#include "cprops/collection.h"
#include "cprops/hashlist.h"
#include "cprops/vector.h"
#include "cprops/hashtable.h"


typedef struct _cpsp_invoker_ctl
{
	short running;
	cp_hashlist *available;
	cp_vector *workers;
	cp_hashtable *session_lookup;
	cp_hashtable *pid_lookup;
	cp_hashtable *session_keys;
	char *pwd;
	char *cpsp_bin_path;
	int max_workers;
	int curr_workers;
	short is_worker;

	cp_mutex worker_lock;
	cp_cond worker_cond;
} cpsp_invoker_ctl;

void init_cpsp_invoker(char *document_root, 
					   char *cpsp_bin_path, 
					   int workers, 
					   int max_workers);

void stop_cpsp_invoker();
void shutdown_cpsp_invoker(void *ctl);

typedef enum { REQUEST = 1, KEEPALIVE, SHUTDOWN } cpsp_cmd;
typedef enum { RESPONSE = 1, RESPONSE_SESSION, KEEPALIVE_ACK } cpsp_cmd_response;

/* the worker descriptor is somewhat different between the platforms that 
 * support fork() and socketpair() and the platform(s) that do(es)n't. 
 */
typedef struct _worker_desc
{
	int index;        /* workers are retrieved by index or by availability */
#if defined(unix) || defined(__unix__) || defined(__MACH__)
	pid_t pid;
	int fd;
#else
#ifdef _WINDOWS
	HANDLE svc_in;
	HANDLE svc_out;
#endif /* _WINDOWS */
#endif /* unix */
	cp_list *session; /* sessions assigned to this worker */
} worker_desc;

/* the worker descriptor create function is somewhat different between the
 * platforms that support fork() and socketpair() and the platform(s) that 
 * do(es)n't. 
 */
#if defined(unix) || defined(__unix__) || defined(__MACH__)
worker_desc *worker_desc_create(int index, pid_t pid, int fd);
#else
#ifdef _WINDOWS
worker_desc *worker_desc_create(int index, HANDLE svc_in, HANDLE svc_out);
#endif /* _WINDOWS */
#endif /* unix */

/* the worker descriptor destruction function is the same on all platforms. */
void worker_desc_destroy(worker_desc *desc);

#if defined(unix) || defined(__unix__) || defined(__MACH__)
void cpsp_worker_run(int fd);
#endif

#ifdef _WINDOWS
void cpsp_worker_run(HANDLE in, HANDLE out);
#endif

int cpsp_invoke(cp_http_request *request, cp_http_response *response);

typedef size_t (*serialize_fn)(cp_string *buf, void *item);
typedef size_t (*deserialize_fn)(cp_string *buf, int offset, void **itemptr);

#endif /* _CP_SVC_CPSP_INVOKER_H */
