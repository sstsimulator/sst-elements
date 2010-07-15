#include <sys/types.h>
#include <errno.h>
#include <stdlib.h>

#if defined(__OpenBSD__) || defined(__NetBSD__) || defined(__FreeBSD__) || defined(__APPLE__)

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#endif

#if defined(sun)
#include <signal.h>
#endif

#include "cprops/config.h"
#include "cprops/http.h"
#include "cprops/util.h"
#include "cprops/log.h"
#include "cpsp_invoker.h"
#include "cpsp.h"

#if defined(unix) || defined(__unix__) || defined(__MACH__)
#include <unistd.h>
#include <sys/wait.h>
#endif /* unix */

/* portability definitions - io routines */
#if defined(unix) || defined(__unix__) || defined(__MACH__)
#define worker_read(rc, w, buf, len) (rc = (read((w)->fd, buf, len)))
#define worker_write(rc, w, buf, len) (rc = (write((w)->fd, buf, len)))
#define worker_read_string(w, len) (cp_string_read(w->fd, len))
#define worker_write_string(rc, w, buf) (rc = cp_string_write(buf, w->fd))
#define worker_proc_read(rc, fd, buf, len) (rc = read(fd, buf, len))
#define worker_proc_write(rc, fd, buf, len) (rc = write(fd, buf, len))
#else
#ifdef _WINDOWS
#define worker_read(rc, w, buf, len) \
	(ReadFile(w->svc_in, buf, len, &rc, NULL))
#define worker_write(rc, w, buf, len) \
	(WriteFile(w->svc_out, buf, len, &rc, NULL))
#define CHUNK 0x1000
static cp_string *worker_read_string_impl(HANDLE in, size_t len)
{
	char buf[CHUNK];
	int read_len, curr;
	cp_string *res = NULL;
	
	if (len == 0)
		read_len = CHUNK;
	else
		read_len = len < CHUNK ? len : CHUNK;

	while (len == 0 || res == NULL || res->len < len)
	{
		int rc = 
			ReadFile(in, buf, read_len, &curr, NULL);
		if (rc <= 0) break;
		if (res == NULL)
		{
			res = cp_string_create(buf, curr);
			if (res == NULL) return NULL;
		}
		else
			cp_string_cat_bin(res, buf, curr);
	}

	return res;
}
#define worker_read_string(w, len) (worker_read_string_impl(w->svc_in, len))
static size_t worker_write_string_impl(cp_string *buf, HANDLE svc_out)
{
	int rc, curr;
	int total = 0;

	errno = 0;
	while (total < buf->len)
	{
		rc = WriteFile(svc_out, &buf->data[total], buf->len - total, 
				&curr, NULL);
		if (rc == 0) break;
		total += curr;
	}

	return total;
}
#define worker_write_string(rc, w, buf) \
	(rc = worker_write_string_impl(buf, w->svc_out))
#define worker_proc_read(rc, fd, buf, len) \
	{ if (!ReadFile(fd, buf, len, &rc, NULL)) rc = -1; }
#define worker_proc_write(rc, fd, buf, len) \
	{ if (!WriteFile(fd, buf, len, &rc, NULL)) rc = -1; }
#endif /* _WINDOWS */
#endif /* unix */

static volatile short initialized = 0;
static cpsp_invoker_ctl ctl;

#if defined(unix) || defined(__unix__) || defined(__MACH__)
worker_desc *worker_desc_create(int index, pid_t pid, int fd)
{
	worker_desc *w = cp_calloc(1, sizeof(worker_desc));
	if (w)
	{
		w->session = cp_list_create();
		w->index = index;
		w->pid = pid;
		w->fd = fd;
	}

	return w;
}
#else
#ifdef _WINDOWS
worker_desc *worker_desc_create(int index, HANDLE svc_in, HANDLE svc_out)
{
	worker_desc *w = cp_calloc(1, sizeof(worker_desc));
	if (w)
	{
		w->session = cp_list_create();
		w->index = index;
		w->svc_in = svc_in;
		w->svc_out = svc_out;
	}

	return w;
}
#endif /* _WINDOWS */
#endif /* unix */

void worker_desc_destroy(worker_desc *desc)
{
	if (desc)
	{
		cp_list_destroy(desc->session);
		cp_free(desc);
	}
}

#if defined(unix) || defined(__unix__) || defined(__MACH__)
worker_desc *cpsp_invoker_create_worker(cpsp_invoker_ctl *controlblock, 
		                                int index)
{
	int rc = 0;
	int fd[2];
	int pid;
	worker_desc *w = NULL;

	rc = socketpair(AF_UNIX, SOCK_STREAM, 0, fd);
	if (rc) perror("socketpair"); //~~

	if ((pid = fork()))
	{
		w = worker_desc_create(controlblock->curr_workers, pid, fd[0]);
		controlblock->curr_workers++;
		cp_hashlist_append(controlblock->available, &w->index, w);
		cp_hashtable_put(controlblock->pid_lookup, &w->pid, w);
		cp_vector_set_element(controlblock->workers, index, w);

		close(fd[1]);
#ifdef __TRACE__
		DEBUGMSG("forked pid %d\n", pid);
#endif /* __TRACE__ */
	}
	else
	{
		close(fd[0]);
		ctl.is_worker = 1;
		cp_http_shutdown();
		/* cpsp_init: 1st prm = 1 for separate process */
		cpsp_init(1, controlblock->cpsp_bin_path, controlblock->pwd); 
		controlblock->session_keys = 
			cp_hashtable_create_by_option(COLLECTION_MODE_COPY | COLLECTION_MODE_DEEP, 
					10, cp_hash_string, cp_hash_compare_string, 
					(cp_copy_fn) strdup, free, NULL, NULL);
		cpsp_worker_run(fd[1]);
	}

	return w;
}
#else /* unix */
#ifdef _WINDOWS
char *flat_env(char *env, char **envp)
{
	int len;
	int i = 0;
	char *p = env;
	while (envp[i])
	{
		strcpy(p, envp[i]);
		len = strlen(envp[i]);
		p[len] = '\0';
		p = &p[len + 1];
		i++;
	}
	*p = '\0';

	return p;
}

