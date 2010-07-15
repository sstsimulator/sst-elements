#ifndef _CP_UTIL_H
#define _CP_UTIL_H

#include "common.h"
#include "config.h"

#ifdef TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif
__BEGIN_DECLS

/**
 * assorted goodies
 */

#ifndef HAVE_STRLCAT
#define strlcat strncat
#endif /* HAVE_STRLCAT */

#ifndef HAVE_STRLCPY
#define strlcpy strncpy
#endif /* HAVE_STRLCPY */

#ifndef HAVE_SNPRINTF
#ifdef _WINDOWS
#define snprintf _snprintf
#define HAVE_SNPRINTF
#endif /* _WINDOWS */
#endif /* HAVE_SNPRINTF */

#ifndef HAVE_STRDUP
/**
 * Copies cp_string into newly allocated memory.
 *
 * @retval pointer to copied cp_string
 * @retval NULL if src was NULL
 */
CPROPS_DLL
char *strdup(char *src);
#endif /* missing strdup */

#ifndef HAVE_STRNDUP
/**
 * Copies up to maxlen characters of src cp_string into newly allocated memory.
 *
 * @retval pointer to copied cp_string
 * @retval NULL if src was NULL
 */
CPROPS_DLL
char *strndup(char *src, int maxlen);
#endif /* missing strndup */

#ifndef HAVE_STRCASECMP
#ifdef _WINDOWS
#define strcasecmp _stricmp
#else
int strcasecmp(const char *a, const char *b);
#endif
#endif /* HAVE_STRNCASECMP */

#ifndef HAVE_STRNCASECMP
#ifdef _WINDOWS
#define strncasecmp _strnicmp
#else
int strncasecmp(const char *a, const char *b, size_t len);
#endif
#endif /* HAVE_STRNCASECMP */

#ifndef HAVE_GETTIMEOFDAY
#ifdef _WINDOWS
CPROPS_DLL
int gettimeofday(struct timeval *res, struct timezone *tz);
#endif
#endif /* missing gettimeofday */

/* cp_sleep - a platform independent function to sleep n seconds */
CPROPS_DLL
int cp_sleep(int sec);

/**
 * @todo check for buffer overflow
 */
CPROPS_DLL
char *str_trim_cpy(char *dst, char *src);


/**
 * scans a cp_string for the first occurence of ch within the first len bytes at
 * most. searching for the null character will return a pointer to the 
 * terminating null. 
 */
CPROPS_DLL
char *strnchr(char *str, char ch, int len);


/**
 * Map character 't', 'T' to true.
 */
CPROPS_DLL
int parse_boolean(char *value);

/**
 * Map integer value to characters 't', 'f'.
 */
#define format_boolean(val) ((val) ? "t" : "f")


/**
 * Get the current time as long value (UNIX).
 */
CPROPS_DLL
long get_timestamp();

/**
 * Return the cp_string or if NULL an empty cp_string.
 */
CPROPS_DLL
char *dbfmt(char *str);


/**
 * Retrieves the ip-adress of the system where it is running on.
 */
CPROPS_DLL
unsigned long get_current_ip();

/**
 * Retrieve the ip-address of the system with the 'hostname'.
 */
CPROPS_DLL
unsigned long get_host_ip(char *hostname);

/**
 * Formats an ip-address (IPv4) with dot notation.
 */
CPROPS_DLL
char *ip_to_string(unsigned long ip, char *buf, size_t len);

/** 
 * converts hex to url encoded binary (eg ABCD => %AB%CD) and returns a newly
 * allocated cp_string
 */
CPROPS_DLL
char *hex_url_encode(char *hex);

/** 
 * return a static string coresponding to the posix regcomp error code given
 * as a parameter. the error descriptions come from the gnu man page. 
 */
CPROPS_DLL
char *regex_compilation_error(int rc);

/**
 * return a static string describing the stat return code given as a parameter.
 * the resulting error string contains 2 '%s' sequences - the first one could 
 * be used for the program name, function description etc, the second one 
 * for the stat()ed path.
 */
CPROPS_DLL
char *stat_error_fmt(int err);

/**
 * check if the directory specified by path exists
 */
CPROPS_DLL
int checkdir(char *path);

/**
 * generates a 16 character id based on the current time. The generated id 
 * followed by a terminating null character are written into buf.
 */
CPROPS_DLL
void gen_id_str(char *buf);

/**
 * generates a 16 character id based on the given time. The generated id 
 * followed by a terminating null character are written into buf.
 */
CPROPS_DLL
void gen_tm_id_str(char *buf, struct timeval *tm);

/**
 * get a timestamp for the last modification to the file designated by path
 */
CPROPS_DLL
time_t last_change_time(char *path);

/** duplicate an integer */
CPROPS_DLL
int *intdup(int *src);

/** duplicate a long integer */
CPROPS_DLL
long *longdup(long *src);

/** duplicate a floating point value */
CPROPS_DLL
float *floatdup(float *src);

/** duplicate a double precision floating point value */
CPROPS_DLL
double *doubledup(double *src);

/** duplicate an array */
CPROPS_DLL
void *memdup(void *src, int len);

/** convert a hex string to an integer value */
CPROPS_DLL
int xtoi(char *p);

/** convert a hex string to a long value */
CPROPS_DLL
int xtol(char *p);

/** return flipped string in a newly allocated buffer */
CPROPS_DLL
char *reverse_string(char *str);

/** flip a string in place */
CPROPS_DLL
char *reverse_string_in_place(char *str);

/** remove all occurrences of letters from str */
CPROPS_DLL
char *filter_string(char *str, char *letters);

/** convert string to lower case characters */
CPROPS_DLL
char *to_lowercase(char *str);

/** convert string to upper case characters */
CPROPS_DLL
char *to_uppercase(char *str);

#ifndef HAVE_GETOPT
extern CPROPS_DLL char *optarg;

CPROPS_DLL
int getopt(int argc, char *argv[], char *fmt);
#endif /* no getopt */

#ifndef HAVE_INET_NTOP
#ifdef _WINDOWS

CPROPS_DLL
char *inet_ntop(int af, const void *src, char *dst, size_t cnt);
#endif /* _WINDOWS */
#endif /* HAVE_INET_NTOP */

#ifndef HAVE_DLFCN_H
#ifdef _WINDOWS
CPROPS_DLL
void *dlopen(char *file, int mode);

CPROPS_DLL
int dlclose(void *handle);

CPROPS_DLL
void *dlsym(void *handle, char *name);

CPROPS_DLL
char *dlerror();

/* none if this is actually supported by the win32 api, just define the symbols
 * so the build doesn't break. 
 */
#define RTLD_LAZY   0
#define RTLD_NOW    0
#define RTLD_LOCAL  0
#define RTLD_GLOBAL 0

#endif /* WINDOWS */
#endif /* HAVE_DLFCN_H */

#ifndef HAVE_GETCWD
#ifdef _WINDOWS
#include "direct.h"
#define getcwd _getcwd
#endif /* _WINDOWS */
#endif /* HAVE_GETCWD */

#ifdef _WINDOWS
/* convenience function to create a process under windows */
CPROPS_DLL
int create_proc(char *path, 
				void *child_stdin, 
				void *child_stdout, 
				void *child_stderr,
				char **envp);
#endif /* _WINDOWS */

CPROPS_DLL
void replace_char(char *buf, char from, char to);

CPROPS_DLL
char *ssl_err_inf(int err);

/* ----------------------------------------------------------------- */

__END_DECLS

/** @} */
#endif

