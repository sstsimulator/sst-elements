#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "cprops/collection.h"
#include "cprops/hashtable.h"
#include "cprops/vector.h"
#include "cprops/http.h"
#include "cprops/util.h"
#include "cprops/str.h"
#include "cpsp.h"

#if defined(unix) || defined(__unix__) || defined(__MACH__)
#include <dlfcn.h>
#include <unistd.h>
#define PATH_SEPARATOR '/'
#define SHELL_SCRIPT_EXTENSION ".sh"
#endif /* unix */

#if defined(unix) || defined (__unix__)
#define SO_EXTENSION ".so"
#define SO_TERM "shared object"
#endif /* unix */

#ifdef __MACH__
#define SO_EXTENSION ".dylib"
#define SO_TERM "dynamic library"
#endif /* __MACH__ */

#ifdef _WINDOWS
#define PATH_SEPARATOR '\\'
#define SO_EXTENSION ".dll"
#define SHELL_SCRIPT_EXTENSION ".bat"
#define SO_TERM "DLL"
#endif /* _WINDOWS */

static cp_hashtable *cpsp_svc_lookup = NULL;
static char *cpsp_document_root = NULL;
static char *cpsp_bin_path = NULL;
static cp_vector *shutdown_hook = NULL;


void *cpsp_add_shutdown_callback(void (*cb)(void *), void *prm)
{
	cp_wrap *wrap;
	
	if (shutdown_hook == NULL)
		shutdown_hook = cp_vector_create(1);
	
	wrap = cp_wrap_new(prm, cb);

	cp_vector_add_element(shutdown_hook, wrap);

	return wrap;
}

void cpsp_shutdown(void *dummy)
{
	if (cpsp_svc_lookup)
	{
		cp_hashtable_destroy(cpsp_svc_lookup);
//		cp_hashtable_destroy_custom(cpsp_svc_lookup, free, 
//									(cp_destructor_fn) cpsp_rec_dispose);
		cpsp_svc_lookup = NULL;
	}

	if (shutdown_hook)
	{
		cp_vector_destroy_custom(shutdown_hook, 
				                 (cp_destructor_fn) cp_wrap_delete);
		shutdown_hook = NULL;
	}

	if (cpsp_bin_path)
	{
		free(cpsp_bin_path);
		cpsp_bin_path = NULL;
	}
}

int set_bin_path(char *path)
{
	int rc = 0;
	char curr[0x400];
	char bindir[0x400];
#ifdef _WINDOWS
	char envpath[0x400];
	char *penvpath;
#endif /* _WINDOWS */
	if (getcwd(curr, 0x400) == NULL) return errno;
	if ((rc = chdir(path))) return rc;
	if (getcwd(bindir, 0x400) == NULL) return errno;
	if ((rc = chdir(curr))) return rc;

#ifdef _WINDOWS
	sprintf(envpath, "CPSP_BIN=%s", bindir);
	_putenv(envpath);

	penvpath = getenv("PATH");
	if (penvpath == NULL) penvpath = "";
	sprintf(envpath, "PATH=%s;%s", penvpath, bindir);
	_putenv(envpath);
#endif /* _WINDOWS */

	cpsp_bin_path = strdup(bindir);

	return rc;
}

void cpsp_init(int separate, char *bin_path, char *doc_root)
{
//	if (separate) 
//		chdir(bin_path);
//	else
		set_bin_path(bin_path);
#if 0
	{
		cp_string *bp;
		int plen = strlen(bin_path);

		
		
		bp = cp_string_create(bin_path, plen);
		if (bin_path[plen - 1] != PATH_SEPARATOR)
			cp_string_cstrcat(bp, "/");
		cpsp_bin_path = cp_string_tocstr(bp);
cp_dump(LOG_LEVEL_DEBUG, bp);
		free(bp);
	}
#endif
	cpsp_document_root = doc_root;
		
	if (!separate)
		cp_http_add_shutdown_callback(NULL, cpsp_shutdown);
}

char *cpsp_err_fmt = 
	"<html>\n"
	"<head>\n"
	"<title> 500 INTERNAL SERVER ERROR </title>\n"
	"</head>\n"
	"<body>\n"
	"<h1> 500 INTERNAL SERVER ERROR </h1>\n"
	"%s could not be executed.\n"
	"</body>\n"
	"</html>";