static int 
	create_worker_proc(char *filename, HANDLE in, HANDLE out, char **envp)
{
	char cmdline[0x100];
	char env[0x1000];
	PROCESS_INFORMATION piProcInfo; 
	STARTUPINFO siStartInfo;
	BOOL bFuncRetn = FALSE; 

	flat_env(env, envp);
	ZeroMemory( &piProcInfo, sizeof(PROCESS_INFORMATION) );
 
	ZeroMemory( &siStartInfo, sizeof(STARTUPINFO) );
 
	sprintf(cmdline, "\"%s\" %ld %ld", filename, (long) in, (long) out);
// Create the child process. 
	bFuncRetn = CreateProcess(NULL, 
		cmdline,     // command line 
		NULL,          // process security attributes 
//		&sec_attr,
		NULL,          // primary thread security attributes 
		TRUE,          // handles are inherited 
		0,             // creation flags 
		env,           // environment 
		NULL,          // use parent's current directory 
		&siStartInfo,  // STARTUPINFO pointer 
		&piProcInfo);  // receives PROCESS_INFORMATION 
   
	if (bFuncRetn == 0) return -1;

	CloseHandle(piProcInfo.hProcess); //~~
	CloseHandle(piProcInfo.hThread); //~~
	return bFuncRetn;
}

worker_desc *cpsp_invoker_create_worker(cpsp_invoker_ctl *controlblock, 
		                                int index)
{
	worker_desc *w = NULL;
	char *filename = "cpsp_worker.exe";
	HANDLE cpsp_in, cpsp_in_dup;
	HANDLE cpsp_out, cpsp_out_dup;
	HANDLE svc_in;
	HANDLE svc_out;
	SECURITY_ATTRIBUTES sec_attr;
	char *cpsp_env[4];
	char env_bin_path[0x200];
	char env_doc_root[0x200];
	char env_path[0x400];
	int pid = 0; //~~

	cpsp_env[0] = env_bin_path;
	cpsp_env[1] = env_doc_root;
	cpsp_env[2] = env_path;
	cpsp_env[3] = NULL;

	sprintf(env_bin_path, "CPSP_BIN=%s", ctl.cpsp_bin_path);
	sprintf(env_doc_root, "DOC_ROOT=%s", ctl.pwd);
	sprintf(env_path, "PATH=%s", getenv("PATH"));

	sec_attr.nLength = sizeof(SECURITY_ATTRIBUTES);
	sec_attr.bInheritHandle = TRUE;
	sec_attr.lpSecurityDescriptor = NULL;

	if (!CreatePipe(&svc_in, &cpsp_out, &sec_attr, 0))
	{
		char msg[0x400];
		DWORD err = GetLastError();
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 
				NULL, err, 0, msg, 0x400 - 1, NULL);
		cp_error(CP_IO_ERROR, "can\'t create pipe: %s", msg);
		goto WORKER_CANCEL;
	}

	if (!CreatePipe(&cpsp_in, &svc_out, &sec_attr, 0))
	{
		char msg[0x400];
		DWORD err = GetLastError();
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 
				NULL, err, 0, msg, 0x400 - 1, NULL);
		cp_error(CP_IO_ERROR, "can\'t create pipe: %s", msg);
		goto WORKER_CANCEL;
	}

	if (!create_worker_proc(filename, cpsp_in, cpsp_out, cpsp_env))
	{
		cp_error(CP_THREAD_CREATION_FAILURE, "can\'t start worker process");
		goto WORKER_CANCEL;
	}

	w = worker_desc_create(controlblock->curr_workers, svc_in, svc_out);
	controlblock->curr_workers++;
	cp_hashlist_append(controlblock->available, &w->index, w);
	cp_vector_set_element(controlblock->workers, index, w);
	return w;

WORKER_CANCEL:
	return NULL;
}
#endif /* _WINDOWS */
#endif /* unix */

/* fwd decl. called if a worker process mysteriously dies */
static void replace_worker(worker_desc *w);

#ifdef CP_HAS_SIGACTION
void cpsp_invoker_signal_handler(int sig)
{
	pid_t pid;
	int status;

	if (sig != SIGCHLD)
#ifdef SIGCLD
		if (sig != SIGCLD)
#endif
		{
			cp_warn("cpsp_invoker_signal_handler: caught unexpected signal %d", sig);
			return;
		}

	pid = waitpid(-1, &status, WNOHANG);
    while (pid > 0) 
	{
//		worker_desc *w;
//		int ipid = pid;
//~~ set flags and perform printouts elsewhere
#ifdef __TRACE__
		if (ctl.running) DEBUGMSG("worker %d exits (status %d)", pid, status);
#endif /* __TRACE__ */
		if (!WIFEXITED(status))
		{
			if (ctl.running) DEBUGMSG("worker %d: abnormal termination", pid);
			if (WIFSIGNALED(status))
			{
				if (ctl.running) DEBUGMSG("worker %d exited on signal: %d", pid, WTERMSIG(status));
			}
		}
//~~ need a different way to invalidate and clean up if a process croaks - cp_hashtable 
//~~ does locking that doesn't work well when called from a signal handler. This is also
//~~ a race condition on shutdown.
#if 0
//		w = cp_hashtable_remove(ctl.pid_lookup, &ipid);
		if (ctl.running && w != NULL) 
			replace_worker(w);
//		ctl.curr_workers--;
#endif
        pid = waitpid(-1, &status, WNOHANG);
    }
}
#endif /* CP_HAS_SIGACTION */

