#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#if defined(unix) || defined(__unix__) || defined(__MACH__)
#include <unistd.h>
#endif

#include "config.h"
#ifdef HAVE_REGEX_H
#include <regex.h>
#else
#ifdef HAVE_PCREPOSIX_H
#include <pcreposix.h>
#endif
#endif /* HAVE_REGEX_H */
#ifdef HAVE_DIRENT_H
#include <dirent.h>
#endif
#include <errno.h>
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

#include <sys/types.h>
#include <sys/stat.h>

#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif

#if defined(__OpenBSD__) || defined(__NetBSD__) || defined(__FreeBSD__) || defined(__APPLE__)
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#endif

#ifdef _WINDOWS
#include <Winsock2.h>
#include <WS2SPI.H>
#include <Windows.h>
#endif

#include "util.h"
#include "log.h"
#include "common.h"

#ifndef HAVE_STRDUP
char *strdup(char *src)
{
	char *dst = NULL;

    if (src != NULL) // || *src == '\0') 
	{
	    int len = strlen(src);
    	dst = (char *) malloc((len + 1) * sizeof(char));
	    cp_assert(*dst != NULL);

		memcpy(*dst, src, len);
		dst[len] = '\0';
	}

    return dst;
}
#endif /* no strdup */

#ifndef HAVE_STRNDUP
char *strndup(char *src, int maxlen)
{
    int len;
	char *p;
	char *dst = NULL;

	if (maxlen < 0)
	{
		cp_error(CP_INVALID_VALUE, "negative string length requested");
		errno = EINVAL;
		return NULL;
	}

    if (src != NULL) 
	{
		/* null termination not guaranteed - can't use strlen */ 
		for (p = src; *p && p - src < maxlen; p++); 
		len = p - src;
		if (len > maxlen) len = maxlen; 
		dst = (char *) malloc((len + 1) * sizeof(char));
		if (dst == NULL)
		{
			errno = ENOMEM;
			return NULL;
		}

		memcpy(dst, src, len); 
		dst[len] = '\0';
	}

    return dst;
}
#endif /* no strndup */

#ifndef HAVE_STRCASECMP
#ifndef _WINDOWS
int strcasecmp(const char *a, const char *b)
{
	while (*a && *b && tolower(*a) == tolower(*b)) 
	{
		a++;
		b++;
	}

	return *a - *b;
}
#endif
#endif /* no strcasecmp */

#ifndef HAVE_STRNCASECMP
#ifndef _WINDOWS
int strncasecmp(const char *a, const char *b, size_t len)
{
	while (len-- > 0 && *a && *b && tolower(*a) == tolower(*b)) 
	{
		a++;
		b++;
	}

	return *a - *b;
}
#endif
#endif /* no strncasecmp */

#ifndef HAVE_GETTIMEOFDAY
#ifdef _WINDOWS

/* the following implementation of gettimeofday for windows is taken from
 * https://projects.cecs.pdx.edu/~brael/j2++/index.cgi/browser/trunk/subst/gettimeofday.c/?rev=46
 * 
 * Here is the copyright notice for this function: 
 * 
 * Copyright (c) 2003 SRA, Inc. 
 * Copyright (c) 2003 SKC, Inc. 
 * 
 * Permission to use, copy, modify, and distribute this software and 
 * its documentation for any purpose, without fee, and without a 
 * written agreement is hereby granted, provided that the above 
 * copyright notice and this paragraph and the following two 
 * paragraphs appear in all copies. 
 * 
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE TO ANY PARTY FOR DIRECT, 
 * INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING 
 * LOST PROFITS, ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS 
 * DOCUMENTATION, EVEN IF THE UNIVERSITY OF CALIFORNIA HAS BEEN ADVISED 
 * OF THE POSSIBILITY OF SUCH DAMAGE. 
 * 
 * THE AUTHOR SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING, BUT NOT 
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
 * A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS 
 * IS" BASIS, AND THE AUTHOR HAS NO OBLIGATIONS TO PROVIDE MAINTENANCE, 
 * SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS. 
 */ 

/* FILETIME of Jan 1 1970 00:00:00. */ 
static const unsigned __int64 epoch = 116444736000000000L; 
 
/* 
 * timezone information is stored outside the kernel so tzp isn't used anymore. 
 */ 
 
