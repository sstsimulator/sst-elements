#include "cprops/http.h"
#include "cprops/str.h"

#include "cgi.h"

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#if defined(unix) || defined(__unix__) || defined(__MACH__)
#include <unistd.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#endif

#ifndef CP_HAS_INET_NTOP
#include "cprops/util.h"
#endif /* no inet_ntop */

#ifdef _WINDOWS
#include <winsock2.h>
#include <windows.h>
#include "cprops/util.h"
#endif


#define BUFSIZE 0x1000

static volatile int initialized = 0;
static char *document_root = NULL;
static char *prefix = NULL;
static int prefix_len = 0;

int init_cgi(char *path, char *prefix_prm)
{
	if (initialized) return initialized;
	initialized = 1;
	document_root = strdup(path);
#ifdef _WINDOWS
	replace_char(document_root, '/', '\\');
#endif /* _WINDOWS */
	prefix = strdup(prefix_prm);
	prefix_len = strlen(prefix);
	cp_http_add_shutdown_callback(shutdown_cgi, document_root);

	return 0;
}
	
void shutdown_cgi(void *dummy)
{
	if (initialized)
	{
		initialized = 0;
		if (document_root)
		{
			free(document_root);
			document_root = NULL;
		}
		if (prefix)
		{
			free(prefix);
			prefix = NULL;
		}
	}
}

char *header2cgi(char *dst, char *src, size_t len)
{
	char *p;

	if (strcmp(src, "Content-Length") && strcmp(src, "Content-Type"))
	{
#ifdef CP_HAS_STRLCPY
		strlcpy(dst, "HTTP_", len);
#else
		strcpy(dst, "HTTP_");
#endif /* CP_HAS_STRLCPY */
		p = &dst[strlen(dst)];
	}
	else
		p = dst;
	
	while (*src)
	{
		if (*src == '-')
			*p = '_';
		else if (*src >= 'a' && *src <= 'z')
			*p = *src + ('A' -'a');
		else 
			*p = *src;

		p++;
		src++;
	}
	*p = '\0';

	return dst;
}
		