#define CMD_LEN 1 + sizeof(size_t)

int stop_worker(worker_desc *w)
{
	int rc;
	char buf[CMD_LEN];

	buf[0] = SHUTDOWN;
	memset(&buf[1], 0, sizeof(size_t));

//	while ((rc = write(w->fd, buf, CMD_LEN)) <= 0)
	while (worker_write(rc, w, buf, CMD_LEN) <= 0)
	{
		if (errno != EAGAIN && errno != EINTR) break;
	}

	if (rc != CMD_LEN) return -1;
#if defined(unix) || defined(__unix__) || defined(__MACH__)
	return close(w->fd);
#else
#ifdef _WINDOWS
	CloseHandle(w->svc_out);
	CloseHandle(w->svc_in);
	return 0;
#endif /* _WINDOWS */
#endif /* unix */
}

void release_worker(worker_desc *w)
{
	stop_worker(w);
	worker_desc_destroy(w);
}

void stop_cpsp_invoker()
{
	ctl.running = 0;
}

void cpsp_invoker_delete_common()
{
	if (ctl.session_keys)
	{
		int i;
		int n = cp_hashtable_count(ctl.session_keys);
		cp_hashtable **session_keys = 
			(cp_hashtable **) cp_hashtable_get_values(ctl.session_keys);
		for (i = 0; i < n; i++)
			cp_hashtable_destroy_custom(session_keys[i], NULL, 
										(cp_destructor_fn) cp_wrap_delete);
		cp_free(session_keys);
		cp_hashtable_destroy(ctl.session_keys);
	}
	cp_free(ctl.pwd);
	cp_free(ctl.cpsp_bin_path);
}

void shutdown_cpsp_invoker(void *dummy)
{
#if 0
	int i;
	int n = cp_vector_size(ctl.workers);
	for (i = 0; i < n; i++)
	{
		worker_desc *w = cp_vector_element_at(ctl.workers, i);
		stop_worker(w);
	}
#endif
	ctl.running = 0;

	if (ctl.is_worker)
		cp_vector_destroy_custom(ctl.workers, (cp_destructor_fn) worker_desc_destroy);
	else
		cp_vector_destroy_custom(ctl.workers, (cp_destructor_fn) release_worker);
	cp_hashlist_destroy(ctl.available);
	cp_hashtable_destroy(ctl.pid_lookup);
	cp_hashtable_destroy(ctl.session_lookup);
//	if (!ctl.is_worker)
//		cpsp_invoker_delete_common();

	cp_mutex_destroy(&ctl.worker_lock);
	cp_cond_destroy(&ctl.worker_cond);

	initialized = 0;
}

void init_cpsp_invoker(char *document_root, 
					   char *cpsp_bin_path, 
					   int workers, 
					   int max_workers)
{
	int rc;
#ifdef CP_HAS_SIGACTION
	struct sigaction act;
#endif /* CP_HAS_SIGACTION */

	if (initialized) return;
	initialized = 1;

#ifdef CP_HAS_SIGACTION
	act.sa_handler = cpsp_invoker_signal_handler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_NOCLDSTOP;
#ifdef SIGCLD
	sigaction(SIGCLD, &act, NULL);
#endif /* SIGCLD */
	sigaction(SIGCHLD, &act, NULL);
#endif /* CP_HAS_SIGACTION */
	
	cp_http_add_shutdown_callback(shutdown_cpsp_invoker, &ctl);

	memset(&ctl, 0, sizeof(ctl));

	ctl.available = 
		cp_hashlist_create(max_workers, cp_hash_int, cp_hash_compare_int);
	ctl.workers = cp_vector_create(1);
	ctl.session_lookup = 
		cp_hashtable_create(1, cp_hash_string, cp_hash_compare_string);
	ctl.pid_lookup = 
		cp_hashtable_create(1, cp_hash_int, cp_hash_compare_int);
//	ctl.pwd = strdup(document_root);
	ctl.pwd = document_root;
//	ctl.cpsp_bin_path = strdup(cpsp_bin_path);
	ctl.cpsp_bin_path = cpsp_bin_path;
	ctl.max_workers = max_workers;
	ctl.curr_workers = 0;
	ctl.is_worker = 0;
	ctl.running = 1;
	rc = cp_mutex_init(&ctl.worker_lock, NULL);
	cp_cond_init(&ctl.worker_cond, NULL);

	while (ctl.curr_workers < workers)
		cpsp_invoker_create_worker(&ctl, ctl.curr_workers);
}

size_t serialize_cstr(cp_string *buf, char *cstr)
{
	size_t len = cstr ? strlen(cstr) : 0;
	size_t total = sizeof(size_t);
	cp_string_cat_bin(buf, &len, sizeof(size_t));
	if (len) 
	{
		cp_string_cat_bin(buf, cstr, len);
		total += len;
	}

	return total;
}

size_t deserialize_cstr(cp_string *buf, int offset, char **cstr)
{
	size_t len;
//	size_t total = 0;

	memcpy(&len, &buf->data[offset], sizeof(size_t));
	
	if (len == 0) 
	{
		*cstr = NULL;
		return sizeof(size_t);
	}
	
	*cstr = cp_malloc(len + 1);
	strncpy(*cstr, &buf->data[offset + sizeof(size_t)], len);
	(*cstr)[len] = '\0';

	return len + sizeof(size_t);
}
	
