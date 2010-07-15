#include "common.h"

#ifdef _WINDOWS
#include <Winsock2.h>
#endif

#include "log.h"
#include "util.h"

#include "hashtable.h"
#include "vector.h"
#include "thread.h"

#include "socket.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>

#if defined(unix) || defined(__unix__) || defined(__MACH__)
#include <unistd.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

/**
 * @addtogroup cp_socket
 */
/** @{ */
/**
 * @file
 * 
 * socket framework implementation
 */

#ifdef CP_USE_SSL
#include <openssl/ssl.h>
#include <openssl/err.h>

#ifdef _WINDOWS
#include <wincrypt.h>
#endif /* _WINDOWS */

static cp_hashtable *ssl_ctx_table = NULL;

volatile int ssl_initialized = 0;

void cp_socket_ssl_init()
{
	if (ssl_initialized) return;
	ssl_initialized = 1;
	
	SSL_library_init();
	SSL_load_error_strings();
	OpenSSL_add_all_algorithms();

	ssl_ctx_table = cp_hashtable_create(1, cp_hash_string, cp_hash_compare_string);
}

void cp_socket_ssl_shutdown()
{
	if (ssl_initialized)
	{
		ssl_initialized = 0;
		if (ssl_ctx_table)
			cp_hashtable_destroy_custom(ssl_ctx_table, free, 
									(cp_destructor_fn) SSL_CTX_free);
		ssl_ctx_table = NULL;

		EVP_cleanup();
		ERR_free_strings();
	}
}
	
SSL_CTX *get_ssl_ctx(char *certificate_file, char *key_file)
{
	int len;
	char *ck;
	SSL_CTX *ctx;
	if (ssl_ctx_table == NULL) cp_socket_ssl_init();

	len = strlen(certificate_file) + strlen(key_file) + 2;
	ck = malloc(len * sizeof(char));
#ifdef HAVE_SNPRINTF
	snprintf(ck, len, "%s*%s", certificate_file, key_file);
#else
	sprintf(ck, "%s*%s", certificate_file, key_file);
#endif /* HAVE_SNPRINTF */

	ctx = cp_hashtable_get(ssl_ctx_table, ck);
	if (ctx == NULL)
	{
		int rc = 0;
		SSL_METHOD *method = SSLv23_server_method();
		ctx = SSL_CTX_new(method);
		if (ctx == NULL)
		{
			rc = ERR_get_error();
			cp_error(CP_SSL_CTX_INITIALIZATION_ERROR, 
					 "can\'t create SSL context: %s", 
					 ERR_reason_error_string(rc));
			goto CTX_ERROR;
		}
		
		if (SSL_CTX_use_certificate_file(ctx, certificate_file, 
					                     SSL_FILETYPE_PEM) != 1)
		{
			rc = ERR_get_error();
			cp_error(CP_SSL_CTX_INITIALIZATION_ERROR, 
					 "certificate file: %s", ERR_reason_error_string(rc));		
			goto CTX_ERROR;
		}

		if (SSL_CTX_use_PrivateKey_file(ctx, key_file, SSL_FILETYPE_PEM) != 1)
		{
			rc = ERR_get_error();
			cp_error(CP_SSL_CTX_INITIALIZATION_ERROR, 
					 "private key file: %s\n", ERR_reason_error_string(rc));	
			goto CTX_ERROR;
		}
	
		if (!(SSL_CTX_check_private_key(ctx)))
		{
			rc = ERR_get_error();
			cp_error(CP_SSL_CTX_INITIALIZATION_ERROR, 
					 "checking private key: %s", ERR_reason_error_string(rc));
			goto CTX_ERROR;
		}

		cp_hashtable_put(ssl_ctx_table, ck, ctx);
	}
		
	return ctx;

CTX_ERROR:
	if (ctx)
		SSL_CTX_free(ctx);

	return NULL;
}
	
