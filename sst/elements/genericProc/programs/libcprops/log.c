/** @{ */
/**
 * @file
 * libcprops logging facilities implementation
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
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
#include <errno.h>

#ifdef _WINDOWS
#include <Windows.h>
#endif

#include "log.h"
#include "util.h"
#include "common.h"
#include "hashtable.h"

static FILE *logfile = NULL;
static char *log_filename = NULL;
static int loglevel = 1;

static cp_hashtable *thread_id;
long thread_count;

#define PRECISE_TIME_FACTOR 1000

typedef struct _error_code_legend
{
	int code;
	char *msg;
} error_code_legend;

error_code_legend error_messages[] = 
{	
	{CP_MEMORY_ALLOCATION_FAILURE, "MEMORY ALLOCATION FAILURE"}, 
	{CP_INVALID_FUNCTION_POINTER, "INVALID FUNCTION POINTER"}, 
	{CP_THREAD_CREATION_FAILURE, "THREAD CREATION FAILURE"}, 
	{CP_INITIALIZATION_FAILURE, "INITIALIZATION FAILURE"}, 

	{CP_LOADLIB_FAILED, "LOADLIB FAILED"}, 
	{CP_LOADFN_FAILED, "LOADFN FAILED"}, 
	{CP_MODULE_NOT_LOADED, "MODULE NOT LOADED"}, 

	{CP_IO_ERROR, "IO ERROR"},
	{CP_OPEN_PORT_FAILED, "OPEN PORT FAILED"}, 
	{CP_HTTP_FETCH_FAILED, "HTTP FETCH FAILED"}, 
	{CP_INVALID_RESPONSE, "INVALID RESPONSE"}, 
    {CP_HTTP_EMPTY_REQUEST, "EMPTY HTTP REQUEST"},
    {CP_HTTP_INVALID_REQUEST_LINE, "INVALID HTTP REQUEST LINE"},
    {CP_HTTP_INVALID_STATUS_LINE, "INVALID HTTP STATUS LINE"},
    {CP_HTTP_UNKNOWN_REQUEST_TYPE, "UNKNOWN HTTP REQUEST TYPE"},
    {CP_HTTP_INVALID_URI, "INVALID URI"},
    {CP_HTTP_INVALID_URL, "INVALID URL"},
    {CP_HTTP_VERSION_NOT_SPECIFIED, "HTTP VERSION NOT SPECIFIED"},
    {CP_HTTP_1_1_HOST_NOT_SPECIFIED, "HTTP 1.1 HOST NOT SPECIFIED"},
    {CP_HTTP_INCORRECT_REQUEST_BODY_LENGTH, "INCORRECT HTTP REQUEST BODY LENGTH"},
    {CP_SSL_CTX_INITIALIZATION_ERROR, "SSL CONTEXT INITIALIZATION ERROR"},
    {CP_SSL_HANDSHAKE_FAILED, "SSL HANDSHAKE FAILED"},
    {CP_SSL_VERIFICATION_ERROR, "SSL VERIFICATION ERROR"},

	{CP_LOG_FILE_OPEN_FAILURE, "LOG FILE OPEN FAILURE"}, 
	{CP_LOG_NOT_OPEN, "LOG NOT OPEN"}, 

	{CP_INVALID_VALUE, "INVALID VALUE"},
	{CP_MISSING_PARAMETER, "MISSING PARAMETER"},
	{CP_BAD_PARAMETER_SET, "BAD PARAMETER SET"},
    {CP_ITEM_EXISTS, "ITEM EXISTS"},
    {CP_ITEM_DOES_NOT_EXIST, "ITEM DOES NOT EXIST"},
    {CP_UNHANDLED_SIGNAL, "UNHANDLED SIGNAL"},
    {CP_FILE_NOT_FOUND, "FILE NOT FOUND"},
	{CP_METHOD_NOT_IMPLEMENTED, "METHOD NOT IMPLEMENTED"},
	{CP_INVALID_FILE_OFFSET, "INVALID FILE OFFSET"},
	{CP_CORRUPT_FILE, "CORRUPT FILE"},
	{CP_CORRUPT_INDEX, "CORRUPT INDEX"},
	{CP_UNIQUE_INDEX_VIOLATION, "UNIQUE INDEX VIOLATION"},

	{CP_REGEX_COMPILATION_FAILURE, "INVALID REGULAR EXPRESSION"},
	{CP_COMPILATION_FAILURE, "COMPILATION FAILED"},

	{CP_DBMS_NO_DRIVER, "NO DRIVER"},
	{CP_DBMS_CONNECTION_FAILURE, "DBMS CONNECTION FAILED"},
	{CP_DBMS_QUERY_FAILED, "DBMS QUERY FAILED"},
	{CP_DBMS_CLIENT_ERROR, "DBMS CLIENT ERROR"},
	{CP_DBMS_STATEMENT_ERROR, "DBMS STATEMENT ERROR"}
}; 

static cp_hashtable *error_message_lookup;

static long get_thread_serial(long tno)
{
	long *num;
	
	if (thread_id == NULL) return 0;

	num = cp_hashtable_get(thread_id, &tno);

	if (num == NULL)
	{
		long *key;
		num = malloc(sizeof(long));
		if (num == NULL)
		{
			cp_error(CP_MEMORY_ALLOCATION_FAILURE, "can\'t allocate thread mapping number");
			return -1L;
		}
		key = malloc(sizeof(long));
		if (key == NULL)
		{
			cp_error(CP_MEMORY_ALLOCATION_FAILURE, "can\'t allocate thread mapping key");
			return -1L;
		}

		*num = ++thread_count;
		*key = tno;
		cp_hashtable_put(thread_id, key, num);
	}

	return *num;
}

#ifdef SIGHUP
void cp_log_default_signal_handler(int sig)
{
	if (sig == SIGHUP)
	{
		signal(SIGHUP, cp_log_default_signal_handler);
#ifdef __TRACE__
		DEBUGMSG("SIGHUP received - reopenning log file [%s]", 
				 log_filename ? log_filename : "NULL");
#endif
		cp_log_reopen();
	}
}
#endif /* SIGHUP */

