#ifndef _CP_HTTP_CLIENT_H
#define _CP_HTTP_CLIENT_H

/**
 * @addtogroup cp_socket
 */
/** @{ */
/**
 * @file
 * definitions for http client socket api
 */

#include "common.h"

#ifdef _WINDOWS
#include <Winsock2.h>
#endif

__BEGIN_DECLS

#include <time.h>

#include "config.h"
#include "hashtable.h"
#include "vector.h"
#include "thread.h"
#include "http.h"
#include "client.h"

#ifdef HAVE_POLL
#include <sys/poll.h>
#else
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#endif /* HAVE_POLL */

#ifdef CP_USE_COOKIES
#include "trie.h"

/**
 * descriptor for an http cookie
 */
typedef CPROPS_DLL struct _cp_http_cookie
{
    char *content;      /**< cookie value */
    char *domain;       /**< domain cookie is valid for */
    char *path;         /**< uri path */
    struct tm expires;  /**< expiry date */
    int secure;       
    int version;
    int http_only;
    cp_hashtable *fld;  /**< unrecognized fields */
} cp_http_cookie;

/** 
 * create a cookie structure. used internally by cp_cookie_parse.
 */
CPROPS_DLL
cp_http_cookie *cp_http_cookie_create(char *content, 
                                      char *domain, 
                                      char *path, 
                                      char *expires, 
                                      int secure,
                                      int version,
                                      int http_only);

/**
 * parse an http cookie
 */
CPROPS_DLL
cp_http_cookie *cp_http_cookie_parse(char *src);

/** release cookie memory */
CPROPS_DLL
void cp_http_cookie_destroy(cp_http_cookie *c);

/** dump cookie fields to log */
CPROPS_DLL
void cp_http_cookie_dump(cp_http_cookie *c);

#endif /* CP_USE_COOKIES */

/** api initialization function */
CPROPS_DLL
int cp_httpclient_init();

/** api cleanup function */
CPROPS_DLL
int cp_httpclient_shutdown();


#define DEFAULT_MAX_REDIRECTS 10
/**
 * http client socket wrapper
 */
typedef CPROPS_DLL struct _cp_httpclient
{
	int id;
    cp_http_version version;
    cp_http_request_type type;
    cp_hashtable *header;
    cp_hashtable *cgi_parameters;
	short auto_drop_parameters;
	short auto_drop_headers;
	cp_string *content;
	char *host; /**< may differ from requested host if redirect enabled */
	int port;   /**< may differ from requested port if redirect enabled */
	char *proxy_host;
	int proxy_port;
#ifdef CP_USE_SSL
	short proxy_ssl;
#endif /* CP_USE_SSL */
#ifdef CP_USE_COOKIES
    cp_trie *cookie_jar;
#endif /* CP_USE_COOKIES */
    short connected;
    short persistent;
    int keep_alive;
    short pipeline_requests;
    short follow_redirects;
    short max_redirects;

    cp_client *socket;
} cp_httpclient;

/** 
 * create an http client wrapper to communicate with the given host on the 
 * given port
 */
CPROPS_DLL
cp_httpclient *cp_httpclient_create(char *host, int port);
/**
 * create an http client wrapper to communicate with the given host on the
 * given port using the given proxy settings
 */
CPROPS_DLL
cp_httpclient *
	cp_httpclient_create_proxy(char *host, int port, 
							   char *proxy_host, int proxy_port);
#ifdef CP_USE_SSL
/**
 * create an https client wrapper to communicate with the given host on the
 * given port using the specified certificate file or path. Verification mode
 * can be either SSL_VERIFY_NONE or SSL_VERIFY_PEER.
 */
CPROPS_DLL
cp_httpclient *cp_httpclient_create_ssl(char *host, int port, 
                                        char *CA_file, char *CA_path, 
                                        int verification_mode);

/**
 * create an https client wrapper to communicate with the given host on the
 * given port using the specified proxy settings and the specified certificate 
 * file or path. Verification mode can be either SSL_VERIFY_NONE or 
 * SSL_VERIFY_PEER.
 */