SSL_CTX *get_client_ssl_ctx(char *CA_file, char *CA_path)
{
	SSL_CTX *ctx;
	cp_string *ckwrap = cp_string_create_empty(80);
	char *ck;

	if (ssl_ctx_table == NULL) cp_socket_ssl_init();
	
	if (CA_file) cp_string_cstrcat(ckwrap, CA_file);
	cp_string_cstrcat(ckwrap, "*");
	if (CA_path) cp_string_cstrcat(ckwrap, CA_path);
	ck = cp_string_tocstr(ckwrap);
	free(ckwrap);
	
	ctx = cp_hashtable_get(ssl_ctx_table, ck);
	if (ctx == NULL)
	{
		int rc;
		SSL_METHOD *method = SSLv23_client_method();
		ctx = SSL_CTX_new(method);

		rc = SSL_CTX_load_verify_locations(ctx, CA_file, CA_path); 
		if (rc == 0) /* error */
		{
			rc = ERR_get_error();
			cp_error(CP_SSL_CTX_INITIALIZATION_ERROR, 
					 "loading CA certificate: %s", ERR_reason_error_string(rc));
			SSL_CTX_free(ctx);
			return NULL;
		}

		cp_hashtable_put(ssl_ctx_table, ck, ctx);
	}
	else 
		free(ck);

	return ctx;
}

#endif

static volatile int socket_reg_id = 0;
static cp_hashlist *socket_registry;
static cp_vector *shutdown_hook = NULL;
static volatile int initialized = 0;
static volatile int shutting = 0;

#if defined(unix) || defined(__unix__) || defined(__MACH__)
void cp_socket_signal_handler(int sig);
#endif

void cp_socket_init()
{
#if defined(unix) || defined(__unix__) || defined(__MACH__)
	struct sigaction act;
#endif

#ifdef _WINDOWS
	WSADATA wsaData;
	WORD version;
	int error;
#endif

	if (initialized) return;
	initialized = 1;

#ifdef _WINDOWS
	version = MAKEWORD( 2, 0 );
	error = WSAStartup( version, &wsaData );

	/* check for error */	
	if ( error != 0 )
	{
		/* error occured */
		initialized = 0;
		return;
	}

	/* check for correct version */
	if ( LOBYTE( wsaData.wVersion ) != 2 ||
		 HIBYTE( wsaData.wVersion ) != 0 )
	{	
		/* incorrect WinSock version */
		initialized = 0;
		WSACleanup();
		return;
	}
#endif

	socket_registry = 
		cp_hashlist_create_by_mode(1, COLLECTION_MODE_DEEP, 
								   cp_hash_int, cp_hash_compare_int);
#if defined(unix) || defined(__unix__) || defined(__MACH__)
	act.sa_handler = cp_socket_signal_handler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	sigaction(SIGPIPE, &act, NULL);
#endif
}

void cp_socket_stop(cp_socket *sock)
{
#ifdef __TRACE__
	DEBUGMSG("stopping socket %lX", sock);
#endif
	sock->closing = 1;
	
	shutdown(sock->fd, SHUT_RD);
}

void cp_tcp_add_shutdown_callback(void (*cb)(void *), void *prm)
{
	cp_wrap *wrap;

	if (shutdown_hook == NULL)
		shutdown_hook = cp_vector_create(1);

	wrap = cp_wrap_new(prm, cb);

	cp_vector_add_element(shutdown_hook, wrap);
}

volatile int stopping_all = 0;

void cp_socket_stop_all()
{
	if (stopping_all) return;
	stopping_all = 1;

	if (socket_registry)
	{
		cp_socket *sock;
		cp_hashlist_iterator i;
		cp_hashlist_iterator_init(&i, socket_registry, COLLECTION_LOCK_READ);
		while ((sock = cp_hashlist_iterator_next_value(&i)))
			cp_socket_stop(sock);
		cp_hashlist_iterator_release(&i);
	}
}

void cp_socket_shutdown()
{
	if (shutting) return;

	shutting = 1;

#ifdef __TRACE__
	DEBUGMSG("shutting down socket layer");
#endif

	if (shutdown_hook)
	{
		cp_vector_destroy_custom(shutdown_hook, 
								 (cp_destructor_fn) cp_wrap_delete);
		shutdown_hook = NULL;
	}

	cp_socket_stop_all();
	if (socket_registry)
	{
		cp_hashlist_destroy(socket_registry);
		socket_registry = NULL;
	}

#ifdef CP_USE_SSL
	cp_socket_ssl_shutdown();
#endif /* CP_USE_SSL */

#ifdef _WINDOWS
	WSACleanup();
#endif
}

#if defined(unix) || defined(__unix__) || defined(__MACH__)
void cp_socket_signal_handler(int sig)
{
	switch (sig)
	{
		case SIGPIPE: 
#ifdef __TRACE__
			DEBUGMSG("SIGPIPE - ignoring\n");
#endif
			break;
	}
}
#endif /* unix */

