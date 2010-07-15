#include "config.h"

#ifdef _WINDOWS
#include <Winsock2.h>
#endif

#ifdef HAVE_STRNDUP
#ifndef _GNU_SOURCE /* required for strndup */
#define _GNU_SOURCE
#endif
#endif /* HAVE_STRNDUP */
#include <string.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>


#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#if defined(unix) || defined(__unix__) || defined(__MACH__)
#include <unistd.h>
#endif

#ifdef HAVE_REGEX_H
#include <regex.h>
#else
#include <pcreposix.h>
#endif /* HAVE_REGEX_H */

/* if poll(2) is available, use it. otherwise, use select (2) */
#ifdef HAVE_POLL
#include <sys/poll.h>
#else
#if defined(sun) || defined(__OpenBSD__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__APPLE__)
#include <sys/types.h>
#include <sys/socket.h>
#endif /* defined */
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif /* HAVE_SYS_SELECT_H */
#endif /* HAVE_POLL */

#include "httpclient.h"
#include "hashtable.h"
#include "hashlist.h"
#include "thread.h"
#include "util.h"

#define RESPONSE_INIT         0
#define RESPONSE_STATUS_LINE  1
#define RESPONSE_HEADERS      2
#define RESPONSE_BODY         3
#define RESPONSE_DONE         4

static char *re_status_line_lit = 
	"^HTTP/([^[:space:]]*)[[:space:]]+([0-9]{3,})[[:space:]]+(.*)$";
static char *re_response_header_lit = 
	"^([[:alnum:][:punct:]]+):[[:space:]]*(.*[^[:space:]])[[:space:]]*$";
static char *re_url_lit = 
	"^(http(s)?:\\/\\/)?([^:\\/]+)(\\:[0-9]{1,5})?(\\/.*)?$";
#ifdef CP_USE_COOKIES
static char *re_cookie_lit = "(;[[:space:]]*(([^=;\n]+)(=([^=;\n]*))?))";
#endif /* CP_USE_COOKIES */

static regex_t re_status_line;
static regex_t re_response_header;
static regex_t re_url;
#ifdef CP_USE_COOKIES
static regex_t re_cookie;

static cp_trie *master_cookie_jar = NULL;
#endif /* CP_USE_COOKIES */

static cp_httpclient_result *
	cp_httpclient_result_new(cp_http_transfer_descriptor *desc,
							 cp_http_result_status status, 
							 cp_http_response *response);

static cp_httpclient_ctl *async_stack = NULL;
static cp_httpclient_ctl *async_bg_stack = NULL;

#define ASYNC_BG_THREAD_MAX 1
static int init_async_bg();
static void stop_async_bg();

volatile short initialized = 0;

int cp_httpclient_init()
{
	int rc;

	if (initialized) return 1;
	initialized = 1;

	cp_client_init();

	if ((rc = regcomp(&re_status_line, 
					re_status_line_lit, REG_EXTENDED | REG_NEWLINE)))
	{
		log_regex_compilation_error(rc, "compiling status line expression");
		goto INIT_ERROR;
	}

	if ((rc = regcomp(&re_response_header, 
					re_response_header_lit, REG_EXTENDED | REG_NEWLINE)))
	{
		log_regex_compilation_error(rc, "compiling response header expression");
		goto INIT_ERROR;
	}

	if ((rc = regcomp(&re_url, re_url_lit, REG_EXTENDED | REG_NEWLINE)))
	{
		log_regex_compilation_error(rc, "compiling url expression");
		goto INIT_ERROR;
	}

#ifdef CP_USE_COOKIES
	if ((rc = regcomp(&re_cookie, re_cookie_lit, REG_EXTENDED | REG_NEWLINE)))
	{
		log_regex_compilation_error(rc, "compiling cookie expression");
		goto INIT_ERROR;
	}

	master_cookie_jar = 
		cp_trie_create_trie(COLLECTION_MODE_DEEP, NULL, 
						    (cp_destructor_fn) cp_trie_destroy);
	if (master_cookie_jar == NULL)
	{
		cp_error(CP_MEMORY_ALLOCATION_FAILURE, "can\'t allocate cookie jar");
		goto INIT_ERROR;
	}
#endif /* CP_USE_COOKIES */
	
	async_stack = cp_httpclient_ctl_create(0);
	if (async_stack == NULL)
	{
		cp_error(CP_MEMORY_ALLOCATION_FAILURE, 
				 "can\'t instantiate asynchronous transfer stack");
		goto INIT_ERROR;
	}

	return 0;

INIT_ERROR:
	initialized = 0;
	return -1;
}

int cp_httpclient_shutdown()
{
	if (initialized == 0) return 1;
	initialized = 0;

	if (async_bg_stack)
		stop_async_bg();

	regfree(&re_status_line);
	regfree(&re_response_header);
	regfree(&re_url);

#ifdef CP_USE_COOKIES
	regfree(&re_cookie);

	cp_trie_destroy(master_cookie_jar);
#endif /* CP_USE_COOKIES */

	cp_httpclient_ctl_destroy(async_stack);

	/* if using ssl, free ssl context objects that may have been allocated */
#ifdef CP_USE_SSL
	cp_socket_shutdown();
#endif /* CP_USE_SSL */
	cp_client_shutdown();

	return 0;
}

#define HTTP_RE_MATCHMAX 10

/* ------------------------------------------------------------------------ *
 *                                cookies                                   *
 * ------------------------------------------------------------------------ */
#ifdef CP_USE_COOKIES

#ifndef HAVE_STRPTIME
//	strptime(expires, "%a, %d-%b-%Y %H:%M:%S GMT", &c->expires);
void parse_cookie_time(char *str, struct tm *res)
{
	char day[8];
	sscanf(str, "%s, %2d-%2d-%4d %2d:%2d:%2d GMT", day, &res->tm_mday, &res->tm_mon, 
		&res->tm_year, &res->tm_hour, &res->tm_min, &res->tm_sec);
}
#endif /* HAVE_STRPTIME */

/** 
 * create a cookie structure. used internally by cp_http_cookie_parse.
 */
cp_http_cookie *cp_http_cookie_create(char *content, 
                                      char *domain, 
                                      char *path, 
                                      char *expires, 
                                      int secure,
                                      int version,
                                      int http_only)
{
	cp_http_cookie *c = calloc(1, sizeof(cp_http_cookie));
	if (c)
	{
		c->content = content;
		c->domain = domain;
		c->path = path;
		if (expires)
#ifdef HAVE_STRPTIME
			strptime(expires, "%a, %d-%b-%Y %H:%M:%S GMT", &c->expires);
#else
			parse_cookie_time(expires, &c->expires);		
#endif
		c->secure = secure;
		c->version = version;
		c->http_only = http_only;
	}

	return c;
}

/**
 * parse an http cookie
 */
cp_http_cookie *cp_http_cookie_parse(char *src)
{
	cp_http_cookie *ck;
	char *content;
	char *ptr = strchr(src, ';');
	
	if (ptr == NULL) 
		ck = cp_http_cookie_create(strdup(src), NULL, NULL, NULL, 0, 0, 0);
	else
	{
		regmatch_t rm[HTTP_RE_MATCHMAX];
		char *fld;
		char *domain = NULL;
		char *expires = NULL;
		char *path = NULL;
		int secure = 0;
		int version = 0;
		int http_only = 0;
		cp_hashtable *unknown = NULL;

		*ptr = '\0';
		content = strdup(src);
		*ptr = ';';

		while (regexec(&re_cookie, ptr, HTTP_RE_MATCHMAX, rm, 0) == 0)
		{
			fld = strndup(&ptr[rm[3].rm_so], rm[3].rm_eo - rm[3].rm_so);

			if (strcasecmp(fld, "domain") == 0 && rm[5].rm_so > -1)
				domain = strndup(&ptr[rm[5].rm_so], rm[5].rm_eo - rm[5].rm_so);
			else if (strcasecmp(fld, "expires") == 0 && rm[5].rm_so > -1)
				expires = strndup(&ptr[rm[5].rm_so], rm[5].rm_eo - rm[5].rm_so);
			else if (strcasecmp(fld, "path") == 0 && rm[5].rm_so > -1)
				path = strndup(&ptr[rm[5].rm_so], rm[5].rm_eo - rm[5].rm_so);
			else if (strcasecmp(fld, "secure") == 0)
				secure = 1;
			else if (strcasecmp(fld, "httpOnly") == 0)
				http_only = 1;
			else if (strcasecmp(fld, "version") == 0 && rm[5].rm_so > -1)
				version = atoi(&ptr[rm[5].rm_so]);
			else 
			{
				char *value;
				if (unknown == NULL) 
					unknown = 
						cp_hashtable_create_by_option(COLLECTION_MODE_DEEP | 
													  COLLECTION_MODE_NOSYNC,
													  1,
													  cp_hash_string,
													  cp_hash_compare_string,
													  NULL, free, NULL, free);
				if (rm[5].rm_so > -1) 
					value = strndup(&ptr[rm[5].rm_so], 
									rm[5].rm_eo - rm[5].rm_so);
				else
					value = strdup("1");

				cp_hashtable_put(unknown, fld, value);
				fld = NULL;
			}
			if (fld) free(fld);
			ptr = &ptr[rm[0].rm_eo];
		}

		ck = cp_http_cookie_create(content, domain, path, expires, 
								   secure, version, http_only);
		if (expires) free(expires);

		ck->fld = unknown;
	}
	
	return ck;
}

