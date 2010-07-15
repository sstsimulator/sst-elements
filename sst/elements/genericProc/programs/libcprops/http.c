
/**
 * @addtogroup cp_socket
 */
/** @{ */
/**
 * @file
 * http framework implementation
 */

/* supress warning for strndup */
#include "http.h"
#include <string.h>

#if defined(__APPLE__)
#include <sys/types.h>
#endif

#include <errno.h>
#include <stdlib.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <signal.h>

#if defined(unix) || defined(__unix__) || defined(__MACH__)
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#ifdef CP_USE_SSL
#include <openssl/ssl.h>
#include <openssl/err.h>
#endif

#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif /* HAVE_STRINGS_H */

#ifdef HAVE_REGEX_H
#include <regex.h>
#else
#ifdef HAVE_PCREPOSIX_H
#include <pcreposix.h>
#endif /* HAVE_PCREPOSIX_H */
#endif /* HAVE_REGEX_H */

#include "hashtable.h"
#include "hashlist.h"
#include "trie.h"
#include "vector.h"
#include "log.h"
#include "util.h"
#include "socket.h"

/****************************************************************************
 *                                                                          *
 *                    constants and initialization code                     *
 *                                                                          *
 ****************************************************************************/

#ifdef __OpenBSD__
#define HTTP_PARSE_BUFSIZE 0x1000
#else
#define HTTP_PARSE_BUFSIZE 0xFFFF
#endif

#define MATCHMAX 10 

static char *re_request_line_str = 
	"^(OPTIONS|GET|HEAD|POST|PUT|DELETE|TRACE|CONNECT)[[:space:]]+([^ ]+)[[:space:]]+HTTP/([^[:space:]]*)[[:space:]]*$";
static char *re_request_header_str = 
	"^([[:alnum:][:punct:]]+): (.*[^[:space:]])([[:space:]]?)$";
//	"^([[:alnum:][:punct:]]+): (.*[^[:space:]])([[:space:]]*)$";
static char *re_request_vars_str = "([^&]+)=([^&]*[^&[:space:]])&?";

static regex_t re_request_line;
static regex_t re_request_header;
static regex_t re_request_vars;

static char *cp_http_version_lit[] = { "1.0", "1.1" };

static char *connection_policy_lit[] = { "Close", "Close", "Keep-Alive" };

static char *content_type_lit[] = { "text/plain", "text/html", "image/jpeg" };

static char *server_error_fmt = 
	"<html>\n"
	"<head>\n"
	"<title> 400 BAD REQUEST </title>\n"
	"</head>\n"
	"<body>\n"
	"<h1> 400 BAD REQUEST: %s </h1>\n"
	"</body>\n"
	"</html>";

static int cp_http_server_error_code[] =
	{
		CP_HTTP_EMPTY_REQUEST,
		CP_HTTP_INVALID_REQUEST_LINE,
		CP_HTTP_UNKNOWN_REQUEST_TYPE,
		CP_HTTP_INVALID_URI,
		CP_HTTP_VERSION_NOT_SPECIFIED,
		CP_HTTP_1_1_HOST_NOT_SPECIFIED,
		CP_HTTP_INCORRECT_REQUEST_BODY_LENGTH,
		-1
	};
	
static char *cp_http_server_error_lit[] = 
	{
		"EMPTY REQUEST",
		"INVALID REQUEST LINE",
		"UNKNOWN REQUEST TYPE",
		"INVALID URI",
		"VERSION NOT SPECIFIED",
		"HTTP/1.1 HOST NOT SPECIFIED",
		"INCORRECT REQUEST BODY LENGTH"
	};

static cp_hashtable *cp_http_server_error;

struct cp_http_status_code_entry 
{
	cp_http_status_code code;
	char *lit;
} cp_http_status_code_list[] = 
	{
		{ HTTP_100_CONTINUE, "100 CONTINUE" },
		{ HTTP_101_SWITCHING_PROTOCOLS, "101 SWITCHING PROTOCOLS" },
		{ HTTP_200_OK, "200 OK" },
		{ HTTP_201_CREATED, "201 CREATED" },
		{ HTTP_202_ACCEPTED, "202 ACCEPTED" },
		{ HTTP_203_NON_AUTHORITATIVE_INFORMATION, "203 NON AUTHORITATIVE INFORMATION" },
		{ HTTP_204_NO_CONTENT, "204 NO CONTENT" },
		{ HTTP_205_RESET_CONTENT, "205 RESET CONTENT" },
		{ HTTP_206_PARTIAL_CONTENT, "206 PARTIAL CONTENT" },
		{ HTTP_300_MULTIPLE_CHOICES, "300 MULTIPLE CHOICES" },
		{ HTTP_301_MOVED_PERMANENTLY, "301 MOVED PERMANENTLY" },
		{ HTTP_302_FOUND, "302 FOUND" },
		{ HTTP_303_SEE_OTHER, "303 SEE OTHER" },
		{ HTTP_304_NOT_MODIFIED, "304 NOT MODIFIED" },
		{ HTTP_305_USE_PROXY, "305 USE PROXY" },
		{ HTTP_307_TEMPORARY_REDIRECT, "307 TEMPORARY REDIRECT" },
		{ HTTP_400_BAD_REQUEST, "400 BAD REQUEST" },
		{ HTTP_401_UNAUTHORIZED, "401 UNAUTHORIZED" },
		{ HTTP_402_PAYMENT_REQUIRED, "402 PAYMENT REQUIRED" },
		{ HTTP_403_FORBIDDEN, "403 FORBIDDEN" },
		{ HTTP_404_NOT_FOUND, "404 NOT FOUND" },
		{ HTTP_405_METHOD_NOT_ALLOWED, "405 METHOD NOT ALLOWED" },
		{ HTTP_406_NOT_ACCEPTABLE, "406 NOT ACCEPTABLE" },
		{ HTTP_407_PROXY_AUTHENTICATION_REQUIRED, "407 PROXY AUTHENTICATION REQUIRED" },
		{ HTTP_408_REQUEST_TIME_OUT, "408 REQUEST TIME OUT" },
		{ HTTP_409_CONFLICT, "409 CONFLICT" },
		{ HTTP_410_GONE, "410 GONE" },
		{ HTTP_411_LENGTH_REQUIRED, "411 LENGTH REQUIRED" },
		{ HTTP_412_PRECONDITION_FAILED, "412 PRECONDITION FAILED" },
		{ HTTP_413_REQUEST_ENTITY_TOO_LARGE, "413 REQUEST ENTITY TOO LARGE" },
		{ HTTP_414_REQUEST_URI_TOO_LARGE, "414 REQUEST URI TOO LARGE" },
		{ HTTP_415_UNSUPPORTED_MEDIA_TYPE, "415 UNSUPPORTED MEDIA TYPE" },
		{ HTTP_416_REQUESTED_RANGE_NOT_SATISFIABLE, "416 REQUESTED RANGE NOT SATISFIABLE" },
		{ HTTP_417_EXPECTATION_FAILED, "417 EXPECTATION FAILED" },
		{ HTTP_500_INTERNAL_SERVER_ERROR, "500 INTERNAL SERVER ERROR" },
		{ HTTP_501_NOT_IMPLEMENTED, "501 NOT IMPLEMENTED" },
		{ HTTP_502_BAD_GATEWAY, "502 BAD GATEWAY" },
		{ HTTP_503_SERVICE_UNAVAILABLE, "503 SERVICE UNAVAILABLE" },
		{ HTTP_504_GATEWAY_TIME_OUT, "504 GATEWAY TIME OUT" },
		{ HTTP_505_HTTP_VERSION_NOT_SUPPORTED, "505 HTTP VERSION NOT SUPPORTED" },
		{ HTTP_NULL_STATUS, NULL}
	};

static cp_hashtable *cp_http_status_lit;

static char *cp_http_request_type_lit[] = 
	{
		"OPTIONS", 
		"GET", 
		"HEAD", 
		"POST", 
		"PUT", 
		"DELETE", 
		"TRACE", 
		"CONNECT" 
	};

static volatile int initialized = 0;

static volatile int socket_reg_id = 0;
static cp_hashlist *socket_registry = NULL;
static cp_vector *shutdown_hook = NULL;

void cp_http_signal_handler(int sig);
			
void *session_cleanup_thread(void *);