#if defined(unix) || defined(__unix__) || defined(__MACH__) 
int setnonblocking(int sock)
{
    int opts;

	/*
	 * fcntl may set: 
	 *
	 * EACCES, EAGAIN: Operation is prohibited by locks held by other 
	 *          processes. Or, operation is prohibited because the file has 
	 *          been memory-mapped by another process. 
	 * EBADF:   fd is not an open file descriptor, or the command was F_SETLK 
	 *          or F_SETLKW and the file descriptor open mode doesn't match 
	 *          with the type of lock requested.
	 * EDEADLK: It was detected that the specified F_SETLKW command would 
	 *          cause a deadlock.
	 * EFAULT:  lock is outside your accessible address space.
	 * EINTR:   For F_SETLKW, the command was interrupted by a signal. For 
	 *          F_GETLK and F_SETLK, the command was interrupted by a signal 
	 *          before the lock was checked or acquired. Most likely when 
	 *          locking a remote file (e.g. locking over NFS), but can 
	 *          sometimes happen locally.
	 * EINVAL:  For F_DUPFD, arg is negative or is greater than the maximum 
	 *          allowable value. For F_SETSIG, arg is not an allowable signal 
	 *          number.
	 * EMFILE:  For F_DUPFD, the process already has the maximum number of 
	 *          file descriptors open. 
	 * ENOLCK:  Too many segment locks open, lock table is full, or a remote 
	 *          locking protocol failed (e.g. locking over NFS).
	 * EPERM:   Attempted to clear the O_APPEND flag on a file that has the 
	 *          append-only attribute set.
	 */
    opts = fcntl(sock, F_GETFL);
    if (opts < 0) 
    {
        perror("fcntl(F_GETFL)");
        return -1;
    }

    opts |= O_NONBLOCK;

    if (fcntl(sock, F_SETFL, opts) < 0) 
    {
        perror("fcntl(F_SETFL)");
        return -1;
    }

    return opts;
}
#else /* unix */
int setnonblocking(int sock)
{
	DWORD ul=1;
	return ioctlsocket(sock, FIONBIO, &ul);
}
#endif /* unix */

cp_connection_descriptor *cp_connection_descriptor_create(cp_socket *sock, 
                                                    struct sockaddr_in *addr, 
                                                    int fd)
{
    cp_connection_descriptor *conn_desc = 
        (cp_connection_descriptor *) calloc(1, sizeof(cp_connection_descriptor));

    if (conn_desc == NULL)
    {
        cp_error(CP_MEMORY_ALLOCATION_FAILURE, 
                 "can\'t allocate cp_connection_descriptor (%d bytes)", 
                 sizeof(cp_connection_descriptor));
        return NULL;
    }

    conn_desc->sock = sock;
    conn_desc->addr = addr;
    conn_desc->fd = fd;

    return conn_desc;
}

#ifdef CP_USE_SSL
cp_connection_descriptor *cp_connection_descriptor_create_ssl(cp_socket *sock, 
                                                    struct sockaddr_in *addr, 
                                                    int fd, 
													SSL *ssl)
{
	cp_connection_descriptor *conn_desc = 
		cp_connection_descriptor_create(sock, addr, fd);

	if (conn_desc) conn_desc->ssl = ssl;

	return conn_desc;
}
#endif

void cp_connection_descriptor_destroy(cp_connection_descriptor *conn_desc)
{
    if (conn_desc)
    {
#ifdef CP_USE_SSL
		if (conn_desc->ssl) 
			cp_socket_ssl_connection_close(conn_desc->sock, conn_desc->ssl);
		else
#endif
        cp_socket_connection_close(conn_desc->sock, conn_desc->fd);
        free(conn_desc);
    }
}