size_t serialize_cp_string(cp_string *buf, cp_string *string)
{
	size_t total = sizeof(size_t);
	size_t len = 0;
	if (string == NULL)
		cp_string_cat_bin(buf, &len, sizeof(size_t));
	else
	{
		len = string->len;
		cp_string_cat_bin(buf, &len, sizeof(size_t));
		cp_string_cat_bin(buf, string->data, string->len);
		total += string->len;
	}

	return total;
}
	
size_t deserialize_cp_string(cp_string *buf, int offset, cp_string **str)
{
	size_t len;
	size_t total = sizeof(size_t);

	memcpy(&len, &buf->data[offset], sizeof(size_t));
	if (len <= 0)
		*str = NULL;
	else
	{
		*str = cp_string_create(&buf->data[offset + total], len);
		total += len;
	}

	return total;
}
		
size_t cp_vector_serialize(cp_string *buf, cp_vector *vector, serialize_fn s_fn)
{
	unsigned int i;
	unsigned int n;
	size_t total = 0;

	n = vector ? cp_vector_size(vector) : 0;
	cp_string_cat_bin(buf, &n, sizeof(int));
	total += sizeof(int);

	if (n)
	{
//		int empty = 0;
		void *e;
		
		cp_string_cat_bin(buf, &vector->mode, sizeof(int));
		total += sizeof(int);
		for (i = 0; i < n; i++)
		{
			e = cp_vector_element_at(vector, i);
			total += (*s_fn)(buf, e);
		}
	}

	return total;
}

size_t cp_vector_deserialize(cp_string *buf, int offset, cp_vector **vector, 
							 deserialize_fn ds_fn)
{
	size_t total = sizeof(int);
	unsigned int len;
	
	memcpy(&len, &buf->data[offset], sizeof(int));
	if (len == 0)
		*vector = NULL;
	else
	{
		int mode;
		void *e;
		memcpy(&mode, &buf->data[offset + total], sizeof(int));
		total += sizeof(int);
		*vector = cp_vector_create_by_option(len, mode, NULL, NULL);

		while (len-- > 0)
		{
			total += (*ds_fn)(buf, offset + total, &e);
			cp_vector_add_element(*vector, e);
		}
	}

	return total;
}

size_t cp_hashtable_serialize(cp_string *buf, 
		                      cp_hashtable *table, 
						      serialize_fn serialize_key, 
						      serialize_fn serialize_value)
{
	unsigned int i;
	unsigned int n;
	size_t total = 0;

	n = table ? cp_hashtable_count(table) : 0;
	cp_string_cat_bin(buf, &n, sizeof(int));
	total += sizeof(int);

	if (n)
	{
		void **key;
		void *value;
		int mode = table ? table->mode : 0;

		cp_string_cat_bin(buf, &mode, sizeof(int));
		total += sizeof(int);
	
		key = cp_hashtable_get_keys(table); //~~ handle multiple values
		for (i = 0; i < n; i++)
		{
			value = cp_hashtable_get(table, key[i]);
			if (value)
			{
				total += (*serialize_key)(buf, key[i]);
				total += (*serialize_value)(buf, value);
			}
		}
		cp_free(key);
	}

	return total;
}
	
size_t cp_hashtable_deserialize(cp_string *buf, int offset, 
							    cp_hashtable **table, 
							 	deserialize_fn deserialize_key,
							 	deserialize_fn deserialize_value)
{
	size_t total = sizeof(int);
	int size;

	memcpy(&size, &buf->data[offset], sizeof(int));
	if (size == 0)
		*table = NULL;
	else
	{
		void *key;
		void *value;
		int mode;
		memcpy(&mode, &buf->data[offset + total], sizeof(int));
		total += sizeof(int);
		*table = 
			cp_hashtable_create_by_option(mode, size, cp_hash_string, 
				                          cp_hash_compare_string, 
										  (cp_copy_fn) strdup, cp_free, 
										  (cp_copy_fn) strdup, cp_free);
		while (size--)
		{
			total += (*deserialize_key)(buf, offset + total, &key);
			total += (*deserialize_value)(buf, offset + total, &value);
			cp_hashtable_put(*table, key, value);
		}
	}

	return total;
}

size_t cp_http_session_serialize(cp_string *buf, cp_http_session *session)
{
	size_t total;
	char not_null = session != NULL;

	cp_string_cat_bin(buf, &not_null, sizeof(char)); 
	total = sizeof(char);
	if (not_null)
	{
		unsigned int i;
		total += serialize_cstr(buf, session->sid);
		i = session->type;
		cp_string_cat_bin(buf, &i, sizeof(int));
		total += sizeof(int);
		cp_string_cat_bin(buf, &session->created, sizeof(time_t));
		total += sizeof(time_t);
		cp_string_cat_bin(buf, &session->access, sizeof(time_t));
		total += sizeof(time_t);
		cp_string_cat_bin(buf, &session->validity, sizeof(long));
		total += sizeof(long);
		cp_string_cat_bin(buf, &session->renew_on_access, sizeof(short));
		total += sizeof(short);
		cp_string_cat_bin(buf, &session->valid, sizeof(short));
		total += sizeof(short);
		cp_string_cat_bin(buf, &session->fresh, sizeof(short));
		total += sizeof(short);
	}

	return total;
}