CPROPS_DLL
cp_httpclient *
	cp_httpclient_create_proxy_ssl(char *host, int port, 
							   	   char *proxy_host, int proxy_port, 
								   char *CA_file, char *CA_path, 
								   int verification_mode);
#endif /* CP_USE_SSL */

/** deallocate a cp_httpclient wrapper. */
CPROPS_DLL
void cp_httpclient_destroy(cp_httpclient *client);

/** set the HTTP version for subsequent requests */
CPROPS_DLL
void cp_httpclient_set_http_version(cp_httpclient *client, 
                                    cp_http_version version);
/** set the request type for subsequent requests. GET by default. */
CPROPS_DLL
void cp_httpclient_set_request_type(cp_httpclient *client, 
                                    cp_http_request_type type);
/** set an arbitrary header. Any existing value is overriden. */
CPROPS_DLL
void cp_httpclient_set_header(cp_httpclient *client, char *header, char *value);
/** 
 * determine whether headers are automatically deleted or not - disabled
 * by default
 */
CPROPS_DLL 
void cp_httpclient_set_auto_drop_headers(cp_httpclient *client, short mode);
/** deallocate cgi parameters */
CPROPS_DLL
void cp_httpclient_drop_headers(cp_httpclient *client);
/** set a cgi parameter */
CPROPS_DLL
void *cp_httpclient_set_parameter(cp_httpclient *client, 
								  char *name, char *value);
/** 
 * determine whether cgi parameters are automatically deleted or not - enabled
 * by default
 */
CPROPS_DLL 
void cp_httpclient_set_auto_drop_parameters(cp_httpclient *client, short mode);
/** deallocate cgi parameters */
CPROPS_DLL
void cp_httpclient_drop_parameters(cp_httpclient *client);
/** set a value for the 'User-Agent' header. Default is libcprops-(version) */
CPROPS_DLL
void cp_httpclient_set_user_agent(cp_httpclient *client, char *agent);
/** set the timeout value for the underlying tcp socket */
CPROPS_DLL
void cp_httpclient_set_timeout(cp_httpclient *client, 
                                      int sec, int usec);
/** set the number of times to retry a connection on failure */
CPROPS_DLL
void cp_httpclient_set_retry(cp_httpclient *client, int retry_count);
/** set client behavior on HTTP 302, 302 etc. responses */
CPROPS_DLL
void cp_httpclient_allow_redirects(cp_httpclient *client, int mode);
/** maximal number of server redirect directives to follow - default is 10 */
CPROPS_DLL
void cp_httpclient_set_max_redirects(cp_httpclient *client, int max);
#ifdef CP_USE_COOKIES
/** 
 * by default all cp_httpclients share the same cookie jar. use this function
 * to specify a different cookie jar.
 */
CPROPS_DLL
void cp_httpclient_set_cookie_jar(cp_httpclient *client, cp_trie *jar);
#endif /* CP_USE_COOKIES */

/** 
 * use the same wrapper for a different address. this will close an existing 
 * connection, change the host and port settings on the wrapper and attempt to
 * connect. maybe rename this function. 
 */
CPROPS_DLL
int cp_httpclient_reopen(cp_httpclient *client, char *host, int port);


typedef CPROPS_DLL struct _cp_url_descriptor
{
    short ssl;
    char *host;
    int port;
    char *uri;
} cp_url_descriptor; /**< wrapper for url information */

CPROPS_DLL
cp_url_descriptor *
    cp_url_descriptor_create(char *host, short ssl, int port, char *uri);
CPROPS_DLL
void cp_url_descriptor_destroy(cp_url_descriptor *desc);
CPROPS_DLL
short cp_url_descriptor_ssl(cp_url_descriptor *desc);
CPROPS_DLL
char *cp_url_descriptor_host(cp_url_descriptor *desc);
CPROPS_DLL
int cp_url_descriptor_port(cp_url_descriptor *desc);
CPROPS_DLL
char *cp_url_descriptor_uri(cp_url_descriptor *desc);
CPROPS_DLL
cp_url_descriptor *cp_url_descriptor_parse(char *url);