cp_socket *cp_socket_create(int port, cp_socket_strategy strategy, void *fn)
{
    cp_socket *sock;

	cp_socket_init();

    sock = calloc(1, sizeof(cp_socket));
    if (sock == NULL) 
    {
        cp_error(CP_MEMORY_ALLOCATION_FAILURE, "can\'t allocate cp_socket (%d bytes)", sizeof(cp_socket));
        return NULL;
    }

    sock->port = port;
    sock->backlog = CPSOCKET_DEFAULT_BACKLOG;
    sock->delay.tv_sec = CPSOCKET_DEFAULT_DELAY_SEC;
    sock->delay.tv_usec = CPSOCKET_DEFAULT_DELAY_USEC;
    sock->strategy = strategy;
    sock->fds = 
		cp_hashlist_create_by_option(COLLECTION_MODE_DEEP, 10, cp_hash_int, 
				cp_hash_compare_int, NULL, free, NULL, free);
	if (sock->fds == NULL)
	{
        cp_error(CP_MEMORY_ALLOCATION_FAILURE, 
				 "can\'t allocate cp_socket fd list");
		free(sock);
		return NULL;
	}

    switch (strategy)
    {
        case CPSOCKET_STRATEGY_CALLBACK: 
            sock->action.callback = (cp_socket_callback) fn; 
//            sock->bufs = cp_hashlist_create(10, cp_hash_int, cp_hash_compare_int);
            break;

        case CPSOCKET_STRATEGY_THREADPOOL:
            sock->poolsize_min = CPSOCKET_THREADPOOL_DEFAULT_SIZE_MIN;
            sock->poolsize_max = CPSOCKET_THREADPOOL_DEFAULT_SIZE_MAX;
            sock->action.thread_fn = (cp_socket_thread_function) fn; 
            break;
    }

	sock->id = socket_reg_id++;
	cp_hashlist_append(socket_registry, &sock->id, sock);

    return sock;
}

#ifdef CP_USE_SSL
cp_socket *cp_socket_create_ssl(int port, 
		                        cp_socket_strategy strategy, 
								void *fn,
								char *certificate_file,
								char *key_file,
								int verification_mode)
{
	cp_socket *socket = cp_socket_create(port, strategy, fn);
	if (socket)
	{
		socket->ctx = get_ssl_ctx(certificate_file, key_file);
		if (socket->ctx == NULL)
		{
			cp_socket_delete(socket);
			return NULL;
		}
			
		socket->use_ssl = 1;
		SSL_CTX_set_verify(socket->ctx, verification_mode, NULL);
	}
	return socket;
}
#endif

void cp_socket_set_backlog(cp_socket *socket, int backlog)
{
	socket->backlog = backlog;
}

void cp_socket_set_delay(cp_socket *socket, struct timeval delay)
{
	socket->delay = delay;
}

void cp_socket_set_delay_sec(cp_socket *socket, long sec)
{
	socket->delay.tv_sec = sec;
}

void cp_socket_set_delay_usec(cp_socket *socket, long usec)
{
	socket->delay.tv_usec = usec;
}

void cp_socket_set_poolsize_min(cp_socket *socket, int min)
{
	socket->poolsize_min = min;
}

void cp_socket_set_poolsize_max(cp_socket *socket, int max)
{
	socket->poolsize_max = max;
}

void cp_socket_set_owner(cp_socket *socket, void *owner)
{
	socket->owner = owner;
}

void *cp_socket_add_shutdown_callback(cp_socket *sock, 
									  void (*cb)(void *),
									  void *prm)
{
	cp_wrap *wrap = cp_wrap_new(prm, (cp_destructor_fn) cb);

	if (sock->shutdown_callback == NULL)
		sock->shutdown_callback = cp_vector_create(1);

	return cp_vector_add_element(sock->shutdown_callback, wrap);
}

void cp_socket_delete(cp_socket *sock)
{
    if (sock)
    {
        sock->closing = 1;
        switch(sock->strategy)
        {
            case CPSOCKET_STRATEGY_THREADPOOL:
                cp_thread_pool_stop(sock->tpool);
                cp_thread_pool_destroy(sock->tpool);
                break;

            case CPSOCKET_STRATEGY_CALLBACK:
                {
                    cp_hashlist_iterator *itr;
                    cp_hashlist_entry *entry;
                    int *pfd;

                    itr = cp_hashlist_create_iterator(sock->fds, COLLECTION_LOCK_NONE);

                    while ((entry = cp_hashlist_iterator_next(itr)))
                    {
                        pfd = (int *) cp_hashlist_entry_get_key(entry);
                        cp_socket_connection_close(sock, *pfd);
                    }

                    cp_hashlist_iterator_destroy(itr);
                }
                break;
        }

// #ifdef CP_USE_SSL
//		if (sock->use_ssl)
//			SSL_CTX_free(sock->ctx);
// #endif

		if (sock->shutdown_callback)
			cp_vector_destroy_custom(sock->shutdown_callback, 
									 (cp_destructor_fn) cp_wrap_delete);

		if (socket_registry)
			cp_hashlist_remove(socket_registry, &sock->id);
        cp_hashlist_destroy_custom(sock->fds, free, free);
        free(sock);
    }
}