static int log_closing = 0;

int cp_log_init(char *filename, int verbosity)
{
	int i, err_code_count;
	unsigned long thread_no;
#ifdef SIGHUP
	struct sigaction act;
#endif

	log_filename = strdup(filename);
	logfile = fopen(filename, "a+");
	if (logfile == NULL)
		return CP_LOG_FILE_OPEN_FAILURE;

	loglevel = verbosity;

	err_code_count = sizeof(error_messages) / sizeof(error_code_legend);
	error_message_lookup = 
		cp_hashtable_create_by_mode(COLLECTION_MODE_NOSYNC, 
				                    err_code_count * 2, 
									cp_hash_int, 
									cp_hash_compare_int);
	for (i = 0; i < err_code_count; i++)
	{
		error_code_legend *entry = &error_messages[i];
		cp_hashtable_put(error_message_lookup, &entry->code, entry->msg);
	}
	
	thread_id = 
		cp_hashtable_create_by_option(COLLECTION_MODE_DEEP, 10,
									  cp_hash_long, cp_hash_compare_long,
									  NULL, free, NULL, free);

	thread_count = 0;
//    thread_no = (unsigned long) pthread_self();
	thread_no = (unsigned long) cp_thread_self();
	get_thread_serial(thread_no); /* establish this as thread number one */

	log_closing = 0;

#ifdef SIGHUP
	act.sa_handler = cp_log_default_signal_handler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	sigaction(SIGHUP, &act, NULL);
#endif
//	signal(SIGHUP, cp_log_default_signal_handler);

	return 0;
}

int cp_log_reopen()
{
	int rc = 0;

	if (logfile)
	{
		fflush(logfile);
		fclose(logfile); //~~ check return code
		logfile = fopen(log_filename, "a+");
		if (logfile == NULL)
			rc = CP_LOG_FILE_OPEN_FAILURE;
	}
	else
		rc = CP_LOG_NOT_OPEN;

	return rc;
}