size_t cp_http_session_deserialize(cp_string *buf, int offset, 
								   cp_http_session **session)
{
	size_t total = sizeof(char);
	char not_null = buf->data[offset];

	if (not_null == 0)
		*session = NULL;
	else
	{
		int i;
		*session = cp_calloc(1, sizeof(cp_http_session));
		total += deserialize_cstr(buf, offset + total, &(*session)->sid);
		memcpy(&i, &buf->data[offset + total], sizeof(int));
		total += sizeof(int);
		(*session)->type = i;
		memcpy(&(*session)->created, &buf->data[offset + total], sizeof(time_t));
		total += sizeof(time_t);
		memcpy(&(*session)->access, &buf->data[offset + total], sizeof(time_t));
		total += sizeof(time_t);
		memcpy(&(*session)->validity, &buf->data[offset + total], sizeof(long));
		total += sizeof(long);
		memcpy(&(*session)->renew_on_access, &buf->data[offset + total], sizeof(short));
		total += sizeof(short);
		memcpy(&(*session)->valid, &buf->data[offset + total], sizeof(short));
		total += sizeof(short);
		memcpy(&(*session)->fresh, &buf->data[offset + total], sizeof(short));
		total += sizeof(short);
	}
	
	return total;
}

size_t cp_http_request_serialize(cp_string *buf, cp_http_request *request)
{
	size_t total = 0;
	unsigned char ch;

	ch = (unsigned char) request->type;
	cp_string_cat_bin(buf, &ch, 1);
	total += sizeof(char);
	ch = (unsigned char) request->version;
	total += sizeof(char);
	cp_string_cat_bin(buf, &ch, 1);
	total += serialize_cstr(buf, request->uri);

	total += cp_hashtable_serialize(buf, request->header, 
			               			(serialize_fn) serialize_cstr, 
									(serialize_fn) serialize_cstr);

	total += cp_hashtable_serialize(buf, request->prm, 
			               			(serialize_fn) serialize_cstr, 
									(serialize_fn) serialize_cstr);
#ifdef CP_USE_COOKIES
	total += cp_vector_serialize(buf, request->cookie, 
								 (serialize_fn) serialize_cstr);
#endif /* CP_USE_COOKIES */
	total += serialize_cstr(buf, request->content);
	cp_string_cat_bin(buf, &request->owner->id, sizeof(int));
	total += sizeof(int);
#ifdef CP_USE_HTTP_SESSIONS
	total += cp_http_session_serialize(buf, request->session);
#endif /* CP_USE_HTTP_SESSIONS */

	return total;
}

size_t cp_http_request_deserialize(cp_string *buf, int offset, 
		                           cp_http_request **request)
{
	size_t total = 0;

	*request = cp_calloc(1, sizeof(cp_http_request));
	(*request)->type = (int) buf->data[offset + total];
	total += sizeof(char);
	(*request)->version = (int) buf->data[offset + total];
	total += sizeof(char);
	total += deserialize_cstr(buf, offset + total, &(*request)->uri);
	total += cp_hashtable_deserialize(buf, offset + total, &(*request)->header, 
							 		  (deserialize_fn) deserialize_cstr, 
									  (deserialize_fn) deserialize_cstr);
	total += cp_hashtable_deserialize(buf, offset + total, &(*request)->prm, 
									  (deserialize_fn) deserialize_cstr, 
									  (deserialize_fn) deserialize_cstr);
#ifdef CP_USE_COOKIES
	total += cp_vector_deserialize(buf, offset + total, &(*request)->cookie, 
								   (deserialize_fn) deserialize_cstr);
#endif /* CP_USE_COOKIES */
	total += deserialize_cstr(buf, offset + total, &(*request)->content);
	total += sizeof(int); //~~ cp_httpsocket?

#ifdef CP_USE_HTTP_SESSIONS
	total += cp_http_session_deserialize(buf, offset + total, 
			                             &(*request)->session);
#endif /* CP_USE_HTTP_SESSIONS */

	return total;
}

size_t cp_http_response_serialize(cp_string *buf, cp_http_response *response)
{
	size_t total = 0;
	int i;

	i = response->version;
	cp_string_cat_bin(buf, &i, sizeof(int));
	total += sizeof(int);
	i = response->status;
	cp_string_cat_bin(buf, &i, sizeof(int));
	total += sizeof(int);
//	total += serialize_cstr(buf, response->servername);
	i = response->connection;
	cp_string_cat_bin(buf, &i, sizeof(int));
	total += sizeof(int);
	total += cp_hashtable_serialize(buf, response->header, 
						            (serialize_fn) serialize_cstr, 
									(serialize_fn) serialize_cstr);
#ifdef CP_USE_COOKIES
	total += cp_vector_serialize(buf, response->cookie, 
								 (serialize_fn) serialize_cstr);
#endif /* CP_USE_COOKIES */
	i = response->content_type;
	cp_string_cat_bin(buf, &i, sizeof(int));
	total += sizeof(int);
	total += serialize_cstr(buf, response->content_type_lit);
	total += serialize_cstr(buf, response->body);
	total += serialize_cp_string(buf, response->content);
	cp_string_cat_bin(buf, &response->len, sizeof(int));
	total += sizeof(int);

	return total;
}