int cp_http_init()
{
	int i;
	int rc;
#if defined(unix) || defined(__unix__) || defined(__MACH__)
	struct sigaction act;
#endif

	if (initialized) return initialized++;
	initialized = 1;

	cp_socket_init();

#if defined(unix) || defined(__unix__) || defined(__MACH__)
	act.sa_handler = cp_http_signal_handler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	sigaction(SIGINT, &act, NULL);
	sigaction(SIGPIPE, &act, NULL);
	sigaction(SIGTERM, &act, NULL);
#endif

	if ((rc = regcomp(&re_request_line, re_request_line_str, 
			REG_EXTENDED | REG_NEWLINE)))
		return log_regex_compilation_error(rc, "cp_http_init - request line");
	
	if ((rc = regcomp(&re_request_header, re_request_header_str, 
			REG_EXTENDED | REG_NEWLINE)))
		return log_regex_compilation_error(rc, "cp_http_init - request header");

	if ((rc = regcomp(&re_request_vars, re_request_vars_str, 
			REG_EXTENDED | REG_NEWLINE)))
		return log_regex_compilation_error(rc, "cp_http_init - request variables");

	cp_http_server_error = cp_hashtable_create(10, cp_hash_int, cp_hash_compare_int);
	for (i = 0; cp_http_server_error_code[i] != -1; i++)
		cp_hashtable_put(cp_http_server_error, 
					  &cp_http_server_error_code[i], 
					  cp_http_server_error_lit[i]);

	cp_http_status_lit = cp_hashtable_create(50, cp_hash_int, cp_hash_compare_int);
	for (i = 0; cp_http_status_code_list[i].lit != NULL; i++)
		cp_hashtable_put(cp_http_status_lit,
					  &cp_http_status_code_list[i].code, 
				      cp_http_status_code_list[i].lit);
	
	socket_registry = 
		cp_hashlist_create_by_mode(1, COLLECTION_MODE_DEEP, 
				                   cp_hash_int, cp_hash_compare_int);

#ifdef CP_USE_HTTP_SESSIONS
	{
		cp_thread _session_cleanup_thread;
		cp_thread_create(_session_cleanup_thread, NULL, 
				         session_cleanup_thread, NULL);
#ifdef HAVE_PTHREAD_H
		cp_thread_detach(_session_cleanup_thread);
#endif
	}
#endif
	
	return 0;
}

volatile int cp_http_stopping = 0;

/* signal all sockets in select() to stop */
void cp_httpsocket_stop_all()
{
	cp_http_stopping = 1;
	cp_socket_stop_all();
}

static volatile int cp_http_shutting_down = 0;

void cp_httpsocket_stop(cp_httpsocket *sock)
{
	if (sock) sock->sock->closing = 1;
}

void *cp_http_add_shutdown_callback(void (*cb)(void *), void *prm)
{
	cp_wrap *wrap;
	
	if (shutdown_hook == NULL)
		shutdown_hook = cp_vector_create(1);
	
	wrap = cp_wrap_new(prm, cb);

	cp_vector_add_element(shutdown_hook, wrap);

	return wrap;
}

void cp_http_shutdown()
{
	if (cp_http_shutting_down) return; /* one shutdown should be enough */

	cp_http_shutting_down = 1;

#ifdef __TRACE__
	DEBUGMSG("shutting down http services");
#endif

	if (shutdown_hook)
	{
		cp_vector_destroy_custom(shutdown_hook, 
				                 (cp_destructor_fn) cp_wrap_delete);
		shutdown_hook = NULL;
	}

	if (initialized)
	{
		regfree(&re_request_line);
		regfree(&re_request_header);
		regfree(&re_request_vars);
	}

	cp_hashtable_destroy(cp_http_server_error);
	cp_hashtable_destroy(cp_http_status_lit);
	cp_hashlist_destroy_custom(socket_registry, NULL, 
							   (cp_destructor_fn) cp_httpsocket_stop);
	socket_registry = NULL;

	cp_socket_shutdown();

	initialized = 0;
}

void cp_http_signal_handler(int sig)
{
	switch (sig)
	{
		case SIGINT: 
#ifdef __TRACE__
			DEBUGMSG("SIGINT - stopping\n");
#endif
			cp_http_shutdown();
			break;

#ifdef SIGPIPE
		case SIGPIPE: 
#ifdef __TRACE__
			DEBUGMSG("SIGPIPE - ignoring\n");
#endif
			break;
#endif /* SIGPIPE */

		case SIGTERM:
#ifdef __TRACE__
			DEBUGMSG("SIGTERM - stopping\n");
#endif
			cp_http_shutdown();
			break;
	}
}

#define HTTP_RE_MATCHMAX 10

#ifdef CP_USE_HTTP_SESSIONS

/****************************************************************************
 *                                                                          *
 *  cp_http_session_* functions:                                            *
 *                                                                          *
 *  o cp_httpsocket_create_session - creates a session [static]             *
 *  o cp_httpsocket_checkout_session - retrieves/creates session [static]   *
 *  o cp_httpsocket_remove_session - drops a session [static]               *
 *  o cp_http_session_delete - deletes a session [static]                   *
 *  o cp_http_session_inc_refcount - increase reference count               *
 *  o cp_http_session_dec_refcount - decrease reference count               *
 *  o cp_http_session_get - retrieves a value by key                        *
 *  o cp_http_session_set - sets a value that doesn't need to be deleted    *
 *  o cp_http_session_set_dtr - sets a value with a destructor callback     *
 *  o cp_http_session_set_validity - sets session validity time             *
 *  o cp_http_session_is_fresh - session is newly created                   *
 *                                                                          *
 *  client code should use cp_http_request_get_session to get a reference   *
 *  to the session. use cp_http_session_increase_refcount and               *
 *  cp_http_session_decrease_refcount only if performing special tricks.    *
 *                                                                          *
 ****************************************************************************/

static cp_http_session *cp_httpsocket_create_session(cp_httpsocket *sock)
{
	cp_http_session *session = calloc(1, sizeof(cp_http_session));
	if (session)
	{
		struct timeval tm;
		gettimeofday(&tm, NULL);
		session->sid = malloc(17 * sizeof(char));
		if (session->sid == NULL)
		{
			free(session);
			return NULL;
		}
		session->key = 
			cp_hashtable_create_by_option(COLLECTION_MODE_COPY |
                                          COLLECTION_MODE_DEEP,
										  1,
										  cp_hash_string, 
										  cp_hash_compare_string,
										  (cp_copy_fn) strdup, 
										  (cp_destructor_fn) free,
										  NULL, 
										  (cp_destructor_fn) cp_wrap_delete);
		if (session->key == NULL)
		{
			free(session->sid);
			free(session);
			return NULL;
		}

		session->created = session->access = tm.tv_sec;
		session->validity = DEFAULT_SESSION_VALIDITY;
		session->renew_on_access = 1;
		session->fresh = 1;
		session->refcount = 1;
		session->valid = 1;

		if (sock) /* may be null if called by separate instance */
		{
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
				if (sock->session == NULL)
				{
					cp_http_session_delete(session);
					return NULL;
				}
			}
		//~~ although improbable (~ 1:100,000), gen_tm_id_str may return 
		// doubles and should be replaced with a more robust function. until
		// then, make sure the id isn't already in there.
			do 
			{ 
				gen_tm_id_str(session->sid, &tm);
			} while (cp_hashlist_get(sock->session, session->sid));

			cp_hashlist_append(sock->session, session->sid, session);
		}
		else
			gen_tm_id_str(session->sid, &tm); //~~ incorporate pid info
	}

	return session;
}


static cp_http_session *
	cp_httpsocket_checkout_session(cp_httpsocket *sock, char *sid)
{
	cp_http_session *session = 
		sock->session ? cp_hashlist_get(sock->session, sid) : NULL;

	if (session)
		session->refcount++;
    //~~ else create invalid session with specified id?

	return session;
}

/**
 * internal ~session() function, called by cp_httpsocket_remove_session when
 * refcount hits 0 - do not invoke directly. 
 */
void cp_http_session_delete(cp_http_session *session)
{
	if (session)
	{
		if (session->sid) free(session->sid);
		if (session->key) 
			cp_hashtable_destroy(session->key); /* calls ~cp_wrap() */
		free(session);
	}
}

void *cp_http_session_get(cp_http_session *session, char *key)
{
	cp_wrap *wrap = cp_hashtable_get(session->key, key);
	if (wrap)
		return wrap->item;
	return NULL;
}

void *cp_http_session_set_dtr(cp_http_session *session, 
		                      char *key, void *value, 
							  cp_destructor_fn dtr)
{
	cp_wrap *wrap = cp_wrap_new(value, dtr);
	if (wrap)
		return cp_hashtable_put(session->key, key, wrap);
	return NULL;
}

void *cp_http_session_set(cp_http_session *session, 
		                         char *key, void *value)
{
	return cp_http_session_set_dtr(session, key, value, NULL);
}

void cp_http_session_set_validity(cp_http_session *session, long sec)
{
	session->validity = sec;
}

short cp_http_session_is_fresh(cp_http_session *session)
{
	return session->fresh;
}

static char *redirect_fmt = "<html><head><title>302 FOUND</title></head><body><h1>302 FOUND</h1>The document has moved <a href=\"%s\"> here </a>.</body></html>";

/**
 * send a redirect and set a session id cookie. This is captured on the next
 * client request, and the original request is then written to the client.
 */