cpsp_rec *cpsp_rec_new(char *path)
{
	cpsp_rec *rec = calloc(1, sizeof(cpsp_rec));
	rec->path = strdup(path);
	return rec;
}

void cpsp_rec_dispose(cpsp_rec *rec)
{
	if (rec)
	{
		if (rec->lib) /* unloading cpsp .so - perform finalization */
		{
			void (*dtr)() = (void (*)()) dlsym(rec->lib, "cpsp_destroy");
			if (dtr) 
			{
#ifdef __TRACE__
				DEBUGMSG("[%s] - calling cpsp_destroy()", rec->path);
#endif
				(*dtr)();
			}
#ifdef __TRACE
			else
				DEBUGMSG("[%s] - no cpsp_destroy() defined", rec->path);
#endif
		
			dlclose(rec->lib);
		}
		if (rec->path) free(rec->path);
		free(rec);
	}
}

int cpsp_gen(cp_http_request *request, cp_http_response *response)
{
	char src_path[0x400];
	long src_time, so_time;
	cpsp_rec *desc;
	char cmd[0x450];
	char so_path[0x400];
	char *so_ext;
	int rc = 0;
	int policy = HTTP_CONNECTION_POLICY_KEEP_ALIVE;

	if (cpsp_svc_lookup == NULL)
		cpsp_svc_lookup = 
			cp_hashtable_create_by_option(COLLECTION_MODE_COPY | 
					                      COLLECTION_MODE_DEEP, 10, 
										  cp_hash_string, 
										  cp_hash_compare_string, 
										  (cp_copy_fn) strdup, free, NULL, 
										  (cp_destructor_fn) cpsp_rec_dispose);

#ifdef CP_HAS_SNPRINTF
	snprintf(src_path, 0x400, "%s%s", cpsp_document_root, request->uri);
#else
	sprintf(src_path, "%s%s", cpsp_document_root, request->uri);
#endif /* CP_HAS_SNPRINTF */
#ifdef _WINDOWS
	replace_char(src_path, '/', '\\');
#endif /* _WINDOWS */
#ifdef __TRACE__
	DEBUGMSG("cpsp: serving [%s]", src_path);
#endif /* __TRACE__ */

	src_time = last_change_time(src_path);
	if (src_time == -1) 
	{
		cp_error(CP_FILE_NOT_FOUND, "%s: file not found", src_path);
		rc = -1;
		goto DONE;
	}

	so_ext = strrchr(src_path, PATH_SEPARATOR);
	if (so_ext)
	{
		*so_ext = '\0';
#ifdef CP_HAS_SNPRINTF
		snprintf(so_path, 0x400, "%s%c", src_path, PATH_SEPARATOR);
#else
		sprintf(so_path, "%s%c", src_path, PATH_SEPARATOR);
#endif /* CP_HAS_SNPRINTF */
		*so_ext = PATH_SEPARATOR;
#ifdef CP_HAS_STRLCAT
		strlcat(so_path, "_cpsp_", 0x400);
#else
		strcat(so_path, "_cpsp_");
#endif /* CP_HAS_STRLCAT */
		so_ext++;
#ifdef CP_HAS_STRLCAT
		strlcat(so_path, so_ext, 0x400);
#else
		strcat(so_path, so_ext);
#endif /* CP_HAS_STRLCAT */
	}
	else
	{
#ifdef CP_HAS_STRLCPY
		strlcpy(so_path, "_cpsp_", 0x400);
#else
		strcpy(so_path, "_cpsp_");
#endif /* CP_HAS_STRLCPY */
#ifdef CP_HAS_STRLCAT
		strlcat(so_path, src_path, 0x400);
#else
		strcat(so_path, src_path);
#endif /* CP_HAS_STRLCAT */
	}
	so_ext = strrchr(so_path, '.');
	if (so_ext) 
	{
		*so_ext = '\0';
#ifdef CP_HAS_STRLCPY
		strlcpy(so_ext, SO_EXTENSION, 0x400 - (so_ext - so_path));
#else
		strcpy(so_ext, SO_EXTENSION);
#endif /* CP_HAS_STRLCPY */
	}

	so_time = last_change_time(so_path);
	if (so_time < src_time)
	{
#ifdef CP_HAS_SNPRINTF
		if (cpsp_bin_path)
			snprintf(cmd, 0x450, "%s%ccpsp-gen%s %s %s", 
					cpsp_bin_path, PATH_SEPARATOR, SHELL_SCRIPT_EXTENSION, 
					src_path, cpsp_bin_path);
		else
			snprintf(cmd, 0x450, ".%ccpsp-gen%s %s", PATH_SEPARATOR, 
					SHELL_SCRIPT_EXTENSION, src_path);
#else
		if (cpsp_bin_path)
			sprintf(cmd, "%s%ccpsp-gen%s %s %s", 
					cpsp_bin_path, PATH_SEPARATOR, SHELL_SCRIPT_EXTENSION, 
					src_path, cpsp_bin_path);
		else
			sprintf(cmd, ".%ccpsp-gen%s %s", PATH_SEPARATOR, 
					SHELL_SCRIPT_EXTENSION, src_path);
#endif /* CP_HAS_SNPRINTF */
#ifdef __TRACE__
		DEBUGMSG("cpsp: executing [%s]", cmd);
#endif

#ifdef _WINDOWS
		/* on windows, you have to release the DLL before you rewrite it */
		desc = cp_hashtable_get(cpsp_svc_lookup, src_path);
		if (desc && desc->lib) 
		{
			dlclose(desc->lib);
			desc->svc = NULL;
		}
#endif /* _WINDOWS */

		rc = system(cmd);
		if (rc) 
		{
			cp_error(CP_COMPILATION_FAILURE, "%s: error (%d)", cmd, rc);
			goto DONE;
		}
	}

	if ((desc = cp_hashtable_get(cpsp_svc_lookup, src_path)) == NULL)
	{
		desc = cpsp_rec_new(src_path);
		cp_hashtable_put(cpsp_svc_lookup, src_path, desc);
	}

#if defined(unix) || defined(__unix__) || defined(__MACH__)
	if (so_time < src_time)
	{
		if (desc->lib) 
		{
			dlclose(desc->lib);
			desc->svc = NULL;
		}
	}
#endif /* unix */

	if (desc->svc == NULL)
	{
		void (*init_fn)();

		desc->lib = dlopen(so_path, RTLD_NOW);
		if (desc->lib == NULL) 
		{
			char *dlerr = (char *) dlerror();
			if (dlerr == NULL) dlerr = "unspecified error";
			rc = -1;
			cp_hashtable_remove(cpsp_svc_lookup, src_path);
			cp_error(CP_LOADLIB_FAILED, 
					 "can\'t load %s [%s]: %s", SO_TERM, so_path, dlerr);
			goto DONE;
		}

		desc->svc = (int(*)(cp_http_request *, cp_http_response *))
			dlsym(desc->lib, "cpsp_service_function");
		if (desc->svc == NULL)
		{
			char *dlerr = (char *) dlerror();
			rc = errno;
			cp_error(CP_LOADFN_FAILED, 
					 "can\'t link cpsp_service_function(): %s", dlerr);
			cp_hashtable_remove(cpsp_svc_lookup, src_path);
			goto DONE;
		}

		/* cpsp initialization function */
		init_fn = (void (*)()) dlsym(desc->lib, "cpsp_init");
		if (init_fn)
		{
#ifdef __TRACE__
			DEBUGMSG("[%s] - calling cpsp_init()", so_path);
#endif /* __TRACE__ */
			(*init_fn)();
		}
	}

	policy = (*desc->svc)(request, response);

DONE:
	if (rc)
	{
		char err[0x400];
#ifdef CP_HAS_SNPRINTF
		snprintf(err, 0x400, cpsp_err_fmt, request->uri);
#else
		sprintf(err, cpsp_err_fmt, request->uri);
#endif /* CP_HAS_SNPRINTF */
		cp_http_response_set_content_type(response, HTML);
		cp_http_response_set_status(response, HTTP_500_INTERNAL_SERVER_ERROR);
		cp_http_response_set_body(response, err);
		return HTTP_CONNECTION_POLICY_CLOSE;
	}

	return policy;
}