int cp_log_close()
{
	int rc;

	if (log_closing) return 0;
	log_closing = 1;

	if (logfile == NULL) return 0;
	rc = fclose(logfile);
	logfile = NULL;
	if (log_filename) 
	{
		free(log_filename);
		log_filename = NULL;
	}

	cp_hashtable_destroy(error_message_lookup);

	if (thread_id)
	{
		cp_hashtable_destroy(thread_id);
		thread_id = NULL;
	}
	
	return rc;
}

void cp_log(const char *fmt, ...)
{
	va_list argp;

	va_start(argp, fmt);
	vprintf(fmt, argp);
	va_end(argp);

	if (logfile == NULL) return;

	va_start(argp, fmt);
	vfprintf(logfile, fmt, argp);
	va_end(argp);
}

void cp_nlog(size_t len, const char *fmt, ...)
{
	va_list argp;
	char *buf = malloc(len + 1);

	if (buf == NULL) return; 

	if (logfile == NULL) return;

	va_start(argp, fmt);
#ifdef HAVE_VSNPRINTF
	vsnprintf(buf, len, fmt, argp);
#else
	vsprintf(buf, fmt, argp);
#endif
	va_end(argp);
	
	buf[len] = '\0';
	fprintf(logfile, buf);
	free(buf);
}

static char *cc_time_format = DEFAULT_TIME_FORMAT;


void cp_log_set_time_format(char *time_format)
{
	cc_time_format = time_format;
}

static void cc_printout(char *type, char *fmt, va_list argp)
{
    char buf[MAX_LOG_MESSAGE_LEN];
    struct tm *local;
    char timestr[256];
	unsigned long tid;
    unsigned long thread_no;

#ifdef PRECISE_TIME
    struct timeval now;
    gettimeofday(&now, NULL);

	local = localtime(&now.tv_sec);
#else		//precise
    time_t now;
    time(&now);
    local = localtime(&now);
#endif		//precise

//    thread_no = (unsigned long) pthread_self();
//	tid = pthread_self();    
	tid = (unsigned long) cp_thread_self();
    thread_no = get_thread_serial(tid);

    strftime(timestr, 256, cc_time_format, local);
#ifdef HAVE_VSNPRINTF
	vsnprintf(buf, MAX_LOG_MESSAGE_LEN, fmt, argp);
#else
	vsprintf(buf, fmt, argp);
#endif /* HAVE_VSNPRINTF */

#ifdef PRECISE_TIME
    printf("%s" PRECISE_TIME_FORMAT " [%lX] %s: %s\n", 
			timestr, now.tv_usec / PRECISE_TIME_FACTOR, thread_no, type, buf);
    if (logfile)
        fprintf(logfile, "%s" PRECISE_TIME_FORMAT " [%lX] %s: %s\n", 
				timestr, now.tv_usec / PRECISE_TIME_FACTOR, thread_no,
			   	type, buf);
#else		/* precise */
    printf("%s [%lu] %s: %s\n", timestr, thread_no, type, buf);
    if (logfile)
        fprintf(logfile, "%s [%lu] %s: %s\n", timestr, thread_no, type, buf);
#endif		/* precise */
    if (logfile) fflush(logfile);
}