void prepare_env(cp_http_request *request, char *filename, char ***env)
{
	char buf[0x400];
	char **key;
//	char *value;
	cp_string *str;
	cp_vector *enw = cp_vector_create(10);
	char *eq = "=";
	int n = cp_hashtable_count(request->header);
	key = cp_http_request_get_headers(request);
	while (n--)
	{
		header2cgi(buf, key[n], 0x400);
		str = cp_string_create(buf, strlen(buf));
		cp_string_cat_bin(str, eq, 1);
		cp_string_cstrcat(str, cp_http_request_get_header(request, key[n]));
		cp_vector_add_element(enw, cp_string_tocstr(str));
		cp_string_drop_wrap(str);
	}
	cp_free(key);

#ifdef CP_HAS_SNPRINTF
	snprintf(buf, 0x400, "DOCUMENT_ROOT=%s", document_root);
#else
	sprintf(buf, "DOCUMENT_ROOT=%s", document_root);
#endif /* CP_HAS_SNPRINTF */
	cp_vector_add_element(enw, strdup(buf));
#ifdef CP_HAS_SNPRINTF
	snprintf(buf, 0x400, "GATEWAY_INTERFACE=CGI/1.1");
#else
	sprintf(buf, "GATEWAY_INTERFACE=CGI/1.1");
#endif /* CP_HAS_SNPRINTF */
	cp_vector_add_element(enw, strdup(buf));
	if (request->type == GET)
	{
#ifdef CP_HAS_SNPRINTF
		snprintf(buf, 0x400, "QUERY_STRING=%s", 
				request->query_string ? request->query_string : "");
#else
		sprintf(buf, "QUERY_STRING=%s", 
				request->query_string ? request->query_string : "");
#endif /* CP_HAS_SNPRINTF */
		cp_vector_add_element(enw, strdup(buf));
	}
#ifdef CP_USE_COOKIES
	if (request->cookie)
	{
		int i;
		char *cookie = cp_vector_element_at(request->cookie, 0);

#ifdef CP_HAS_SNPRINTF
		snprintf(buf, 0x400, "HTTP_COOKIE=%s", cookie);
#else
		sprintf(buf, "HTTP_COOKIE=%s", cookie);
#endif /* CP_HAS_SNPRINTF */
		for (i = 1; i < cp_vector_size(request->cookie); i++)
		{
			cookie = cp_vector_element_at(request->cookie, i);
#ifdef CP_HAS_STRLCAT
			strlcat(buf, "; ", 0x400);
			strlcat(buf, cookie, 0x400);
#else
			strcat(buf, "; ");
			strcat(buf, cookie);
#endif /* CP_HAS_STRLCAT */
		}
		cp_vector_add_element(enw, strdup(buf));
	}
#endif
#ifdef CP_HAS_SNPRINTF
	snprintf(buf, 0x400, "REQUEST_METHOD=%s", get_http_request_type_lit(request->type));
#else
	sprintf(buf, "REQUEST_METHOD=%s", get_http_request_type_lit(request->type));
#endif /* CP_HAS_SNPRINTF */
	cp_vector_add_element(enw, strdup(buf));
#ifdef CP_HAS_SNPRINTF
	snprintf(buf, 0x400, "REQUEST_URI=%s", request->uri);
#else
	sprintf(buf, "REQUEST_URI=%s", request->uri);
#endif /* CP_HAS_SNPRINTF */
	cp_vector_add_element(enw, strdup(buf));
#ifdef CP_HAS_SNPRINTF
	snprintf(buf, 0x400, "SCRIPT_FILENAME=%s", filename);
#else
	sprintf(buf, "SCRIPT_FILENAME=%s", filename);
#endif /* CP_HAS_SNPRINTF */
	cp_vector_add_element(enw, strdup(buf));
#ifdef CP_HAS_SNPRINTF
	snprintf(buf, 0x400, "SCRIPT_NAME=%s", request->uri);
#else
	sprintf(buf, "SCRIPT_NAME=%s", request->uri);
#endif /* CP_HAS_SNPRINTF */
	cp_vector_add_element(enw, strdup(buf));
#ifdef CP_HAS_SNPRINTF
	snprintf(buf, 0x400, "SERVER_PORT=%d", request->owner->sock->port);
#else
	sprintf(buf, "SERVER_PORT=%d", request->owner->sock->port);
#endif /* CP_HAS_SNPRINTF */
	cp_vector_add_element(enw, strdup(buf));
#ifdef CP_HAS_SNPRINTF
	snprintf(buf, 0x400, "SERVER_PROTOCOL=HTTP/1.1");
#else
	sprintf(buf, "SERVER_PROTOCOL=HTTP/1.1");
#endif /* CP_HAS_SNPRINTF */
	cp_vector_add_element(enw, strdup(buf));
	{
		char addr[0x30];
#ifdef CP_HAS_INET_NTOP
		inet_ntop(AF_INET, &request->connection->addr->sin_addr, addr, sizeof(addr));
#else
#ifdef _WINDOWS
		char *tmpaddr = inet_ntoa(request->connection->addr->sin_addr);
		strcpy(addr, tmpaddr);
#endif /* _WINDOWS */
#endif /* unix */
#ifdef CP_HAS_SNPRINTF
		snprintf(buf, 0x400, "REMOTE_ADDR=%s", addr);
#else
		sprintf(buf, "REMOTE_ADDR=%s", addr);
#endif /* CP_HAS_SNPRINTF */
		cp_vector_add_element(enw, strdup(buf));
	}
#ifdef CP_HAS_SNPRINTF
	snprintf(buf, 0x400, "REMOTE_PORT=%d", ntohs(request->connection->addr->sin_port));
#else
	sprintf(buf, "REMOTE_PORT=%d", ntohs(request->connection->addr->sin_port));
#endif /* CP_HAS_SNPRINTF */
	cp_vector_add_element(enw, strdup(buf));

	cp_vector_add_element(enw, NULL);
	*env = (char **) enw->mem;
	cp_free(enw);
}

static int make_filename(char *buf, char *uri)
{
	struct stat statbuf;

	if (strncmp(uri, prefix, prefix_len) == 0)
		uri += prefix_len;

#ifdef CP_HAS_SNPRINTF
	if (document_root)
		snprintf(buf, 0x400, "%s%s", document_root, uri);
	else
		snprintf(buf, 0x400, ".%s", uri);
#else
	if (document_root)
		sprintf(buf, "%s%s", document_root, uri);
	else
		sprintf(buf, ".%s", uri);
#endif /* CP_HAS_SNPRINTF */

	/* some operating systems use '\\' rather than '/' as path separator */
#ifdef _WINDOWS
	replace_char(buf, '/', '\\');
#endif /* _WINDOWS */

	return stat(buf, &statbuf);
}