static cp_http_response *
	cp_http_new_session_redirect(cp_http_request *req, int step)
{
	cp_http_response *res = cp_http_response_create(req);
	
	if (res)
	{
		char redirect_url[MAX_URL_LENGTH]; 
#ifdef CP_USE_COOKIES
		char cookie_path[MAX_URL_LENGTH]; 
		char *cp;
#endif /* CP_USE_COOKIES */
		char *host; 
		char buf[HTTP_PARSE_BUFSIZE];
		cp_string *content;
		char *protocol;

#ifdef CP_USE_SSL
		if (req->owner->sock->use_ssl)
			protocol = "https";
		else
#endif
		protocol = "http";

		if ((host = cp_http_request_get_header(req, "Host")) == NULL)
			host = "localhost"; //~~ figure out hostname
		
#ifdef CP_USE_COOKIES
		if (step == 2)
#ifdef HAVE_SNPRINTF
			snprintf(redirect_url, MAX_URL_LENGTH, 
					 "%s://%s%s", protocol, host, req->uri);
#else
			sprintf(redirect_url, "%s://%s%s", protocol, host, req->uri);
#endif /* HAVE_SNPRINTF */
		else
#endif /* CP_USE_COOKIES */
#ifdef HAVE_SNPRINTF
		snprintf(redirect_url, MAX_URL_LENGTH, "%s://%s%s?%s=%s", protocol, 
                 host, req->uri, CP_HTTP_SESSION_PRM, req->session->sid);
#else
		sprintf(redirect_url, "%s://%s%s?%s=%s", protocol, host, req->uri,
				CP_HTTP_SESSION_PRM, req->session->sid);
#endif /* HAVE_SNPRINTF */

		cp_http_response_set_status(res, HTTP_302_FOUND);
		cp_http_response_set_header(res, "Location", redirect_url);
		cp_http_response_set_connection_policy(res, 
				HTTP_CONNECTION_POLICY_KEEP_ALIVE);

#ifdef HAVE_SNPRINTF
		snprintf(buf, HTTP_PARSE_BUFSIZE, redirect_fmt, redirect_url);
#else
		sprintf(buf, redirect_fmt, redirect_url);
#endif /* HAVE_SNPRINTF */
		content = cp_string_cstrdup(buf);
		cp_http_response_set_content(res, content);

#ifdef CP_USE_COOKIES
		if (step == 1)
		{
			strlcpy(cookie_path, req->uri, MAX_URL_LENGTH);
			if ((cp = strrchr(cookie_path, '/')))
			{
				cp++;
				*cp = '\0';
			}
		
			cp_http_response_set_cookie(res, CP_HTTP_SESSION_PRM, 
					req->session->sid, host, cookie_path, 
//					req->session->validity, 
					31536000, // 1 yr == 60 * 60 * 24 * 365
#ifdef CP_USE_SSL
				req->owner->sock->use_ssl
#else
				0
#endif /* CP_USE_SSL */
				);
		}
#endif /* CP_USE_COOKIES */
	}

	return res;
}

/** session cleanup thread function */
void *session_cleanup_thread(void *prm)
{
	cp_hashlist_iterator i, j;
	cp_httpsocket *curr;
	cp_http_session *session;
	time_t now;
	int rc;

	while (!(cp_http_shutting_down || cp_http_stopping))
	{
		rc = cp_sleep(900);
		if (cp_http_shutting_down || cp_http_stopping) break;
#ifdef __TRACE__
		DEBUGMSG("session_cleanup_thread running");
#endif
		now = time(NULL);
		cp_hashlist_iterator_init(&i, socket_registry, COLLECTION_LOCK_READ);
		while ((curr = cp_hashlist_iterator_next_value(&i)))
		{
			if (curr->session == NULL) continue;
			cp_hashlist_iterator_init(&j, curr->session, COLLECTION_LOCK_WRITE);
			session = cp_hashlist_iterator_curr_value(&j);
			while (session != NULL)
			{
				time_t expiration = (session->renew_on_access ? 
						session->access : session->created) + session->validity;
				if (now > expiration) /* expired */
				{
					if (session->refcount <= 0)
					{
						if (curr->pending)
						{
							cp_http_response *res = 
								cp_hashlist_remove(curr->pending, session->sid);
							if (res) 
								cp_http_response_delete(res);
						}
						cp_hashlist_iterator_remove(&j);
					}
				}
				session = cp_hashlist_iterator_next_value(&j);
			}
			cp_hashlist_iterator_release(&j); /* release locks */
		}
		cp_hashlist_iterator_release(&i); /* release locks here too */
#ifdef __TRACE__
		DEBUGMSG("session_cleanup_thread - cleanup done");
#endif
	}
#ifdef __TRACE__
	DEBUGMSG("session_cleanup_thread exits");
#endif
		
	return NULL;
}
#endif /* CP_USE_HTTP_SESSIONS */

/****************************************************************************
 *                                                                          *
 *  cp_http_request_* functions:                                            *
 *                                                                          *
 *  o cp_http_request_delete - performs cleanup                             *
 *  o cp_http_request_parse - parses a request from a char buffer           *
 *  o cp_http_request_get_header - returns headers by name                  *
 *  o cp_http_request_dump - dumps request to stdout and log                *
 *                                                                          *
 ****************************************************************************/

void cp_http_request_delete(cp_http_request *request)
{
	if (request)
	{
		free(request->uri);
#ifdef CP_USE_COOKIES
		if (request->cookie)
			cp_vector_destroy_custom(request->cookie, free);
#endif /* CP_USE_COOKIES */
		if (request->header)
			cp_hashtable_destroy(request->header);
		if (request->query_string)
			free(request->query_string);
		if (request->prm)
			cp_hashtable_destroy(request->prm);
#ifdef CP_USE_HTTP_SESSIONS
		if (request->session) request->session->refcount--;
#endif /* CP_USE_HTTP_SESSIONS */
		free(request);
	}
}

char *get_http_request_type_lit(cp_http_request_type type)
{
	return 	cp_http_request_type_lit[type];
}

static int parse_request_line(cp_http_request *req, char *request, int *plen)
{
	regmatch_t rm[HTTP_RE_MATCHMAX];
	int len;
	int valid;

	if (regexec(&re_request_line, request, HTTP_RE_MATCHMAX, rm, 0))
		return CP_HTTP_INVALID_REQUEST_LINE;

	valid = 0;
	len = rm[1].rm_eo - rm[1].rm_so;
	if (len)
		for (req->type = OPTIONS; req->type <= CONNECT; req->type++)
			if (strncasecmp(&request[rm[1].rm_so], 
				cp_http_request_type_lit[req->type], len) == 0)
			{
				valid = 1;
				break;
			}

	if (!valid) /* unknown or missing method */
		return CP_HTTP_UNKNOWN_REQUEST_TYPE;

	valid = 0;
	len = rm[2].rm_eo - rm[2].rm_so;
	if (len)
	{
		char *q = strnchr(&request[rm[2].rm_so], '?', rm[0].rm_eo);
		if (q) 
		{
			//~~ check if this is a GET - if not, reject
			len = q - request;
			req->query_string = strndup(q + 1, rm[2].rm_eo - len - 1);
			len -= rm[2].rm_so;
		}

		req->uri = strndup(&request[rm[2].rm_so], len);
		valid = 1;
	}
	
	if (!valid) /* invalid uri */
		return CP_HTTP_INVALID_URI;

	valid = 0;
	len = rm[3].rm_eo - rm[3].rm_so;

	if (len)
	{
		if (strncmp(&request[rm[3].rm_so], "1.0", len) == 0)
		{
			req->version = HTTP_1_0;
			valid = 1;
		}
		else if (strncmp(&request[rm[3].rm_so], "1.1", len) == 0)
		{
			req->version = HTTP_1_1;
			valid = 1;
		}
	}

	if (!valid) /* HTTP version not specified */
		return CP_HTTP_VERSION_NOT_SPECIFIED;

	*plen = rm[0].rm_eo;

	return 0;
}

#define REQUEST_STAGE_START        0
#define REQUEST_STAGE_STATUS_LINE  1
#define REQUEST_STAGE_HEADERS      2
#define REQUEST_STAGE_BODY         3
#define REQUEST_STAGE_DONE         4

static int is_empty_line(char *c)
{
	return strncmp(c, "\r\n\r\n", 4) == 0 || strncmp(c, "\n\n", 2) == 0;
}

static void skip_eol(char **c)
{
	if (**c == '\r') (*c)++;
	if (**c == '\n') (*c)++;
	if (**c == '\r') (*c)++;
	if (**c == '\n') (*c)++;
}

