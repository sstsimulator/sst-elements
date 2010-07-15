#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <errno.h>

#include "config.h"
#include "socket.h"
#include "http.h"
#include "util.h"
#include "hashtable.h"
#include "log.h"
#include "str.h"

#ifdef CP_USE_SSL
#include <openssl/ssl.h>
#endif

#ifndef HAVE_STRLCAT
#define strlcat(s1, s2, len) strcat((s1), (s2))
#endif /* HAVE_STRLCAT */

#define BUFLEN 0x800
#define LINELEN 0x200

static int port = 5000;

#define DEFAULT_DOCUMENT_ROOT "."
#define DEFAULT_MIME_TYPES_DEFINITION_FILE "svc/mime.types"

static char *document_root = DEFAULT_DOCUMENT_ROOT;
static char *mimetypes_filename = DEFAULT_MIME_TYPES_DEFINITION_FILE;
static cp_hashtable *mimemap;

int load_mime_types(char *filename)
{
    FILE *fp;
    char mimebuf[LINELEN];
    int rc = 0;
    char *name;
    char *ext;
    char *curr;

    mimemap = 
        cp_hashtable_create_by_option(COLLECTION_MODE_NOSYNC | 
                                      COLLECTION_MODE_COPY | 
                                      COLLECTION_MODE_DEEP, 
                                      500,
                                      cp_hash_string,
                                      cp_hash_compare_string,
                                      (cp_copy_fn) strdup, 
                                      (cp_destructor_fn) free,
                                      (cp_copy_fn) strdup, 
                                      (cp_destructor_fn) free);
    fp = fopen(filename, "r");
    if (fp == NULL)
    {
        cp_error(CP_INVALID_VALUE, "can\'t open %s", filename);
        cp_hashtable_destroy(mimemap);
        return -1;
    }

    while (fgets(mimebuf, LINELEN, fp))
    {
        if (mimebuf[0] == '#') continue;
        name = curr = mimebuf;
        while (*curr && !isspace(*curr)) curr++;
        if (*curr == '\0') continue; /* no extension for this type */
        *curr++ = '\0';

        while (1)
        {
            while (*curr && isspace(*curr)) curr++;
            ext = curr;
            while (*curr && !isspace(*curr)) curr++;
            if (strlen(ext))
            {
                *curr++ = '\0';
                cp_hashtable_put(mimemap, ext, name);
            }
            else
                break;
        }
    }

    fclose(fp);
    return rc;
}

static int init_file_service(char *mimetypes_filename, char *doc_path)
{
    int rc = 0;

    if ((rc = checkdir(doc_path)))
        cp_fatal(rc, "can\'t open document root at [%s]", doc_path);

    if ((rc = load_mime_types(mimetypes_filename)))
        cp_fatal(rc, "can\'t load mime types from [%s], sorry", 
                 mimetypes_filename); 

    document_root = doc_path;
    
    return rc;
}

static int stop_file_service()
{
    cp_hashtable_destroy(mimemap);
    return 0;
}

char *HTTP404_PAGE = 
    "<html><head><title>404 NOT FOUND</title></head>\n"
    "<body><h1>404 NOT FOUND</h1>\n"
    "The page you are looking for is not here.\n<p>"
    "It may be somewhere else."
    "</body></html>";
    
char *HTTP404_PAGE_uri = 
    "<html><head><title>404 NOT FOUND</title></head>\n"
    "<body><h1>404 NOT FOUND</h1>\n"
    "Can\'t find %s on this server;\n<p>"
    "No. it is not here."
    "</body></html>";

#define FBUFSIZE 0x4000
#define PATHLEN 0x400

