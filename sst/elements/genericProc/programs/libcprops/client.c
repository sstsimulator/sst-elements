#include "log.h"
#include "socket.h"
#include "client.h"
#include "collection.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <sys/types.h>

#if defined(unix) || defined(__unix__) || defined(__MACH__)
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif

#ifdef CP_USE_SSL
#include <openssl/ssl.h>
#include <openssl/err.h>

#ifdef _WINDOWS
#include <wincrypt.h>
#endif /* _WINDOWS */

#endif /* CP_USE_SSL */

#ifdef HAVE_STRINGS_H
#include <strings.h>
#else
#include "util.h"
#endif /* HAVE_STRINGS_H */

#ifdef _WINDOWS
#include <Winsock2.h>
#include <Ws2tcpip.h>
#include <WS2SPI.H>
#endif

#ifdef CP_USE_SSL

char *ssl_verification_error_str(int code)
{
	switch (code)
	{
		case X509_V_OK:
		return "X509_V_OK the operation was successful.";

		case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT:
		return
		"X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT the issuer certificate could "
		"not be found";

		case X509_V_ERR_UNABLE_TO_GET_CRL:
		return 
		"X509_V_ERR_UNABLE_TO_GET_CRL the CRL of a certificate could "
		"not be found."; /* unused */

		case X509_V_ERR_UNABLE_TO_DECRYPT_CERT_SIGNATURE:
		return
		"X509_V_ERR_UNABLE_TO_DECRYPT_CERT_SIGNATURE the certificate signature"
		" could not be decrypted.";

		case X509_V_ERR_UNABLE_TO_DECRYPT_CRL_SIGNATURE:
		return
		"X509_V_ERR_UNABLE_TO_DECRYPT_CRL_SIGNATURE the CRL signature could "
		"not be decrypted"; /* unused */

		case X509_V_ERR_UNABLE_TO_DECODE_ISSUER_PUBLIC_KEY:
		return
		"X509_V_ERR_UNABLE_TO_DECODE_ISSUER_PUBLIC_KEY the public key in the "
		"certificate SubjectPublicKeyInfo could not be read.";

		case X509_V_ERR_CERT_SIGNATURE_FAILURE:
		return
		"X509_V_ERR_CERT_SIGNATURE_FAILURE the signature of the certificate "
		"is invalid.";

		case X509_V_ERR_CRL_SIGNATURE_FAILURE:
		return 
		"X509_V_ERR_CRL_SIGNATURE_FAILURE the signature of the "
		" certificate is invalid."; /* unused */

		case X509_V_ERR_CERT_NOT_YET_VALID:
		return
		"X509_V_ERR_CERT_NOT_YET_VALID the certificate is not yet valid: the "
		" notBefore date is after the current time.";

		case X509_V_ERR_CERT_HAS_EXPIRED:
		return
		"X509_V_ERR_CERT_HAS_EXPIRED the certificate has expired: the "
		"notAfter date is before the current time.";

		case X509_V_ERR_CRL_NOT_YET_VALID:
		return "X509_V_ERR_CRL_NOT_YET_VALID the CRL is not yet valid."; /* unused */

		case X509_V_ERR_CRL_HAS_EXPIRED:
		return "X509_V_ERR_CRL_HAS_EXPIRED the CRL has expired."; /* unused */

		case X509_V_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD:
		return
		"X509_V_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD the certificate notBefore "
		"field contains an invalid time.";

		case X509_V_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD:
		return
		"X509_V_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD the certificate notAfter "
		"field contains an invalid time.";

		case X509_V_ERR_ERROR_IN_CRL_LAST_UPDATE_FIELD:
		return
		"X509_V_ERR_ERROR_IN_CRL_LAST_UPDATE_FIELD the CRL lastUpdate field "
		"contains an invalid time."; /* unused */

		case X509_V_ERR_ERROR_IN_CRL_NEXT_UPDATE_FIELD:
		return
		"X509_V_ERR_ERROR_IN_CRL_NEXT_UPDATE_FIELD the CRL nextUpdate field "
		"contains an invalid time."; /* unused */

		case X509_V_ERR_OUT_OF_MEM:
		return
		"X509_V_ERR_OUT_OF_MEM an error occurred trying to allocate memory.";

		case X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT:
		return
		"X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT the passed certificate is self"
		" signed and the same certificate cannot be found in the list of "
		"trusted certificates.";

		case X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN:
		return
		"X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN the certificate chain could be "
		"built up using the untrusted certificates but the root could not be "
		"found locally.";

		case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY:
		return
		"X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY the issuer certificate "
		"of a locally looked up certificate could not be found.";

		case X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE:
		return
		"X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE no signatures could be "
		"verified because the chain contains only one certificate and it is "
		"not self signed.";

		case X509_V_ERR_CERT_CHAIN_TOO_LONG:
		return
		"X509_V_ERR_CERT_CHAIN_TOO_LONG the certificate chain length is "
		"greater than the supplied maximum depth.";

		case X509_V_ERR_CERT_REVOKED:
		return
		"X509_V_ERR_CERT_REVOKED the certificate has been revoked."; /* unused */

		case X509_V_ERR_INVALID_CA:
		return
		"X509_V_ERR_INVALID_CA a CA certificate is invalid. Either it is not a"
		" CA or its extensions are not consistent with the supplied purpose.";

		case X509_V_ERR_PATH_LENGTH_EXCEEDED:
		return
		"X509_V_ERR_PATH_LENGTH_EXCEEDED the basicConstraints pathlength "
		"parameter has been exceeded.";

		case X509_V_ERR_INVALID_PURPOSE:
		return 
		"X509_V_ERR_INVALID_PURPOSE the supplied certificate cannot be used "
		"for the specified purpose.";

		case X509_V_ERR_CERT_UNTRUSTED:
		return
		"X509_V_ERR_CERT_UNTRUSTED the root CA is not marked as trusted for "
		"the specified purpose.";

		case X509_V_ERR_CERT_REJECTED:
		return
		"X509_V_ERR_CERT_REJECTED the root CA is marked to reject the "
		"specified purpose.";

		case X509_V_ERR_SUBJECT_ISSUER_MISMATCH:
		return 
		"X509_V_ERR_SUBJECT_ISSUER_MISMATCH the current candidate issuer "
		"certificate was rejected because its subject name did not match the "
		"issuer name of the current certificate.";

		case X509_V_ERR_AKID_SKID_MISMATCH:
		return
		"X509_V_ERR_AKID_SKID_MISMATCH the current candidate issuer certificate"
		" was rejected because its subject key identifier was present and did "
		"not match the authority key identifier of the current certificate.";

		case X509_V_ERR_AKID_ISSUER_SERIAL_MISMATCH:
		return 
		"X509_V_ERR_AKID_ISSUER_SERIAL_MISMATCH the current candidate issuer "
		"certificate was rejected because its"
		" issuer name and serial number was present and did not match the"
		" authority key identifier of the current certificate. Only displayed"
		" when the -issuer_checks option is set.";

		case X509_V_ERR_KEYUSAGE_NO_CERTSIGN:
		return
			"X509_V_ERR_KEYUSAGE_NO_CERTSIGN the current candidate issuer "
			"certificate was rejected because its"
            " keyUsage extension does not permit certificate signing.";

		case X509_V_ERR_APPLICATION_VERIFICATION: 
		   return 
			   "X509_V_ERR_APPLICATION_VERIFICATION "
			   "server presented no certificate";
	}

	return "unknown verification error";
}