static int 
	parse_headers(cp_http_request *req, char *request, int *plen, int *stage)
{
	char *curr = request;
	regmatch_t rm[MATCHMAX];
	int len;
	char *name, *value;
	char *eol;
			
	if (req->header == NULL)
		req->header = 
			cp_hashtable_create_by_option(COLLECTION_MODE_NOSYNC | 
					                      COLLECTION_MODE_DEEP, 10,
										  cp_hash_string, 
										  cp_hash_compare_string, 
										  NULL, free, NULL, free);
			
	while (regexec(&re_request_header, curr, MATCHMAX, rm, 0) == 0)
	{
		len = rm[1].rm_eo - rm[1].rm_so;
		if ((name = (char *) malloc((len + 1) * sizeof(char))) == NULL) 
			return CP_MEMORY_ALLOCATION_FAILURE;
		strncpy(name, curr + rm[1].rm_so, len);
		name[len] = '\0';

		len = rm[2].rm_eo - rm[2].rm_so;
		if ((value = (char *) malloc((len + 1) * sizeof(char))) == NULL) 
		{
			free(name);
			return CP_MEMORY_ALLOCATION_FAILURE;
		}
		strncpy(value, curr + rm[2].rm_so, len);
		value[len] = '\0';
#ifdef CP_USE_COOKIES
		if (strcmp(name, "Cookie") == 0)
		{
			if (req->cookie == NULL)
			{
				req->cookie = cp_vector_create(1);
				if (req->cookie == NULL)
					return CP_MEMORY_ALLOCATION_FAILURE;
			}
			cp_vector_add_element(req->cookie, value);
			free(name);
		}
		else
#endif /* CP_USE_COOKIES */
		cp_hashtable_put(req->header, name, value);

		eol = &curr[rm[2].rm_eo];
		curr = &curr[rm[0].rm_eo];
		if (is_empty_line(eol)) 
		{
			*stage = REQUEST_STAGE_DONE;
			break;
		}
	}
	
	skip_eol(&curr);
	*plen = curr - request;

	if ((req->version == HTTP_1_1) && 
		(cp_http_request_get_header(req, "Host") == NULL))
		return CP_HTTP_1_1_HOST_NOT_SPECIFIED;
			
	return 0;
}

static int parse_cp_http_request_body(cp_http_request *req, 
								      char *request, 
									  int *used)
{
	int len;
	int dlen;
	char *clen;
	char *p = &request[*used];

	clen = cp_http_request_get_header(req, "Content-Length");
	if (clen == NULL) 
	{ 
		cp_warn("no length header"); 
		return CP_HTTP_INCORRECT_REQUEST_BODY_LENGTH; 
	}
	dlen = atoi(clen);
	
	if (req->query_string)
	{
		int before = strlen(req->query_string);
		len = dlen - before;
		strncat(req->query_string, request, len);
		len = strlen(req->query_string) - before;
	}
	else
	{
		req->query_string = malloc(dlen + 1);
		if (*p == '\r') { p++; (*used)++; }
		if (*p == '\n') { p++; (*used)++; }
		strncpy(req->query_string, p, dlen);
		req->query_string[dlen] = '\0';
		len = strlen(req->query_string);
	}

	*used += len;
	p = &request[*used];
	if (*p == '\r') { p++; (*used)++; }
	if (*p == '\n') { p++; (*used)++; }

	return 0;
}

#define HEXDIGIT(c) ((c) >= '0' && (c) <= '9' ? (c) - '0' : \
		             ((c) >= 'A' && (c) <= 'F' ? (c) - 'A' + 0xA : \
		             ((c) >= 'a' && (c) <= 'f' ? (c) - 'a' + 0xA : -1)))

void urldecode(char *str, char **decoded)
{
	char dbuf[HTTP_PARSE_BUFSIZE];
	char *src, *dst;
	int len;

	for (src = str, dst = dbuf, len = 0; *src && len < HTTP_PARSE_BUFSIZE; src++, dst++, len++)
	{
		switch (*src)
		{
			case '+': 
				*dst = ' '; 
				break; 
				
			case '%': 
				src++;
				if (*src == '%') 
					*dst = '%';
				else
					*dst = (HEXDIGIT(src[0])) << 4 | (HEXDIGIT(src[1]));
					src++;
				break;

			default:
				*dst = *src;
				break;
		}
	}

	*dst = '\0';
	
	*decoded = strndup(dbuf, len);
}

static int parse_cgi_vars(cp_http_request *req)
{
	regmatch_t rm[MATCHMAX];
	int len;
	char *pos;
	char *raw_name, *raw_value;
	char *name, *value;

	req->prm = 
		cp_hashtable_create_by_option(COLLECTION_MODE_NOSYNC | 
				                      COLLECTION_MODE_DEEP, 10,
									  cp_hash_string, cp_hash_compare_string,
									  NULL, (cp_destructor_fn) free,
									  NULL, (cp_destructor_fn) free);

	for (pos = req->query_string; 
		 regexec(&re_request_vars, pos, MATCHMAX, rm, 0) == 0; 
		 pos = &pos[rm[0].rm_eo])
	{
		len = rm[1].rm_eo - rm[1].rm_so;
		raw_name = strndup(&pos[rm[1].rm_so], len);

		len = rm[2].rm_eo - rm[2].rm_so;
		raw_value = strndup(&pos[rm[2].rm_so], len);

		urldecode(raw_name, &name);
		urldecode(raw_value, &value);
		free(raw_name);
		free(raw_value);
		
		cp_hashtable_put(req->prm, name, value);
	}

	return 0;
}

char *cp_http_request_get_header(cp_http_request *req, char *header)
{
	char *res = NULL;
	
	if (req && header)
		res = cp_hashtable_get(req->header, header);

	return res;
}

char **cp_http_request_get_headers(cp_http_request *request)
{
	char **res = NULL;

	if (request) res = (char **) cp_hashtable_get_keys(request->header);

	return res;
}

char *cp_http_request_get_parameter(cp_http_request *request, char *name)
{
	char *res = NULL;

	if (request && name && request->prm)
		res = cp_hashtable_get(request->prm, name);

	return res;
}

#ifdef CP_USE_HTTP_SESSIONS
cp_http_session *
	cp_http_request_get_session(cp_http_request *request, int create)
{
	if (request->session == NULL && create)
		request->session = cp_httpsocket_create_session(request->owner);

	return request->session;
}
#endif /* CP_USE_HTTP_SESSIONS */

void cp_http_request_dump(cp_http_request *req)
{
	int i;
	char **keys;
	
	cp_info("dumping http request (%lX)", (long) req);
	cp_info("--------------------------------------------------------------");
	cp_info("method: %s", cp_http_request_type_lit[req->type]);
	cp_info("version: HTTP/%s", cp_http_version_lit[req->version]);
	cp_info("uri: [%s]", req->uri);
	
	cp_info("headers:\n--------");
	keys = (char **) cp_hashtable_get_keys(req->header);
	for (i = 0; i < cp_hashtable_count(req->header); i++)
		cp_info("%s: %s", keys[i], (char *) cp_hashtable_get(req->header, keys[i]));
	free(keys);

#ifdef CP_USE_COOKIES
	if (req->cookie)
	{
		int n;
		cp_info("cookies:\n--------");
		n = cp_vector_size(req->cookie);
		for (i = 0; i < n; i++)
			cp_info("%s", cp_vector_element_at(req->cookie, i));
	}
#endif /* CP_USE_COOKIES */

	if (req->prm)
	{
		cp_info("cgi parameters:\n---------------");
		keys = (char **) cp_hashtable_get_keys(req->prm);
		for(i = 0; i < cp_hashtable_count(req->prm); i++)
			cp_info("%s: %s", keys[i], (char *) cp_hashtable_get(req->prm, keys[i]));
		free(keys);
	}

	cp_info("--------------------------------------------------------------");
}


/****************************************************************************
 *                                                                          *
 *  cp_http_response_* functions:                                           *
 *                                                                          *
 *  o cp_http_response_create - allocates & initialized response struct     *
 *  o cp_http_response_delete - releases a response struct                  *
 *  o cp_http_response_write - writes response to client                    *
 *                                                                          *
 *  o cp_http_response_set_status - set the status code for a response      *
 *  o cp_http_response_get_status - get the status code of a response       *
 *  o cp_http_response_set_content_type - set content type code [deprecated]*
 *  o cp_http_response_set_content_type_string - set content type           *
 *  o cp_http_response_get_content_type - get content type                  *
 *  o cp_http_response_set_header - set a header                            *
 *  o cp_http_response_get_header - get a header value                      *
 *  o cp_http_response_get_header_names - get a vector of header names      *
 *  o cp_http_response_set_body - set the response content [deprecated]     *
 *  o cp_http_response_set_content - set the response content               *
 *  o cp_http_response_get_content - get the response content               *
 *  o cp_http_response_set_connection_policy - Close / Keep-Alive           *
 *  o cp_http_response_skip - framework doesn't write response object       *
 *                                                                          *
 ****************************************************************************/

