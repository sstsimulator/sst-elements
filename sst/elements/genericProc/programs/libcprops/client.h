#ifndef _CP_TCP_CLIENT_H
#define _CP_TCP_CLIENT_H

/**
 * @addtogroup cp_socket
 */
/** @{ */
/**
 * @file
 * definitions for client socket abstraction api
 */

#ifdef _WINDOWS
#include <winsock2.h>
#endif

#include "common.h"

__BEGIN_DECLS

#include "config.h"

#include "hashlist.h"
#include "vector.h"
#include "thread.h"

/* this provides get_client_ssl_ctx */
#include "socket.h"

#if defined(unix) || defined(__unix__) || defined(__MACH__)
#include <sys/time.h>
#include <netinet/in.h>
#endif

#ifdef CP_USE_SSL
#include <openssl/ssl.h>
#endif

#if defined(sun) || defined(__OpenBSD__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__APPLE__)
#include <sys/types.h>
#include <sys/socket.h>
#endif

#ifdef _WINDOWS
#include "Windows.h"
#include "Winsock.h"
#endif

/** seconds to wait for responses */
#define DEFAULT_CLIENT_TIMEOUT 300

/** default connect retries */
#define DEFAULT_RETRIES 3

/** recommended to call before using tcp client socket functions */
CPROPS_DLL
void cp_client_init();
/** 
 * call from signal handler to stop sockets in waiting select() and close all 
 * connections
 */
CPROPS_DLL
void cp_client_stop_all();
/** performs cleanup */
CPROPS_DLL
void cp_client_shutdown();

#ifdef CP_USE_SSL
CPROPS_DLL
void cp_client_ssl_init();
CPROPS_DLL
void cp_client_ssl_shutdown();

CPROPS_DLL
char *ssl_verification_error_str(int code);
#endif /* CP_USE_SSL */

CPROPS_DLL
void cp_client_init();

CPROPS_DLL
void cp_client_shutdown();

/**
 * add callback to be made on tcp layer shutdown
 */
CPROPS_DLL
void cp_tcp_add_shutdown_callback(void (*cb)(void *), void *prm);


CPROPS_DLL
struct _cp_client; /* fwd declaration */

/**
 * cp_client is a 'client socket'. create a cp_client with address information,
 * call cp_client_connect() and communicate on the resulting file descriptor. 
 */
typedef CPROPS_DLL struct _cp_client
{
	int id;						  /**< socket id                         */
	struct sockaddr_in *addr;     /**< address                           */
	struct hostent *hostspec;     /**< aliases                           */
	char *host;                   /**< host literal                      */
	char *found_host;             /**< actual hostname                   */
	int port;                     /**< the port this socket listens on   */
	struct timeval timeout;       /**< max blocking time                 */
	struct timeval created;		  /**< creation time                     */
	unsigned long bytes_read;     /**< bytes read                        */
	unsigned long bytes_sent;     /**< bytes written                     */
#if defined(unix) || defined(__unix__) || defined(__MACH__)
	int fd;                       /**< listener                          */
#else 
#ifdef _WINDOWS
	SOCKET fd;
#endif /* _WINDOWS */
#endif /* unix */
	int closing; 				  /**< shutdown flag                     */
	int retry;
	void *owner;
#ifdef CP_USE_SSL
	int use_ssl;				  /**< ssl connection                    */
	SSL_CTX *ctx;				  /**< ssl context                       */
	SSL *ssl;					  /**< ssl object                        */
	X509 *server_certificate;     /**< server certificate if provided    */
#endif /* CP_USE_SSL */
} cp_client;

/** create a client socket */
CPROPS_DLL
cp_client *cp_client_create(char *host, int port);
/** create a client socket with a struct sockaddr * */
CPROPS_DLL
cp_client *cp_client_create_addr(struct sockaddr_in *);
#ifdef CP_USE_SSL
/** create an ssl client socket */
CPROPS_DLL
cp_client *cp_client_create_ssl(char *host, int port, 
		                        char *CA_file, char *CA_path, 
								int verification_mode);
/** create an ssl client socket with a struct sockaddr * */
CPROPS_DLL
cp_client *cp_client_create_ssl_addr(struct sockaddr *,
		                             char *CA_file, char *CA_path);
#endif /* CP_USE_SSL */
/** set the timeout - (0, 0) for no timeout */
CPROPS_DLL
void cp_client_set_timeout(cp_client *client, int sec, int usec);
/** number of connection retries */
CPROPS_DLL
void cp_client_set_retry(cp_client *client, int retry_count);
/** useful free pointer for client code */
CPROPS_DLL
void cp_client_set_owner(cp_client *client, void *owner);
/** 
 * opens a connection. may return: 
 * <li> 0 on success
 * <li> -1 on failure
 * <li> > 0 on SSL verification errors. a connection has been established but 
 * couldn't be properly verified. Client code should decide whether to proceed 
 * or not. The <code>server_certificate</code> field on the <code>client</code>
 * will have been set to the server certificate if one has been presented. 
 * <br>
 * return codes correspond to SSL_get_verify_result codes with the addition
 * of X509_V_ERR_APPLICATION_VERIFICATION possibly indicating that no server
 * certificate was presented. In this case the <code>server_certificate</code>
 * field will be <code>NULL</code>.
 */
CPROPS_DLL
int cp_client_connect(cp_client *client);

#ifdef CP_USE_SSL
/** (internal) perform ssl handshake on a new connection */
CPROPS_DLL
int cp_client_connect_ssl(cp_client *client);
#endif /* CP_USE_SSL */
/** 
 * use the same wrapper for a different address. this will close an existing 
 * connection, change the host and port settings on the wrapper and attempt to
 * connect. maybe rename this function. 
 */
CPROPS_DLL
int cp_client_reopen(cp_client *client, char *host, int port);

#ifdef CP_USE_SSL
/** return the server certificate */
CPROPS_DLL
X509 *cp_client_get_server_certificate(cp_client *client);
/** check whether the host name given in the server certificate matches the name used for the connection */
CPROPS_DLL
int cp_client_verify_hostname(cp_client *client);
#endif /* CP_USE_SSL */

/** close a client connection */
CPROPS_DLL
int cp_client_close(cp_client *client);
/** deallocate a cp_client */
CPROPS_DLL
void cp_client_destroy(cp_client *client);

CPROPS_DLL
int cp_client_read(cp_client *client, char *buf, int len);
CPROPS_DLL
int cp_client_read_string(cp_client *client, cp_string *str, int len);
CPROPS_DLL
int cp_client_write(cp_client *client, char *buf, int len);
CPROPS_DLL
int cp_client_write_string(cp_client *client, cp_string *str);

__END_DECLS

/** @} */

#endif /* _CP_TCP_CLIENT_H */