/** result codes for HTTP requests */
typedef enum 
	{ 
		CP_HTTP_RESULT_SUCCESS,
		CP_HTTP_RESULT_CONNECTION_FAILED,
		CP_HTTP_RESULT_CONNECTION_RESET,
		CP_HTTP_RESULT_TIMED_OUT,
		CP_HTTP_RESULT_WRITE_ERROR,
		CP_HTTP_RESULT_READ_ERROR
	} cp_http_result_status;

/**
 * wrapper for asynchronous interface callback
 */
typedef CPROPS_DLL struct _cp_httpclient_result
{
	void *id;
	cp_http_result_status result;
	cp_httpclient *client;
	cp_http_response *response;
} cp_httpclient_result;

/** (internal) create a cp_httpclient_result object */
CPROPS_DLL
cp_httpclient_result * cp_httpclient_result_create(cp_httpclient *client);

/** destructor for cp_httpclient_result objects */
CPROPS_DLL
void cp_httpclient_result_destroy(cp_httpclient_result *res);
/** get the client assigned id from a result descriptor */
CPROPS_DLL
void *cp_httpclient_result_id(cp_httpclient_result *res);
/** get the result status code from a result descriptor */
CPROPS_DLL 
cp_http_result_status cp_httpclient_result_status(cp_httpclient_result *res);
/** get the response object from a result descriptor */
CPROPS_DLL 
cp_http_response *cp_httpclient_result_get_response(cp_httpclient_result *res);
/** get the client object the response was made on */
CPROPS_DLL 
cp_httpclient *cp_httpclient_result_get_client(cp_httpclient_result *res);

/** read uri content - synchronous interface */
CPROPS_DLL
cp_httpclient_result *cp_httpclient_fetch(cp_httpclient *client, char *uri);


/* ------------------------------------------------------------------------ *
 *                             async interface                              *
 * ------------------------------------------------------------------------ */


/**
 * asynchronous interface control block. By default asynchronous transfers use
 * a static block shared between all client transfers. To separate transfers 
 * into different groups, create a cp_httpclient_ctl control block and use 
 * _fetch_ctl_nb and _fetch_ctl_nb_exec().
 */
typedef CPROPS_DLL struct _cp_httpclient_ctl
{
	cp_hashlist *stack;
#ifdef HAVE_POLL
	struct pollfd *ufds; /* pollfd allows specifying poll type (r/w etc) */
#else
	fd_set ufds_r; /* select requires separate sets for reading and writing */
	fd_set ufds_w;
	unsigned int fdmax;
#endif /* HAVE_POLL */
	void **desc;
	int size;
	int nfds;

	/* background transfers */
	int running;
	cp_thread bg;
	cp_thread_pool *pool;
	cp_mutex bg_lock;
	cp_cond bg_cond;
} cp_httpclient_ctl;

/**
 * create an asynchronous client transfer block for separate transfers. if
 * background is 0, transfers must be driven by calling 
 * cp_httpclient_fetch_ctl_exec(). If background is 1, transfers will be run 
 * on a separate thread. If background is greater than 1, a thread pool is 
 * created to perform background transfers and the given value is used as the 
 * maximal size of the thread pool. 
 */
CPROPS_DLL
cp_httpclient_ctl *cp_httpclient_ctl_create(int background);

/**
 * transfer blocks created with cp_httpclient_ctl_create() are not released by
 * the framework and must be finalized by calling cp_httpclient_ctl_destroy().
 */
CPROPS_DLL
void cp_httpclient_ctl_destroy(cp_httpclient_ctl *ctl);

#ifdef HAVE_POLL
/** 
 * retrieve a pollfd array prepared for polling for the default transfer
 * control block. If num is non-null it is set to the number of entries in the 
 * pollfd array.
 */
CPROPS_DLL
struct pollfd *cp_httpclient_ctl_default_get_pollfds(int *num);

/** 
 * retrieve a pollfd array prepared for polling. If num is non-null it is set 
 * to the number of entries in the pollfd array.
 */