char *ssl_err_inf(int err)
{
	switch (err)
	{
		case SSL_ERROR_NONE: return "SSL_ERROR_NONE"; break;
		case SSL_ERROR_ZERO_RETURN: return "SSL_ERROR_ZERO_RETURN"; break;
		case SSL_ERROR_WANT_READ: return "SSL_ERROR_WANT_READ"; break;
		case SSL_ERROR_WANT_WRITE: return "SSL_ERROR_WANT_WRITE"; break;
		case SSL_ERROR_WANT_CONNECT: return "SSL_ERROR_WANT_CONNECT"; break;
#ifdef SSL_ERROR_WANT_ACCEPT
		case SSL_ERROR_WANT_ACCEPT: return "SSL_ERROR_WANT_ACCEPT"; break;
#endif /* SSL_ERROR_WANT_ACCEPT */
		case SSL_ERROR_WANT_X509_LOOKUP: return "SSL_ERROR_WANT_X509_LOOKUP"; break;
		case SSL_ERROR_SYSCALL: return "SSL_ERROR_SYSCALL"; break;
		case SSL_ERROR_SSL: return "SSL_ERROR_SSL"; break;
	}

	return "unknown";
}

void cp_client_ssl_init()
{
	cp_socket_ssl_init();
}

void cp_client_ssl_shutdown()
{
	cp_socket_ssl_shutdown();
}