/** release cookie memory */
void cp_http_cookie_destroy(cp_http_cookie *c)
{
	if (c)
	{
		if (c->content) free(c->content);
		if (c->domain) free(c->domain);
		if (c->path) free(c->path);
		if (c->fld) cp_hashtable_destroy(c->fld);
		free(c);
	}
}

/** dump cookie fields to log */
void cp_http_cookie_dump(cp_http_cookie *c)
{
	char dbuf[0x100];
	
	cp_info("Cookie\n======\n");
	cp_info("content:   %s\n", c->content);
	cp_info("domain:    %s\n", c->domain ? c->domain : "");
	cp_info("path:      %s\n", c->path ? c->path : "");
	if (c->expires.tm_year)
	{
		strftime(dbuf, 0x100, "%a, %d-%b-%Y %H:%M:%S GMT", &c->expires);
		cp_info("expires:   %s\n", dbuf);
	}
	cp_info("secure:    %s\n", c->secure ? "yes" : "no");
	if (c->version) cp_info("version:   %d\n", c->version);
	if (c->http_only) cp_info("http only: %s\n", c->http_only ? "yes" : "no");
	if (c->fld)
	{
		int i;
		char **key;
		char *value;
		key = (char **) cp_hashtable_get_keys(c->fld);
		for (i = 0; i < cp_hashtable_count(c->fld); i++)
		{
			value = cp_hashtable_get(c->fld, key[i]);
			cp_info("%s:   %s\n", key[i], value);
		}
		free(key);
	}
}

void cp_httpclient_set_cookie_jar(cp_httpclient *client, cp_trie *jar)
{
	client->cookie_jar = jar;
}

#endif /* CP_USE_COOKIES */

cp_httpclient *cp_httpclient_allocate(char *host, int port)
{
	cp_httpclient *client = calloc(1, sizeof(cp_httpclient));
	if (client == NULL) goto ALLOC_ERROR;

	client->header = 
		cp_hashtable_create_by_option(COLLECTION_MODE_COPY | 
									  COLLECTION_MODE_DEEP | 
									  COLLECTION_MODE_NOSYNC,
									  5,
									  cp_hash_istring,
									  cp_hash_compare_istring,
									  (cp_copy_fn) strdup, free,
									  (cp_copy_fn) strdup, free);
	if (client->header == NULL) goto ALLOC_ERROR;

	client->host = host ? strdup(host) : NULL;
	client->port = port;
	client->version = HTTP_1_1;
	client->type = GET;
	client->pipeline_requests = 0;
	client->follow_redirects = 1;
	client->auto_drop_parameters = 1;
	client->auto_drop_headers = 0;
	client->max_redirects = DEFAULT_MAX_REDIRECTS;
#ifdef CP_USE_COOKIES
	client->cookie_jar = master_cookie_jar;
#endif

	return client;

ALLOC_ERROR:
	cp_httpclient_destroy(client);
	return NULL;
}

cp_httpclient *cp_httpclient_create(char *host, int port)
{
	cp_httpclient *client = cp_httpclient_allocate(host, port);
	if (client == NULL) goto CREATE_ERROR;

	client->socket = cp_client_create(host, port);
	if (client->socket == NULL) goto CREATE_ERROR;

	return client;

CREATE_ERROR:
	cp_httpclient_destroy(client);
	return NULL;
}

cp_httpclient *
	cp_httpclient_create_proxy(char *host, int port, 
							   char *proxy_host, int proxy_port)
{
	cp_httpclient *client = cp_httpclient_allocate(host, port);
	if (client == NULL) goto CREATE_ERROR;

	client->proxy_host = proxy_host ? strdup(proxy_host) : NULL;
	if (client->proxy_host == NULL) goto CREATE_ERROR;
	client->proxy_port = proxy_port;

	client->socket = cp_client_create(proxy_host, proxy_port);
	if (client->socket == NULL) goto CREATE_ERROR;

	return client;

CREATE_ERROR:
	cp_httpclient_destroy(client);
	return NULL;
}

#ifdef CP_USE_SSL
cp_httpclient *cp_httpclient_create_ssl(char *host, int port, 
										char *CA_file, char *CA_path, 
										int verification_mode)
{
	cp_httpclient *client = cp_httpclient_allocate(host, port);
	if (client == NULL) goto CREATE_ERROR;

	client->socket = 
		cp_client_create_ssl(host, port, CA_file, CA_path, verification_mode);
	if (client->socket == NULL) goto CREATE_ERROR;

	return client;

CREATE_ERROR:
	cp_httpclient_destroy(client);
	return NULL;
}

cp_httpclient *
	cp_httpclient_create_proxy_ssl(char *host, int port, 
							   	   char *proxy_host, int proxy_port, 
								   char *CA_file, char *CA_path, 
								   int verification_mode)
{
	cp_httpclient *client = cp_httpclient_allocate(host, port);
	if (client == NULL) goto CREATE_ERROR;

	if (client)
	{
		client->proxy_host = proxy_host ? strdup(proxy_host) : NULL;
		client->proxy_port = proxy_port;
	}

	client->socket = 
		cp_client_create_ssl(proxy_host, proxy_port, 
							 CA_file, CA_path, verification_mode);

	client->proxy_ssl = 1;

	return client;

CREATE_ERROR:
	cp_httpclient_destroy(client);
	return NULL;
}
#endif /* CP_USE_SSL */

void cp_httpclient_destroy(cp_httpclient *client)
{
	if (client)
	{
		if (client->connected)
			cp_client_close(client->socket);
		if (client->socket)
			cp_client_destroy(client->socket);
		if (client->header)
			cp_hashtable_destroy(client->header);
		if (client->cgi_parameters)
			cp_hashtable_destroy(client->cgi_parameters);
		if (client->host) free(client->host);
		if (client->proxy_host) free(client->proxy_host);
		free(client);
	}
}

void cp_httpclient_set_http_version(cp_httpclient *client, 
									 cp_http_version version)
{
	client->version = version;
}

void cp_httpclient_set_request_type(cp_httpclient *client, 
									 cp_http_request_type type)
{
	client->type = type;
}

void cp_httpclient_set_header(cp_httpclient *client, char *header, char *value)
{
	if (value != NULL)
		cp_hashtable_put(client->header, header, value);
	else
		cp_hashtable_remove(client->header, header);
}

void cp_httpclient_drop_headers(cp_httpclient *client)
{
	if (client->header) cp_hashtable_remove_all(client->header);
}
		
void cp_httpclient_set_auto_drop_headers(cp_httpclient *client, short mode)
{
	client->auto_drop_headers = mode;
}

void cp_httpclient_set_user_agent(cp_httpclient *client, char *agent)
{
	cp_hashtable_put(client->header, "User-Agent", agent);
}

void cp_httpclient_set_timeout(cp_httpclient *client, 
							   int sec, int usec)
{
	cp_client_set_timeout(client->socket, sec, usec);
}

void cp_httpclient_set_retry(cp_httpclient *client, int retry_count)
{
	cp_client_set_retry(client->socket, retry_count);
}

void cp_httpclient_allow_redirects(cp_httpclient *client, int mode)
{
	client->follow_redirects = mode;
}

void cp_httpclient_set_max_redirects(cp_httpclient *client, int max)
{
	client->max_redirects = max;
}

int cp_httpclient_connect(cp_httpclient *client)
{
	return cp_client_connect(client->socket);
}

#define LINEBUFSIZE 0x1000

static cp_string *cgi_param_string(cp_httpclient *client)
{
	cp_string *str = NULL;

	if (client->cgi_parameters)
	{
		int len = cp_hashtable_count(client->cgi_parameters);
		if (len)
		{
			int i;
			char *value;
			char **keys = (char **) cp_hashtable_get_keys(client->cgi_parameters);
			str = cp_string_create("", 0);
			for (i = 0; i < len; i++)
			{
				if (i > 0) cp_string_append_char(str, '&');
				value = cp_hashtable_get(client->cgi_parameters, keys[i]);
				cp_string_cstrcat(str, keys[i]);
				cp_string_append_char(str, '=');
				cp_string_cstrcat(str, value);
			}
			free(keys);
		}
	}
	
	return str;
}

