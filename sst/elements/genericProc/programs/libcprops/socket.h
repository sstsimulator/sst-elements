#ifndef _CP_SOCKET_H
#define _CP_SOCKET_H

/**
 * @addtogroup cp_socket
 */
/** @{ */
/**
 * @file
 * definitions for socket abstraction api
 */

#include "common.h"

#ifdef _WINDOWS
#include <Winsock2.h>
#endif

__BEGIN_DECLS

#include "config.h"

#include "hashlist.h"
#include "vector.h"
#include "thread.h"

#if defined(unix) || defined(__unix__) || defined(__MACH__)
#include <sys/time.h>
#include <netinet/in.h>
#endif

#ifdef CP_USE_SSL
#include <openssl/ssl.h>
#endif

#if defined(sun) || defined(__OpenBSD__)
#include <sys/types.h>
#include <sys/socket.h>
#endif

#ifdef _WINDOWS
#include "Winsock2.h"
#endif 

#define CPSOCKET_DEFAULT_BACKLOG                 50
#define CPSOCKET_DEFAULT_DELAY_SEC                1
#define CPSOCKET_DEFAULT_DELAY_USEC               0 
#define CPSOCKET_THREADPOOL_DEFAULT_SIZE_MIN      5
#define CPSOCKET_THREADPOOL_DEFAULT_SIZE_MAX     50

/** recommended to call before using socket functions */
CPROPS_DLL
void cp_socket_init();
/** 
 * call from signal handler to stop sockets in waiting select() and close all 
 * connections
 */
CPROPS_DLL
void cp_socket_stop_all();
/** performs cleanup */
CPROPS_DLL
void cp_socket_shutdown();

#ifdef CP_USE_SSL
CPROPS_DLL
void cp_socket_ssl_init();
CPROPS_DLL
void cp_socket_ssl_shutdown();

CPROPS_DLL
SSL_CTX *get_ssl_ctx(char *certificate_file, char *key_file);
CPROPS_DLL
SSL_CTX *get_client_ssl_ctx(char *CA_file, char *CA_path);
#endif

/**
 * add callback to be made on tcp layer shutdown
 */
CPROPS_DLL
void cp_tcp_add_shutdown_callback(void (*cb)(void *), void *prm);


/**
 * cp_socket connection handling strategy
 */
typedef CPROPS_DLL enum 
{ 
	CPSOCKET_STRATEGY_CALLBACK,
	CPSOCKET_STRATEGY_THREADPOOL
} cp_socket_strategy;

CPROPS_DLL 
struct _cp_socket; /* fwd, uh, declaration */

/** thread function for threadpool implementation */
typedef void *(*cp_socket_thread_function)(void *);
/** function prototype for callback implementation */
typedef int (*cp_socket_callback)(struct _cp_socket *, int fd);
#ifdef CP_USE_SSL
/** function prototype for callback implementation with SSL */
typedef int (*cp_socket_ssl_callback)(struct _cp_socket *, SSL *ssl);
#endif

/**
 * cp_socket is a 'server socket'. create a cp_socket to listen on a port, 
 * specify a callback to implemet your communication protocol (libcprops 
 * provides a basic http implementation you could plug in) and the strategy
 * you want for handling handling multiple connections - use a thread pool
 * to serve each connection on its own thread or a more select()-like approach. 
 * <p>
 * If you create a socket with the CPSOCKET_STRATEGY_THREADPOOL, supply a 
 * thread function to handle communication once a connection has been 
 * accept()-ed. Your cp_thread function should close the connection when done 
 * and check the 'closing' flag, which is set to 1 on shutdown. 
 * <p>
 * If you create a socket with the CPSOCKET_STRATEGY_CALLBACK, supply a callback
 * to process incremental feeds as they come along.
 */
typedef CPROPS_DLL struct _cp_socket
{
	int id;						  /**< socket id */
	int port;                     /**< the port this socket listens on */
	int backlog;                  /**< max backlog */
	struct timeval delay;         /**< max blocking time */
	int connections_served;		  /**< number of connections accepted */
	struct timeval created;       /**< creation time */
#if defined(unix) || defined(__unix__) || defined(__MACH__)
	int fd;                       /**< listener */
#else
#ifdef _WINDOWS
	SOCKET fd;
#endif /* _WINDOWS */
#endif /* unix */
	cp_hashlist *fds;             /**< all available file descriptors */
	cp_thread_pool *tpool;        /**< cp_thread pool (if requested) */
	int poolsize_min;             /**< min cp_thread count for cp_thread pool */
	int poolsize_max;             /**< max cp_thread count for cp_thread pool */
	unsigned int fdn;             /**< largest fd - needed for select() */
	cp_socket_strategy strategy;  /**< threadpool or callback */	
	union
	{
		cp_socket_thread_function thread_fn;
		cp_socket_callback callback;
#ifdef CP_USE_SSL
		cp_socket_ssl_callback ssl_callback;
#endif
	} action; 					  /**< placeholder for action */
	int closing; 				  /**< shutdown flag */
	void *owner;
#ifdef CP_USE_SSL
	int use_ssl;				  /**< ssl connection */
	SSL_CTX *ctx;				  /**< ssl context */
#endif
	cp_vector *shutdown_callback; /**< shutdown callback list */
} cp_socket;