/**
 * cp_socket_listen attempts to create a SOCK_STREAM socket with socket(), 
 * bind to a local port with bind(), and set the socket to listen for 
 * connections with listen(). 
 */
int cp_socket_listen(cp_socket *sock)
{
    struct sockaddr_in insock;
    char *errmsg;
    int yes = 1;
    int fd;
    int rc = -1;

    if (sock->strategy == CPSOCKET_STRATEGY_THREADPOOL)
    {
        sock->tpool = cp_thread_pool_create(sock->poolsize_min, 
                                         sock->poolsize_max);

        if (sock->tpool == NULL)
        {
            errmsg = "can\'t create cp_thread pool";
            goto LISTEN_ERROR;
        }
    }

    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
		/* socket may set:
		 * EPROTONOSUPPORT: The protocol type or the specified protocol is not 
		 *                  supported within this domain. 
		 * EAFNOSUPPORT: The implementation does not support the specified 
		 *               address family. 
		 * ENFILE: Not enough kernel memory to allocate a new socket structure. 
		 * EMFILE: Process file table overflow. 
		 * EACCES: Permission to create a socket of the specified type and/or 
		 *         protocol is denied. 
		 * ENOMEM: Insufficient memory is available. The socket cannot be 
		 *         created until sufficient resources are freed. 
		 * EINVAL: Unknown protocol, or protocol family not available.  
		 */
        rc = errno;
        errmsg = "socket";
        goto LISTEN_ERROR;
    }

//	setnonblocking(fd);
#ifdef TRACE	
    DEBUGMSG("got socket: fd %d\n", fd);
#endif
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, 
                    (char *) &yes, sizeof(int)) < 0)
    {
        errmsg = "setsockopt";
        goto LISTEN_ERROR;
    }

    sock->fd = fd;

    memset(&insock, 0, sizeof(struct sockaddr_in));
    insock.sin_port = htons((unsigned short) sock->port);
    insock.sin_family = AF_INET;

    if (bind(sock->fd, (struct sockaddr *) &insock, 
             sizeof(struct sockaddr_in)) < 0)
    {
		/* bind may set:
		 * EBADF: sockfd is not a valid descriptor 
		 * EINVAL: The socket is already bound to an address or the addrlen is 
		 *         wrong
		 * EACCES: The address is protected, and the user is not the 
		 *         super-user or search permission is denied on a component of 
		 *         the path prefix.  
		 * ENOTSOCK: Argument is a descriptor for a file, not a socket.         
		 * EROFS: The socket inode would reside on a read-only file system. 
		 * EFAULT: my_addr points outside the user's accessible address space. 
		 * ENAMETOOLONG: my_addr is too long. 
		 * ENOENT: The file does not exist.  
		 * ENOMEM: Insufficient kernel memory was available. 
		 * ENOTDIR: A component of the path prefix is not a directory. 
		 * ELOOP: Too many symbolic links were encountered in resolving my_addr 
		 */
		rc = errno;
        errmsg = "cp_socket_listen: bind";
        goto LISTEN_ERROR;
    }

    if (listen(sock->fd, sock->backlog) < 0)
    {
		/* listen may set:
		 * EADDRINUSE: Another socket is already listening on the same port. 
		 * EBADF: The argument s is not a valid descriptor. 
		 * ENOTSOCK: The argument s is not a socket. 
		 * EOPNOTSUPP: The socket is not of a type that supports the listen 
		 *             operation. 
		 */
        rc = errno;
        errmsg = "listen";
        goto LISTEN_ERROR;
    }
	gettimeofday(&sock->created, NULL);
	sock->connections_served = 0;
#ifdef DEBUG
    DEBUGMSG("listening on port %d (fd %d)\n", sock->port, sock->fd); //~~ debug
#endif

    return 0;

LISTEN_ERROR:
    cp_perror(CP_IO_ERROR, rc, "cp_socket_listen: failed on %s", errmsg); 
    return rc;
}