cp_http_response *cp_http_response_create(cp_http_request *req)
{
	cp_http_response *res = NULL;
	
	if ((res = (cp_http_response *) calloc(1, sizeof(cp_http_response))))
	{
		res->request = req;
        res->header = 
            cp_hashtable_create_by_option(COLLECTION_MODE_NOSYNC |
										  COLLECTION_MODE_COPY |
                                          COLLECTION_MODE_DEEP,
                                          10, 
                                          cp_hash_string, 
                                          cp_hash_compare_string,
                                          (cp_copy_fn) strdup, free, 
                                          (cp_copy_fn) strdup, free);
		if (res->header == NULL)
		{
			free(res);
			return NULL;
		}
		res->status = HTTP_200_OK;
		res->version = req ? req->version : HTTP_1_1;
		res->servername = req && req->owner ? req->owner->server_name : DEFAULT_SERVER_NAME;
	}

	return res;
}

void cp_http_response_delete(cp_http_response *response)
{
	if (response)
	{
		if (response->header)
			cp_hashtable_destroy(response->header);
#ifdef CP_USE_COOKIES
		if (response->cookie)
			cp_vector_destroy_custom(response->cookie, free);
#endif /* CP_USE_COOKIES */
		if (response->content_type_lit)
			free(response->content_type_lit);
		if (response->content)
			cp_string_delete(response->content);
//~~ check this free
//		if (response->body)
//			free(response->body);
		if (response->status_lit) 
			free(response->status_lit);
		free(response);
	}
}

void cp_http_response_destroy(cp_http_response *res)
{
	cp_http_response_delete(res);
}

int cp_http_response_write(cp_connection_descriptor *cdesc, 
		                   cp_http_response *res)
{
	char rbuf[HTTP_PARSE_BUFSIZE];
	char *content = NULL;
	int content_len = 0;
	int len;
	int rc, total;
	int fd = cp_connection_descriptor_get_fd(cdesc);
#ifdef CP_USE_SSL
	cp_socket *sock = cp_connection_descriptor_get_socket(cdesc);
#endif

	if (res->len == 0 && res->body) res->len = strlen(res->body);

#ifdef HAVE_SNPRINTF
	snprintf(rbuf, HTTP_PARSE_BUFSIZE, 
		"HTTP/%s %s\r\n"
		"Server: %s\r\n"
		"Connection: %s\r\n", 
			cp_http_version_lit[res->version], 
			(char *) cp_hashtable_get(cp_http_status_lit, &res->status), 
			res->servername ? res->servername : DEFAULT_SERVER_NAME, 
			connection_policy_lit[res->connection]);
#else
	sprintf(rbuf, 
		"HTTP/%s %s\r\n"
		"Server: %s\r\n"
		"Connection: %s\r\n", 
			cp_http_version_lit[res->version], 
			(char *) cp_hashtable_get(cp_http_status_lit, &res->status), 
			res->servername ? res->servername : DEFAULT_SERVER_NAME, 
			connection_policy_lit[res->connection]);
#endif /* HAVE_SNPRINTF */

	if (res->header)
	{
		char hbuf[HTTP_PARSE_BUFSIZE];
		int i;
		char **key = (char **) cp_hashtable_get_keys(res->header);
		for (i = 0; i < cp_hashtable_count(res->header); i++)
		{
#ifdef HAVE_SNPRINTF
			snprintf(hbuf, HTTP_PARSE_BUFSIZE, "%s: %s\r\n", 
					key[i], (char *) cp_hashtable_get(res->header, key[i]));
#else
			sprintf(hbuf, "%s: %s\r\n", 
					key[i], (char *) cp_hashtable_get(res->header, key[i]));
#endif /* HAVE_SNPRINTF */
			strlcat(rbuf, hbuf, HTTP_PARSE_BUFSIZE);
		}
		free(key);
	}
				
#ifdef CP_USE_COOKIES
	if (res->cookie)
	{
		char cbuf[HTTP_PARSE_BUFSIZE];
		int i;
		for (i = 0; i < cp_vector_size(res->cookie); i++)
		{
#ifdef HAVE_SNPRINTF
			snprintf(cbuf, HTTP_PARSE_BUFSIZE, "Set-Cookie: %s\r\n", 
					(char *) cp_vector_element_at(res->cookie, i));
#else
			sprintf(cbuf, "Set-Cookie: %s\r\n", 
					(char *) cp_vector_element_at(res->cookie, i));
#endif /* HAVE_SNPRINTF */
			strlcat(rbuf, cbuf, HTTP_PARSE_BUFSIZE);
		}
	}
#endif /* CP_USE_COOKIES */

	if (res->content)
	{
		char bbuf[HTTP_PARSE_BUFSIZE];
#ifdef HAVE_SNPRINTF
		snprintf(bbuf, HTTP_PARSE_BUFSIZE, 
			"Content-Type: %s\r\nContent-Length: %d\r\n\r\n",
			res->content_type_lit ? 
				res->content_type_lit : content_type_lit[res->content_type],
			res->content->len); //~~
#else
		sprintf(bbuf, "Content-Type: %s\r\nContent-Length: %d\r\n\r\n",
			res->content_type_lit ? 
				res->content_type_lit : content_type_lit[res->content_type],
			res->content->len); //~~
#endif /* HAVE_SNPRINTF */
		strlcat(rbuf, bbuf, HTTP_PARSE_BUFSIZE);
		content = res->content->data;
		content_len = res->content->len;
	}
	else if (res->body)
	{
		char bbuf[HTTP_PARSE_BUFSIZE];
		content_len = strlen(res->body);
#ifdef HAVE_SNPRINTF
		snprintf(bbuf, HTTP_PARSE_BUFSIZE, 
			"Content-Type: %s\r\nContent-Length: %d\r\n\r\n",
			res->content_type_lit ? 
				res->content_type_lit : content_type_lit[res->content_type],
			content_len);
#else
		sprintf(bbuf, "Content-Type: %s\r\nContent-Length: %d\r\n\r\n",
			res->content_type_lit ? 
				res->content_type_lit : content_type_lit[res->content_type],
			content_len);
#endif /* HAVE_SNPRINTF */
		strlcat(rbuf, bbuf, HTTP_PARSE_BUFSIZE);
		content = res->body;
	}

	len = strlen(rbuf);

#ifdef CP_USE_SSL
	if (sock->use_ssl)
	{
		rc = SSL_write(cdesc->ssl, rbuf, len);
		total = rc;
		if (content_len)
		{
			rc = SSL_write(cdesc->ssl, content, content_len);
			total += rc;
		}
	}
	else
#endif /* CP_USE_SSL */
	{
		rc = send(fd, rbuf, len, 0); //~~ handle errors etc.
		total = rc;
		if (content_len)
		{
			rc = send(fd, content, content_len, 0);
			total += rc;
		}
	}
	
#ifdef __TRACE__
	cp_info("----- http response headers -----\n%s", rbuf);
	if (res->content) 
		cp_ndump(LOG_LEVEL_INFO, res->content, 0x400);
	else if (res->body) 
		cp_nlog(0x400, content);
	if (content_len > 0x400) 
		cp_log("\n ... truncated (%d bytes)\n", content_len);
	cp_info("---------------------------------\n");
#endif

	if (total > 0) cdesc->bytes_sent += total;

#ifdef DEBUG
	DEBUGMSG("%d bytes written", total);
#endif
	
	return rc;
}

void cp_http_response_set_status(cp_http_response *response, 
		                                cp_http_status_code status)
{
	response->status = status;
}

cp_http_status_code cp_http_response_get_status(cp_http_response *response)
{
	return response->status;
}

void cp_http_response_set_content_type(cp_http_response *response, 
		                                      cp_http_content_type type)
{
	response->content_type = type;
}

void cp_http_response_set_content_type_string(cp_http_response *response,
		                                             char *content_type)
{
	response->content_type_lit = content_type ? strdup(content_type) : NULL;
}

char *cp_http_response_get_content_type(cp_http_response *response)
{
	return response->header ? cp_hashtable_get(response->header, "Content-Type"): NULL;
}

void cp_http_response_set_header(cp_http_response *response, 
		                                char *name, char *value)
{
	cp_hashtable_put(response->header, name, value);
}

char *cp_http_response_get_header(cp_http_response *response, char *name)
{
	return cp_hashtable_get(response->header, name);
}

cp_vector *cp_http_response_get_header_names(cp_http_response *response)
{
	cp_vector *names = NULL;
	if (response->header)
	{
		int len = cp_hashtable_count(response->header);
		names = cp_vector_wrap(cp_hashtable_get_keys(response->header), len, 0);
		if (names == NULL) return NULL;
	}

	return names;
}

void cp_http_response_set_body(cp_http_response *response, 
	                                  char *body)
{
	response->body = strdup(body);
}

void cp_http_response_set_content(cp_http_response *response, 
	                                     cp_string *content)
{
	response->content = content;
}