/** set number of outstanding requests before accept() fails new connections */
CPROPS_DLL
void cp_socket_set_backlog(cp_socket *socket, int backlog);
/** delay time before re-accept()ing */
CPROPS_DLL
void cp_socket_set_delay(cp_socket *socket, struct timeval delay);
/** seconds before re-accept()ing */
CPROPS_DLL
void cp_socket_set_delay_sec(cp_socket *socket, long sec);
/** microseconds before re-accept()ing */
CPROPS_DLL
void cp_socket_set_delay_usec(cp_socket *socket, long usec);
/** lower size limit for threadpool implementation */
CPROPS_DLL
void cp_socket_set_poolsize_min(cp_socket *socket, int min);
/** upper size limit for threadpool implementation */
CPROPS_DLL
void cp_socket_set_poolsize_max(cp_socket *socket, int max);
/** useful free pointer for client code */
CPROPS_DLL
void cp_socket_set_owner(cp_socket *socket, void *owner);

/** create a new cp_socket struct */
CPROPS_DLL
cp_socket *cp_socket_create(int port, cp_socket_strategy strategy, void *fn);
#ifdef CP_USE_SSL
/** create a new ssl enabled cp_socket struct */
CPROPS_DLL
cp_socket *cp_socket_create_ssl(int port, 
		                        cp_socket_strategy strategy, 
								void *fn,
								char *certificate_file,
								char *key_file,
								int verification_mode);
/** close an ssl connection */
CPROPS_DLL
int cp_socket_ssl_connection_close(cp_socket *sock, SSL *ssl);
#endif
/** deallocate a socket descriptor */
CPROPS_DLL
void cp_socket_delete(cp_socket *sock);
/** bind socket */
CPROPS_DLL
int cp_socket_listen(cp_socket *sock);
/** block and wait for connections */
CPROPS_DLL
int cp_socket_select(cp_socket *sock);
/** close a (non ssl) connection */
CPROPS_DLL
int cp_socket_connection_close(cp_socket *sock, int fd);
/** add a callback to be made on socket shutdown */
CPROPS_DLL
void *cp_socket_add_shutdown_callback(cp_socket *sock, 
									  void (*cb)(void *),
									  void *prm);

/** connection descriptor structure */
typedef CPROPS_DLL struct _cp_connection_descriptor
{
	cp_socket *sock;			/**< socket the connection was made on */
	struct sockaddr_in *addr;	/**< client address */
#if defined(unix) || defined(__unix__) || defined(__MACH__)
	int fd;						/**< file descriptor */
#else
#ifdef _WINDOWS
	SOCKET fd;
#endif /* _WINDOWS */
#endif /* unix */
	void *owner; 				/**< placeholder for client identifier */
#ifdef CP_USE_SSL
	SSL *ssl;					/**< ssl object */
#endif
	struct timeval created; 	/**< creation time */
	unsigned long bytes_read;   /**< bytes read */
	unsigned long bytes_sent;   /**< bytes written */
} cp_connection_descriptor;

/** internal: create a new connection descriptor */
CPROPS_DLL
cp_connection_descriptor *cp_connection_descriptor_create(cp_socket *sock, 
													struct sockaddr_in *addr, 
													int fd);
#ifdef CP_USE_SSL
/** internal: create a new ssl connection descriptor */
CPROPS_DLL
cp_connection_descriptor *cp_connection_descriptor_create_ssl(cp_socket *sock, 
													struct sockaddr_in *addr, 
													int fd, 
													SSL *ssl);
#endif
/** deallocate a connection descriptor */
CPROPS_DLL
void cp_connection_descriptor_destroy(cp_connection_descriptor *conn_desc);

/** return cp_socket struct for given connection descriptor */
#define cp_connection_descriptor_get_socket(cd) ((cd)->sock)
/** return client address for given connection descriptor */
#define cp_connection_descriptor_get_addr(cd) ((cd)->addr)
/** return file descriptor for given connection descriptor */
#define cp_connection_descriptor_get_fd(cd) ((cd)->fd)

/** read from a connection descriptor */
CPROPS_DLL
int cp_connection_descrpitor_read(cp_connection_descriptor *desc, 
		                          char *buf, int len);

/** write on a connection descriptor */
CPROPS_DLL
int cp_connection_descriptor_write(cp_connection_descriptor *desc, 
								   char *buf, int len);

__END_DECLS

/** @} */

#endif /* _CP_SOCKET_H */