void *cp_httpclient_set_parameter(cp_httpclient *client, 
								  char *name, char *value)
{
	if (value == NULL)
	{
		if (client->cgi_parameters)
			return cp_hashtable_remove(client->cgi_parameters, name);
		return NULL;
	}

	if (client->cgi_parameters == NULL)
	{
		client->cgi_parameters = 
			cp_hashtable_create_by_option(COLLECTION_MODE_DEEP | 
										  COLLECTION_MODE_COPY | 
										  COLLECTION_MODE_NOSYNC, 
										  1, 
										  cp_hash_string, 
										  cp_hash_compare_string, 
										  (cp_copy_fn) strdup, free,
										  (cp_copy_fn) strdup, free);
		if (client->cgi_parameters == NULL) return NULL;
	}

	return cp_hashtable_put(client->cgi_parameters, name, value);
}
		
void cp_httpclient_drop_parameters(cp_httpclient *client)
{
	if (client->cgi_parameters) cp_hashtable_remove_all(client->cgi_parameters);
}
		
void cp_httpclient_set_auto_drop_parameters(cp_httpclient *client, short mode)
{
	client->auto_drop_parameters = mode;
}

static void append_header(cp_string *buf, char *key, char *value)
{
	cp_string_cstrcat(buf, key);
	cp_string_cstrcat(buf, ": ");
	cp_string_cstrcat(buf, value);
	cp_string_cstrcat(buf, "\r\n");
}

static void append_host_header(cp_httpclient *client, cp_string *request)
{
	if (client->socket->host) 
	{
		int need_port = 0;
#if CP_USE_SSL
		if (client->socket->use_ssl && client->port != 443)
			need_port = 1;
		else
#endif /* CP_USE_SSL */
		if (client->port != 80)
			need_port = 1;

		if (need_port)
		{
			char *host;
			size_t len = strlen(client->host) + 7;
			host = (char *) malloc(len);
			if (host == NULL) return;
#ifdef HAVE_SNPRINTF
			snprintf(host, len, "%s:%d", client->host, client->port);
#else
			sprintf(host, "%s:%d", client->host, client->port);
#endif /* HAVE_SNPRINTF */
			append_header(request, "Host", host);
			free(host);
		}
		else
			append_header(request, "Host", client->host);
	}
}

cp_string *cp_httpclient_format_request(cp_httpclient *client, char *uri)
{
	char buf[LINEBUFSIZE];
	cp_string *request;
	int len;

	/* print request line */
	if (client->proxy_host == NULL)
#ifdef HAVE_SNPRINTF
		snprintf(buf, LINEBUFSIZE, "%s %s", 
			get_http_request_type_lit(client->type), uri);
#else
		sprintf(buf, "%s %s", get_http_request_type_lit(client->type), uri);
#endif /* HAVE_SNPRINTF */
	else
	{
		char *method = "http";
		char port[8];
#ifdef CP_USE_SSL
		if (client->socket->use_ssl) 
		{
			method = "https";
			if (client->port == 443) 
				port[0] = '\0';
			else
#ifdef HAVE_SNPRINTF
				snprintf(port, 8, ":%d", client->proxy_port);
#else
				sprintf(port, ":%d", client->proxy_port);
#endif /* HAVE_SNPRINTF */
		}
		else
#endif /* CP_USE_SSL */
		{
			if (client->port == 80) 
				port[0] = '\0';
			else
#ifdef HAVE_SNPRINTF
				snprintf(port, 8, ":%d", client->proxy_port);
#else
				sprintf(port, ":%d", client->proxy_port);
#endif /* HAVE_SNPRINTF */
		}

#ifdef HAVE_SNPRINTF
		snprintf(buf, LINEBUFSIZE, 
				 "%s %s://%s%s%s", get_http_request_type_lit(client->type), 
				 method, client->host, port, uri);
#else
		sprintf(buf, "%s %s://%s%s%s", get_http_request_type_lit(client->type), 
				method, client->host, port, uri);
#endif /* HAVE_SNPRINTF */
	}
			
	request = cp_string_create(buf, strlen(buf));

	if (client->type == GET)
	{
		cp_string *prm = cgi_param_string(client);
		if (prm) /* may be null if no parameters */
		{
			cp_string_append_char(request, '?');
			cp_string_cat(request, prm);
			cp_string_destroy(prm);
		}
	}

	cp_string_cstrcat(request, " HTTP/");
	cp_string_cstrcat(request, client->version == HTTP_1_1 ? "1.1" : "1.0");
	cp_string_cstrcat(request, "\r\n");
	
	/* print headers */
	append_host_header(client, request);
	if (cp_hashtable_get(client->header, "User-Agent") == NULL)
		append_header(request, "User-Agent", DEFAULT_SERVER_NAME);

	if ((len = cp_hashtable_count(client->header)))
	{
		int i;
		char **key = (char **) cp_hashtable_get_keys(client->header);
		char *value;

		for (i = 0; i < len; i++)
		{
			value = cp_hashtable_get(client->header, key[i]);
			if (value) append_header(request, key[i], value);
		}
	}
	
	/* set Connection header if necessary */
	if (cp_hashtable_get(client->header, "Connection") == NULL)
	{
		if (client->type == HTTP_1_1)
		{
			append_header(request, "Connection", "keep-alive");
			append_header(request, "Keep-Alive", "300"); //~~
		}
		else
			append_header(request, "Connection", "close");
	}
	
	/* check cookie jar for applicable cookies */
#ifdef CP_USE_COOKIES
	{
		cp_string *cookie_lit = NULL;
		char *domain = reverse_string(client->socket->host);
		cp_vector *urilist = cp_trie_fetch_matches(client->cookie_jar, domain);
		if (urilist)
		{
			int i, j;
			int len, jlen;
			cp_trie *container;
			cp_vector *cooklist;
			cp_http_cookie *ck;

			len = cp_vector_size(urilist);
			cookie_lit = cp_string_create_empty(80);
			for (i = 0; i < len; i++)
			{
				container = cp_vector_element_at(urilist, i);
				cooklist = cp_trie_fetch_matches(client->cookie_jar, uri);
				if (cooklist == NULL) continue;
				jlen = cp_vector_size(cooklist);
				for (j = 0; j < jlen; j++)
				{
					ck = cp_vector_element_at(cooklist, j);
					if (cookie_lit->len > 0)
						cp_string_cstrcat(cookie_lit, "; ");
					cp_string_cstrcat(cookie_lit, ck->content);
				}
				if (cookie_lit->len > 0)
					append_header(request, "Cookie", cookie_lit->data);
			}
			cp_string_destroy(cookie_lit);
			cp_vector_destroy(urilist);
		}
		free(domain);
	}
#endif /* CP_USE_COOKIES */
	
	/* for POST requests, print request body */
	if (client->type == POST)
	{
		cp_string *prm = cgi_param_string(client);
		if (prm)
		{
			char len[16];
#ifdef HAVE_SNPRINTF
			snprintf(len, 16, "%d", prm->len);
#else
			sprintf(len, "%d", prm->len);
#endif /* HAVE_SNPRINTF */
			append_header(request, 
					"Content-Type", "application/x-www-form-urlencoded");
			append_header(request, "Content-Length", len);
			cp_string_cstrcat(request, "\r\n\r\n");
			cp_string_cat(request, prm);
			cp_string_destroy(prm);
		}
	}

	if (client->content)
	{
		char len[16];
#ifdef HAVE_SNPRINTF
		snprintf(len, 16, "%d", client->content->len);
#else
		sprintf(len, "%d", client->content->len);
#endif /* HAVE_SNPRINTF */
		append_header(request, "Content-Length", len);
		cp_string_cstrcat(request, "\r\n\r\n");
		cp_string_cat(request, client->content);
	}
	
	cp_string_cstrcat(request, "\r\n");
	return request;
}

#ifdef CP_USE_SSL
//~~ the right way to do this would be to add modes for 'PROXY_CONNECT_WRITE' 
//~~ and 'PROXY_CONNECT_READ' to prevent blocking. coming in the next version.
static int do_proxy_ssl_connect(cp_httpclient *client)
{
	char cbuf[0x400];
	char *pbuf;
	int total;
	int len;
	
	int rc;
	cp_client *sock = client->socket;
	sock->use_ssl = 0;
	
	rc = cp_client_connect(sock);
	sock->use_ssl = 1;
	if (rc) return rc;

	if (client->port != 443)
#ifdef HAVE_SNPRINTF
		snprintf(cbuf, 0x400, "CONNECT %s:%d HTTP/1.0\r\n\r\n", 
				client->host, client->port);
#else
		sprintf(cbuf, "CONNECT %s:%d HTTP/1.0\r\n\r\n", 
				client->host, client->port);
#endif /* HAVE_SNPRINTF */
	else
#ifdef HAVE_SNPRINTF
		snprintf(cbuf, 0x400,  "CONNECT %s HTTP/1.0\r\n\r\n", client->host);
#else
		sprintf(cbuf, "CONNECT %s HTTP/1.0\r\n\r\n", client->host);
#endif /* HAVE_SNPRINTF */

	len = strlen(cbuf);

	for (total = 0; total < len; total+= rc)
	{
		rc = send(sock->fd, cbuf, len - total, 0);
		if (rc <= 0) break;
	}
	if (rc == 0) return -1;
	if (rc < 0) return rc;

	pbuf = cbuf;
	cbuf[0] = '\0';
	len = 0;
	do
	{
		rc = recv(sock->fd, pbuf, 0x400 - len, 0);
		if (rc == -1 && (errno == EAGAIN || errno == EINTR)) continue;
		if (rc <= 0) break;
		pbuf[rc] = '\0';
		sock->bytes_read += rc;
#ifdef _HTTP_DUMP
		DEBUGMSG("\n---------------------------------------------------------------------------" "\n%s\n" "---------------------------------------------------------------------------", pbuf); 
#endif /* _HTTP_DUMP */
		pbuf = &pbuf[rc];
	} while (strstr(cbuf, "\r\n\r\n") == NULL);

	if (rc < 0) return rc;
	if (strstr(cbuf, "HTTP/1.0 200 Connection established") == NULL)
		return -1;

	return cp_client_connect_ssl(sock);
}
#endif /* CP_USE_SSL */