cp_string *cp_http_response_get_content(cp_http_response *response)
{
	return response->content;
}

void cp_http_response_set_connection_policy(cp_http_response *response, 
		                                    connection_policy policy)
{
	response->connection = policy;
}

#ifdef CP_USE_COOKIES
int cp_http_response_set_cookie(cp_http_response *response, char *name, 
		char *content, char *host, char *path, long validity, int secure)
{
	char cookie[MAX_COOKIE_LENGTH];
	char expiration[0x50];
	time_t now;
	struct tm gmt;
#ifndef HAVE_GMTIME_R
	struct tm *pgmt;
#endif /* HAVE_GMTIME_R */
	char *domain;

	now = time(NULL);

	now += validity;
#ifdef HAVE_GMTIME_R
	gmtime_r(&now, &gmt);
#else
	pgmt = gmtime(&now);
	gmt = *pgmt;
#endif /* HAVE_GMTIME_R */
	strftime(expiration, 0x50, COOKIE_TIME_FMT, &gmt);

	if (host) 
	{
		if (strncmp(host, "www.", 4) == 0) 
			domain = &host[3];
		else
			domain = NULL; //~~
	}
	else
		domain = NULL;

#ifdef HAVE_SNPRINTF
	snprintf(cookie, MAX_COOKIE_LENGTH, "%s=%s; expires=%s", name, content, expiration);
#else
	sprintf(cookie, "%s=%s; expires=%s", name, content, expiration);
#endif /* HAVE_SNPRINTF */
	if (path)
	{
		strlcat(cookie, "; path=", MAX_COOKIE_LENGTH);
		strlcat(cookie, path, MAX_COOKIE_LENGTH);
	}
	if (domain)
	{
		strlcat(cookie, "; domain=", MAX_COOKIE_LENGTH);
		strlcat(cookie, domain, MAX_COOKIE_LENGTH);
	}
	if (secure) strlcat(cookie, "; secure", MAX_COOKIE_LENGTH);

#ifdef __TRACE__
	DEBUGMSG("setting cookie (%d chars): %s", strlen(cookie), cookie);
#endif
	if (response->cookie == NULL)
	{
		response->cookie = cp_vector_create(1);
		if (response->cookie == NULL) return -1;
	}

	cp_vector_add_element(response->cookie, strdup(cookie)); //~~ vector mode
	
	return 0;
}
#endif

void cp_http_response_skip(cp_http_response *response)
{
	response->skip = 1;
}


static int cp_http_write_server_error(cp_connection_descriptor *cdesc, 
									  int status_code)
{
	char errbuf[HTTP_PARSE_BUFSIZE];
	char *lit;
	int rc;
	
	cp_http_response *res = cp_http_response_create(NULL);

	if (res == NULL) return CP_MEMORY_ALLOCATION_FAILURE;

	cp_http_response_set_status(res, HTTP_400_BAD_REQUEST);

	lit = cp_hashtable_get(cp_http_server_error, &status_code);

	if (lit)
#ifdef HAVE_SNPRINTF
		snprintf(errbuf, HTTP_PARSE_BUFSIZE, server_error_fmt, lit);
#else
		sprintf(errbuf, server_error_fmt, lit);
#endif /* HAVE_SNPRINTF */
	else
		errbuf[0] = '\0';
	
	cp_http_response_set_content_type(res, HTML);

	res->body = errbuf;

	rc = cp_http_response_write(cdesc, res);

	res->body = NULL;
	cp_http_response_delete(res);
		
	return rc;
}


/****************************************************************************
 *                                                                          *
 * cp_httpsocket_* functions                                                *
 *                                                                          *
 ****************************************************************************/

cp_httpsocket *cp_httpsocket_create(int port, cp_http_service_callback service)
{
	cp_socket *sock;
	cp_httpsocket *psock;

	cp_http_init(); /* idempotent */

	sock = cp_socket_create(port, CPSOCKET_STRATEGY_THREADPOOL, cp_http_thread_fn);
	if (sock == NULL) return NULL; //~~ how about an error code?

	psock = (cp_httpsocket *) calloc(1, sizeof(cp_httpsocket));
	if (psock == NULL)
	{
		cp_socket_delete(sock);
		cp_error(CP_MEMORY_ALLOCATION_FAILURE, "can\'t allocate cp_httpsocket");
	}

	psock->id = socket_reg_id++;
	cp_hashlist_append(socket_registry, &psock->id, psock); /* register */
	
	psock->sock = sock;
	cp_socket_set_owner(sock, psock);

	psock->default_service = service;
	cp_httpsocket_set_keepalive(psock, HTTP_KEEPALIVE);

	return psock;
}

#ifdef CP_USE_SSL
cp_httpsocket *cp_httpsocket_create_ssl(int port, 
		                                cp_http_service_callback service, 
										char *certificate_file,
										char *key_file,
										int verification_mode)
{
	cp_socket *sock;
	cp_httpsocket *psock;

	cp_http_init(); /* idempotent */

	sock = cp_socket_create_ssl(port, CPSOCKET_STRATEGY_THREADPOOL, 
								cp_http_thread_fn, certificate_file, 
								key_file, verification_mode);

	if (sock == NULL) return NULL; //~~ how about an error code?

	psock = (cp_httpsocket *) calloc(1, sizeof(cp_httpsocket));
	if (psock == NULL)
	{
		cp_socket_delete(sock);
		cp_error(CP_MEMORY_ALLOCATION_FAILURE, "can\'t allocate cp_httpsocket");
	}
	
	psock->sock = sock;
	cp_socket_set_owner(sock, psock);

	psock->default_service = service;
	cp_httpsocket_set_keepalive(psock, HTTP_KEEPALIVE);

	return psock;
}
#endif
	
void cp_httpsocket_set_keepalive(cp_httpsocket *socket, int sec)
{
	socket->keepalive = sec;
}

void cp_httpsocket_set_server_name(cp_httpsocket *socket, char *name)
{
	socket->server_name = strdup(name);
}

void cp_httpsocket_set_backlog(cp_httpsocket *socket, int backlog)
{
	cp_socket_set_backlog(socket->sock, backlog);
}

void cp_httpsocket_set_delay(cp_httpsocket *socket, struct timeval delay)
{
	cp_socket_set_delay(socket->sock, delay);
}

void cp_httpsocket_set_delay_sec(cp_httpsocket *socket, long sec)
{
	cp_socket_set_delay_sec(socket->sock, sec);
}

void cp_httpsocket_set_delay_usec(cp_httpsocket *socket, long usec)
{
	cp_socket_set_delay_usec(socket->sock, usec);
}

void cp_httpsocket_set_poolsize_min(cp_httpsocket *socket, int min)
{
	cp_socket_set_poolsize_min(socket->sock, min);
}

void cp_httpsocket_set_poolsize_max(cp_httpsocket *socket, int max)
{
	cp_socket_set_poolsize_max(socket->sock, max);
}

void cp_httpsocket_delete(cp_httpsocket *sock)
{
	if (sock)
	{
		if (socket_registry)
			cp_hashlist_remove(socket_registry, &sock->id);

		cp_socket_delete(sock->sock);
#ifdef CP_USE_HTTP_SESSIONS
		if (sock->session)
			cp_hashlist_destroy_deep(sock->session);
		if (sock->pending)
			cp_hashlist_destroy_custom(sock->pending, NULL, 
					(cp_destructor_fn) cp_http_response_delete);
#endif /* CP_USE_HTTP_SESSIONS */
		if (sock->service)
			cp_trie_destroy(sock->service);
		if (sock->server_name) 
			free(sock->server_name);
		free(sock);
	}
}

void *cp_httpsocket_add_shutdown_callback(cp_httpsocket *socket, 
										  void (*cb)(void *),
										  void *prm)
{
	return cp_socket_add_shutdown_callback(socket->sock, cb, prm);
}

int cp_httpsocket_listen(cp_httpsocket *sock)
{
	int rc;

	rc = cp_socket_listen(sock->sock);
	if (rc == 0) rc = cp_socket_select(sock->sock);

	return rc;
}

int cp_httpsocket_register_service(cp_httpsocket *server, 
		                           cp_http_service *service)
{
	int rc = 0;

	if (server->service == NULL)
	{
		server->service = 
			cp_trie_create_trie(COLLECTION_MODE_DEEP, NULL, 
								(cp_destructor_fn) cp_http_service_delete);
		if (server->service == NULL)
			return -1; //~~ error code
	}

	rc = cp_trie_add(server->service, service->path, service);

	return rc;
}

void *cp_httpsocket_unregister_service(cp_httpsocket *server, 
		                               cp_http_service *service)
{
	if (server->service)
	{
		void *rm = NULL;
		cp_trie_remove(server->service, service->path, &rm);
		return rm;
	}

	return NULL;
}


/****************************************************************************
 *                                                                          *
 * cp_http_service_* functions                                              *
 *                                                                          *
 ****************************************************************************/