#endif

#ifndef HAVE_FUNC_GETHOSTBYNAME_R_6
static cp_lock *cp_gethostbyname_lock = NULL;
#endif

void cp_client_init()
{
#ifndef HAVE_FUNC_GETHOSTBYNAME_R_6
	if (cp_gethostbyname_lock == NULL)
	{
		cp_gethostbyname_lock = (cp_lock *) calloc(1, sizeof(cp_lock));
		if (cp_gethostbyname_lock == NULL)
			cp_fatal(CP_MEMORY_ALLOCATION_FAILURE, "can\'t allocate lock");

		if (cp_lock_init(cp_gethostbyname_lock, NULL))
			cp_fatal(CP_INITIALIZATION_FAILURE, "can\'t initialize lock");
	}
#endif
	cp_socket_init();
}

void cp_client_shutdown()
{
#ifndef HAVE_FUNC_GETHOSTBYNAME_R_6
    if (cp_gethostbyname_lock != NULL)
	{
		cp_lock_destroy(cp_gethostbyname_lock);
		free(cp_gethostbyname_lock);
		cp_gethostbyname_lock = NULL;
	}
#endif
	cp_socket_shutdown();
}

static void delete_hostent(struct hostent *hp)
{
	if (hp)
	{
#ifndef HAVE_FUNC_GETHOSTBYNAME_R_6
		int i;

		if (hp->h_name) free(hp->h_name);

		if (hp->h_aliases)
		{
			for (i = 0; hp->h_aliases[i]; i++)
				free(hp->h_aliases[i]);
			free(hp->h_aliases);
		}

		if (hp->h_addr_list)
		{
			for (i = 0; hp->h_addr_list[i]; i++)
				free(hp->h_addr_list[i]);
			free(hp->h_addr_list);
		}
#endif

		free(hp);
	}
}

#ifdef HAVE_FUNC_GETHOSTBYNAME_R_6
#define RESOLVE_BUFLEN 0x1000
static int resolve_addr(cp_client *client)
{
	struct hostent *res = (struct hostent *) calloc(1, sizeof(struct hostent));
	struct hostent *result;
	char buf[RESOLVE_BUFLEN];
	int rc, err;

	rc = gethostbyname_r(client->host, res, buf, RESOLVE_BUFLEN, &result, &err);
	if (!(rc == 0 && result != NULL && err == 0))
	{
		free(res);
		rc = -1;
	}
	else
		client->hostspec = res;

	return rc;
}

#else /* HAVE_FUNC_GETHOSTBYNAME_R_6 */

struct hostent *hostentdup(struct hostent *src)
{
	int i, count;
	struct hostent *dup = (struct hostent *) calloc(1, sizeof(struct hostent));
	if (src->h_name) dup->h_name = strdup(src->h_name);
	for (count = 0; src->h_aliases[count] != NULL; count++);
	dup->h_aliases = (char **) calloc(count + 1, sizeof(char *));
	if (count)
	{
		for (i = 0; i < count; i++)
			dup->h_aliases[i] = strdup(src->h_aliases[i]);
	}
	dup->h_aliases[count] = NULL;
	dup->h_addrtype = src->h_addrtype;
	dup->h_length = src->h_length;
	for (count = 0; src->h_addr_list[count] != NULL; count++);
	dup->h_addr_list = (char **) calloc(count + 1, sizeof(char *));
	if (count)
	{
		for (i = 0; i < count; i++)
		{
			dup->h_addr_list[i] = (char *) malloc(src->h_length);
			memcpy(dup->h_addr_list[i], src->h_addr_list[i], src->h_length);
		}
	}
	dup->h_addr_list[count] = NULL;

	return dup;
}

