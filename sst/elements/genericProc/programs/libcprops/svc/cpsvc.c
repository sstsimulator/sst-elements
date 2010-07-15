#include "cprops/config.h"
#include "cprops/socket.h"
#include "cprops/http.h"
#include "cprops/util.h"
#include "cprops/hashtable.h"
#include "cprops/log.h"
#include "cprops/str.h"

#include "file.h"
#include "cgi.h"

#ifdef CP_USE_CPSP
#include "cpsp.h"
#include "cpsp_invoker.h"
#endif /* CP_USE_CPSP */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#if defined(unix) || defined(__unix__) || defined(__MACH__)
#include <unistd.h>
#endif
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <errno.h>

#ifdef CP_USE_SSL
#include <openssl/ssl.h>
#endif

static int port = 5000;

static int log_level = 0;

#define DEFAULT_DOCUMENT_ROOT "."
#ifdef CP_USE_CPSP
#define DEFAULT_CPSP_BINARIES  "cpsp"
#endif /* CP_USE_CPSP */
#define DEFAULT_CGI_PATH "/cgi-bin"
#define DEFAULT_CGI_PREFIX "/cgi-bin"
#define DEFAULT_MIME_TYPES_DEFINITION_FILE "mime.types"

static char *document_root = NULL;
static int free_document_root = 1;
#ifdef CP_USE_CPSP
static char *cpsp_bin_path = NULL;
#endif /* CP_USE_CPSP */
static int free_cgi_path = 1;
static char *cgi_path = NULL;
static int free_cgi_prefix = 1;
static char *cgi_prefix = NULL;
static char *mimetypes_filename = DEFAULT_MIME_TYPES_DEFINITION_FILE;
#ifdef CP_USE_CPSP
static short fork_cpsp = 1;
#endif /* CP_USE_CPSP */

#ifdef CP_USE_SSL
char *usage = "%s [-p port] [-s -c certificate_file -k key_file -v -f -o] [-m mimetypes] [-d path] [-l loglevel]"
#ifdef CP_USE_CPSP
"[-u -b path] "
#endif /* CP_USE_CPSP */
"[-C path -P prefix]\n"
"  -s            - use ssl\n"
"  -v            - verify client\n"
"  -f            - fail connection on verification failure\n"
"  -o            - only verify client once\n"
#else
char *usage = "%s [-p port] [-m mimetypes] [-d path] [-l loglevel]"
#ifdef CP_USE_CPSP
"[-u -b path]"
#endif /* CP_USE_CPSP */
"\n"
#endif
"  -l number     -  log level (0 debug, 1 info, 2 warn, 3 error, 4 fatal)"
"  -m mimetypes  -  mime types definition file\n"
"  -d path       -  document root\n"
#ifdef CP_USE_CPSP
"  -b path       -  path to cpsp binaries\n"
"  -u            -  run cpsp scripts in main process\n"
#endif /* CP_USE_CPSP */
"  -C path       -  cgi-bin path\n"
"  -P prefix     -  uri prefix for CGIs\n"
"  -h            -  print this message\n";

#ifdef CP_USE_SSL
char *certificate_file = NULL;
char *key_file = NULL;
int verify = 0;
int use_ssl = 0;
#endif

char *prefix_cwd(char *dir) //~~ FIX THIS
{
	if (*dir == '/')
		return strdup(dir);
	else
	{
		cp_string *path;
		char buf[0x400];
		char *ret;

		getcwd(buf, 0x400);
		path = cp_string_create(buf, strlen(buf));
		cp_string_cstrcat(path, "/");
		cp_string_cstrcat(path, dir);
		ret = cp_string_tocstr(path);
		cp_string_drop_wrap(path);

		return ret;
	}
}