static int do_connect(cp_httpclient *client)
{
	int rc;
	
#ifdef CP_USE_SSL
	if (client->proxy_ssl)
		rc = do_proxy_ssl_connect(client);
	else
#endif /* CP_USE_SSL */
	rc = cp_client_connect(client->socket);
	if (rc)
	{
#ifdef CP_USE_SSL
		if (rc > 0)
		{
			cp_error(CP_SSL_VERIFICATION_ERROR, "%s:%d : %s", 
					 client->socket->host, client->socket->port, 
					 ssl_verification_error_str(rc));
		}
		else
#endif
		{
			cp_error(CP_IO_ERROR, "can\'t connect to %s on port %d", 
					 client->socket->host, client->socket->port);
			return -1;
		}
	}
	client->connected = 1;
	return 0;
}

#ifdef __OpenBSD__
#define HTTP_PARSE_BUFSIZE 0x1000
#else
#define HTTP_PARSE_BUFSIZE 0xFFFF
#endif

cp_http_response *cp_http_client_response_create()
{
	cp_http_response *res = NULL;
	
	if ((res = (cp_http_response *) calloc(1, sizeof(cp_http_response))))
	{
        res->header = 
            cp_hashtable_create_by_option(COLLECTION_MODE_NOSYNC |
										  COLLECTION_MODE_COPY |
                                          COLLECTION_MODE_DEEP,
                                          10, 
                                          cp_hash_istring, 
                                          cp_hash_compare_istring,
                                          (cp_copy_fn) strdup, free, 
                                          (cp_copy_fn) strdup, free);
		if (res->header == NULL)
		{
			free(res);
			return NULL;
		}
	}

	return res;
}

cp_url_descriptor *
	cp_url_descriptor_create(char *host, short ssl, int port, char *uri)
{
	cp_url_descriptor *desc = calloc(1, sizeof(cp_url_descriptor));
	if (desc == NULL) return NULL;

	desc->host = host;
	desc->uri = uri;

	desc->ssl = ssl;
	desc->port = port;

	return desc;
}

void cp_url_descriptor_destroy(cp_url_descriptor *desc)
{
	if (desc)
	{
		if (desc->host) free(desc->host);
		if (desc->uri) free(desc->uri);
		free(desc);
	}
}

short cp_url_descriptor_ssl(cp_url_descriptor *desc)
{
	return desc->ssl;
}

char *cp_url_descriptor_host(cp_url_descriptor *desc)
{
	return desc->host;
}

int cp_url_descriptor_port(cp_url_descriptor *desc)
{
	return desc->port;
}

char *cp_url_descriptor_uri(cp_url_descriptor *desc)
{
	return desc->uri;
}

cp_url_descriptor *cp_url_descriptor_parse(char *url)
{
	cp_string *host;
	char *chost;
	short ssl;
	int port;
	cp_string *uri;
	char *curi;
	regmatch_t rm[6];
	
	if (url[0] == '/') /* it's not a url, it's a uri */
		return cp_url_descriptor_create(NULL, 0, 0, strdup(url));
	
	if (regexec(&re_url, url, 6, rm, 0))
	{
		cp_error(CP_HTTP_INVALID_URL, "can\'t parse url: %s\n", url);
		return NULL;
	}
	
	ssl = rm[2].rm_so != -1;
	host = cp_string_create(&url[rm[3].rm_so], rm[3].rm_eo - rm[3].rm_so);
	if (rm[4].rm_so != -1)
		port = atoi(&url[rm[4].rm_so + 1]);
	else
		port = ssl ? 443 : 80;
	if (rm[5].rm_so != -1)
		uri = cp_string_create(&url[rm[5].rm_so], rm[5].rm_eo - rm[5].rm_so);
	else
		uri = cp_string_create("/", 1);

	chost = cp_string_tocstr(host);
	free(host);
	curi = cp_string_tocstr(uri);
	free(uri);

	return cp_url_descriptor_create(chost, ssl, port, curi);
}

int read_status_line(cp_http_response *res, char **ptr, int len)
{
	regmatch_t rm[HTTP_RE_MATCHMAX];

	if (regexec(&re_status_line, *ptr, HTTP_RE_MATCHMAX, rm, 0))
	{
		cp_error(CP_HTTP_INVALID_STATUS_LINE, "can\'t parse server response");
		return CP_HTTP_INVALID_STATUS_LINE;
	}

	if (strncmp(&(*ptr)[rm[1].rm_so], "1.1", 3) == 0)
		res->version = HTTP_1_1;
	else
		res->version = HTTP_1_0;

	res->status = (cp_http_status_code) atoi(&(*ptr)[rm[2].rm_so]);
	if (rm[3].rm_so > -1)
	{
		cp_string *status_lit = 
			cp_string_create(&(*ptr)[rm[3].rm_so], rm[3].rm_eo - rm[3].rm_so);
		res->status_lit = status_lit->data;
		free(status_lit);
	}

	*ptr = &(*ptr)[rm[0].rm_eo];
	return 0;
}

static char *find_headers_end(char *buf)
{
	char *end = strstr(buf, "\r\n\r\n");
	if (end == NULL)
		end = strstr(buf, "\n\n");
	if (end == NULL)
		end = strstr(buf, "\n\r\n\r");

	return end;
}

int read_headers(cp_httpclient *client, cp_http_response *res, 
				 char *uri, char **ptr, int srclen)
{
	char *curr = *ptr;
	int len;
	regmatch_t rm[HTTP_RE_MATCHMAX];
	char *name, *value;
	char *headers_end;
	char ch;

	while (*curr == '\r' || *curr == '\n') curr++; //~~ place curr properly 

	headers_end = find_headers_end(curr);
	if (headers_end == NULL) return EAGAIN;
	ch = *headers_end;
	*headers_end = '\0';

	while (regexec(&re_response_header, curr, HTTP_RE_MATCHMAX, rm, 0) == 0)
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
		if (strcasecmp(name, "Set-Cookie") == 0)
		{
			cp_http_cookie *ck = cp_http_cookie_parse(value);
			cp_trie *uri_trie;
			if (ck)
			{
				char *domain = NULL;
				if (ck->domain)
					domain = reverse_string(ck->domain);
				else
					domain = reverse_string(client->socket->host);
				uri_trie = 
					cp_trie_exact_match(client->cookie_jar, domain);
				if (uri_trie == NULL)
				{
					uri_trie = 
						cp_trie_create_trie(COLLECTION_MODE_DEEP, NULL,
								(cp_destructor_fn) cp_http_cookie_destroy);
					cp_trie_add(client->cookie_jar, domain, uri_trie);
				}
				free(domain);
				if (ck->path)
					cp_trie_add(uri_trie, ck->path, ck);
				else
					cp_trie_add(uri_trie, uri, ck);
			}
			free(name);
			free(value);
		}
		else
#endif /* CP_USE_COOKIES */
		cp_hashtable_put_by_option(res->header, name, value, 
			(cp_hashtable_get_mode(res->header) | COLLECTION_MODE_COPY) 
				^ COLLECTION_MODE_COPY);
		curr = &curr[rm[0].rm_eo];
	}
	*ptr = curr;

	*headers_end = ch;
	return 0;
}

static char *find_response_end(char *curr)
{
	char *src = strstr(curr, "\r\n\r\n");
	if (src) 
		src += 4;
	else 
	{
		src = strstr(curr, "\n\n");
		if (src) src += 2;
	}

	return src;
}

// #if 0
static int read_chunk(cp_httpclient *client, char *buf, int len)
{
	int rc;
	while (1)
	{
#ifdef CP_USE_SSL
		if (client->socket->use_ssl)
			rc = SSL_read(client->socket->ssl, buf, len);
		else
#endif /* CP_USE_SSL */
		rc = recv(client->socket->fd, buf, len, 0);
		if (rc == -1 && (errno == EAGAIN || errno == EINTR)) continue;
		if (rc >= 0) 
		{
			buf[rc] = '\0';
			client->socket->bytes_read += rc;
#ifdef _HTTP_DUMP
			cp_info("\n---------------------------------------------------------------------------" "\n%s\n" "---------------------------------------------------------------------------", buf); 
#endif /* _HTTP_DUMP */
		}
		break;
	}

	return rc;
}
// #endif