int 
gettimeofday(struct timeval * tp, struct timezone * tzp) 
{ 
        FILETIME        file_time; 
        SYSTEMTIME      system_time; 
        ULARGE_INTEGER ularge; 
 
        GetSystemTime(&system_time); 
        SystemTimeToFileTime(&system_time, &file_time); 
        ularge.LowPart = file_time.dwLowDateTime; 
        ularge.HighPart = file_time.dwHighDateTime; 
 
        tp->tv_sec = (long) ((ularge.QuadPart - epoch) / 10000000L); 
        tp->tv_usec = (long) (system_time.wMilliseconds * 1000); 
 
        return 0; 
} 
#endif /* _WINDOWS */
#endif /* no gettimeofday */

/* cp_sleep - a platform independent function to sleep n seconds */
int cp_sleep(int sec)
{
	int rc = -1;
#if defined(unix) || defined(__unix__) || defined(__MACH__)
	struct timeval delay;
	delay.tv_sec = sec; /* run session cleanup every 15 minutes */
	delay.tv_usec = 0;
	rc = select(0, NULL, NULL, NULL, &delay); 
#else
#ifdef _WINDOWS
	rc = 0;
	Sleep(sec * 1000);
#endif /* _WINDOWS */
#endif /* unix */
	return rc;
}

char *hex_url_encode(char *hex)
{
	int len = strlen(hex) * 3 / 2 + 1;
	char *res = (char *) malloc(len * sizeof(char));
	char *idx = res;

	while (*hex)
	{
		*idx++ = '%';
		*idx++ = *hex++;
		if (*hex == 0) break;
		*idx++ = *hex++;
	}

	*idx = '\0';

	return res;
}

char *strnchr(char *str, char ch, int len)
{
	char *res = NULL;

	if (len < 0)
	{
		cp_error(CP_INVALID_VALUE, "negative string length requested");
		errno = EINVAL;
		return NULL;
	}

	while (len-- > 0)
	{
		if (*str == ch)
		{
			res = str;
			break;
		}
		if (*str++ == '\0')
			break;
	}

	return res;
}


char *str_trim_cpy(char *dst, char *src)
{
    char *top;
    char *last;
    top = dst;
    while (isspace(*src)) src++;
    last = dst;
    while ((*dst = *src)) {
        dst++;
        if (!isspace(*src)) last = dst;
        src++;
    }

    *last = '\0';
    return top;
}

int parse_boolean(char *value)
{
    return value != NULL && (*value == 't' || *value =='T');
}

long get_timestamp()
{
    return time(NULL);
}

/**
 * Static empty cp_string.
 */
static char *empty_cp_string = "";

char *dbfmt(char *str)
{
    return str ? str : empty_cp_string;
}

unsigned long get_current_ip()
{
    char hostname[256];
    int rc;

    rc = gethostname(hostname, 256);
    return rc ? 0 : get_host_ip(hostname);
}

#ifndef HAVE_GETHOSTBYNAME
int h_errno;
struct hostent* gethostbyname(const char*name)
{
    h_errno = NO_RECOVERY;
    return NULL;
}
#endif

unsigned long get_host_ip(char *hostname)
{
#ifdef HAVE_GETADDRINFO
    struct addrinfo hints, *res, *res0;
    int rc;
    unsigned long ip = 0;
    unsigned long addr = 0;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    rc = getaddrinfo(hostname, "http", &hints, &res0);
    for (res = res0; res; res = res->ai_next) {
        addr = ((struct sockaddr_in *) res->ai_addr)->sin_addr.s_addr;
        if (ip && (addr & 0xff) == 0x7f) continue;
        ip = addr;
    }

    return ip;
#else
	unsigned long ulIP;	
	struct hostent* phostent;	

	phostent = gethostbyname(hostname);	
	if (phostent == NULL) return -1;
#ifdef _WINDOWS
	ulIP = *(DWORD*)(*phostent->h_addr_list);
#else
	ulIP = *(unsigned long *)(*phostent->h_addr_list);
#endif

	return ulIP;
#endif
}

char *ip_to_string(unsigned long ip, char *buf, size_t len)
{
#ifdef HAVE_SNPRINTF
    snprintf(buf, len, "%ld.%ld.%ld.%ld", ip & 0xff, (ip >> 8) & 0xff
           , (ip >> 16) & 0xff, (ip >> 24) & 0xff);
#else
    sprintf(buf, "%ld.%ld.%ld.%ld", ip & 0xff, (ip >> 8) & 0xff
           , (ip >> 16) & 0xff, (ip >> 24) & 0xff);
#endif /* HAVE_SNPRINTF */
    return buf;
}