#ifdef CP_HAS_VARIADIC_MACROS
static void cp_error_printout(char *type, char *fmt, char *file, int line, va_list argp)
#else
static void cp_error_printout(char *type, char *fmt, va_list argp)
#endif
{
    char buf[MAX_LOG_MESSAGE_LEN];
    struct tm *local;
    char timestr[256];
	unsigned long tid;
    unsigned long thread_no;
#ifdef PRECISE_TIME
    struct timeval now;
    gettimeofday(&now, NULL);
    local = localtime(&now.tv_sec);
#else		//precise
    time_t now;
    time(&now);
    local = localtime(&now);
#endif		//precise

//	tid = (unsigned long) pthread_self();
	tid = (unsigned long) cp_thread_self();
    thread_no = get_thread_serial(tid);

    strftime(timestr, 256, cc_time_format, local);
#ifdef HAVE_VSNPRINTF
    vsnprintf(buf, MAX_LOG_MESSAGE_LEN, fmt, argp);
#else
    vsprintf(buf, fmt, argp);
#endif /* HAVE_VSNPRINTF */

#ifdef PRECISE_TIME
    printf("%s" PRECISE_TIME_FORMAT " [%lX] %s: %s "
#ifdef CP_HAS_VARIADIC_MACROS
		   "(%s line %d)"
#endif
		   "\n", 
		   timestr, now.tv_usec / PRECISE_TIME_FACTOR, thread_no, 
		   type, buf
#ifdef CP_HAS_VARIADIC_MACROS
		   , file, line
#endif
		   );
    if (logfile)
        fprintf(logfile, "%s" PRECISE_TIME_FORMAT " [%lX] %s: %s "
#ifdef CP_HAS_VARIADIC_MACROS
				"(%s line %d)"
#endif
				"\n"
               ,timestr, now.tv_usec / PRECISE_TIME_FACTOR, thread_no, 
			   type, buf
#ifdef CP_HAS_VARIADIC_MACROS
			   , file, line
#endif
			   );
#else		//precise
    printf("%s [%lu] %s: %s "
#ifdef CP_HAS_VARIADIC_MACROS
			"(%s line %d)"
#endif
			"\n", timestr, thread_no, type, buf
#ifdef CP_HAS_VARIADIC_MACROS
			, file, line
#endif
			);
    if (logfile)
        fprintf(logfile, "%s [%lu] %s: %s (%s line %d)\n", timestr, thread_no, type, buf, file, line);
#endif		//precise
    if (logfile) fflush(logfile);
}

#ifdef CP_HAS_VARIADIC_MACROS
void cp_debug_message(char *fmt, char *file, int line, ...)
#else
void cp_debug(char *fmt, ...)
#endif
{
	va_list argp;

	if (loglevel > LOG_LEVEL_DEBUG) return;
//	if (fmt == NULL) return;

#ifdef CP_HAS_VARIADIC_MACROS
	va_start(argp, line);
#else
	va_start(argp, fmt);
#endif
	cp_error_printout("debug", fmt
#ifdef CP_HAS_VARIADIC_MACROS
		, file, line
#endif
		, argp);
	va_end(argp);
}

void cp_debuginfo(char *fmt, ...)
{
	va_list argp;

	if (loglevel > LOG_LEVEL_DEBUG) return;

	va_start(argp, fmt);
	cc_printout("debug", fmt, argp);
	va_end(argp);
}

void cp_info(char *fmt, ...)
{
	va_list argp;

	if (loglevel > LOG_LEVEL_INFO) return;

	va_start(argp, fmt);
	cc_printout("info", fmt, argp);
	va_end(argp);
}

void cp_warn(char *fmt, ...)
{
	va_list argp;

	if (loglevel > LOG_LEVEL_WARNING) return;

	va_start(argp, fmt);
	cc_printout("warning", fmt, argp);
	va_end(argp);
}

#ifdef CP_HAS_VARIADIC_MACROS
void cp_error_message(int code, char *fmt, char *file, int line, ...)
#else
void cp_error(int code, char *fmt, ...)
#endif
{
	va_list argp;
	char errcode[256];
	char errmsg[224];
	char *msg;

	if (loglevel > LOG_LEVEL_ERROR) return;
//	if (fmt == NULL) return;

	msg = cp_hashtable_get(error_message_lookup, &code);
	if (msg)
#ifdef HAVE_SNPRINTF
		snprintf(errmsg, 224, " - %s", msg);
#else
		sprintf(errmsg, " - %s", msg);
#endif /* HAVE_SNPRINTF */
	else
		errmsg[0] = '\0';

#ifdef HAVE_SNPRINTF
	snprintf(errcode, 256, "error [%d%s]", code, errmsg);
#else
	sprintf(errcode, "error [%d%s]", code, errmsg);
#endif /* HAVE_SNPRINTF */

#ifdef CP_HAS_VARIADIC_MACROS
	va_start(argp, line);
#else
	va_start(argp, fmt);
#endif
	cp_error_printout(errcode, fmt
#ifdef CP_HAS_VARIADIC_MACROS
		, file, line
#endif
		, argp);
	va_end(argp);
}