int file_service(cp_http_request *request, cp_http_response *response)
{
    int rc = 0;
    char *ext;
    char path[PATHLEN];
    int uri_len;
    char buf[FBUFSIZE];
    FILE *fp;
    cp_string *body = NULL;

#ifdef DEBUG
    cp_http_request_dump(request);
#endif

    ext = strrchr(request->uri, '.');
    if (ext) 
        cp_http_response_set_content_type_string(response, 
                                                 cp_hashtable_get(mimemap, ++ext));

    /* check len, avoid buffer overrun */
    uri_len = strlen(request->uri);
    if (uri_len + strlen(document_root) >= PATHLEN)
    {
        cp_http_response_set_content_type(response, HTML);
        cp_http_response_set_status(response, HTTP_404_NOT_FOUND);
        response->body = strdup(HTTP404_PAGE);
        return HTTP_CONNECTION_POLICY_CLOSE;
    }
        
#ifdef HAVE_SNPRINTF
    snprintf(path, PATHLEN, "%s%s", document_root, request->uri);
#else
    sprintf(path, "%s%s", document_root, request->uri);
#endif /* HAVE_SNPRINTF */
    if (path[strlen(path) - 1] == '/') 
    {
        strlcat(path, "index.html", PATHLEN);
        response->content_type = HTML;
    }

    fp = fopen(path, "rb");
    if (fp == NULL)
    {
        cp_http_response_set_content_type(response, HTML);
        cp_http_response_set_status(response, HTTP_404_NOT_FOUND);
#ifdef HAVE_SNPRINTF
        snprintf(buf, FBUFSIZE, HTTP404_PAGE_uri, request->uri);
#else
        sprintf(buf, HTTP404_PAGE_uri, request->uri);
#endif /* HAVE_SNPRINTF */
        response->body = strdup(buf);
        return HTTP_CONNECTION_POLICY_CLOSE;
    }

#ifdef __TRACE__
    DEBUGMSG("retrieving [%s]", path);
#endif
    while ((rc = fread(buf, 1, FBUFSIZE, fp)) > 0)
    {
        if (body == NULL)
            body = cp_string_create(buf, rc);
        else
            cp_string_cat_bin(body, buf, rc);
    }
    fclose(fp);
    
    cp_http_response_set_status(response, HTTP_200_OK);

    response->content = body;

    return HTTP_CONNECTION_POLICY_KEEP_ALIVE;
}

#ifdef CP_USE_SSL
char *usage = "%s [-p port] [-s -c certificate_file -k key_file -v -f -o -m mimetypes -d path]\n"
"  -s   use ssl\n"
"  -v   verify client\n"
"  -f   fail connection on verification failure\n"
"  -o   only verify client once\n"
#else
char *usage = "%s [-p port] [-m mimetypes] [-d path]\n"
#endif
"  -m mimetypes  --  mime types definition file\n"
"  -d path       --  document root\n";

#ifdef CP_USE_SSL
char *certificate_file = NULL;
char *key_file = NULL;
int verify = 0;
int use_ssl = 0;
#endif

void process_cmdline(int argc, char *argv[])
{
    int c;
#ifdef CP_USE_SSL
    int rc;
    struct stat buf;

    while ((c = getopt(argc, argv, "sp:c:k:vfom:d:")) != -1)
#else
    while ((c = getopt(argc, argv, "p:")) != -1)
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

            case 'd':
                document_root = optarg;
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
            default:  
                fprintf(stderr, "%s: unknown option \'%c\'\n", argv[0], c);
                fprintf(stderr, usage, argv[0]);
                exit(1);
                break;
                
        }
    }
}

static int hitcount = 1;