#define C2HEX(p) ((p) >= 'a' && (p) <= 'f' ? (p) - 'a' : \
					(p) >= 'A' && (p) <= 'F' ? (p) - 'A' : (p) - '0')

/* xtoid - xtoi with diagnostic: convert hex string to integer or return -1 if 
 * no conversion was done
 */
int xtoid(char *p)
{
	int curr;
	int res = -1;

	while (*p)
	{
		curr = C2HEX(*p);
		if (curr == -1) break;
		res = res * 0x10 + curr;
		p++;
	}
	
	return res;
}

static int read_chunked(cp_httpclient *client, 
						cp_http_response *res, 
						char *tmpbuf, 
						int buflen,
						char *curr)
{
	int chunk;
	int inc, cat;
	int len = -1;
	res->content = NULL;

	while (1)
	{
		while (*curr == '\r' || *curr == '\n') curr++;
		if (*curr == 0)
		{
			curr = tmpbuf;
			buflen = read_chunk(client, tmpbuf, HTTP_PARSE_BUFSIZE);
//			buflen = cp_client_read(client->socket, tmpbuf, HTTP_PARSE_BUFSIZE);
			if (buflen <= 0) break;
		}

		len = xtoi(curr);
		if (len == 0) break;

		curr = strchr(curr, '\n');
		if (curr == NULL) return -1; /* bad chunk format */
		curr++;
		inc = buflen - (curr - tmpbuf); 
		cat = (len < inc) ? len : inc;		
		if (res->content)
			cp_string_cat_bin(res->content, curr, cat);
		else
			res->content = cp_string_create(curr, cat);
		if (len < inc) curr = &curr[cat];
		len -= cat;

		for (chunk = 0; chunk < len; )
		{
			curr = tmpbuf;
			buflen = read_chunk(client, tmpbuf, HTTP_PARSE_BUFSIZE);
//			buflen = cp_client_read(client->socket, tmpbuf, HTTP_PARSE_BUFSIZE);
			chunk += buflen;
			if (chunk <= len)
				cp_string_cat_bin(res->content, tmpbuf, buflen);
			else
			{
				cp_string_cat_bin(res->content, tmpbuf, buflen - (chunk - len));
				curr = &tmpbuf[buflen - (chunk - len)];
			}
		}
	}

	return len;
}

static cp_http_response *read_response(cp_httpclient *client, char *uri)
{
	char tmpbuf[HTTP_PARSE_BUFSIZE + 1];
	cp_string *buf = cp_string_create_empty(HTTP_PARSE_BUFSIZE);
	char *curr;
	cp_http_response *res = cp_http_client_response_create();
	int stage = 0;
	int rc;
	int skip = 0;
	
	curr = buf->data;

	while (stage < RESPONSE_DONE)
	{
		rc = read_chunk(client, tmpbuf, HTTP_PARSE_BUFSIZE);
//		rc = cp_client_read(client->socket, tmpbuf, HTTP_PARSE_BUFSIZE);
		if (rc <= 0) break;

		cp_string_cat_bin(buf, tmpbuf, rc);
		buf->data[buf->len] = '\0';
		
		if (stage == RESPONSE_INIT)
		{
			if (read_status_line(res, 
					&curr, buf->len - (curr - buf->data))) break;
			stage = RESPONSE_HEADERS;
		}

		if (stage == RESPONSE_HEADERS)
		{
			int rv;
			char *prm;

			rv = read_headers(client, res, uri, &curr, 
							 buf->len + skip - (curr - buf->data));
			if (rv == -1) 
				break;
			else if (rv == EAGAIN) 
			{
				skip += rc;
				continue;
			}

			if ((prm = cp_hashtable_get(res->header, "Transfer-Encoding"))
					&& strcasecmp(prm, "chunked") == 0)
			{
				int len;
				curr = &tmpbuf[curr - buf->data];
				len = read_chunked(client, res, tmpbuf, rc, curr);
				if (len == -1) break;
				if (len == 0) stage = RESPONSE_DONE;
				break;
			}
			else if ((prm = cp_hashtable_get(res->header, "Content-Length")))
			{
				int len;
				char *src = find_response_end(curr);
				if (src == NULL) break;
				curr = src;
				res->len = atoi(prm);
				stage = RESPONSE_BODY;
				res->content = 
					cp_string_create(curr, rc + skip - (curr - buf->data));
				len = res->len - res->content->len;

				if (len > 0)
					cp_client_read_string(client->socket, res->content, len);
				stage = RESPONSE_DONE;
				break;
			}
			/* some servers neglect to specify content length */
			else if (cp_hashtable_get(res->header, "Content-Type"))
			{
				char *src = find_response_end(curr);
				if (src == NULL) break; 
				curr = src;
				stage = RESPONSE_BODY;
				res->content = 
					cp_string_create(curr, rc + skip - (curr - buf->data));
				continue;
			}
			else 
				stage = RESPONSE_DONE;
		}

		if (stage == RESPONSE_BODY)
			cp_string_cat_bin(res->content, tmpbuf, rc);
	}

	cp_string_destroy(buf);
	
	if ((stage == RESPONSE_DONE) || (stage == RESPONSE_BODY && res->len == 0))
		return res;

	cp_error(CP_IO_ERROR, "error %s", 
			stage == RESPONSE_INIT ? "initializing response" : 
			stage == RESPONSE_HEADERS ? "reading headers" : "reading content");

	cp_http_response_delete(res);
	return NULL;
}

cp_httpclient_result *cp_httpclient_fetch(cp_httpclient *client, char *uri)
{
	cp_httpclient_result *res;
	cp_string *request = cp_httpclient_format_request(client, uri);
	cp_http_response *response;
	int redirects = 0;

	res = cp_httpclient_result_create(client);
	if (res == NULL) return NULL;
	
	if (!client->connected) 
		if (do_connect(client))
		{
			res->result = CP_HTTP_RESULT_CONNECTION_FAILED;
			return res;
		}
	
	if (cp_client_write_string(client->socket, request) == 0)
	{
		if (do_connect(client)) 
		{
			res->result = CP_HTTP_RESULT_CONNECTION_FAILED;
			return res;
		}
		if (cp_client_write_string(client->socket, request) == 0)
		{
			cp_string_destroy(request);
			res->result = CP_HTTP_RESULT_WRITE_ERROR;
			return res;
		}
	}
	
	response = read_response(client, uri);
	if (response == NULL)
	{
		res->result = CP_HTTP_RESULT_READ_ERROR;
		return res;
	}

	while (client->follow_redirects && (response->status == HTTP_302_FOUND ||
			response->status == HTTP_301_MOVED_PERMANENTLY ||
			response->status == HTTP_300_MULTIPLE_CHOICES) && 
			redirects < client->max_redirects)
	{
		int rc;
		int retry = 3;
		cp_http_response *redirected;
		cp_url_descriptor *udesc;
		char *url = cp_hashtable_get(response->header, "Location");

		if (url == NULL) break;
		udesc = cp_url_descriptor_parse(url);
		if (udesc == NULL) break;
		cp_string_destroy(request);

		if (udesc->host == NULL) /* client returned a uri */
		{
			udesc->host = strdup(client->host);
			udesc->port = client->port;
#ifdef CP_USE_SSL
			udesc->ssl = client->socket->use_ssl;
#endif /* CP_USE_SSL */
		}
		else if (strcmp(udesc->host, client->host) || (udesc->port != client->port)) 
		{
			free(client->host);
			client->host = strdup(udesc->host);
			if (!client->proxy_host)/* if using proxy connection doesn't change */
				cp_client_reopen(client->socket, udesc->host, udesc->port);
			//~~ what if redirecting to https address?
		}
		
		request = cp_httpclient_format_request(client, udesc->uri);
		while (cp_client_write_string(client->socket, request) == 0)
		{
			rc = errno;
			if (rc == EINTR || rc == EAGAIN) continue;
			if (rc == EPIPE && retry-- > 0) 
			{
				cp_client_reopen(client->socket, udesc->host, udesc->port);
				continue;
			}
			cp_string_destroy(request);
			res->result = CP_HTTP_RESULT_WRITE_ERROR;
			return res;
		}

		redirected = read_response(client, udesc->uri);
		if (redirected)
		{
			cp_http_response_delete(response);
			response = redirected;
			redirects++;
		}
		cp_url_descriptor_destroy(udesc);
	}

	cp_string_destroy(request);

	if (client->header && client->auto_drop_headers)
		cp_httpclient_drop_headers(client);

	if (client->cgi_parameters && client->auto_drop_parameters)
		cp_httpclient_drop_parameters(client);

	if (response == NULL)
	{
		res->result = CP_HTTP_RESULT_READ_ERROR;
		return res;
	}

	res->response = response;
	return res;
}