void print_addr(struct hostent *hp)
{
	int i;

	printf("host %s: ", hp->h_name);
	for (i = 0; hp->h_addr_list[i] != NULL; i++)
		printf("addr[%d]: %s\n", i, inet_ntoa(*(struct in_addr *) hp->h_addr_list[i]));
}

#ifndef HAVE_GETHOSTBYNAME
extern int h_errno;
#endif

static int resolve_addr(cp_client *sock)
{
	int rc = 0;
	struct hostent* phostent;

	if ((rc = cp_lock_wrlock(cp_gethostbyname_lock)) != 0) return -1;
	phostent = gethostbyname(sock->host);

	if (phostent)
	{
		sock->hostspec = hostentdup(phostent);
		rc = 0;
	}

	cp_lock_unlock(cp_gethostbyname_lock);

	return rc;
}

#endif /* HAVE_FUNC_GETHOSTBYNAME_R_6 */

/* create a client socket */
cp_client *cp_client_create(char *host, int port)
{
	cp_client *client = calloc(1, sizeof(cp_client));
	if (client == NULL) return NULL;
	
	client->retry = DEFAULT_RETRIES;
	client->host = strdup(host);
	client->port = port;

	if ((resolve_addr(client)))
	{
		cp_client_destroy(client);
		return NULL;
	}

	client->fd = socket(PF_INET, SOCK_STREAM, 0);
	if (client->fd == -1)
	{
		cp_perror(CP_IO_ERROR, errno, "can\'t create socket");
		cp_client_destroy(client);
		return NULL;
	}

	return client;
}

#ifdef CP_USE_SSL
cp_client *cp_client_create_ssl(char *host, 
							    int port, 
								char *CA_file, 
								char *CA_path,
								int verification_mode)
{
	cp_client *client = cp_client_create(host, port);
	if (client == NULL) return NULL;

	client->ctx = get_client_ssl_ctx(CA_file, CA_path);
	if (client->ctx == NULL)
	{
		cp_client_destroy(client);
		return NULL;
	}
	
	SSL_CTX_set_verify(client->ctx, verification_mode, NULL);
	SSL_CTX_set_mode(client->ctx, SSL_MODE_AUTO_RETRY);

	client->use_ssl = 1;
	client->ssl = SSL_new(client->ctx);
	SSL_set_fd(client->ssl, client->fd);

	return client;
}
#endif /* CP_USE_SSL */

int cp_client_reopen(cp_client *client, char *host, int port)
{
	cp_client_close(client);

	if (client->host) 
	{
		free(client->host);
		client->host = NULL;
	}
	if (client->hostspec) 
	{
		delete_hostent(client->hostspec); 
		client->hostspec = NULL;
	}

	if (client->addr) 
	{
		free(client->addr);
		client->addr = NULL;
	}
	
	client->host = strdup(host);
	client->port = port;

	if ((resolve_addr(client)))
	{
		cp_error(CP_IO_ERROR, "can\'t resolve address for %s", host);
		return -1;
	}

	client->fd = socket(PF_INET, SOCK_STREAM, 0);
	if (client->fd == -1)
	{
		cp_perror(CP_IO_ERROR, errno, "can\'t create socket");
		return -1;
	}

#ifdef CP_USE_SSL
	if (client->use_ssl)
		SSL_set_fd(client->ssl, client->fd);
#endif /* CP_USE_SSL */

	return cp_client_connect(client);
}

/** create a client socket with a struct sockaddr * */
cp_client *cp_client_create_addr(struct sockaddr_in *addr)
{
	cp_client *client = calloc(1, sizeof(cp_client));
	if (client == NULL) return NULL;

	client->retry = DEFAULT_RETRIES;
	client->addr = malloc(sizeof(struct sockaddr_in));
	if (client->addr == NULL)
	{
		free(client);
		return NULL;
	}
	*client->addr = *addr;

	return client;
}

/** set the timeout - (0, 0) for no timeout */
void cp_client_set_timeout(cp_client *client, int sec, int usec)
{
	client->timeout.tv_sec = sec;
	client->timeout.tv_usec = usec;
}

/** number of connection retries */
void cp_client_set_retry(cp_client *client, int retry_count)
{
	client->retry = retry_count;
}