char *regex_compilation_error(int rc)
{
	char *msg = NULL;

	switch (rc)
	{
		case REG_BADRPT:
			msg = "Invalid use of repetition operators such as using `*' as the first character."; 
			break;

		case REG_BADBR: 
			msg = "Invalid use of back reference operator.";
			break;

		case REG_EBRACE:
			msg = "Un-matched brace interval operators.";
			break;

		case REG_EBRACK:
			msg = "Un-matched bracket list operators.";
			break;

		case REG_ERANGE:
			msg = "Invalid  use  of  the  range operator, eg. the ending point of the range occurs prior to the starting point.";
			break;

   		case REG_ECTYPE:
			msg = "Unknown character class name.";
			break;

   		case REG_ECOLLATE:
			msg = "Invalid collating element.";
			break;

		case REG_EPAREN:
			msg = "Un-matched parenthesis group operators.";
			break;

		case REG_ESUBREG:
			msg = "Invalid back reference to a subexpression.";
			break;

#ifdef linux
		case REG_EEND:
			msg = "Non specific error.";
			break;
#endif

		case REG_EESCAPE:
			msg = "Trailing backslash.";
			break;

		case REG_BADPAT:
			msg = "Invalid use of pattern operators such as group or list.";
			break;

#ifdef linux
		case REG_ESIZE:
			msg = "Compiled regular expression requires a pattern buffer larger than  64Kb.";
			break;
#endif

		case REG_ESPACE:
			msg = "The regex routines ran out of memory.";
			break;
	}

	return msg;
}

char *stat_error_fmt(int err)
{
	char *fmt = "%s: resolving %s: unknown stat error\n";
	switch (err)
	{
		case ENOENT: 
			fmt = "%s: A component of the path \'%s\' does not exist\n";
			break;

		case ENOTDIR: 
			fmt = "%s: A component of the path \'%s\' is not a directory\n";
			break;

#ifdef ELOOP
		case ELOOP:
			fmt = "%s: Too many symbolic links encountered while "
				  "traversing the path %s\n";
			break;
#endif

		case EFAULT:
			fmt = "%s: Bad address resolving %s\n";
			break;

		case EACCES:
			fmt = "%s: %s: Permission denied.\n";
			break;

		case ENOMEM: 
			fmt = "%s: resolving %s: Out of memory\n";
			break;

		case ENAMETOOLONG:
			fmt = "%s: resolving %s: File name too long.\n";
			break;
	}

	return fmt;
}

time_t last_change_time(char *path)
{
#ifdef HAVE_STAT
	struct stat buf;

	if (stat(path, &buf)) return -1;

	return buf.st_mtime;
#else
#ifdef _WINDOWS
	struct _stat buf;
	if (_stat(path, &buf)) return -1;
	return buf.st_mtime;
#endif
#endif /* HAVE_STAT */
}

/**
 * check if the directory specified by path exists
 */
int checkdir(char *path)
{
#ifdef HAVE_DIRENT_H
	int rc = 0;
	DIR *dir = opendir(path);

	/*  opendir may set:
	 *  EACCES  Permission denied.
     *  EMFILE  Too many file descriptors in use by process.
     *  ENFILE  Too many files are currently open in the system.
     *  ENOENT  Directory does not exist, or name is an empty string.
     *  ENOMEM  Insufficient memory to complete the operation.
     *  ENOTDIR name is not a directory.
	 */
	if (dir == NULL)
	{
		int rc = errno;
		char *err = strerror(rc);
		cp_error(rc, "%s: opendir failed - %s", path, err);
	}
	else
		closedir(dir);

	return rc;
#else
#ifdef _WINDOWS
	struct _stat buf;
	return _stat(path, &buf) != 0;
#endif
#endif /* HAVE_DIRENT_H */
}

/**
 * generates a 16 character id based on the current time - buffer must be at
 * least 17 bytes long. 
 */
void gen_id_str(char *buf)
{
	struct timeval tm;
	gettimeofday(&tm, NULL);
	gen_tm_id_str(buf, &tm);
}

static int id_factor[] = {   2,   3,   5,   7,  11, 
                            13,  17,  19,  23,  29, 
                            31,  37,  41,  43,  47, 
                            53,  59,  61,  67,  71,
                            73,  79,  83,  89,  97, 
                           101, 103, 107, 109, 113,
                           127, 131};

/**
 * generates a 16 character id based on the given timeval - buffer must be at
 * least 17 bytes long
 */