int cp_socket_select_callback_impl(cp_socket *sock)
{
    fd_set fds;
    unsigned int *pfd;
    cp_hashlist_iterator *itr;
    cp_hashlist_entry *entry;
    int rc = -1;
#ifdef CP_USE_SSL
	cp_hashtable *ssl_entry = NULL;
	
	if (sock->use_ssl)
		ssl_entry = cp_hashtable_create(10, cp_hash_int, cp_hash_compare_int);
#endif

    itr = cp_hashlist_create_iterator(sock->fds, COLLECTION_LOCK_NONE);

    while (!sock->closing)
    {
        FD_ZERO(&fds);
        
        sock->fdn = sock->fd;

        FD_SET(sock->fd, &fds);
        
        cp_hashlist_iterator_head(itr);
        while ((entry = cp_hashlist_iterator_next(itr)))
        {
            pfd = (unsigned int *) cp_hashlist_entry_get_key(entry);
            FD_SET(*pfd, &fds);
            if (sock->fdn < *pfd) sock->fdn = *pfd;
        }

        if ((rc = select(sock->fdn + 1, &fds, NULL, NULL, &sock->delay)) < 0)
        {
			cp_perror(CP_IO_ERROR, errno, 
				"cp_socket_select_callback_impl: select failed");

            sock->closing = 1;
            break;
        }

        if (rc == 0) continue;

        if (FD_ISSET(sock->fd, &fds)) /* handle new connections */
        {
            int newconn;
            unsigned int l;
            struct sockaddr_in *insock = calloc(1, sizeof(struct sockaddr_in));

            newconn = accept(sock->fd, (struct sockaddr *) insock, &l);
            if (newconn < 0)
            {
				cp_perror(CP_IO_ERROR, errno, 
					"cp_socket_select_callback_impl: accept failed");
                free(insock);
				if (newconn == EBADF || 
#ifdef ENOTSOCK
					newconn == ENOTSOCK || 
#endif 
#ifdef EOPNOTSUPP
					newconn == EOPNOTSUPP || 
#endif
					newconn == EINVAL || 
					newconn == EFAULT || newconn == EPERM)
					break; 
				/* else if (newconn == EAGAIN || newconn == EINTR || 
				 *          newconn == ECONNABORTED || newconn == EMFILE || 
				 *          newconn == ENFILE || newconn == ENOMEM ||
				 *          newconn == EPROTO) 
				 */
				continue;
            }

#ifdef CP_USE_SSL
			if (sock->use_ssl)
			{
				SSL *ssl = SSL_new(sock->ctx);
				SSL_set_fd(ssl, newconn);
				rc = SSL_accept(ssl);
				if (rc <= 0)
				{
					int err = SSL_get_error(ssl, rc);
					cp_error(CP_SSL_HANDSHAKE_FAILED, "cp_socket_select_callback_impl: %s", ERR_reason_error_string(err));
					SSL_free(ssl);
					free(insock);
					continue;
				}
				cp_hashtable_put(ssl_entry, &newconn, ssl);
			}
#endif

            cp_hashlist_append(sock->fds, &newconn, insock);
        }

        cp_hashlist_iterator_head(itr);
        while ((entry = cp_hashlist_iterator_next(itr)))
        {
            pfd = (unsigned int *) cp_hashlist_entry_get_key(entry);
            if (FD_ISSET(*pfd, &fds))
			{
#ifdef CP_USE_SSL
			    if (sock->use_ssl)
				{
					SSL *ssl = cp_hashtable_get(ssl_entry, pfd);
					(*sock->action.ssl_callback)(sock, ssl);
			    }
				else
#endif
                (*sock->action.callback)(sock, *pfd);
			}
        }
    }

    cp_hashlist_iterator_destroy(itr);

#ifdef CP_USE_SSL
	if (sock->use_ssl)
		cp_hashtable_destroy_custom(ssl_entry, NULL, 
				                    (cp_destructor_fn)SSL_free);
#endif

    return 0;
}