size_t cp_http_response_deserialize(cp_string *buf, int offset, 
									cp_http_response **response)
{
	size_t total = 0;
	int i;
	*response = cp_calloc(1, sizeof(cp_http_response));
	
	memcpy(&i, &buf->data[offset + total], sizeof(int));
	(*response)->version = i;
	total += sizeof(int);
	memcpy(&i, &buf->data[offset + total], sizeof(int));
	(*response)->status = i;
	total += sizeof(int);
//	total += deserialize_cstr(buf, offset + total, &(*response)->servername);
	memcpy(&i, &buf->data[offset + total], sizeof(int));
	(*response)->connection = i;
	total += sizeof(int);
	total += cp_hashtable_deserialize(buf, offset + total, 
			                          &(*response)->header, 
									  (deserialize_fn) deserialize_cstr, 
									  (deserialize_fn) deserialize_cstr);
#ifdef CP_USE_COOKIES
	total += cp_vector_deserialize(buf, offset + total, &(*response)->cookie, 
								   (deserialize_fn) deserialize_cstr);
#endif /* CP_USE_COOKIES */
	memcpy(&i, &buf->data[offset + total], sizeof(int));
	(*response)->content_type = i;
	total += sizeof(int);
	total += deserialize_cstr(buf, offset + total, 
							  &(*response)->content_type_lit);
	total += deserialize_cstr(buf, offset + total, &(*response)->body);
	total += deserialize_cp_string(buf, offset + total, &(*response)->content);
	memcpy(&(*response)->len, &buf->data[offset + total], sizeof(int));
	total += sizeof(int);
	
	return total;
}

void cp_http_session_dump(cp_http_session *session)
{
	if (session == NULL)
	{
		DEBUGMSG("session dump: NULL session");
		return;
	}

	DEBUGMSG(" vvvvvvvvvvvvvvvv session dump vvvvvvvvvvvvvvvv ");
	DEBUGMSG(" sid:      %s", session->sid ? session->sid : "NULL");
	DEBUGMSG(" type:     %d", session->type);
	DEBUGMSG(" created:  %ld", session->created);
	DEBUGMSG(" access:   %ld", session->access);
	DEBUGMSG(" validity: %ld", session->validity);
	DEBUGMSG(" renew:    %d", session->renew_on_access);
	DEBUGMSG(" valid:    %d", session->valid);
	DEBUGMSG(" fresh:    %d", session->fresh);
	if (session->key)
	{
		char **key;
		int n = cp_hashtable_count(session->key);
		key = (char **) cp_hashtable_get_keys(session->key);
		while (n--)
		{
			DEBUGMSG(" key:      [%s]", key[n]);
		}
		cp_free(key);
	}
	DEBUGMSG(" ^^^^^^^^^^^^^^^^ session dump ^^^^^^^^^^^^^^^^ ");
}

/* send a keepalive packet and receive a result */
int test_worker(worker_desc *w)
{
	char buf[CMD_LEN];
	time_t now = time(NULL);
//	time_t then;
	size_t snow, sthen;
	int rc;

	buf[0] = KEEPALIVE;
	snow = (size_t) now;
	memcpy(&buf[1], &snow, sizeof(size_t));

//	while ((rc = write(w->fd, buf, CMD_LEN)) < 0)
	while (worker_write(rc, w, buf, CMD_LEN) <= 0)
	{
		if (errno != EAGAIN && errno != EINTR) break;
	}
	if (rc != CMD_LEN) return -1;

//	while ((rc = read(w->fd, buf, CMD_LEN)) < 0)
	while (worker_read(rc, w, buf, CMD_LEN) <= 0)
	{
		if (errno != EAGAIN && errno != EINTR) break;
	}
	if (rc != CMD_LEN) return -1;

	if (buf[0] != KEEPALIVE_ACK) return -1;
	memcpy(&sthen, &buf[1], sizeof(size_t));
	if (snow != sthen) return -1;

	return 0;
}
	
void shutdown_worker(worker_desc *w)
{
	cp_list_iterator i;
	char *sid;
#ifdef __TRACE__
#if defined(unix) || defined(__unix__) || defined(__MACH__)
	DEBUGMSG("shutting down worker %d [%d]", w->index, w->pid);
#else
	DEBUGMSG("shutting down worker %d", w->index);
#endif /* unix */
#endif /* __TRACE__ */

#if defined(unix) || defined(__unix__) || defined(__MACH__)
	shutdown(w->fd, SHUT_WR); //~~ how
	close(w->fd);
#else
#ifdef _WINDOWS
	CloseHandle(w->svc_out);
	CloseHandle(w->svc_in);
#endif /* _WINDOWS */
#endif /* unix */

	ctl.curr_workers--;

	/* (1) drop from available list */
	cp_hashlist_remove(ctl.available, &w->index);
	
	/* (2) remove from workers vector */
	cp_vector_set_element(ctl.workers, w->index, NULL);

	/* (3) drop session lookups */
	cp_list_iterator_init(&i, w->session, COLLECTION_LOCK_NONE);
	sid = cp_list_iterator_curr(&i);
	while (sid)
	{
		worker_desc *w;
		w = cp_hashtable_remove(ctl.session_lookup, sid);
		sid = cp_list_iterator_next(&i);
	}
	cp_list_iterator_release(&i);
}

static void replace_worker(worker_desc *w)
{
	int index = w->index;
	worker_desc *tmp = w;

	cp_mutex_lock(&ctl.worker_lock);
	shutdown_worker(w);
	w = cpsp_invoker_create_worker(&ctl, index);
	cp_cond_broadcast(&ctl.worker_cond);
	cp_mutex_unlock(&ctl.worker_lock);
	worker_desc_destroy(tmp);
}
	
int cp_hashtable_copy_mappings(cp_hashtable *dst, cp_hashtable *src)
{
	int i = 0;
	void **keys;
	int n;

	if (src == NULL) return 0;

	n = cp_hashtable_count(src);
	keys = cp_hashtable_get_keys(src);
	for (i = 0; i < n; i++)
		cp_hashtable_put(dst, keys[i], cp_hashtable_get(src, keys[i]));
	cp_free(keys);

	return n;
}
	