void gen_tm_id_str(char *buf, struct timeval *tm)
{
	int i;
	unsigned long t = 0;
	unsigned long v = 0;
	
	memset(buf, 0, 17);
	t = tm->tv_sec;
	for (i = 0; i < 32; i++)
	{
		v |= (t % id_factor[i]);
		v <<= 1;
		buf[i % 16] = (unsigned char)
			('a' + (((((t * (i + 1)) % 26) | (buf[i % 16] % 'a')) ^ (unsigned char)(v % 26))) % 26);
	}
	t = tm->tv_usec;
	for (i = 0; i < 32; i++)
	{
		v ^= (t % id_factor[i]);
		v <<= 1;
		buf[i % 16] = (unsigned char) 
			('a' + (((((t * (i + 1)) % 26) | (buf[i % 16] % 'a')) ^ (unsigned char)(v % 26))) % 26);
	}
}

/** duplicate an integer */
int *intdup(int *src)
{
	int *res = malloc(sizeof(int));
	if (res) *res = *src;
	return res;
}

/** duplicate a long integer */
long *longdup(long *src)
{
	long *res = malloc(sizeof(long));
	if (res) *res = *src;
	return res;
}

/** duplicate a floating point value */
float *floatdup(float *src)
{
	float *res = malloc(sizeof(float));
	if (res) *res = *src;
	return res;
}

/** duplicate a double precision floating point value */
double *doubledup(double *src)
{
	double *res = malloc(sizeof(double));
	if (res) *res = *src;
	return res;
}

/** duplicate an array */
void *memdup(void *src, int len)
{
	void *res = malloc(len);
	if (res)
		memcpy(res, src, len);
	return res;
}

#define C2HEX(ch) ((ch) >= '0' && (ch) <= '9' ? (ch) - '0' : \
                   (ch) >= 'A' && (ch) <= 'F' ? (ch) - 'A' + 10 : \
                   (ch) >= 'a' && (ch) <= 'f' ? (ch) - 'a' + 10 : -1)

/** convert a hex string to an integer value */
int xtoi(char *p)
{
	int curr;
	int res = 0;

	while (*p)
	{
		curr = C2HEX(*p);
		if (curr == -1) break;
		res = res * 0x10 + curr;
		p++;
	}
	
	return res;
}

/** convert a hex string to a long value */
int xtol(char *p)
{
	int curr;
	long res = 0;

	while (*p)
	{
		curr = C2HEX(*p);
		if (curr == -1) break;
		res = res * 0x10 + curr;
		p++;
	}
	
	return res;
}

char *reverse_string(char *str)
{
    int len;
    char *res;
    char *src, *dst;

    if (str == NULL) return NULL;

    len = strlen(str);
    res = malloc(len * sizeof(char) + 1);
    if (res == NULL) return NULL;

    dst = res;
    src = &str[len -1];
    while (src > str)
        *dst++ = *src--;
    *dst = '\0';

    return res;
}


char *reverse_string_in_place(char *str)
{
    char *i, *f;
    char ch;
    int len = strlen(str);
    if (len == 0) return str;
    i = str;
    f = &str[len - 1];
    while (i < f)
    {
        ch = *i;
        *i = *f;
        *f = ch;
        i++;
        f--;
    }

    return str;
}

char *filter_string(char *str, char *letters)
{
    char *i;
    char *f;
    int len = strlen(str);

    i = str;
    while ((f = strpbrk(i, letters)))
    {
        i = f;
        while (*f && strchr(letters, *f)) f++;
        if (*f)
        {
            memmove(i, f, len - (f - str));
            len -= f - i;
            str[len] = '\0';
        }
        else
        {
            *i = '\0';
            len -= len - (i - str);
            break;
        }
    }

    return str;
}

char *to_lowercase(char *str)
{
	char *p;
	for (p = str; *p; p++)
		if (isupper(*p)) *p += 'a' - 'A';
	return str;
}

char *to_uppercase(char *str)
{
	char *p;
	for (p = str; *p; p++)
		if (islower(*p)) *p -= 'a' - 'A';
	return str;
}

#ifndef HAVE_GETOPT
CPROPS_DLL char *optarg;
static int opt_idx = 0;

int getopt(int argc, char *argv[], char *fmt)
{
	char *p;

	opt_idx++;
	if (opt_idx == argc) return -1;

	if ((p = strchr(fmt, argv[opt_idx][1])) != NULL)
	{
		if (p[1] == ':') optarg = argv[++opt_idx];
		return p[0];
	}

	return 0;
}
#endif /* HAVE_GETOPT */