int cp_socket_select_threadpool_impl(cp_socket *sock)
{
    fd_set fds;
    int rc;
    struct timeval delay;

    while (!sock->closing)
    {
        FD_ZERO(&fds);
        
        sock->fdn = sock->fd;

        FD_SET(sock->fd, &fds);

        delay = sock->delay;

        rc = select(sock->fdn + 1, &fds, NULL, NULL, &delay);
        if (rc < 0)
        {
			/* select may set:
			 * EBADF: An invalid file descriptor was given in one of the sets. 
			 * EINTR: A non blocked signal was caught. 
			 * EINVAL: n is negative or the value contained within timeout is 
			 *         invalid. 
			 * ENOMEM: select was unable to allocate memory for internal tables
			 */
			rc = errno;
			/* ignore interrupts unless shutting down*/
			if (rc == EINTR || rc == EAGAIN) 
			{
				if (stopping_all) //~~
				{
					sock->closing = 1;
					break;
				}
				else
					continue;
			}

			cp_perror(CP_IO_ERROR, rc, 
				"cp_socket_select_threadpool_impl: select failed");
            sock->closing = 1;
            break;
        }
            
        if (rc == 0) continue;

        if (FD_ISSET(sock->fd, &fds)) /* handle new connections */
        {
            int *newconn = malloc(sizeof(int));
            cp_connection_descriptor *conn_desc;
            unsigned int l;
            struct sockaddr_in *insock = calloc(1, sizeof(struct sockaddr_in));
            l = sizeof(struct sockaddr_in);

            *newconn = accept(sock->fd, (struct sockaddr *) insock, &l);
            if (*newconn < 0)
            {
				/* accept might set:
				 * EAGAIN: The socket is marked non-blocking and no connections
				 *         are present to be accepted.
				 * EBADF: The descriptor is invalid.         
				 * ENOTSOCK: The descriptor references a file, not a socket. 
				 * EOPNOTSUPP: The referenced socket is not of type SOCK_STREAM
				 * EINTR: The system call was interrupted by a signal that was 
				 *        caught before a valid connection arrived. 
				 * ECONNABORTED: A connection has been aborted. 
				 * EINVAL: Socket is not listening for connections. 
				 * EMFILE: The per-process limit of open file descriptors has 
				 *         been reached. 
				 * ENFILE: The system maximum for file descriptors has been 
				 *         reached. 
				 * EFAULT: The addr parameter is not in a writable part of the 
				 *         user address space. 
				 * ENOMEM: Not enough free memory. This often means that the 
				 *         memory allocation is limited by the socket buffer 
				 *         limits, not by the system memory. 
				 * EPROTO: Protocol error. 
				 * EPERM: Firewall rules forbid connection.  
				 */
				rc = errno;
				if (!(sock->closing 
#ifdef ECONNABORTED
					&& rc == ECONNABORTED
#endif
					))
					cp_perror(CP_IO_ERROR, rc, 
						"cp_socket_select_threadpool_impl: accept failed");
                free(insock);
				free(newconn);
				if (rc == EBADF || 
#ifdef ENOTSOCK
					rc == ENOTSOCK || 
#endif
#ifdef EOPNOTSUPP
					rc == EOPNOTSUPP || 
#endif
					rc == EINVAL || rc == EFAULT || rc == EPERM)
					break; 
				/* else if (rc == EAGAIN || rc == EINTR || rc == ECONNABORTED 
				 *          || rc == EMFILE || rc == ENFILE || rc == ENOMEM ||
				 *          rc == EPROTO) 
				 */
				continue;
            }

			sock->connections_served++;
#ifdef __TRACE__
            DEBUGMSG("adding connection on fd %d (%lX)", *newconn, (long) insock);
            DEBUGMSG("connection from %s", inet_ntoa(insock->sin_addr));
#endif
	    
#ifdef CP_USE_SSL
			if (sock->use_ssl)
			{
				SSL *ssl = SSL_new(sock->ctx);
				SSL_set_fd(ssl, *newconn);
				rc = SSL_accept(ssl);
				if (rc <= 0)
				{
					int err = SSL_get_error(ssl, rc);
					cp_error(CP_SSL_HANDSHAKE_FAILED, "cp_socket_select_callback_impl: %s", ERR_reason_error_string(err));
					SSL_free(ssl);
					free(insock);
					continue;
				}

				conn_desc = cp_connection_descriptor_create_ssl(sock, insock, 
                                                                *newconn, ssl);
			}
			else
#endif
            conn_desc = cp_connection_descriptor_create(sock, insock, *newconn);
			gettimeofday(&conn_desc->created, NULL);
            cp_hashlist_append(sock->fds, newconn, insock);
#ifdef __TRACE__
            cp_info("added: %lX (table: %lX)", (long) cp_hashlist_get(sock->fds, newconn), (long) sock->fds);
#endif
            cp_thread_pool_get(sock->tpool, sock->action.thread_fn, conn_desc);
	    
#ifdef __TRACE__
            cp_info("dispatched; addr: %lX (table: %lX)", (long) cp_hashlist_get(sock->fds, newconn), (long) sock->fds);
#endif
        }
    }

    return 0;
}