int cp_httpclient_reopen(cp_httpclient *client, char *host, int port)
{
	client->connected = 0;
	return cp_client_reopen(client->socket, host, port);
}

/* ------------------------------------------------------------------------ *
 *                                async stack                               *
 * ------------------------------------------------------------------------ */

#define TRANSFER_STAGE_CONNECT      1
#define TRANSFER_STAGE_WRITE        2
#define TRANSFER_STAGE_READ         3
#define TRANSFER_STAGE_DONE         4

#define TRANSFER_TYPE_LENGTH        1
#define TRANSFER_TYPE_CHUNKED       2
#define TRANSFER_TYPE_READ_TO_EOF   3

static int set_async_stack_size(cp_httpclient_ctl *ctl, int size)
{
	void *p;
	if (size > ctl->size)
	{
		ctl->size = size * 2;
#ifdef HAVE_POLL
		p = realloc(ctl->ufds, ctl->size * sizeof(struct pollfd));
		if (p == NULL) return -1;
		ctl->ufds = p;
// #else
//		p = realloc(ctl->ufds, ctl->size * sizeof(int));
#endif
		p = realloc(ctl->desc, 
					ctl->size * sizeof(cp_http_transfer_descriptor *));
		if (p == NULL) return -1;
		ctl->desc = p;
	}
	ctl->nfds = size;
	return 0;
}

cp_httpclient_result *cp_httpclient_result_create(cp_httpclient *client)
{
	cp_httpclient_result *res = calloc(1, sizeof(cp_httpclient_result));
	if (res == NULL) return NULL;
	res->client = client;
	return res;
}

static cp_httpclient_result *
	cp_httpclient_result_new(cp_http_transfer_descriptor *desc,
							 cp_http_result_status status, 
							 cp_http_response *response)
{
	cp_httpclient_result *res = calloc(1, sizeof(cp_httpclient_result));
	if (res == NULL) goto RESULT_ERROR;
	res->id = desc->id;
	res->client = desc->owner;
	res->result = status;
	res->response = response;

	return res;

RESULT_ERROR:
	cp_httpclient_result_destroy(res);
	return NULL;
}

void cp_httpclient_result_destroy(cp_httpclient_result *res)
{
	if (res) 
	{
		if (res->response) cp_http_response_delete(res->response);
		free(res);
	}
}

void *cp_httpclient_result_id(cp_httpclient_result *res)
{
	return res->id;
}

cp_http_result_status cp_httpclient_result_status(cp_httpclient_result *res)
{
	return res->result;
}

cp_http_response *cp_httpclient_result_get_response(cp_httpclient_result *res)
{
	return res->response;
}

cp_httpclient *cp_httpclient_result_get_client(cp_httpclient_result *res)
{
	return res->client;
}

static void 
	cp_http_transfer_descriptor_callback(cp_http_transfer_descriptor *desc)
{
	cp_httpclient_result *res;
	if (desc->callback)
	{
		res = cp_httpclient_result_new(desc,
			(cp_http_result_status) desc->status, desc->res);
		if (res) //~~ handle error
		{
			desc->callback(res);
			cp_httpclient_result_destroy(res);
		}
	}
}

static void prepare_async_stack_fds(cp_httpclient_ctl *ctl)
{
	cp_hashlist_iterator *i;
	int j;
	int rc;
	cp_http_transfer_descriptor *desc;

	i = cp_hashlist_create_iterator(ctl->stack, COLLECTION_LOCK_READ);
	j = 0;
#ifndef HAVE_POLL
	FD_ZERO(&ctl->ufds_r);
	FD_ZERO(&ctl->ufds_w);
	ctl->fdmax = 0;
#endif
	while ((desc = cp_hashlist_iterator_next_value(i)) != NULL)
	{
		if (desc->stage == TRANSFER_STAGE_CONNECT)
		{
			if (!desc->owner->connected)
			{
				rc = do_connect(desc->owner);
				if (rc)
				{
					cp_hashlist_iterator_remove(i);
					desc->stage = TRANSFER_STAGE_DONE;
					desc->status = CP_HTTP_RESULT_CONNECTION_FAILED;
					continue;
				}
			}
			desc->stage = TRANSFER_STAGE_WRITE;
		}

		ctl->desc[j] = desc;
#ifdef HAVE_POLL
		ctl->ufds[j].fd = desc->owner->socket->fd;
		ctl->ufds[j].events = 0;
		if (desc->stage == TRANSFER_STAGE_WRITE)
			ctl->ufds[j].events |= POLLOUT;
		else
			ctl->ufds[j].events |= POLLIN;
#else
		if (desc->stage == TRANSFER_STAGE_WRITE)
			FD_SET(desc->owner->socket->fd, &ctl->ufds_w);
		else
			FD_SET(desc->owner->socket->fd, &ctl->ufds_r);
		if (desc->owner->socket->fd > ctl->fdmax)
			ctl->fdmax = desc->owner->socket->fd;
#endif /* HAVE_POLL */
		j++;
	}
	ctl->nfds = j;
	cp_hashlist_iterator_destroy(i);
}

static void 
	cp_http_transfer_descriptor_do_write(cp_http_transfer_descriptor *desc)
{
	int rc;
	char *index = &desc->buf->data[desc->buf->len - desc->remaining];
#ifdef CP_USE_SSL
	if (desc->owner->socket->use_ssl)
		rc = SSL_write(desc->owner->socket->ssl, index, desc->remaining);
	else
#endif /* CP_USE_SSL */
	rc = send(desc->owner->socket->fd, index, desc->remaining, 0);

	if (rc == -1)
	{
		int err = errno;
		if (!(err == EAGAIN || err == EINTR))
		{
			desc->stage = TRANSFER_STAGE_DONE;
			desc->status = CP_HTTP_RESULT_WRITE_ERROR;
			return;
		}
	}
	else
		desc->owner->socket->bytes_sent += rc;

	desc->remaining -= rc;
	if (desc->remaining == 0)
	{
		desc->stage = TRANSFER_STAGE_READ;
		desc->buf->len = 0;
	}
}

static void transfer_chunked(cp_http_transfer_descriptor *desc, 
							 char *tmpbuf, 
							 int index, 
							 int len)
{
	char *curr = &tmpbuf[index];
	int read_len;

	if (desc->remaining) 
	{
		read_len = len - index;
		if (read_len <= desc->remaining) /* need another read before next chunk */
		{
			cp_string_cat_bin(desc->res->content, curr, read_len);
			desc->remaining -= read_len;
			return;
		}
		else
		{
			cp_string_cat_bin(desc->res->content, curr, desc->remaining);
			curr = &tmpbuf[index + desc->remaining];
			desc->remaining = 0;
		}
	}
	
	while (*curr == '\r' || *curr == '\n') curr++;
	if (*curr == 0) return; 

	read_len = xtoi(curr);
	if (read_len == 0) /* last chunk length is 0 - done reading */
	{
		desc->stage = TRANSFER_STAGE_DONE;
		desc->status = CP_HTTP_RESULT_SUCCESS;
		return;
	}
	else
	{
		char *str = strchr(curr, '\n');
		if (str == NULL) 
		{
			desc->stage = TRANSFER_STAGE_DONE;
			desc->status = CP_HTTP_RESULT_READ_ERROR;
		}
		curr = str + 1;
	}
	desc->remaining = read_len;

	
	/* recursive call for the case there's still more in the buffer */
	if (curr - tmpbuf < len)
		transfer_chunked(desc, tmpbuf, (curr - tmpbuf), len);
}