#ifdef CP_HAS_VARIADIC_MACROS
void cp_perror_message(int code, int errno_code, 
		               char *fmt, char *file, int line, ...)
#else
void cp_perror(int code, int errno_code, char *fmt, ...)
#endif
{
	va_list argp;
	char errcode[512];
	char errmsg[224];
#ifdef HAVE_STRERROR_R
	char perrmsg[256];
#endif
	char *msg;

	if (loglevel > LOG_LEVEL_ERROR) return;
//	if (fmt == NULL) return;

	msg = cp_hashtable_get(error_message_lookup, &code);
	if (msg)
#ifdef HAVE_SNPRINTF
		snprintf(errmsg, 224, " - %s", msg);
#else
		sprintf(errmsg, " - %s", msg);
#endif /* HAVE_SNPRINTF */
	else
		errmsg[0] = '\0';

#ifdef HAVE_STRERROR_R
	sprintf(perrmsg, "unknown error");
	strerror_r(errno_code, perrmsg, 255);

#ifdef HAVE_SNPRINTF
	snprintf(errcode, 512, "error [%d%s / %s] -", code, errmsg, perrmsg);
#else
	sprintf(errcode, "error [%d%s / %s] -", code, errmsg, perrmsg);
#endif /* HAVE_SNPRINTF */
#else
#ifdef HAVE_SNPRINTF
	snprintf(errcode, 512, "error [%d%s / %s] -", code, errmsg,
			strerror(errno_code));
#else
	sprintf(errcode, "error [%d%s / %s] -", code, errmsg,
			strerror(errno_code));
#endif /* HAVE_SNPRINTF */
#endif /* HAVE_STRERROR_R */

#ifdef CP_HAS_VARIADIC_MACROS
	va_start(argp, line);
#else
	va_start(argp, fmt);
#endif
	cp_error_printout(errcode, fmt
#ifdef CP_HAS_VARIADIC_MACROS
		, file, line
#endif
		, argp);
	va_end(argp);
}

#ifdef CP_HAS_VARIADIC_MACROS
void cp_fatal_message(int code, char *fmt, char *file, int line, ...)
#else
void cp_fatal(int code, char *fmt, ...)
#endif
{
	va_list argp;
	char errcode[30];

//	if (fmt == NULL) return;
	if (loglevel <= LOG_LEVEL_FATAL)
	{
#ifdef HAVE_SNPRINTF
		snprintf(errcode, 30, "fatal (%d)", code);
#else
		sprintf(errcode, "fatal (%d)", code);
#endif /* HAVE_SNPRINTF */
#ifdef CP_HAS_VARIADIC_MACROS
		va_start(argp, line);
#else
		va_start(argp, fmt);
#endif
		cp_error_printout(errcode, fmt
#ifdef CP_HAS_VARIADIC_MACROS
			, file, line
#endif
			, argp);
		va_end(argp);
	}

	cp_log_close();
	exit(code);
}

#ifdef DEBUG
#ifdef CP_HAS_VARIADIC_MACROS
void DEBUGMSG_impl(char *fmt, char *file, int line, ...)
#else
void DEBUGMSG(char *fmt, ...)
#endif
{
	va_list argp;

	if (loglevel > LOG_LEVEL_DEBUG) return;
//	if (fmt == NULL) return;

#ifdef CP_HAS_VARIADIC_MACROS
	va_start(argp, line);
#else
	va_start(argp, fmt);
#endif
	cp_error_printout("debug", fmt
#ifdef CP_HAS_VARIADIC_MACROS
		, file, line
#endif
		, argp);
	va_end(argp);
}
#endif /* DEBUG */