int hitcount_service(cp_http_request *request, cp_http_response *response)
{
    char buf[BUFLEN];
    char sbuf[BUFLEN / 2];
    char **key;
    int i;
    int *pcount;

    cp_http_session *session = cp_http_request_get_session(request, 1);
    if (cp_http_session_is_fresh(session))
    {
        pcount = malloc(sizeof(int *));
        *pcount = 1;
        cp_http_session_set_dtr(session, "pcount", pcount, free);
    }
    else
    {
        pcount = cp_http_session_get(session, "pcount");
        (*pcount)++;
    }

    cp_http_response_set_content_type(response, HTML);
#ifdef HAVE_SNPRINTF
    snprintf(buf, BUFLEN, 
        "<html>\n<head>\n<title>cp_httpsocket test </title>\n</head>\n"
        "<body><h1>hit count: %d</h1>\n"
        "you've visited this page %d times.<p>", hitcount++, *pcount);
#else
    sprintf(buf, 
        "<html>\n<head>\n<title>cp_httpsocket test </title>\n</head>\n"
        "<body><h1>hit count: %d</h1>\n"
        "you've visited this page %d times.<p>", hitcount++, *pcount);
#endif /* HAVE_SNPRINTF */

    if (cp_hashtable_count(request->header))
    {
        strlcat(buf, "<hr>\n<h3> your headers: </h3>\n<ul>\n", BUFLEN);
        key = (char **) cp_hashtable_get_keys(request->header);
        for (i = 0; i < cp_hashtable_count(request->header); i++)
        {
#ifdef HAVE_SNPRINTF
            snprintf(sbuf, BUFLEN / 2, "<li> %s: %s\n", 
                    key[i], cp_http_request_get_header(request, key[i]));
#else
            sprintf(sbuf, "<li> %s: %s\n", 
                    key[i], cp_http_request_get_header(request, key[i]));
#endif /* HAVE_SNPRINTF */
            strlcat(buf, sbuf, BUFLEN);
        }
        if (key) free(key);
        strlcat(buf, "</ul>\n", BUFLEN);
    }
    
    if (request->cookie)
    {
        int n = cp_vector_size(request->cookie);
        strlcat(buf, "<hr>\n<h3> cookies: </h3>\n<ul>\n", BUFLEN);
        for (i = 0; i < n; i++)
        {
#ifdef HAVE_SNPRINTF
            snprintf(sbuf, BUFLEN / 2, "<li> %s\n", (char *) cp_vector_element_at(request->cookie, i));
#else
            sprintf(sbuf, "<li> %s\n", (char *) cp_vector_element_at(request->cookie, i));
#endif /* HAVE_SNPRINTF */
            strlcat(buf, sbuf, BUFLEN);
        }
        strlcat(buf, "</ul>\n", BUFLEN);
    }
        
    if (request->prm)
    {
        strlcat(buf, "<hr>\n<h3> cgi variables: </h3>\n<ul>\n", BUFLEN);
        key = (char **) cp_hashtable_get_keys(request->prm);
        for (i = 0; i < cp_hashtable_count(request->prm); i++)
        {
#ifdef HAVE_SNPRINTF
            snprintf(sbuf, BUFLEN / 2, "<li> %s: %s\n", 
                    key[i], cp_http_request_get_parameter(request, key[i]));
#else
            sprintf(sbuf, "<li> %s: %s\n", 
                    key[i], cp_http_request_get_parameter(request, key[i]));
#endif /* HAVE_SNPRINTF */
            strlcat(buf, sbuf, BUFLEN);
        }
        if (key) free(key);
        strlcat(buf, "</ul>\n", BUFLEN);
    }

    strlcat(buf, "<hr>\nfree image: <img src=\"/img/free_image.jpg\">\n", BUFLEN);
    strlcat(buf, "</body>\n</html>\n", BUFLEN);
    response->body = strdup(buf);
    
    DEBUGMSG("hitcount_service: dumping request");
    cp_http_request_dump(request);

    return HTTP_CONNECTION_POLICY_KEEP_ALIVE;
}

void cpsvc_signal_handler(int sig)
{
    switch (sig)
    {
        case SIGINT:
            DEBUGMSG("testhttpsrv: SIGINT - stopping");
            cp_socket_stop_all();
            break;

        case SIGTERM:
            DEBUGMSG("testhttpsrv: SIGTERM - stopping");
            cp_socket_stop_all();
            break;
    }
}

int main(int argc, char *argv[])
{
    int rc = 0;
    struct sigaction act;
    cp_httpsocket *sock;

    cp_http_service *hitcount_svc = 
        cp_http_service_create("hitcount", "/test", hitcount_service);
    
    process_cmdline(argc, argv);

    act.sa_handler = cpsvc_signal_handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;

    cp_log_init("testhttpsrv.log", 0);
    cp_http_init();

    /* override default http signal handler for stopping */
    sigaction(SIGINT, &act, NULL);
    sigaction(SIGTERM, &act, NULL);

    cp_info("%s: starting", argv[0]);

    if ((rc = init_file_service(mimetypes_filename, document_root)))
    {
        cp_error(rc, "%s: can\'t start", argv[0]);
        goto DONE;
    }
        
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
    cp_httpsocket_register_service(sock, hitcount_svc);
    if (sock)
    {
        cp_info("%s: cp_httpsocket server starting on port %d", argv[0], port);
        cp_httpsocket_listen(sock);
        cp_info("%s: cp_httpsocket server on port %d: stopping", argv[0], port);
        cp_httpsocket_delete(sock);
    }

DONE:
    stop_file_service();
    cp_http_shutdown();
    cp_info("done");
    cp_log_close();

    return rc;
}