CPROPS_DLL
struct pollfd *cp_httpclient_ctl_get_pollfds(cp_httpclient_ctl *ctl, int *num);
#else
/** 
 * retrieve a read file descriptor set for select() for the default transfer 
 * control block. If num is non-null it is set to the number of entries in the
 * file descriptor array.
 */
CPROPS_DLL
fd_set *cp_httpclient_ctl_default_get_read_fd_set(int *num);

/** 
 * retrieve a read file descriptor set prepared for select(). If num is non-
 * null it is set to the number of entries in the file descriptor array.
 */
CPROPS_DLL
fd_set *cp_httpclient_ctl_get_read_fd_set(cp_httpclient_ctl *ctl, int *num);

/** 
 * retrieve a write file descriptor set for select() for the default transfer 
 * control block. If num is non-null it is set to the number of entries in the
 * file descriptor array.
 */
CPROPS_DLL
fd_set *cp_httpclient_ctl_default_get_write_fd_set(int *num);

/** 
 * retrieve a write file descriptor set prepared for select(). If num is non-
 * null it is set to the number of entries in the file descriptor array.
 */
CPROPS_DLL
fd_set *cp_httpclient_ctl_get_write_fd_set(cp_httpclient_ctl *ctl, int *num);
#endif /* HAVE_POLL */
typedef 
void (*cp_httpclient_callback)(cp_httpclient_result *response);

/** transfer descriptor for async interface */
typedef CPROPS_DLL struct _cp_http_transfer_descriptor
{
	void *id;				/**< user assigned id */
	char *uri;      		/**< target uri */
	cp_httpclient *owner;	/**< client */
	cp_http_response *res;	/**< result */
	cp_string *buf; 		/**< io buffer */
	char *read_ptr; 		/**< pointer to processed stage within buf */
	int skip;               /**< read pointer offset */
	int stage;      		/**< connecting, writing, reading */
	int read_stage; 		/**< read status line, header, body */
	int transfer;   		/**< chunked, content-length, EOF */
	int remaining;  		/**< io bytes yet to be transfered */
	int redirects;  		/**< number of redirects */
	int status;     		/**< transfer result */
	cp_httpclient_callback callback;
} cp_http_transfer_descriptor;

CPROPS_DLL
cp_http_transfer_descriptor *
	cp_http_transfer_descriptor_create(void *id, 
									   cp_httpclient *client, 
									   cp_httpclient_callback callback, 
									   char *uri);
CPROPS_DLL
void cp_http_transfer_descriptor_destroy(cp_http_transfer_descriptor *desc);

/** 
 * perform an asynchronous fetch. If background is non-zero, the fetch happens
 * in a separate thread and the callback is invoked asynchronously. Otherwise
 * the caller must make repeated calls to cp_httpclient_fetch_nb_exec() until
 * all transfers complete. cp_httpclient_fetch_nb_next() returns the number of
 * active transfers. cp_httpclient_fetch_nb returns zero if the request could
 * be registered, non-zero otherwise.
 */
CPROPS_DLL 
int cp_httpclient_fetch_nb(cp_httpclient *client, char *uri, void *id, 
						   cp_httpclient_callback callback, int background);

/**
 * same as cp_httpclient_fetch_nb but allows specifying a different 
 * asynchronous transfer control block.
 */
CPROPS_DLL
int cp_httpclient_fetch_ctl(cp_httpclient_ctl *ctl, cp_httpclient *client, 
							char *uri, void *id, 
							cp_httpclient_callback callback);

/**
 * non-blocking fetches executing in the foreground must be driven by calls to
 * cp_httpclient_fetch_nb_exec(). The callback for each request is made once it
 * completes or if an error occurs. cp_httplcient_fetch_nb_exec() returns the
 * number of active requests. Zero indicates all transfers have completed. 
 */
CPROPS_DLL
int cp_httpclient_fetch_nb_exec();

/**
 * same as cp_httpclient_fetch_nb_exec() but allows specifying a different
 * aysnchronous transfer control block.
 */
CPROPS_DLL
int cp_httpclient_fetch_ctl_exec(cp_httpclient_ctl *ctl);

__END_DECLS

/** @} */

#endif /* _CP_HTTP_CLIENT_H */