void process_cmdline(int argc, char *argv[])
{
	int c;
#ifdef CP_USE_SSL
	int rc;
	struct stat buf;

    while ((c = getopt(argc, argv, "sp:c:k:vfom:d:ub:C:P:h")) != -1)
#else
    while ((c = getopt(argc, argv, "p:m:d:ub:C:P:h")) != -1)
#endif
    {
        switch (c)
        {
            case 'p': 
				port = atoi(optarg);
				if (port == 0)
				{
					fprintf(stderr, "%s: bad port number [%s]\n", 
							argv[0], optarg);
					exit(1);
				}
                break;
				
			case 'm':
				mimetypes_filename = optarg;
				break; 

			case 'l':
				log_level = atoi(optarg);
				break;

			case 'd':
				document_root = prefix_cwd(optarg);
				break;

#ifdef CP_USE_CPSP
			case 'u': 
				fork_cpsp = 0;
				break;

			case 'b':
				cpsp_bin_path = prefix_cwd(optarg);
				break;
#endif /* CP_USE_CPSP */
			
			case 'C':
				cgi_path = prefix_cwd(optarg);
				break;
			
			case 'P':
				cgi_prefix = strdup(optarg);
				break;

#ifdef CP_USE_SSL					  
            case 's': 
				use_ssl = 1;
				break;

            case 'c': 
				rc = stat(optarg, &buf);
				if (rc)
				{
					fprintf(stderr, stat_error_fmt(errno), argv[0], optarg);
					exit(1);
				}
				certificate_file = strdup(optarg); 
                break;

            case 'k': 
				rc = stat(optarg, &buf);
				if (rc)
				{
					fprintf(stderr, stat_error_fmt(errno), argv[0], optarg);
					exit(1);
				}
				key_file = strdup(optarg); 
                break;

            case 'v': 
				verify |= SSL_VERIFY_PEER;
                break;

			case 'f': 
				verify |= SSL_VERIFY_FAIL_IF_NO_PEER_CERT;
				break;

			case 'o': 
				verify |= SSL_VERIFY_CLIENT_ONCE;
				break;
#endif

			case 'h':
				fprintf(stderr, usage, argv[0]);
				exit(0);
				break;

			default:  
				fprintf(stderr, "%s: unknown option \'%c\'\n", argv[0], c);
				fprintf(stderr, usage, argv[0]);
				exit(1);
				break;
				
        }
    }

	if (document_root == NULL)
	{
		document_root = DEFAULT_DOCUMENT_ROOT;
		free_document_root = 0;
	}

	if (cgi_path == NULL)
	{
		cgi_path = DEFAULT_CGI_PATH;
		free_cgi_path = 0;
	}

	if (cgi_prefix == NULL)
	{
		cgi_prefix = DEFAULT_CGI_PREFIX;
		free_cgi_prefix = 0;
	}

#ifdef CP_USE_CPSP
	if (cpsp_bin_path == NULL)
		cpsp_bin_path = prefix_cwd(DEFAULT_CPSP_BINARIES);
#endif /* CP_USE_CPSP */
}

void cpsvc_signal_handler(int sig)
{
	switch (sig)
	{
		case SIGINT:
#ifdef CP_USE_CPSP
			if (fork_cpsp) stop_cpsp_invoker();
#endif /* CP_USE_CPSP */
			cp_httpsocket_stop_all();
			break;

		case SIGTERM:
#ifdef CP_USE_CPSP
			if (fork_cpsp) stop_cpsp_invoker();
#endif /* CP_USE_CPSP */
			cp_httpsocket_stop_all();
			break;
	}
}

int main(int argc, char *argv[])
{
	int rc = 0;
#ifdef CP_HAS_SIGACTION
	struct sigaction act;
#endif /* CP_HAS_SIGACTION */
	cp_httpsocket *sock;

	cp_http_service *cgi_service;
#ifdef CP_USE_CPSP
	cp_http_service *cpsp_service;
#endif /* CP_USE_CPSP */

	process_cmdline(argc, argv);
	if (free_cgi_path)
		cp_http_add_shutdown_callback(cp_free, cgi_path);

#ifdef CP_HAS_SIGACTION
	act.sa_handler = cpsvc_signal_handler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
#endif /* CP_HAS_SIGACTION */

	cp_log_init("cpsvc.log", log_level);
	cp_http_init();

	/* override default http signal handler for stopping */
#ifdef CP_HAS_SIGACTION
	sigaction(SIGINT, &act, NULL);
	sigaction(SIGTERM, &act, NULL);
#else
	signal(SIGINT, cpsvc_signal_handler);
	signal(SIGTERM, cpsvc_signal_handler);
#endif /* CP_HAS_SIGACTION */

	cp_info("%s: starting", argv[0]);

#ifdef CP_USE_CPSP
	if (fork_cpsp)
	{
		init_cpsp_invoker(document_root, cpsp_bin_path, 5, 50); /* min 5 workers, max 50 */
		cpsp_service = cp_http_service_create("cpsp", "cpsp", cpsp_invoke);
	}
	else
	{
		cpsp_init(0, cpsp_bin_path, document_root);
		cpsp_service = cp_http_service_create("cpsp", "cpsp", cpsp_gen);
	}
#endif /* CP_USE_CPSP */

	if ((rc = init_file_service(mimetypes_filename, document_root)))
	{
		cp_error(rc, "%s: can\'t start", argv[0]);
		goto DONE;
	}
		
//	init_cgi(document_root);
	init_cgi(cgi_path, cgi_prefix);
	cgi_service = cp_http_service_create("CGI", cgi_prefix, cgi);

#ifdef CP_USE_SSL
	if (use_ssl)
	{
		sock = cp_httpsocket_create_ssl(port, 
					(cp_http_service_callback) file_service,				    
					certificate_file, key_file, verify);
	}
	else
#endif
	sock = cp_httpsocket_create(port, file_service);

	cp_httpsocket_register_service(sock, cgi_service);	
#ifdef CP_USE_CPSP
	register_ext_svc("cpsp", cpsp_service);
#endif /* CP_USE_CPSP */

	if (sock)
	{
		cp_info("%s: cp_httpsocket server starting on port %d", argv[0], port);
		cp_httpsocket_listen(sock);
		cp_info("%s: cp_httpsocket server on port %d: stopping", argv[0], port);
		cp_httpsocket_delete(sock);
	}

DONE:
	cp_http_shutdown();
#ifdef CP_USE_CPSP
	cp_free(cpsp_bin_path);
#endif /* CP_USE_CPSP */
	if (free_document_root) cp_free(document_root);
	if (free_cgi_prefix) cp_free(cgi_prefix);
	cp_info("done");
	cp_log_close();

	return rc;
}