static void 
	cp_http_transfer_descriptor_do_read(cp_http_transfer_descriptor *desc)
{
	char tmpbuf[HTTP_PARSE_BUFSIZE + 1];
	int read_len;
	int rc;

	/* perform a read */
	if (desc->remaining && desc->remaining < HTTP_PARSE_BUFSIZE)
		read_len = desc->remaining;
	else 
		read_len = HTTP_PARSE_BUFSIZE;

	rc = read_chunk(desc->owner, tmpbuf, read_len);
//	rc = cp_client_read(desc->owner->socket, tmpbuf, read_len);

	if (rc < 0) goto READ_ERROR;
	if (rc == 0)
	{
		if (desc->transfer != TRANSFER_TYPE_READ_TO_EOF) goto READ_ERROR;
		desc->stage = TRANSFER_STAGE_DONE;
		desc->status = CP_HTTP_RESULT_SUCCESS;
		return;
	}

	/* handle response body according to transfer type */
	if (desc->read_stage == RESPONSE_BODY)
	{
		switch (desc->transfer)
		{
			case TRANSFER_TYPE_CHUNKED: 
				transfer_chunked(desc, tmpbuf, 0, rc); 
				break;
				
			case TRANSFER_TYPE_LENGTH: 
				cp_string_cat_bin(desc->res->content, tmpbuf, rc);
				desc->remaining -= rc;
				if (desc->remaining == 0)
				{
					desc->stage = TRANSFER_STAGE_DONE;
					desc->status = CP_HTTP_RESULT_SUCCESS;
				}
				break;

			case TRANSFER_TYPE_READ_TO_EOF:
				cp_string_cat_bin(desc->res->content, tmpbuf, rc);
				break;
		}

		return;
	}
	
	/* still reading header - concatenate to header buffer */
	cp_string_cat_bin(desc->buf, tmpbuf, rc);
	desc->buf->data[desc->buf->len] = '\0';

	if (desc->read_stage == RESPONSE_INIT)
	{
		desc->read_ptr = desc->buf->data;
		desc->res = cp_http_client_response_create();
		if (read_status_line(desc->res, &desc->read_ptr, desc->buf->len)) 
			return;
		
		desc->read_stage = RESPONSE_HEADERS;
	}

	/* read headers, determing transfer type and begin transfer */
	if (desc->read_stage == RESPONSE_HEADERS)
	{
		int rv;
		char *prm;

		rv = read_headers(desc->owner, desc->res, desc->uri, &desc->read_ptr, 
				desc->buf->len + desc->skip - 
				(desc->read_ptr - desc->buf->data)); 
		if (rv == -1)
			return; //~~ fail
		else if (rv == EAGAIN) 
		{
			desc->skip += rc;
			return;
		}

		desc->read_stage = RESPONSE_BODY;
		if ((prm = cp_hashtable_get(desc->res->header, "Transfer-Encoding"))
			&& strcasecmp(prm, "chunked") == 0)
		{
			desc->transfer = TRANSFER_TYPE_CHUNKED;

			desc->res->content = cp_string_create_empty(0x400);
			transfer_chunked(desc, tmpbuf, 
							 desc->read_ptr - desc->buf->data, rc);
			return;
		}
		else if ((prm = cp_hashtable_get(desc->res->header, "Content-Length")))
		{
			char *src = find_response_end(desc->read_ptr);
			if (src == NULL) goto READ_ERROR;
			desc->transfer = TRANSFER_TYPE_LENGTH;

			desc->read_ptr = src;
			desc->res->len = atoi(prm);
			desc->res->content = 
				cp_string_create(desc->read_ptr, 
							 rc + desc->skip - 
							 (desc->read_ptr - desc->buf->data));
			desc->remaining = 
				desc->res->len - desc->res->content->len;
			if (desc->remaining == 0)
			{
				desc->stage = TRANSFER_STAGE_DONE;
				desc->status = CP_HTTP_RESULT_SUCCESS;
				return;
			}
		}
		else if (cp_hashtable_get(desc->res->header, "Content-Type"))
		{
			char *src = find_response_end(desc->read_ptr);
			if (src == NULL) goto READ_ERROR;
			desc->transfer = TRANSFER_TYPE_READ_TO_EOF;

			desc->read_ptr = src;
			desc->res->content = 
				cp_string_create(desc->read_ptr, 
						 rc + desc->skip - (desc->read_ptr - desc->buf->data));
			return;
		}
	}

	return;

READ_ERROR:
	desc->stage = TRANSFER_STAGE_DONE;
	desc->status = CP_HTTP_RESULT_READ_ERROR;
}

/** perform a redirect if required */
static void
	cp_http_transfer_descriptor_do_redirect(cp_http_transfer_descriptor *desc)
{
	if (desc->owner->follow_redirects && 
		(desc->res->status == HTTP_302_FOUND ||
		 desc->res->status == HTTP_301_MOVED_PERMANENTLY ||
		 desc->res->status == HTTP_300_MULTIPLE_CHOICES) &&
		desc->redirects < desc->owner->max_redirects)
	{
		cp_url_descriptor *udesc;
		char *url = cp_hashtable_get(desc->res->header, "Location");
		if (url == NULL) return;
		udesc = cp_url_descriptor_parse(url);
		if (udesc == NULL) return;

		if (udesc->host == NULL) 
		{
			udesc->host = strdup(desc->owner->host);
			udesc->port = desc->owner->port;
#ifdef CP_USE_SSL
			udesc->ssl = desc->owner->socket->use_ssl;
#endif /* CP_USE_SSL */
		}
		else if (strcmp(udesc->host, desc->owner->host) || 
			udesc->port != desc->owner->port)
		{
			free(desc->owner->host);
			desc->owner->host = strdup(udesc->host);
			if (!desc->owner->proxy_host)
				cp_client_reopen(desc->owner->socket, udesc->host, udesc->port);
			//~~ redirect to https issue
		}

		cp_string_destroy(desc->buf);
		desc->buf = cp_httpclient_format_request(desc->owner, udesc->uri);
		desc->remaining = desc->buf->len;
		desc->stage = TRANSFER_STAGE_WRITE; //~~ or STAGE_CONNECT 
		desc->read_stage = RESPONSE_INIT;
		cp_http_response_delete(desc->res);
		desc->res = NULL;
		desc->redirects++;

		cp_url_descriptor_destroy(udesc);
	}
}


/** used for parallel request processing (background >= 2) */
typedef struct _async_struct_entry
{
	cp_httpclient_ctl *ctl;
	int index;
} async_struct_entry;

static void *process_async_stack_entry(void *prm)
{
	async_struct_entry *entry = prm;
	cp_http_transfer_descriptor *desc;
	cp_httpclient_ctl *ctl = entry->ctl;
	int i = entry->index;

	desc = ctl->desc[i];
	if (desc->stage == TRANSFER_STAGE_WRITE)
	{
#ifdef HAVE_POLL
		if (ctl->ufds[i].revents & POLLOUT)
#else
		if (FD_ISSET(desc->owner->socket->fd, &ctl->ufds_w))
#endif /* HAVE_POLL */
			cp_http_transfer_descriptor_do_write(desc);
	}
	/* don't try to read before making another poll()/select() */
	else if (desc->stage == TRANSFER_STAGE_READ)
	{
#ifdef HAVE_POLL
		if (ctl->ufds[i].revents & (POLLIN | POLLPRI))
#else
		if (FD_ISSET(desc->owner->socket->fd, &ctl->ufds_r))
#endif /* HAVE_POLL */
		{
			cp_http_transfer_descriptor_do_read(desc);
			if (desc->res)
				cp_http_transfer_descriptor_do_redirect(desc);
		}
	}

	/* no need to poll()/select() again if already done */
	if (desc->stage == TRANSFER_STAGE_DONE)
	{
		cp_hashlist_remove(ctl->stack, desc);
		cp_http_transfer_descriptor_callback(desc);
	}
	
	free(entry);

	return NULL;
}

static void scan_async_stack(cp_httpclient_ctl *ctl)
{
	int i;
	cp_http_transfer_descriptor *desc;

	if (ctl->pool)
	{
		for (i = 0; i < ctl->nfds; i++)
		{
			async_struct_entry *entry = calloc(1, sizeof(async_struct_entry));
			entry->ctl = ctl;
			entry->index = i;
			cp_thread_pool_get(ctl->pool, process_async_stack_entry, entry);
		}
		cp_thread_pool_wait(ctl->pool);
		return;
	}
		
	for (i = 0; i < ctl->nfds; i++)
	{
		desc = ctl->desc[i];
		if (desc->stage == TRANSFER_STAGE_WRITE)
		{
#ifdef HAVE_POLL
			if (ctl->ufds[i].revents & POLLOUT)
#else
			if (FD_ISSET(desc->owner->socket->fd, &ctl->ufds_w))
#endif /* HAVE_POLL */
				cp_http_transfer_descriptor_do_write(desc);
		}
		/* don't try to read before making another poll()/select() */
		else if (desc->stage == TRANSFER_STAGE_READ)
		{
#ifdef HAVE_POLL
			if (ctl->ufds[i].revents & (POLLIN | POLLPRI))
#else
			if (FD_ISSET(desc->owner->socket->fd, &ctl->ufds_r))
#endif /* HAVE_POLL */
			{
				cp_http_transfer_descriptor_do_read(desc);
				if (desc->res)
					cp_http_transfer_descriptor_do_redirect(desc);
			}
		}

		/* no need to poll()/select() again if already done */
		if (desc->stage == TRANSFER_STAGE_DONE)
		{
			cp_hashlist_remove(ctl->stack, desc);
			cp_http_transfer_descriptor_callback(desc);
			cp_http_transfer_descriptor_destroy(desc);
		}
	}
}