/** useful free pointer for client code */
void cp_client_set_owner(cp_client *client, void *owner)
{
	client->owner = owner;
}

#ifdef CP_USE_SSL
int cp_client_connect_ssl(cp_client *client)
{
	int rc = SSL_connect(client->ssl);
	if (rc <= 0)
	{
		int err = SSL_get_error(client->ssl, rc);
		const char *reason = ERR_reason_error_string(err);

		cp_error(CP_SSL_HANDSHAKE_FAILED, "can\'t connect to %s: %s", 
				 inet_ntoa(client->addr->sin_addr), 
				 reason ? reason : ssl_err_inf(err));
		if (rc == 0) rc = -1; //~~ ahem
	}
	else
	{
		client->server_certificate = SSL_get_peer_certificate(client->ssl);
//		SSL_set_connect_state(client->ssl); //~~ necessary?
		if (client->server_certificate == NULL)
		{
			cp_error(CP_SSL_VERIFICATION_ERROR, 
					"connecting to %s: server presented no certificate", 
					inet_ntoa(client->addr->sin_addr));
			/* connection is up; client code should decide whether to 
			 * continue or not. */
			return X509_V_ERR_APPLICATION_VERIFICATION; 
		}
        if ((rc = SSL_get_verify_result(client->ssl)) != X509_V_OK) /* certificate doesn't verify */
   	    {
            cp_error(CP_SSL_VERIFICATION_ERROR, 
					"SSL_get_verify_result: %d\n", rc);
			return rc;
       	}
	}

	return rc;
}
#endif /* CP_USE_SSL */

/** open connection */
int cp_client_connect(cp_client *client)
{
	struct sockaddr_in addr;
	int rc = 0;
	int connected = 0;
	int retry = client->retry;

	if (client->addr == NULL && client->hostspec == NULL)
	{
		cp_error(CP_INITIALIZATION_FAILURE, "client socket not initialized");
		return CP_INITIALIZATION_FAILURE;
	}

	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = client->hostspec->h_addrtype;
	addr.sin_port = htons(client->port);

	while (!connected && retry--)
	{
		if (client->addr)
		{
			rc = connect(client->fd, (struct sockaddr *) client->addr, 
					sizeof(struct sockaddr_in));
			if (rc == 0) 
			{
				connected = 1;
				break;
			}
		}

		if (client->hostspec)
		{
			int i;

			for (i = 0; client->hostspec->h_addr_list[i] != NULL; i++)
			{
				addr.sin_addr = 
					*((struct in_addr *) client->hostspec->h_addr_list[i]);
				rc = connect(client->fd, (struct sockaddr *) &addr, 
							 sizeof(struct sockaddr_in));
				if (rc == 0)
				{
					if (client->addr == NULL)
					{
						client->addr = 
							(struct sockaddr_in *) malloc(sizeof(struct sockaddr_in));
					}
					if (client->addr == NULL)
						cp_error(CP_MEMORY_ALLOCATION_FAILURE, "can\'t allocate address storage memory");
					else
						memcpy(client->addr, &addr, sizeof(struct sockaddr_in));

					connected = 1;
					break;
				}
			}
		}
	}

#ifdef CP_USE_SSL
	if (client->use_ssl && connected)
		rc = cp_client_connect_ssl(client);
#endif
	
    if (connected && rc == 0)
        gettimeofday(&client->created, NULL);

	return rc;
}


#ifdef CP_USE_SSL
int verify_host_name(X509 *cert, char *host)
{
	char buf[0x100];
	X509_NAME *name = X509_get_subject_name(cert);
	X509_NAME_get_text_by_NID(name, NID_commonName, buf, 0x100);
#ifdef __TRACE__
	DEBUGMSG("certificate for %s\n", buf);
#endif
	return (strcasecmp(buf, host) == 0);
}

X509 *cp_client_get_server_certificate(cp_client *client)
{
	return client->server_certificate;
}

/** check whether the host name given in the server certificate matches the name used for the connection */
int cp_client_verify_hostname(cp_client *client)
{
	return verify_host_name(client->server_certificate, client->host);
}
#endif /* CP_USE_SSL */

