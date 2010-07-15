#include <ctype.h>
#include <stdlib.h>
#include "cprops/config.h"
#include "cprops/util.h"
#include "file.h"

#define LINELEN 0x200

//static int port = 5000;

#define DEFAULT_DOCUMENT_ROOT "."
#define DEFAULT_MIME_TYPES_DEFINITION_FILE "mime.types"

static char *document_root = DEFAULT_DOCUMENT_ROOT;
//static char *mimetypes_filename = DEFAULT_MIME_TYPES_DEFINITION_FILE;
static cp_hashtable *mimemap;
static cp_hashtable *ext_svc = NULL;

void *register_ext_svc(char *ext, cp_http_service *svc)
{
	if (ext_svc == NULL)
		ext_svc = 
			cp_hashtable_create_by_option(COLLECTION_MODE_DEEP, 
										  10, 
										  cp_hash_string, 
										  cp_hash_compare_string, 
										  NULL, NULL, NULL, 
										  (cp_destructor_fn)
										      cp_http_service_delete);

	return cp_hashtable_put(ext_svc, ext, svc);
}
	
cp_http_service *get_ext_svc(char *ext)
{
	cp_http_service *svc = NULL;

	if (ext_svc)
		svc = cp_hashtable_get(ext_svc, ext);

	return svc;
}

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

void stop_file_service(void *dummy)
{
	if (mimemap)
	{
		cp_hashtable_destroy(mimemap);
		mimemap = NULL;
	}

	if (ext_svc)
	{
		cp_hashtable_destroy(ext_svc);
		ext_svc = NULL;
	}
}

int init_file_service(char *mimetypes_filename, char *doc_path)
{
	int rc = 0;

	if ((rc = checkdir(doc_path)))
		cp_fatal(rc, "can\'t open document root at [%s]", doc_path);

	if ((rc = load_mime_types(mimetypes_filename)))
		cp_fatal(rc, "can\'t load mime types from [%s], sorry", 
				 mimetypes_filename); 

	document_root = doc_path;

	cp_http_add_shutdown_callback(stop_file_service, NULL);
	
	return rc;
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
	{
		cp_http_service *alt;
		ext++;
		alt = get_ext_svc(ext);
		if (alt)
		{
#ifdef __TRACE__
			DEBUGMSG("[%s] file - delegating to %s service", ext, alt->name);
#endif
			return (*alt->service)(request, response);
		}
		/* else */
		cp_http_response_set_content_type_string(response, 
				                                 cp_hashtable_get(mimemap, ext));
	}

	/* check len, avoid buffer overrun */
	uri_len = strlen(request->uri);
	if (uri_len + strlen(document_root) >= PATHLEN)
	{
		cp_http_response_set_content_type(response, HTML);
		cp_http_response_set_status(response, HTTP_404_NOT_FOUND);
		response->body = strdup(HTTP404_PAGE);
		return HTTP_CONNECTION_POLICY_CLOSE;
	}
		
#ifdef CP_HAS_SNPRINTF
	snprintf(path, PATHLEN, "%s%s", document_root, request->uri);
#else
	sprintf(path, "%s%s", document_root, request->uri);
#endif /* CP_HAS_SNPRINTF */
	if (path[strlen(path) - 1] == '/') 
	{
#ifdef CP_HAS_STRLCAT
		strlcat(path, "index.html", PATHLEN);
#else
		strcat(path, "index.html");
#endif /* CP_HAS_STRLCAT */
		response->content_type = HTML;
	}

	fp = fopen(path, "rb");
	if (fp == NULL)
	{
		cp_http_response_set_content_type(response, HTML);
		cp_http_response_set_status(response, HTTP_404_NOT_FOUND);
#ifdef CP_HAS_SNPRINTF
		snprintf(buf, FBUFSIZE, HTTP404_PAGE_uri, request->uri);
#else
		sprintf(buf, HTTP404_PAGE_uri, request->uri);
#endif /* CP_HAS_SNPRINTF */
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