int cp_socket_select(cp_socket *sock)
{
    int rc = 0;

    switch (sock->strategy)
    {
        case CPSOCKET_STRATEGY_CALLBACK:
            rc = cp_socket_select_callback_impl(sock);
            break;

        case CPSOCKET_STRATEGY_THREADPOOL:
            rc = cp_socket_select_threadpool_impl(sock);
            break;
    }

    return rc;
}

#ifdef CP_USE_SSL
int cp_socket_ssl_connection_close(cp_socket *sock, SSL *ssl)
{
	int rc = 0;
	int fd;
    struct sockaddr_in *insock;

	fd = SSL_get_fd(ssl);
	setnonblocking(fd);
    insock = (struct sockaddr_in *) cp_hashlist_get(sock->fds, &fd);
	if (insock)
	{
        rc = SSL_shutdown(ssl);
		if (rc == 0)
			rc = SSL_shutdown(ssl); 
		
		if (shutdown(fd, SHUT_WR) < 0)
    	{
    		/* shutdown might set:
    		 * EBADF:    s is not a valid descriptor. 
    		 * ENOTSOCK: s is a file, not a socket. 
    		 * ENOTCONN: The specified socket is not connected. 
    		 */
    	    cp_perror(CP_IO_ERROR, errno, 
					  "shutdown error on connection from %s", 
                       inet_ntoa(insock->sin_addr));
        }

        SSL_free(ssl);
        if (close(fd) < 0)
        {
            /* close might set:
             * EBADF: fd isn't a valid open file descriptor. 
             * EINTR: The close() call was interrupted by a signal. 
             * EIO:   An I/O error occurred. 
             */
             struct sockaddr_in *insock;
             rc = errno;
             insock = (struct sockaddr_in *) cp_hashlist_get(sock->fds, &fd);
             cp_perror(CP_IO_ERROR, rc, "error closing connection to %s", 
                       inet_ntoa(insock->sin_addr));
		}
		
        cp_hashlist_remove_deep(sock->fds, &fd);
	}

	return rc;
}
#endif

int cp_socket_connection_close(cp_socket *sock, int fd)
{
    struct sockaddr_in *insock;
    int rc = 0;

#ifdef __TRACE__
    DEBUGMSG("closing fd %d (table: %lX)", fd, (long) sock->fds);
#endif
    insock = (struct sockaddr_in *) cp_hashlist_get(sock->fds, &fd);
    if (insock) 
    {
#ifdef __TRACE__
    DEBUGMSG("closing connection to %s", inet_ntoa(insock->sin_addr));
#endif
        if (shutdown(fd, SHUT_RDWR) < 0)
			cp_perror(CP_IO_ERROR, errno, "shutdown error on connection to %s", 
					  inet_ntoa(insock->sin_addr));

        if ((rc = close(fd)) < 0)
		    cp_perror(CP_IO_ERROR, rc, "error closing connection to %s", 
                      inet_ntoa(insock->sin_addr));
        
        cp_hashlist_remove_deep(sock->fds, &fd);
    }
    
    return rc;
}

int cp_connection_descrpitor_read(cp_connection_descriptor *desc, 
		                          char *buf, int len)
{
	int rc;

#ifdef CP_USE_SSL
	if (desc->sock->use_ssl)
	{
		rc = SSL_read(desc->ssl, buf, len);
		if (rc <= 0)
		{
			int err = SSL_get_error(desc->ssl, rc);
			const char *reason = ERR_reason_error_string(err);
			cp_error(CP_IO_ERROR, "write failed: %s", reason ? reason : ssl_err_inf(err));
		}
	}
	else
#endif /* CP_USE_SSL */
	rc = read(desc->fd, buf, len);

	if (rc > 0) desc->bytes_read += rc;

	return rc;
}

int cp_connection_descriptor_write(cp_connection_descriptor *desc, 
								   char *buf, int len)
{
	int rc;
#ifdef CP_USE_SSL
	if (desc->sock->use_ssl)
	{
		rc = SSL_write(desc->ssl, buf, len);
		if (rc <= 0)
		{
			int err = SSL_get_error(desc->ssl, rc);
			const char *reason = ERR_reason_error_string(err);
			cp_error(CP_IO_ERROR, "write failed: %s", reason ? reason : ssl_err_inf(err));
		}
	}
	else
#endif
	rc = send(desc->fd, buf, len, 0);

	if (rc > 0) desc->bytes_sent += rc;

	return rc;
}

/** @} */