static void *async_bg_thread_run(void *prm)
{
	int err;
	int rcount;
	cp_httpclient_ctl *ctl = prm;
#ifndef HAVE_POLL
	struct timeval delay;
	delay.tv_sec = 0;
	delay.tv_usec = 50;
#endif /* ndef HAVE_POLL */

	while (ctl->running)
	{
		cp_mutex_lock(&ctl->bg_lock);
		while ((rcount = cp_hashlist_item_count(ctl->stack) == 0) &&
				ctl->running)
			cp_cond_wait(&ctl->bg_cond, &ctl->bg_lock);
		cp_mutex_unlock(&ctl->bg_lock);

		if (ctl->running == 0) break; /* shutdown */
			
		if (set_async_stack_size(ctl, rcount))
		{
			cp_error(CP_MEMORY_ALLOCATION_FAILURE, "background fetch failed");
			break;
		}

		prepare_async_stack_fds(ctl);
		err = 0;
		do
		{
#ifdef HAVE_POLL
			if (poll(ctl->ufds, ctl->nfds, 0) == -1)
				err = errno;
#else
			if (select(ctl->fdmax + 1, &ctl->ufds_r, &ctl->ufds_w, NULL, 
						&delay) == -1)
				err = errno;
#endif /* HAVE_POLL */
		} while (err == EINTR);
		scan_async_stack(ctl);
	}

	return NULL;
}

int cp_httpclient_fetch_nb_exec()
{
	return cp_httpclient_fetch_ctl_exec(async_stack);
}

int cp_httpclient_fetch_ctl_exec(cp_httpclient_ctl *ctl)
{
	int rcount;
	int err;
#ifndef HAVE_POLL
	struct timeval delay;
	delay.tv_sec = 0;
	delay.tv_usec = 50;
#endif /* ndef HAVE_POLL */
	
	rcount = cp_hashlist_item_count(ctl->stack);
	if (rcount == 0) return 0;
	
	if (set_async_stack_size(ctl, rcount))
	{
		cp_error(CP_MEMORY_ALLOCATION_FAILURE, "asynchronous fetch failed");
		return -1;
	}

	prepare_async_stack_fds(ctl);
	
	err = 0;
	do
	{
#ifdef HAVE_POLL
		if (poll(ctl->ufds, ctl->nfds, 0) == -1)
			err = errno;
#else
		if (select(ctl->fdmax + 1, &ctl->ufds_r, &ctl->ufds_w, NULL, 
					&delay) == -1)
			err = errno;
#endif /* HAVE_POLL */
	} while (err == EINTR);
	scan_async_stack(ctl);

	return cp_hashlist_item_count(ctl->stack);
}

cp_httpclient_ctl *cp_httpclient_ctl_create(int background)
{
	cp_httpclient_ctl *ctl = calloc(1, sizeof(cp_httpclient_ctl));
	if (ctl == NULL) goto CREATE_ERROR;
	ctl->stack = cp_hashlist_create(1, cp_hash_addr, cp_hash_compare_addr);
	if (ctl->stack == NULL) goto CREATE_ERROR;
	ctl->size = 10;
#ifdef HAVE_POLL
	ctl->ufds = malloc(10 * sizeof(struct pollfd));
	if (ctl->ufds == NULL) goto CREATE_ERROR;
// #else
//	FD_ZERO(&ctl->ufds_r);
//	FD_ZERO(&ctl->ufds_w);
#endif /* HAVE_POLL */
	ctl->desc = malloc(10 * sizeof(cp_http_transfer_descriptor *));
	if (ctl->desc == NULL) goto CREATE_ERROR;

	if (background)
	{
		int rc;
		if ((rc = cp_mutex_init(&ctl->bg_lock, NULL)))
			goto CREATE_ERROR;
		cp_cond_init(&ctl->bg_cond, NULL);
		ctl->running = 1;
		if (background > 1)
		{
			ctl->pool = cp_thread_pool_create(1, background);
			if (ctl->pool == NULL) goto CREATE_ERROR;
		}
		if (cp_thread_create(ctl->bg, NULL, async_bg_thread_run, ctl) != 0)
			goto CREATE_ERROR;
	}
	
	return ctl;

CREATE_ERROR:
	cp_httpclient_ctl_destroy(ctl);
	return NULL;
}

void cp_httpclient_ctl_destroy(cp_httpclient_ctl *ctl)
{
	if (ctl)
	{
		if (ctl->running)
		{
			cp_mutex_lock(&ctl->bg_lock);
			ctl->running = 0;
			cp_cond_signal(&ctl->bg_cond);
			cp_mutex_unlock(&ctl->bg_lock);

			if (ctl->pool)
			{
				cp_thread_pool_stop(ctl->pool);
				cp_thread_pool_destroy(ctl->pool);
			}
			
			cp_thread_join(ctl->bg, NULL);
			cp_mutex_destroy(&ctl->bg_lock);
			cp_cond_destroy(&ctl->bg_cond);
		}
			
		cp_hashlist_destroy_custom(ctl->stack, NULL, 
					   (cp_destructor_fn) cp_http_transfer_descriptor_destroy);
#ifdef HAVE_POLL
		if (ctl->ufds) free(ctl->ufds);
#endif
		if (ctl->desc) free(ctl->desc);
		free(ctl);
	}
}

#ifdef HAVE_POLL
struct pollfd *cp_httpclient_ctl_default_get_pollfds(int *num)
{
	return cp_httpclient_ctl_get_pollfds(async_stack, num);
}

struct pollfd *cp_httpclient_ctl_get_pollfds(cp_httpclient_ctl *ctl, int *num)
{
	prepare_async_stack_fds(ctl);
	if (num) *num = cp_hashlist_item_count(ctl->stack);
	return ctl->ufds;
}
#else
fd_set *cp_httpclient_ctl_default_get_read_fd_set(int *num)
{
	return cp_httpclient_ctl_get_read_fd_set(async_stack, num);
}

fd_set *cp_httpclient_ctl_get_read_fd_set(cp_httpclient_ctl *ctl, int *num)
{
	prepare_async_stack_fds(ctl);
	if (num) *num = ctl->fdmax;
	return &ctl->ufds_r;
}

fd_set *cp_httpclient_ctl_default_get_write_fd_set(int *num)
{
	return cp_httpclient_ctl_get_write_fd_set(async_stack, num);
}

fd_set *cp_httpclient_ctl_get_write_fd_set(cp_httpclient_ctl *ctl, int *num)
{
	prepare_async_stack_fds(ctl);
	if (num) *num = ctl->fdmax;
	return &ctl->ufds_w;
}
#endif /* HAVE_POLL */


static int init_async_bg()
{
	async_bg_stack = cp_httpclient_ctl_create(ASYNC_BG_THREAD_MAX);
	if (async_bg_stack == NULL) goto INIT_ERROR;
	return 0;

INIT_ERROR:
	cp_error(CP_THREAD_CREATION_FAILURE, 
			"can\'t start background transfer handler");
	stop_async_bg();
	return -1;
}

static void stop_async_bg()
{
	if (async_bg_stack)
		cp_httpclient_ctl_destroy(async_bg_stack);
}


cp_http_transfer_descriptor *
	cp_http_transfer_descriptor_create(void *id, 
									   cp_httpclient *client, 
									   cp_httpclient_callback callback,
									   char *uri)
{
	cp_http_transfer_descriptor *desc = 
		calloc(1, sizeof(cp_http_transfer_descriptor));
	if (desc == NULL) return NULL;

	desc->id = id;
	desc->owner = client;
	desc->callback = callback;
	desc->stage = TRANSFER_STAGE_CONNECT;
	desc->uri = strdup(uri);

	return desc;
}

void cp_http_transfer_descriptor_destroy(cp_http_transfer_descriptor *desc)
{
	if (desc)
	{
		if (desc->buf) cp_string_destroy(desc->buf);
		if (desc->owner->cgi_parameters && desc->owner->auto_drop_parameters)
			cp_httpclient_drop_parameters(desc->owner);
		if (desc->owner->header && desc->owner->auto_drop_headers)
			cp_httpclient_drop_headers(desc->owner);
		if (desc->uri) free(desc->uri);
		free(desc);
	}
}


int cp_httpclient_fetch_nb(cp_httpclient *client, char *uri, void *id, 
						   cp_httpclient_callback callback, int background)
{
	if (background && async_bg_stack == NULL)
		if (init_async_bg()) return -1;

	return cp_httpclient_fetch_ctl(background ? async_bg_stack : async_stack, 
			client, uri, id, callback);
}

int cp_httpclient_fetch_ctl(cp_httpclient_ctl *ctl, cp_httpclient *client, 
							char *uri, void *id, 
							cp_httpclient_callback callback)
{
	cp_http_transfer_descriptor *desc = 
		cp_http_transfer_descriptor_create(id, client, callback, uri);
	if (desc == NULL) return -1;

	desc->buf = cp_httpclient_format_request(client, uri);
	desc->remaining = desc->buf->len;

	if (ctl->running) /* background transfer */
	{
		cp_hashlist_append(ctl->stack, desc, desc);
		cp_mutex_lock(&ctl->bg_lock);
		cp_cond_broadcast(&ctl->bg_cond);
		cp_mutex_unlock(&ctl->bg_lock);
	}
	else
		cp_hashlist_append(ctl->stack, desc, desc);
		
	return 0;
}