/** close a client connection */
int cp_client_close(cp_client *client)
{
	int rc = 0;
	
	if (client)
	{
#ifdef CP_USE_SSL
		if (client->use_ssl)
		{
			rc = SSL_shutdown(client->ssl);
			if (!rc)
			{
				shutdown(client->fd, SHUT_WR);
				rc = SSL_shutdown(client->ssl);
			}
			if (client->server_certificate)
			{
				X509_free(client->server_certificate);
				client->server_certificate = NULL;
			}
		}
		else
#endif /* CP_USE_SSL */
		rc = shutdown(client->fd, SHUT_RDWR);
		if (rc == -1) 
			cp_perror(CP_IO_ERROR, errno, 
					"shutdown error on connection to %s",
					inet_ntoa(client->addr->sin_addr)); //~~ do something
		rc = close(client->fd);
		if (rc == -1)
			cp_perror(CP_IO_ERROR, errno, 
					"error closing connection to %s",
					inet_ntoa(client->addr->sin_addr));
	}

	return rc;
}

/** deallocate a cp_client */
void cp_client_destroy(cp_client *client)
{
	if (client)
	{
		if (client->host) free(client->host);
		if (client->hostspec) delete_hostent(client->hostspec);
		if (client->addr) free(client->addr);
#ifdef CP_USE_SSL
		if (client->use_ssl)
			SSL_free(client->ssl);
#endif /* CP_USE_SSL */
		free(client);
	}
}

int cp_client_read(cp_client *client, char *buf, int len)
{
	int rc;
#ifdef CP_USE_SSL
	if (client->use_ssl)
		rc = SSL_read(client->ssl, buf, len);
	else
#endif /* CP_USE_SSL */
//	rc = read(client->fd, buf, len);
	rc = recv(client->fd, buf, len, 0);
	if (rc > 0) client->bytes_read += rc;

#ifdef _TCP_DUMP
	if (rc > 0)
	{
		buf[rc] = '\0';
		fprintf(stderr, "--- TCP DUMP ---\r\n%s\r\n --- PMUD PCT --- \r\n", 
				buf);
	}
#endif

	return rc;
}

#define CHUNK 0x1000

int cp_client_read_string(cp_client *client, cp_string *str, int len)
{
	int rc;
	char buf[CHUNK];
	int read_len;
	int total = 0;

	while (len == 0 || total < len)
	{
		read_len = (len == 0 || (len - total) > CHUNK) ? CHUNK : len - total;
#ifdef CP_USE_SSL
		if (client->use_ssl)
			rc = SSL_read(client->ssl, buf, read_len);
		else
#endif /* CP_USE_SSL */
//		rc = read(client->fd, buf, read_len);
		rc = recv(client->fd, buf, read_len, 0);

		if (rc <= 0) 
			break;
		else
			client->bytes_read += rc;
		cp_string_cat_bin(str, buf, rc);
		total += rc;
	}
	
	return total;
}

//~~ int cp_client_read_to(cp_client *client, char *buf, int len, char *marker)

int cp_client_write(cp_client *client, char *buf, int len)
{
	int rc;
#ifdef CP_USE_SSL
	if (client->use_ssl)
	{
		rc = SSL_write(client->ssl, buf, len);
		if (rc <= 0)
		{
			int err = SSL_get_error(client->ssl, rc);
			const char *reason = ERR_reason_error_string(err);
			cp_error(CP_IO_ERROR, "write failed: %s", reason ? reason : ssl_err_inf(err));
		}
	}
	else
#endif
	rc = send(client->fd, buf, len, 0);

	if (rc > 0) client->bytes_sent += rc;

	return rc;
}

int cp_client_write_string(cp_client *client, cp_string *str)
{
	int rc;
	int total = 0;
	char *pos = cp_string_tocstr(str);
	int remaining = cp_string_len(str);

	while (total < cp_string_len(str))
	{
#ifdef CP_USE_SSL
		if (client->use_ssl)
			rc = SSL_write(client->ssl, pos, remaining);
		else
#endif /* CP_USE_SSL */
		rc = send(client->fd, pos, remaining, 0);

		if (rc <= 0) 
			break;
		else
			client->bytes_sent += rc;

		total += rc;
		remaining -= rc;
		pos = &pos[rc];
	}
	
	return total;
}