void swap_response(cp_http_response *dst, cp_http_response *src) //~~ ...
{
	cp_http_request *request = dst->request;
	cp_hashtable *header = dst->header;
	char *servername = dst->servername;
	memcpy(dst, src, sizeof(cp_http_response));
	dst->request = request;
	if (dst->header == NULL) 
		dst->header = header;
	else
	{
		cp_hashtable_copy_mappings(header, dst->header);
		cp_hashtable_destroy(dst->header);
		dst->header = header;
	}
	dst->servername = servername;
}

/**
 * this function runs on the main instance and delegates a request to a worker
 * instance. The request is serialized and written to a worker and a response
 * is deserialized and returned to the framework. 
 */
int cpsp_invoke(cp_http_request *request, cp_http_response *response)
{
	worker_desc *w = NULL;
	cp_string *buf;
	int rc;
	cp_http_response *internal;
	char cmd[CMD_LEN];
	size_t len;

	/* if session assigned to a particular instance, wait for instance */
	if (request->session)
	{
		int index;
		/* get worker index for this session */
		w = cp_hashtable_get(ctl.session_lookup, request->session->sid);
		if (w)
		{
			index = w->index; 
#ifdef __TRACE__
#if defined(unix) || defined(__unix__) || defined(__MACH__)
			DEBUGMSG("looking up worker %d [%d] for session %s", 
					 w->index, w->pid, request->session->sid);
#else
			DEBUGMSG("looking up worker %d for session %s", 
					 w->index, request->session->sid);
#endif /* unix */
#endif /* __TRACE__ */
			/* now wait for this index to become available */
			cp_mutex_lock(&ctl.worker_lock);
			while ((w = cp_hashlist_get(ctl.available, &index)) == NULL &&
					ctl.running) 
				cp_cond_wait(&ctl.worker_cond, &ctl.worker_lock);
			cp_mutex_unlock(&ctl.worker_lock);
			if (!ctl.running)
				return HTTP_CONNECTION_POLICY_DEFAULT;

			if (test_worker(w)) /* connection failed */
			{
				replace_worker(w);
				w = NULL; /* session irrelevant, get any worker */
			}
		}
	}

	while (w == NULL) /* choose random worker_desc */
	{
		cp_mutex_lock(&ctl.worker_lock);
		while (cp_hashlist_item_count(ctl.available) == 0 && ctl.running)
			cp_cond_wait(&ctl.worker_cond, &ctl.worker_lock);
		w = cp_hashlist_remove_head(ctl.available);
		cp_mutex_unlock(&ctl.worker_lock);

		if (!ctl.running) return HTTP_CONNECTION_POLICY_DEFAULT;

		if (test_worker(w)) /* failed connection - close and open substitute */
		{
			replace_worker(w);
			w = NULL;
		}
	}

#ifdef __TRACE__
#if defined(unix) || defined(__unix__) || defined(__MACH__)
	DEBUGMSG("delegating to worker %d [%d]", w->index, w->pid);
#else
	DEBUGMSG("delegating to worker %d", w->index);
#endif /* unix */
#endif /* __TRACE__ */
	
	/* write a request to the worker process */
	buf = cp_string_create("dummy", CMD_LEN); /* 1st CMD_LEN chars REQUEST header */
	buf->data[0] = REQUEST;
	cp_http_request_serialize(buf, request);
	len = buf->len;
	memcpy(&buf->data[1], &len, sizeof(size_t));
//	while ((rc = cp_string_write(buf, w->fd)) <= 0)
	while (worker_write_string(rc, w, buf) <= 0)
		if (errno != EAGAIN && errno != EINTR) break;

	if (rc <= 0) 
	{
		cp_http_response_report_error(response, HTTP_500_INTERNAL_SERVER_ERROR, 
				"cpsp service: internal invocation error");
		//goto DONE; //~~
		return HTTP_CONNECTION_POLICY_CLOSE;
	}

	cp_string_delete(buf);
	

// 	while ((rc = read(w->fd, cmd, CMD_LEN)) <= 0)
	while (worker_read(rc, w, cmd, CMD_LEN) <= 0)
		if (errno != EAGAIN && errno != EINTR) break;

	if (rc <= 0)
	{
		cp_http_response_report_error(response, HTTP_500_INTERNAL_SERVER_ERROR, 
				"cpsp service: invalid internal invocation result");
		//goto DONE; //~~
		return HTTP_CONNECTION_POLICY_CLOSE;
	}

	memcpy(&len, &cmd[1], sizeof(size_t));
//	buf = cp_string_read(w->fd, len);
	buf = worker_read_string(w, len);
	if (buf == NULL)
	{
		cp_http_response_report_error(response, HTTP_500_INTERNAL_SERVER_ERROR,
				"cpsp service: internal invocation failed");
		return HTTP_CONNECTION_POLICY_CLOSE;
	}

	/* read the response from the worker process */
	len = cp_http_response_deserialize(buf, 0, &internal);
	swap_response(response, internal);
	cp_free(internal);

	/* if a new session was created, replicate in on the request */
	if (cmd[0] == RESPONSE_SESSION)
	{
		cp_http_session_deserialize(buf, len, &request->session);
		if (request->session)
		{
			cp_httpsocket *sock = request->owner;
			cp_list_append(w->session, request->session->sid);
			cp_hashtable_put(ctl.session_lookup, request->session->sid, w);
			if (sock->session == NULL)
			{
				sock->session = 
					cp_hashlist_create_by_option(COLLECTION_MODE_COPY |
						                         COLLECTION_MODE_DEEP,
										         1,
											     cp_hash_string,
											     cp_hash_compare_string, 
											     (cp_copy_fn) strdup,
											     (cp_destructor_fn) free,
											     NULL,
										    	 (cp_destructor_fn) 
											    	 cp_http_session_delete);
				//~~ if (sock->session == NULL) ...
			} 

			cp_hashlist_append(sock->session, request->session->sid, request->session);
		}
	}
	cp_string_delete(buf);

//DONE:
	/* return worker to available list and signal waiters */
	cp_mutex_lock(&ctl.worker_lock);
	cp_hashlist_append(ctl.available, &w->index, w);
	cp_cond_broadcast(&ctl.worker_cond); //~~ is _cond_signal enough?
	cp_mutex_unlock(&ctl.worker_lock);

	return HTTP_CONNECTION_POLICY_DEFAULT;
}