#ifndef HAVE_INET_NTOP
#ifdef _WINDOWS
char *inet_ntop(int af, void *src, char *dst, size_t cnt)
{
	int rc;

	WSAPROTOCOL_INFO pinfo;
	memset(&pinfo, 0, sizeof(WSAPROTOCOL_INFO));
	pinfo.iAddressFamily = AF_INET;
	rc = WSAAddressToString(src, sizeof(struct sockaddr_in), &pinfo, dst, &cnt);
#ifdef DEBUG
	if (rc)
	{
		rc = WSAGetLastError();
		DEBUGMSG("WSAAddressToString: %d", rc);
	}
#endif /* DEBUG */
	return dst;
}

#if 0 //~~ check windows version - some platforms don't have getnameinfo
char *inet_ntop(int af, const void *src, char *dst, size_t cnt)
{
	if (af == AF_INET)
	{
		struct sockaddr_in in;
		memset(&in, 0, sizeof(in));
		in.sin_family = AF_INET;
		memcpy(&in.sin_addr, src, sizeof(struct in_addr));
		getnameinfo((struct sockaddr *)&in, sizeof(struct sockaddr_in), 
					dst, cnt, NULL, 0, 0); //NI_NUMERICHOST);
		return dst;
	}
	else if (af == AF_INET6)
	{
		struct sockaddr_in6 in;
		memset(&in, 0, sizeof(in));
		in.sin6_family = AF_INET6;
		memcpy(&in.sin6_addr, src, sizeof(struct in_addr6));
		getnameinfo((struct sockaddr *)&in, sizeof(struct sockaddr_in6), 
					dst, cnt, NULL, 0, NI_NUMERICHOST);
		return dst;
	}
	return NULL;
}
#endif //~~ platform check

#else
char *inet_ntop(int af, void *src, char*dst, size_t cnt)
{
    errno = EAFNOSUPPORT;
    return NULL;
}
#endif /* _WINDOWS */
#endif /* HAVE_INET_NTOP */

#ifndef HAVE_DLFCN_H
#ifdef _WINDOWS
void *dlopen(char *file, int mode)
{
	return LoadLibrary(file);
}

int dlclose(void *handle)
{
	return FreeLibrary(handle);
}

void *dlsym(void *handle, char *name)
{
	return GetProcAddress(handle, name);
}

char *dlerror()
{
	static char msg[0x400];
	DWORD err = GetLastError();
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 
			NULL, err, 0, msg, 0x400 - 1, NULL);
	return msg;
}

#endif /* _WINDOWS */
#endif /* HAVE_DLFCN_H */

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

int create_proc(char *path, 
				void *child_stdin, 
				void *child_stdout, 
				void *child_stderr,
				char **envp)
{
	TCHAR *szCmdline; 
	PROCESS_INFORMATION piProcInfo; 
	STARTUPINFO siStartInfo;
	BOOL bFuncRetn = FALSE; 
	char env[0x1000]; /* 4K of environment is enough for anyone */
 
	flat_env(env, envp);
// Set up members of the PROCESS_INFORMATION structure. 
	ZeroMemory( &piProcInfo, sizeof(PROCESS_INFORMATION) );
 
// Set up members of the STARTUPINFO structure. 
	ZeroMemory( &siStartInfo, sizeof(STARTUPINFO) );
	siStartInfo.cb = sizeof(STARTUPINFO); 
	siStartInfo.hStdError = child_stderr;
	siStartInfo.hStdOutput = child_stdout;
	siStartInfo.hStdInput = child_stdin;
	siStartInfo.dwFlags |= STARTF_USESTDHANDLES;
 
	szCmdline = (TCHAR *) malloc(strlen(path) + 3);
	sprintf(szCmdline, "\"%s\"", path);
    
// Create the child process. 
	bFuncRetn = CreateProcess(NULL, 
		szCmdline,     // command line 
		NULL,          // process security attributes 
		NULL,          // primary thread security attributes 
		TRUE,          // handles are inherited 
		0,             // creation flags 
		env,           // environment 
		NULL,          // use parent's current directory 
		&siStartInfo,  // STARTUPINFO pointer 
		&piProcInfo);  // receives PROCESS_INFORMATION 
   
	free(szCmdline);

	if (bFuncRetn == 0) return -1;

	CloseHandle(piProcInfo.hProcess);
	CloseHandle(piProcInfo.hThread);
	return bFuncRetn;
}

#endif /* _WINDOWS */

void replace_char(char *buf, char from, char to)
{
	while (*buf)
	{
		if (*buf == from) *buf = to;
		buf++;
	}
}