static char *cgi_err_fmt = 
 	"<html><head><title> CGI error </title></head>\n"
	"<body>can\'t execute CGI script %s: %s\n</body></html>";

#if defined(unix) || defined(__unix__) || defined(__MACH__)
int cgi(cp_http_request *request, cp_http_response *response)
{
	int rc = 0;
	pid_t pid;
	int status;
	char filename[0x400];
	char err[0x400];
	int inpipe[2]; /* pipe for stdin if method is POST */
	int write_stdin;
#ifdef CP_HAS_STRERROR_R
	char reason[0x100];
#endif

/* for SSL, you can't just hook up stdout to the connection file descriptor */
#ifdef CP_USE_SSL
	char buf[BUFSIZE];
	int fd[2];
#endif

#ifdef __TRACE__
	DEBUGMSG("cgi service starting");
#endif /* __TRACE__ */

	if ((rc = make_filename(filename, request->uri)))
	{
#ifdef __TRACE__
		DEBUGMSG("can\'t perform request for %s", filename);
#endif /* __TRACE__ */
		cp_http_response_set_content_type(response, HTML);
		switch (rc)
		{
			case EACCES:
				cp_http_response_set_status(response, HTTP_500_INTERNAL_SERVER_ERROR);
#ifdef CP_HAS_SNPRINTF
				snprintf(err, 0x400, cgi_err_fmt, request->uri, "Permission denied.");
#else
				sprintf(err, cgi_err_fmt, request->uri, "Permission denied.");
#endif /* CP_HAS_SNPRINTF */
				break;

			case ENOENT:
				cp_http_response_set_status(response, HTTP_404_NOT_FOUND);
#ifdef CP_HAS_SNPRINTF
				snprintf(err, 0x400, cgi_err_fmt, request->uri, "File not found.");
#else
				sprintf(err, cgi_err_fmt, request->uri, "File not found.");
#endif /* CP_HAS_SNPRINTF */
				break;

			default:
				cp_http_response_set_status(response, HTTP_500_INTERNAL_SERVER_ERROR);
#ifdef CP_HAS_STRERROR_R
				strerror_r(rc, reason, 0x100);
#ifdef CP_HAS_SNPRINTF
				snprintf(err, 0x400, cgi_err_fmt, request->uri, reason);
#else
				sprintf(err, cgi_err_fmt, request->uri, reason);
#endif /* CP_HAS_SNPRINTF */
#else
#ifdef CP_HAS_SNPRINTF
				snprintf(err, 0x400, cgi_err_fmt, request->uri, strerror(rc));
#else
				sprintf(err, cgi_err_fmt, request->uri, strerror(rc));
#endif /* CP_HAS_SNPRINTF */
#endif
				break;
		}

		cp_http_response_set_body(response, err);
		return HTTP_CONNECTION_POLICY_CLOSE;
	}
#ifdef __TRACE__
		DEBUGMSG("received request for %s", filename);
#endif /* __TRACE__ */

	/* it's a POST and there is content to write - initialize a pipe for CGI stdin */
	write_stdin = request->type == POST && 
				  request->query_string && (*request->query_string);
	if (write_stdin)
	{
		rc = pipe(inpipe);
		if (rc)
		{
			rc = errno;
			cp_perror(CP_IO_ERROR, rc, "can\'t initialize CGI input pipe");
			cp_http_response_set_status(response, HTTP_500_INTERNAL_SERVER_ERROR);
#ifdef CP_HAS_STRERROR_R
			strerror_r(rc, reason, 0x100);
#ifdef CP_HAS_SNPRINTF
			snprintf(err, 0x400, cgi_err_fmt, request->uri, reason);
#else
			sprintf(err, cgi_err_fmt, request->uri, reason);
#endif /* CP_HAS_SNPRINTF */
#else
#ifdef CP_HAS_SNPRINTF
			snprintf(err, 0x400,  cgi_err_fmt, request->uri, strerror(rc));
#else
			sprintf(err, cgi_err_fmt, request->uri, strerror(rc));
#endif /* CP_HAS_SNPRINTF */
#endif /* CP_HAS_STRERROR_R */
			return HTTP_CONNECTION_POLICY_CLOSE;
		}
	}

#ifdef CP_USE_SSL
	if (request->owner->sock->use_ssl)
	{
		rc = socketpair(AF_UNIX, SOCK_STREAM, 0, fd);
		if (rc)
		{
			rc = errno;
			cp_perror(CP_IO_ERROR, rc, "can\'t initialize CGI sockets");
			cp_http_response_set_status(response, HTTP_500_INTERNAL_SERVER_ERROR);
#if CP_HAS_STRERROR_R
			strerror_r(rc, reason, 0x100);
#ifdef CP_HAS_SNPRINTF
			snprintf(err, 0x400, cgi_err_fmt, request->uri, reason);
#else
			sprintf(err, cgi_err_fmt, request->uri, reason);
#endif /* CP_HAS_SNPRINTF */
#else
#ifdef CP_HAS_SNPRINTF
			snprintf(err, cgi_err_fmt, request->uri, strerror(rc));
#else
			sprintf(err, 0x400, cgi_err_fmt, request->uri, strerror(rc));
#endif /* CP_HAS_SNPRINTF */
#endif /* CP_HAS_STRERROR_R */
			return HTTP_CONNECTION_POLICY_CLOSE;
		}
	}
#endif /* CP_USE_SSL */
	
#ifdef __TRACE__
	DEBUGMSG("forking cgi process for %s [%s]", filename, request->uri);
#endif /* __TRACE__ */
	if ((pid = fork()))
	{
		cp_http_response_skip(response);

		/* if it's a POST, write request body to script stdin */
		if (write_stdin)
		{
			int len = strlen(request->query_string);
			int written = 0;
			close(inpipe[0]);
			while (written < len)
			{
				rc = write(inpipe[1], &request->query_string[written], len - written);
				if (rc <= 0) break;
				written += rc;
			}
			close(inpipe[1]);
		}
		
#ifdef CP_USE_SSL
		if (request->owner->sock->use_ssl)
		{
			close(fd[1]);
			while (1)
			{
				rc = read(fd[0], buf, BUFSIZE);
				if (rc == -1)
				{
					if (errno == EINTR) 
						continue;
					else
						break;
				}
				if (rc == 0) break;
				SSL_write(request->connection->ssl, buf, rc);
			}
		}
#endif /* CP_USE_SSL */
		
		waitpid(pid, &status, 0);
		if (!WIFEXITED(status))
		{
			cp_warn("CGI script [%s]: abnormal termination", request->uri);
			if (WIFSIGNALED(status))
				cp_warn("CGI script [%s] exited on signal %d", 
						request->uri, WTERMSIG(status));
		}
	}
	else /* child - invoke CGI script */
	{
		int fdoutput;
		char *argv[2];
		char **env;
//		strcpy(filename, request->uri);

		prepare_env(request, filename, &env);
		argv[0] = filename;
		argv[1] = NULL;

		/* prepare pipe to read POST data */
		if (write_stdin)
		{
			close(0);
			dup(inpipe[0]);
			close(inpipe[0]);
			close(inpipe[1]);
		}
		
#ifdef CP_USE_SSL
		if (request->owner->sock->use_ssl)
		{
			close(fd[0]);
			fdoutput = fd[1];
		}
		else
#endif /* CP_USE_SSL */
		fdoutput = request->connection->fd;
		/* such browsers as mozilla firefox require this */
		write(fdoutput, "HTTP/1.0 200 Ok\r\n", 17);

		close(1);
		dup(fdoutput);
		execve(filename, argv, env); //~~
		exit(-1);
	}

	return HTTP_CONNECTION_POLICY_CLOSE; //~~ DEFAULT or KEEP-ALIVE relies
	// on scripts writing the Content-Length correctly.
}
#else /* unix */