cp_http_service *cp_http_service_create(char *name, 
                                        char *path, 
                                        cp_http_service_callback callback)
{
	cp_http_service *svc;
	
	svc = (cp_http_service *) calloc(1, sizeof(cp_http_service));
	if (svc == NULL)
	{
		cp_error(CP_MEMORY_ALLOCATION_FAILURE, "can\'t allocate cp_http_service");
		return NULL;
	}

	svc->name = strdup(name);
	svc->path = strdup(path);
	svc->service = callback;
	
	return svc;
}

void cp_http_service_delete(cp_http_service *service)
{
	if (service)
	{
		free(service->name);
		free(service->path);
		free(service);
	}
	else
		cp_warn("cp_http_service_delete: attempting to delete NULL service");
}


/****************************************************************************
 *                                                                          *
 * cp_http implementation helper functions                                  *
 *                                                                          *
 * o cp_http_default_response - creates a 501 NOT IMPLEMENTED response for  *
 *   the case that an cp_httpsocket instance is created with a NULL         *
 *   default_service                                                        *
 * o get_keepalive returns the keep-alive time for a request                *
 *                                                                          *
 ****************************************************************************/

static int cp_http_default_response(cp_http_request *request, cp_http_response *response)
{
	char buf[0x400];

	cp_warn("cp_http: no service defined for uri [%s]", request->uri);

	cp_http_response_set_content_type(response, HTML);
	cp_http_response_set_status(response, HTTP_501_NOT_IMPLEMENTED);

#ifdef HAVE_SNPRINTF
	snprintf(buf, 0x400, "<html><head><title>%s: 501 NOT IMPLEMENTED</title></head><body><h1>Welcome to %s</h1>Your request [%s] could not be served: this server is not configured.<h1></body></html>", response->servername ? response->servername : DEFAULT_SERVER_NAME, response->servername ? response->servername : DEFAULT_SERVER_NAME, request->uri);
#else
	sprintf(buf, "<html><head><title>%s: 501 NOT IMPLEMENTED</title></head><body><h1>Welcome to %s</h1>Your request [%s] could not be served: this server is not configured.<h1></body></html>", response->servername ? response->servername : DEFAULT_SERVER_NAME, response->servername ? response->servername : DEFAULT_SERVER_NAME, request->uri);
#endif /* HAVE_SNPRINTF */

	response->body = strdup(buf);

	return HTTP_CONNECTION_POLICY_CLOSE;
}

void cp_http_response_report_error(cp_http_response *response, 
		                           cp_http_status_code code, 
								   char *message)
{
	char buf[0x400];
	char *err = cp_hashtable_get(cp_http_status_lit, &code);

	cp_http_response_set_content_type(response, HTML);
	cp_http_response_set_status(response, code);
	
#ifdef HAVE_SNPRINTF
	snprintf(buf, 0x400, "<html><head><title>%s: %s</title></head>"
			 "<body><h1>%s</h1>"
			 "%s</body></html>", 
			 response->servername ? response->servername : DEFAULT_SERVER_NAME, 
			 err, err, message);
#else
	sprintf(buf, "<html><head><title>%s: %s</title></head>"
			"<body><h1>%s</h1>"
			"%s</body></html>", 
			response->servername ? response->servername : DEFAULT_SERVER_NAME, 
			err, err, message);
#endif /* HAVE_SNPRINTF */

	cp_http_response_set_body(response, buf);
}

static int get_keepalive(cp_httpsocket *sock, cp_http_request *request)
{
	int keepalive = 0;
	char *ka_lit;
	char *con_lit;
	
	con_lit = cp_hashtable_get(request->header, "Connection");
	if (con_lit && strcmp(con_lit, "close") == 0) return 0;
	
	ka_lit = cp_hashtable_get(request->header, "Keep-Alive");
	if (ka_lit)
		keepalive = atoi(ka_lit);
	
	if (keepalive > sock->keepalive || 
		(keepalive == 0 && request->version == HTTP_1_1)) 
		keepalive = sock->keepalive;

	return keepalive;
}

static int cp_httpselect(cp_httpsocket *hsock, int sec, int fd)
{
	int rc = -1;

	fd_set rd;
	struct timeval tv;

#ifdef __TRACE__
	DEBUGMSG("keepalive: %d", sec);
#endif
				
	FD_ZERO(&rd);
	FD_SET((unsigned int) fd, &rd);

	tv.tv_sec = sec;
	tv.tv_usec = 0;
	rc = select(fd + 1, &rd, NULL, NULL, &tv);
#ifdef __TRACE__		
	DEBUGMSG("read fd: %d", FD_ISSET(fd, &rd));  
#endif

	if (rc > -1) return FD_ISSET(fd, &rd) - 1;
#ifdef __TRACE__
	else DEBUGMSG("select: %s", strerror(errno));
#endif
	
	return rc;
}

static int find_newline(char *buf)
{
	char *p = strstr(buf, "\r\n");
	if (p == NULL) p = strchr(buf, '\n');
	if (p == NULL) return -1;
	return p - buf;
}

static int check_newline(char *buf, int *index)
{
	return (strncmp(buf, "\r\n\r\n", 4) == 0) ||
		   (strncmp(buf, "\n\n", 2) == 0);
}
	
static int 
	incremental_request_parse(cp_httpsocket *sock, cp_http_request **request, 
							  int *stage, cp_string *state, char *curr, 
							  int len, int *used)
{
	int rc;
	char *buf;

	/* if carrying over unprocessed bytes from a previous call, concatenate 
	 * the new data to the old data
	 */
	if (state->len)
	{
		cp_string_cat_bin(state, curr, len);
		len = state->len;
		cp_string_tocstr(state); /* place '\0' */
		buf = state->data;
	}
	else
		buf = curr;
	
	switch (*stage)
	{
		case REQUEST_STAGE_START:
			*used = 0;
			*request = calloc(1, sizeof(cp_http_request));
			if (*request == NULL) return CP_MEMORY_ALLOCATION_FAILURE;
			(*request)->owner = sock;
			*stage = REQUEST_STAGE_STATUS_LINE;

		case REQUEST_STAGE_STATUS_LINE:
			if (find_newline(buf) == -1) /* not done reading status line */
			{
				if (buf != state->data)  /* store content for next attempt */
					cp_string_cstrcpy(state, buf);
				break;
			}
			if ((rc = parse_request_line(*request, buf, used)))
				return rc;
			if (check_newline(buf, used))
			{
				*stage = REQUEST_STAGE_DONE;
				break;
			}
			*stage = REQUEST_STAGE_HEADERS;
			if (*used == len) break;

		case REQUEST_STAGE_HEADERS:
			if ((rc = parse_headers(*request, buf, used, stage)))
				return rc;
			if (*stage == REQUEST_STAGE_DONE && 
				cp_http_request_get_header(*request, "Content-Length"))
				*stage = REQUEST_STAGE_BODY;
			if (*used == len) break;

		case REQUEST_STAGE_BODY:
			if ((rc = parse_cp_http_request_body(*request, buf, used)))
				return rc;
			if (*used == len) *stage = REQUEST_STAGE_DONE;
	}

	return 0;
}

/* handle cgi parameters, sessions and cookies */
static int post_process(cp_httpsocket *socket, cp_http_request *req)
{
	int rc;

	/* parse cgi parameters */
	if (req->query_string)
	{
		if ((rc = parse_cgi_vars(req)))
			return rc;
	}

	/* session handling if enabled */
#ifdef CP_USE_HTTP_SESSIONS
	{
		char *sid;
#ifdef CP_USE_COOKIES
		if (req->cookie) //~~ move this to cookie parsing to avoid looping?
		{
			char *cookie;
			int i;
			int n = cp_vector_size(req->cookie);
			for (i = 0; i < n; i++)
			{
				cookie = cp_vector_element_at(req->cookie, i);
				if (strncmp(cookie, 
					CP_HTTP_SESSION_MARKER, CP_HTTP_SESSION_MARKER_LEN) == 0)
				{
					sid = &cookie[CP_HTTP_SESSION_MARKER_LEN];
					req->session = cp_httpsocket_checkout_session(socket, sid);
					if (req->session == NULL)
						cp_warn("can\'t find session record %s", sid);
					else
					{
						req->session->type = SESSION_TYPE_COOKIE;
						req->session->access = time(NULL);
//						req->session->fresh = 0;
					}
				}
			}
		}
		if (req->session == NULL)
#endif /* CP_USE_COOKIES */
		{
			sid = cp_http_request_get_parameter(req, CP_HTTP_SESSION_PRM);
			if (sid)
			{
				req->session = cp_httpsocket_checkout_session(socket, sid);
				if (req->session == NULL)
					cp_warn("can\'t find session record %s", sid);
				else
				{
					req->session->type = SESSION_TYPE_URLREWRITE;
					req->session->access = time(NULL);
				}
			}
		}
	}
#endif /* CP_USE_HTTP_SESSIONS */

	return 0;
}

