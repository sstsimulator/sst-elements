#ifndef _CP_LOG_H
#define _CP_LOG_H

/** @{ */
/**
 * @file
 * libcprops logging facilities<p>
 *
 * cp_log_init(filename, level) should be called on startup with a log file
 * name and a log level. the following macros may be used for logging 
 * messages.<p>
 * 
 * before exiting, cp_log_close() should be called to ensure the log is 
 * flushed.
 * <ul>
 * <li> DEBUGMSG(fmt, ...) for printouts on LOG_LEVEL_DEBUG if compiled with -DDEBUG
 * <li> cp_info(fmt, ...) for printouts on LOG_LEVEL_INFO
 * <li> cp_warn(fmt, ...) for warning printouts - LOG_LEVEL_WARNING or lower
 * <li> cp_error(code, fmt, ...) for error messages - LOG_LEVEL_ERROR or lower
 * <li> cp_fatal(code, fmt, ...) for fatal error messages (LOG_LEVEL_FATAL)
 * </ul>
 */

#include "common.h"
#include "str.h"

__BEGIN_DECLS

#include "config.h"

/** debug level */
#define LOG_LEVEL_DEBUG				0
/** normal log level */
#define LOG_LEVEL_INFO				1
/** relatively quiet - warnings only */
#define LOG_LEVEL_WARNING			2
/** quit - severe errors only */
#define LOG_LEVEL_ERROR				3
/** very quiet - report fatal errors only */
#define LOG_LEVEL_FATAL				4
/** no logging */
#define LOG_LEVEL_SILENT			5

#include <stdio.h>

#define cp_assert(cond) (cond == 0 ? die(1, "assertion %s failed, in %s line %d\n", #cond, __FILE__, __LINE__) : 0)

#ifdef __OpenBSD__
#define MAX_LOG_MESSAGE_LEN 0x1000
#else
#define MAX_LOG_MESSAGE_LEN 0x10000
#endif

/** precision of seconds: undefine or use 1-6 */
#define PRECISE_TIME "3" 
#ifdef PRECISE_TIME
#define PRECISE_TIME_FORMAT ".%0" PRECISE_TIME "ld"
#else		//precise
#define PRECISE_TIME_FORMAT ""
#endif	

CPROPS_DLL
int cp_log_init(char *filename, int verbosity);

CPROPS_DLL
int cp_log_reopen(void);

CPROPS_DLL
int cp_log_close(void);

CPROPS_DLL
void die(int code, const char *msg, ...);

CPROPS_DLL
void cp_log_set_time_format(char *time_format);

/**
 * unconditionally log a message
 */
CPROPS_DLL
void cp_log(const char *msg, ...);

/**
 * unconditionally log a limited length message
 */
CPROPS_DLL
void cp_nlog(size_t len, const char *msg, ...);

#ifdef CP_HAS_VARIADIC_MACROS
#define cp_debug(msg, ...) \
	(cp_debug_message(msg, __FILE__, __LINE__ , ## __VA_ARGS__))
CPROPS_DLL
void cp_debug_message(char *msg, char *file, int line, ...);
#else
CPROPS_DLL
void cp_debug(char *msg, ...);
#endif

CPROPS_DLL
void cp_debuginfo(char *msg, ...);

CPROPS_DLL
void cp_info(char *msg, ...);

/**
 * print out a LOG_LEVEL_WARNING log message 
 */
CPROPS_DLL
void cp_warn(char *msg, ...);

/**
 * print out a LOG_LEVEL_ERROR log message
 */
#ifdef CP_HAS_VARIADIC_MACROS
#define cp_error(code, msg, ...) \
		(cp_error_message(code, msg, __FILE__, __LINE__ , ## __VA_ARGS__))
CPROPS_DLL
void cp_error_message(int code, char *msg, char *file, int line, ...);
#else
CPROPS_DLL
void cp_error(int code, char *msg, ...);
#endif

/**
 * print out a LOG_LEVEL_ERROR log message with an errno code
 */
#ifdef CP_HAS_VARIADIC_MACROS
#define cp_perror(code, errno_code, msg, ...) \
		(cp_perror_message(code, errno_code, msg, __FILE__, __LINE__ , ## __VA_ARGS__))
CPROPS_DLL
void cp_perror_message(int code, int errno_code, 
                       char *msg, char *file, int line, ...);
#else
CPROPS_DLL
void cp_perror(int code, int errno_code, char *msg, ...);
#endif

/**
 * print out a LOG_LEVEL_FATAL log message and exit. if log_level is 
 * LOG_LEVEL_SILENT, the error message is supressed but the process still
 * exits.
 */
#ifdef CP_HAS_VARIADIC_MACROS
#define cp_fatal(code, msg, ...) \
		(cp_fatal_message(code, msg, __FILE__, __LINE__ , ## __VA_ARGS__))
CPROPS_DLL
void cp_fatal_message(int code, char *msg, char *file, int line, ...);
#else
CPROPS_DLL
void cp_fatal(int code, char *msg, ...);
#endif

/** hex dump a cp_string */
CPROPS_DLL
void cp_dump(int log_level, cp_string *str);

/** hex dump up to len bytes of a cp_string */
CPROPS_DLL
void cp_ndump(int log_level, cp_string *str, size_t len);

#ifdef DEBUG
#ifdef CP_HAS_VARIADIC_MACROS
#define DEBUGMSG(msg, ...) DEBUGMSG_impl(msg, __FILE__, __LINE__ , ## __VA_ARGS__)
CPROPS_DLL
void DEBUGMSG_impl(char *msg, char *file, int line, ...);
#else
CPROPS_DLL
void DEBUGMSG(char *msg, ...);
#endif
#else
#ifdef CP_HAS_VARIADIC_MACROS
#define DEBUGMSG(...)
#else
#define DEBUGMSG()
#endif
#endif

#if defined(HAVE_REGEX_H) || defined(HAVE_PCREPOSIX_H)
CPROPS_DLL
int log_regex_compilation_error(int rc, char *msg);
#endif /* HAVE_REGEX_H */

__END_DECLS

/** @} */

#endif