#define HTTP_REQUEST_BUFSIZE 0x1000

/***************************************************************************
 *                                                                         *
 * actual cpsp invocation code runs in a separate process so that scripts  *
 * can't crash the main service.                                           *
 *                                                                         *
 ***************************************************************************/

#if defined(unix) || defined(__unix__) || defined(__MACH__)
#define cp_http_request_delete_local cp_http_request_delete
#endif
#ifdef _WINDOWS
void cp_http_request_delete_local(cp_http_request *request)
{
	if (request)
	{
		if (request->uri) cp_free(request->uri);
		if (request->header) cp_hashtable_destroy(request->header);
		if (request->prm) cp_hashtable_destroy(request->prm);
#ifdef CP_USE_COOKIES
		if (request->cookie) cp_vector_destroy(request->cookie);
#endif /* CP_USE_COOKIES */
		if (request->content) cp_free(request->content);
		cp_free(request);
	}
}
#endif

#if defined(unix) || defined(__unix__) || defined(__MACH__)
void cpsp_worker_run(int fd)
{
	int fd_in = fd;
	int fd_out = fd;
#else
#ifdef _WINDOWS
void cpsp_worker_run(HANDLE fd_in, HANDLE fd_out)
{
#endif /* _WINDOWS */
#endif /* unix */
	char cmd[1 + sizeof(size_t)];
	size_t len;
	char buf[HTTP_REQUEST_BUFSIZE];
	short done = 0;
	cp_string in, out;
	int rc;
#ifdef __TRACE__
	pid_t pid = getpid();
//	DEBUGMSG("cpsp_worker running");
#endif /* __TRACE__ */

#ifdef _WINDOWS
	/* regrettably, this initialization can only be done here */
	ctl.session_keys = 
		cp_hashtable_create_by_option(COLLECTION_MODE_COPY | 
									  COLLECTION_MODE_DEEP, 10, 
									  cp_hash_string, 
									  cp_hash_compare_string, 
								  	  (cp_copy_fn) strdup, free, 
								  	  NULL, NULL);
#endif /* _WINDOWS */
	while (!done)
	{
		cp_http_request *request = NULL;
		memset(&in, 0, sizeof(cp_string));
		/* read a command structure */
		worker_proc_read(rc, fd_in, cmd, 1 + sizeof(size_t));
#ifdef __TRACE__
		DEBUGMSG("worker: read command %d", cmd[0]);
#endif /* __TRACE__ */
		
		/* perform request */
		switch(cmd[0])
		{
			case REQUEST: 
				memcpy(&len, &cmd[1], sizeof(size_t));
				worker_proc_read(rc, fd_in, buf, len);
//				rc = read(fd, buf, len);
				in.len = rc;
				in.data = buf;
				cp_http_request_deserialize(&in, 0, &request);
				if (request->session)
				{
					request->session->key = 
						cp_hashtable_get(ctl.session_keys, request->session->sid);
					/* occurs if session created on a different instance ~~ */
					if (request->session->key ==  NULL) 
					{
						cp_http_session_delete(request->session);
						request->session = NULL;
					}
//						request->session->key =         
//							cp_hashtable_create(1, cp_hash_string, cp_hash_compare_string);
				}

				if (request) 
				{
					cp_http_response *response = 
						cp_http_response_create(request);
					memset(&out, 0, sizeof(cp_string));
					cpsp_gen(request, response);

					len = cp_http_response_serialize(&out, response);
					if (len > 0)
					{
						if (request->session && request->session->fresh)
						{
							buf[0] = RESPONSE_SESSION;
							len += cp_http_session_serialize(&out, 
									                         request->session);
							cp_hashtable_put(ctl.session_keys, 
									request->session->sid, request->session->key);
						}
						else
							buf[0] = RESPONSE;

						memcpy(&buf[1], &len, sizeof(size_t));
						worker_proc_write(rc, fd_out, buf, CMD_LEN);
						worker_proc_write(rc, fd_out, out.data, len);
						cp_free(out.data);
					}
					if (request->session)
					{
						cp_free(request->session->sid);
						cp_free(request->session);
						request->session = NULL;
					}
					cp_http_request_delete_local(request);
					cp_http_response_delete(response);
				}
				break;

			case KEEPALIVE:
				cmd[0] = KEEPALIVE_ACK;
				worker_proc_write(rc, fd_out, cmd, CMD_LEN);
				break;

			case SHUTDOWN: 
				done = 1;
				break;
		}
	}
#ifdef __TRACE__
	DEBUGMSG("worker [%d]: done", pid);
#endif /* __TRACE__ */

#if defined(unix) || defined(__unix__) || defined(__MACH__)
	close(fd);
#endif /* unix */

#ifdef _WINDOWS
	CloseHandle(fd_in);
	CloseHandle(fd_out);
#endif /* _WINDOWS */

	cpsp_invoker_delete_common();
	cpsp_shutdown(NULL);
	cp_log_close();

	exit(0);
}