void die(int code, const char *fmt, ...)
{
	va_list argp;
	char buf[MAX_LOG_MESSAGE_LEN];
	
	if (logfile == NULL) logfile = stderr; /* if called before initialization */

	va_start(argp, fmt);
#ifdef HAVE_VSNPRINTF
	vsnprintf(buf, MAX_LOG_MESSAGE_LEN, fmt, argp);
#else
	vsprintf(buf, fmt, argp);
#endif /* HAVE_VSNPRINTF */
	va_end(argp);

	cp_log(buf);
	cp_fatal(code, buf);
}

#define LINELEN 81
#define CHARS_PER_LINE 16

static char *print_char = 
	"                "
	"                "
	" !\"#$%&'()*+,-./"
	"0123456789:;<=>?"
	"@ABCDEFGHIJKLMNO"
	"PQRSTUVWXYZ[\\]^_"
	"`abcdefghijklmno"
	"pqrstuvwxyz{|}~ "
	"                "
	"                "
	" ¡¢£¤¥¦§¨©ª«¬­®¯"
	"°±²³´µ¶·¸¹º»¼½¾¿"
	"ÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏ"
	"ÐÑÒÓÔÕÖ×ØÙÚÛÜÝÞß"
	"àáâãäåæçèéêëìíîï"
	"ðñòóôõö÷øùúûüýþÿ";

void cp_dump(int levelprm, cp_string *str)
{
	int rc;
	int idx;
	char prn[LINELEN];
	char lit[CHARS_PER_LINE + 2];
	char hc[4];
	short line_done = 1;

	if (levelprm >= loglevel)
	{
		rc = str->len;
		idx = 0;
		lit[CHARS_PER_LINE] = '\0';
		while (rc > 0)
		{
#ifdef HAVE_SNPRINTF
			if (line_done) snprintf(prn, LINELEN, "%08X: ", idx);
#else
			if (line_done) sprintf(prn, "%08X: ", idx);
#endif /* HAVE_SNPRINTF */
			do
			{
				unsigned char c = str->data[idx];
#ifdef HAVE_SNPRINTF
				snprintf(hc, 4, "%02X ", (int) c);
#else
				sprintf(hc, "%02X ", (int) c);
#endif /* HAVE_SNPRINTF */
#ifdef HAVE_STRLCAT
				strlcat(prn, hc, LINELEN);
#else
				strcat(prn, hc);
#endif /* HAVE_STRLCAT */
				lit[idx % CHARS_PER_LINE] = print_char[c];
			} while (--rc > 0 && (++idx % CHARS_PER_LINE != 0));
			line_done = (idx % CHARS_PER_LINE) == 0;
			if (line_done) 
			{
				printf("%s  %s\n", prn, lit);
				if (logfile) fprintf(logfile, "%s  %s\n", prn, lit);
			}
		}
		if (!line_done)
		{
			int ldx = idx % CHARS_PER_LINE;
			lit[ldx++] = print_char[(int) str->data[idx]];
			lit[ldx] = '\0';
			while ((++idx % CHARS_PER_LINE) != 0) 
#ifdef HAVE_STRLCAT
				strlcat(prn, "   ", LINELEN);
#else
				strcat(prn, "   ");
#endif /* HAVE_STRLCAT */
			printf("%s  %s\n", prn, lit);
			if (logfile) fprintf(logfile, "%s  %s\n", prn, lit);

		}
	}
}

void cp_ndump(int levelprm, cp_string *str, size_t len)
{
	if (levelprm >= loglevel)
	{
		int olen = str->len;
		if ((unsigned) str->len > len) str->len = len;
		cp_dump(levelprm, str);
		if (olen != str->len) str->len = olen;
	}
}

#if defined(HAVE_REGEX_H) || defined(HAVE_PCREPOSIX_H)
int log_regex_compilation_error(int rc, char *msg)
{
	char *smsg = regex_compilation_error(rc);
	cp_error(CP_REGEX_COMPILATION_FAILURE, "%s: %s", msg, smsg);
	return rc;
}
#endif /* HAVE_REGEX_H */

/** @} */