static int read_request(cp_list *req_list, cp_connection_descriptor *cdesc)
{
	char tmpbuf[HTTP_PARSE_BUFSIZE + 1];
	cp_string *buf = cp_string_create_empty(HTTP_PARSE_BUFSIZE);
	int rc;
	int len;
	int fd = 0;
	cp_socket *sock = cp_connection_descriptor_get_socket(cdesc);
	cp_http_request *request = NULL;
	int done_reading = 0;
	int stage = REQUEST_STAGE_START;
	int used;

#ifdef CP_USE_SSL	
	if (!sock->use_ssl)
#endif
	fd = cp_connection_descriptor_get_fd(cdesc);

	while (!done_reading)
	{
#ifdef CP_USE_SSL
		if (sock->use_ssl)
		{
			if ((len = SSL_read(cdesc->ssl, tmpbuf, HTTP_PARSE_BUFSIZE)) <= 0)
				break;
		}
		else
#endif
	    if ((len = recv(fd, tmpbuf, HTTP_PARSE_BUFSIZE, 0)) <= 0) break;
		if (len <= 0) break;
		tmpbuf[len] = '\0';
		cdesc->bytes_read += len;
		used = 0;

#ifdef _HTTP_DUMP
		DEBUGMSG("\n---------------------------------------------------------------------------" "\n%s\n" "---------------------------------------------------------------------------", tmpbuf); 
#endif /* _HTTP_DUMP */

		/* parse request by stage - initial, status line, headers, body */
		rc = incremental_request_parse((cp_httpsocket *) sock->owner, &request,
									   &stage, buf, tmpbuf, len, &used);
		if (rc)
		{
			if (request)
				cp_http_request_delete(request);
			return rc;
		}

		if (stage == REQUEST_STAGE_DONE)
		{
			if (request) 
			{
				/* handle cgi parameters, cookies and sessions */
				if ((rc = post_process(sock->owner, request)))
				{
					cp_list_append(req_list, (void *) (long) -rc);
					cp_http_request_delete(request);
					break;
				}
				else
					cp_list_append(req_list, request);
			}
			else
				cp_list_append(req_list, (void *) (long) -rc);

			if (used == len) /* done reading */
				done_reading = 1;
			else 			/* piplining - get next request */
			{
				request = NULL;
				stage = REQUEST_STAGE_START;
				buf->len = 0;
			}
		}
	}

	if (buf) cp_string_destroy(buf);
	return cp_list_item_count(req_list);
}

/****************************************************************************
 *                                                                          *
 * cp_http_thread_fn - a cp_socket based partial http implementation        *
 *                                                                          *
 * the void *prm is a cp_socket cp_connection_descriptor. The               *
 * cp_socket->owner is a cp_httpsocket                                      *
 *                                                                          *
 ****************************************************************************/

void *cp_http_thread_fn(void *prm)
{
	cp_connection_descriptor *cdesc;
	cp_socket *sock;
	struct sockaddr_in *addr;
	int fd;
	int rc = 0;
	int done;
	int action;
	int keepalive = HTTP_CONNECTION_POLICY_DEFAULT;
	cp_http_request *request = NULL; // prevents gcc warning
	cp_httpsocket *hsock;
	cp_http_service_callback svc;
	cp_list *req_list = 
		cp_list_create_list(COLLECTION_MODE_NOSYNC | 
							COLLECTION_MODE_MULTIPLE_VALUES, NULL, NULL, NULL);

	cdesc = (cp_connection_descriptor *) prm;
	sock = cp_connection_descriptor_get_socket(cdesc);
	fd = cp_connection_descriptor_get_fd(cdesc);
	hsock = (cp_httpsocket *) sock->owner;

	addr = cp_connection_descriptor_get_addr(cdesc);

#ifdef __TRACE__
	cp_info("\n\n *** connection from %s:%d\n", 
			inet_ntoa(addr->sin_addr), ntohs(addr->sin_port));
#endif
	
	done = 0;

	while (!done && !sock->closing && !cp_http_stopping)
	{
		svc = hsock->default_service;
		action = HTTP_CONNECTION_POLICY_DEFAULT;

		/* read client request */
		if ((rc = read_request(req_list, cdesc)) == -1) /* io error */
		{
			rc = errno;
			cp_perror(CP_IO_ERROR, rc, "serving request from %s:%d on port %d",
					  inet_ntoa(addr->sin_addr), addr->sin_port,
					  sock->port);
			continue; //~~ check errno to decide whether to continue or break
		}

		/* handle empty requests or EOF */
		if (rc == 0) break; 

		/* parse request and write response */
		while ((request = cp_list_remove_head(req_list)))
		{
			cp_http_response *res;
			long err = (long) request;
			if (err < 0)
			{
				cp_http_write_server_error(cdesc, -err);
				continue;
			}
			
			request->connection = cdesc;
#ifdef CP_USE_HTTP_SESSIONS
			/* catch new session redirect and send pending responses */
			res = NULL;
			if (request->session && request->session->fresh)
			{
#ifdef CP_USE_COOKIES
				if (request->session->type == SESSION_TYPE_COOKIE &&
					cp_http_request_get_parameter(request, CP_HTTP_SESSION_PRM))
					res = cp_http_new_session_redirect(request, 2);
				else
#endif /* CP_USE_COOKIES */
				{
					res = cp_hashlist_get(hsock->pending, request->session->sid);
					if (res)
					{
						res->request = request;
						request->session->fresh = 0;
						cp_hashlist_remove(hsock->pending, request->session->sid);
					}
				}
			}

			if (res == NULL)
#endif /* CP_USE_HTTP_SESSIONS */
			{
				res = cp_http_response_create(request);
				if (res == NULL)
				{
					cp_error(CP_MEMORY_ALLOCATION_FAILURE, "couldn\'t create response wrapper");
					cp_http_request_dump(request);
					cp_http_request_delete(request);
					continue;
				}

				/* determine service to invoke */
				if (hsock->service)
				{
					cp_http_service *desc;
					void *ptr;
					cp_trie_prefix_match(hsock->service, request->uri, &ptr);
					desc = ptr; /* prevents 'dereferencing type-punned pointer 
								   will break strict-aliasing rules' warning */
					if (desc) svc = desc->service;
#ifdef __TRACE__
					if (desc) DEBUGMSG("invoking service [%s]", desc->name);
#endif /* __TRACE__ */
				}
				if (svc == NULL)
					svc = cp_http_default_response;

				action = (*svc)(request, res);

				if (action != res->connection) //~~
					if (res->connection == HTTP_CONNECTION_POLICY_DEFAULT)
						res->connection = (connection_policy) action;
				
#ifdef CP_USE_HTTP_SESSIONS
				/* if service created a new session, start it */
				if (res->request->session)
				{ /* fresh session - rewrite url, set cookie, see what works */
					if (res->request->session->fresh)
					{
						if (hsock->pending == NULL)
							hsock->pending = 
								cp_hashlist_create(1, cp_hash_string, 
								                   cp_hash_compare_string);
						cp_hashlist_append(hsock->pending, 
										   res->request->session->sid, res);
						/* redirect to uri?cpsid=... */
						res = cp_http_new_session_redirect(request, 1);
					}
				}
#endif /* CP_USE_HTTP_SESSIONS */
			}

			if (!res->skip) /* skip flag suppresses response writing */
				cp_http_response_write(cdesc, res);
			cp_http_response_delete(res);

			keepalive = get_keepalive(hsock, request); //~~ check all requests
			if (action == HTTP_CONNECTION_POLICY_DEFAULT)
				action = request->version == HTTP_1_1 ? 
					HTTP_CONNECTION_POLICY_KEEP_ALIVE : 
					HTTP_CONNECTION_POLICY_CLOSE;
		
			cp_http_request_delete(request);
		}
//		else /* parse failed, rc should be set */
//			cp_http_write_server_error(cdesc, rc);

		/* handle connection (keepalive or close as required) */
		switch (action)
		{
			case HTTP_CONNECTION_POLICY_KEEP_ALIVE: // select here
				if ((rc = cp_httpselect(hsock, keepalive, fd)) < 0)
					done = 1;
#ifdef __TRACE__
				DEBUGMSG("rc: %d", rc);
#endif 
				break;

			case HTTP_CONNECTION_POLICY_CLOSE:
				done = 1;
				break;
		}
	}

	cp_list_destroy_custom(req_list, 
			(cp_destructor_fn) cp_http_request_delete);

#ifdef __TRACE__
	if (rc == 0)
		cp_info("done serving connection from %s:%d\n", 
				inet_ntoa(addr->sin_addr), ntohs(addr->sin_port));
#endif

	cp_connection_descriptor_destroy(cdesc);

	return NULL;
}

/** @} */