int cgi(cp_http_request *request, cp_http_response *response)
{
	int rc = 0;
	int status;
	char filename[0x400];
	char err[0x400];
//	char env[0x400];
	char **envp;
	int write_stdin;
#ifdef CP_USE_SSL
	char buf[BUFSIZE];
#endif
	HANDLE script_stdin;
	HANDLE script_stdout;
	HANDLE svc_in;
	HANDLE svc_out;
	SECURITY_ATTRIBUTES sec_attr;

	if ((rc = make_filename(filename, request->uri)))
	{
		switch (rc)
		{
			case EACCES:
				cp_http_response_set_status(response, HTTP_500_INTERNAL_SERVER_ERROR);
				sprintf(err, cgi_err_fmt, request->uri, "Permission denied.");
				break;

			case ENOENT:
				cp_http_response_set_status(response, HTTP_404_NOT_FOUND);
				sprintf(err, cgi_err_fmt, request->uri, "File not found.");
				break;

			default:
				cp_http_response_set_status(response, HTTP_500_INTERNAL_SERVER_ERROR);
#ifdef CP_HAS_STRERROR_R
				strerror_r(rc, reason, 0x100);
				sprintf(err, cgi_err_fmt, request->uri, reason);
#else
				sprintf(err, cgi_err_fmt, request->uri, strerror(rc));
#endif
				break;
		}

		goto CGI_CANCEL;
	}

	sec_attr.nLength = sizeof(SECURITY_ATTRIBUTES);
	sec_attr.bInheritHandle = TRUE;
	sec_attr.lpSecurityDescriptor = NULL;

	if (!CreatePipe(&svc_in, &script_stdout, &sec_attr, 0))
	{
		cp_http_response_set_status(response, HTTP_500_INTERNAL_SERVER_ERROR);
		sprintf(err, cgi_err_fmt, request->uri, "can\'t create pipe.");
		goto CGI_CANCEL;
	}
	SetHandleInformation(svc_in, HANDLE_FLAG_INHERIT, 0);

	/* it's a POST and there is content to write - initialize a pipe for CGI stdin */
	write_stdin = request->type == POST && 
				  request->query_string && (*request->query_string);
	if (write_stdin)
	{
//		svc_out = GetStdHandle(STD_OUTPUT_HANDLE);
		if (!CreatePipe(&script_stdin, &svc_out, &sec_attr, 0))
		{
			cp_http_response_set_status(response, HTTP_500_INTERNAL_SERVER_ERROR);
			sprintf(err, cgi_err_fmt, request->uri, "can\'t initialize CGI input pipe");
			goto CGI_CANCEL;
		}
		SetHandleInformation(svc_out, HANDLE_FLAG_INHERIT, 0);
	}
	else /* no request body */
		script_stdin = NULL;

	prepare_env(request, filename, &envp);
//	prepare_env_w(request, filename, env);
	if (!create_proc(filename, script_stdin, 
					 script_stdout, script_stdout, envp))
	{
		cp_http_response_set_status(response, HTTP_500_INTERNAL_SERVER_ERROR);
		sprintf(err, cgi_err_fmt, request->uri, "can\'t create process.");
		goto CGI_CANCEL;
	}

	cp_http_response_skip(response);

	/* write request body to CGI script */
	if (write_stdin)
	{
		unsigned int total = 0;
		unsigned int len = strlen(request->query_string);
		unsigned int wlen;
		while (total < len)
		{
			if (!WriteFile(svc_out, &request->query_string[total], len - total, 
				&wlen, NULL)) break;
			total += wlen;
		}
		CloseHandle(svc_out);
	}

	CloseHandle(script_stdin);
	/* read response and write it to client */
	for (;;) 
	{
		char buf[BUFSIZE];
		int rc;
		unsigned int len, total;
		rc = ReadFile(svc_in, buf, BUFSIZE - 1, &len, NULL);
		if (rc == 0 || len == 0) break;

		total = 0;
		while (total < len)
		{
			if ((rc = cp_connection_descriptor_write(request->connection, 
									&buf[total], len - total)) <= 0) break;
			total += rc;
		}
		if (rc <= 0) break;

		if (len < BUFSIZE - 1) break; //~~
	} 

	CloseHandle(svc_in);

	return HTTP_CONNECTION_POLICY_CLOSE;

CGI_CANCEL:
	cp_http_response_set_content_type(response, HTML);
	cp_http_response_set_body(response, err);
#ifdef _HTTP_DUMP
	DEBUGMSG(err);
#endif
	return HTTP_CONNECTION_POLICY_CLOSE;	
}
#endif /* unix */

