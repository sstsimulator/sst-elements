#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <cprops/str.h>
#include <cprops/log.h>
#include <cprops/client.h>

char *NOURI = "/";

char *request_fmt = "GET %s HTTP/1.1\nHost: %s\nConnection: close\n\n";

void sigpipe_handler(int sig)
{
	switch (sig)
	{
		case SIGPIPE: 
			printf("SIGPIPE\n"); /* ignore SIGPIPE */
			break;

		default:
			printf("unhandled signal %d\n", sig);
			break;
	}
}

#if 0
char *ssl_verification_error_str(int code)
{
	switch (code)
	{
		case X509_V_OK:
		return "the operation was successful.";

		case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT:
		return
		"the issuer certificate could not be found: this occurs if the"
		" issuer certificate of an untrusted certificate cannot be found.";

		case X509_V_ERR_UNABLE_TO_GET_CRL:
		return "the CRL of a certificate could not be found."; /* unused */

		case X509_V_ERR_UNABLE_TO_DECRYPT_CERT_SIGNATURE:
		return
		"the certificate signature could not be decrypted. This means that"
		" the actual signature value could not be determined rather than it"
		" not matching the expected value, this is only meaningful for RSA"
		" keys.";

		case X509_V_ERR_UNABLE_TO_DECRYPT_CRL_SIGNATURE:
		return
		"the CRL signature could not be decrypted: this means that the"
		" actual signature value could not be determined rather than it not"
		" matching the expected value."; /* unused */

		case X509_V_ERR_UNABLE_TO_DECODE_ISSUER_PUBLIC_KEY:
		return
		"the public key in the certificate SubjectPublicKeyInfo could not be"
		" read.";

		case X509_V_ERR_CERT_SIGNATURE_FAILURE:
		return
		"the signature of the certificate is invalid.";

		case X509_V_ERR_CRL_SIGNATURE_FAILURE:
		return "the signature of the certificate is invalid."; /* unused */

		case X509_V_ERR_CERT_NOT_YET_VALID:
		return
		"the certificate is not yet valid: the notBefore date is after the"
		" current time.";

		case X509_V_ERR_CERT_HAS_EXPIRED:
		return
		"the certificate has expired: that is the notAfter date is before"
		" the current time.";

		case X509_V_ERR_CRL_NOT_YET_VALID:
		return "the CRL is not yet valid."; /* unused */

		case X509_V_ERR_CRL_HAS_EXPIRED:
		return "the CRL has expired."; /* unused */

		case X509_V_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD:
		return
		"the certificate notBefore field contains an invalid time.";

		case X509_V_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD:
		return
		"the certificate notAfter field contains an invalid time.";

		case X509_V_ERR_ERROR_IN_CRL_LAST_UPDATE_FIELD:
		return
		"the CRL lastUpdate field contains an invalid time."; /* unused */

		case X509_V_ERR_ERROR_IN_CRL_NEXT_UPDATE_FIELD:
		return
		"the CRL nextUpdate field contains an invalid time."; /* unused */

		case X509_V_ERR_OUT_OF_MEM:
		return
		"an error occurred trying to allocate memory. This should never"
		" happen.";

		case X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT:
		return
		"the passed certificate is self signed and the same certificate"
		" cannot be found in the list of trusted certificates.";

		case X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN:
		return
		"the certificate chain could be built up using the untrusted"
		" certificates but the root could not be found locally.";

		case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY:
		return
		"the issuer certificate of a locally looked up certificate could not"
		" be found. This normally means the list of trusted certificates is"
		" not complete.";

		case X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE:
		return
		"no signatures could be verified because the chain contains only one"
		" certificate and it is not self signed.";

		case X509_V_ERR_CERT_CHAIN_TOO_LONG:
		return
		"the certificate chain length is greater than the supplied maximum"
		" depth.";

		case X509_V_ERR_CERT_REVOKED:
		return
		"the certificate has been revoked."; /* unused */

		case X509_V_ERR_INVALID_CA:
		return
		"a CA certificate is invalid. Either it is not a CA or its"
		" extensions are not consistent with the supplied purpose.";

		case X509_V_ERR_PATH_LENGTH_EXCEEDED:
		return
		"the basicConstraints pathlength parameter has been exceeded.";

		case X509_V_ERR_INVALID_PURPOSE:
		return 
		"the supplied certificate cannot be used for the specified purpose.";

		case X509_V_ERR_CERT_UNTRUSTED:
		return
		"the root CA is not marked as trusted for the specified purpose.";

		case X509_V_ERR_CERT_REJECTED:
		return
		"the root CA is marked to reject the specified purpose.";

		case X509_V_ERR_SUBJECT_ISSUER_MISMATCH:
		return 
		"the current candidate issuer certificate was rejected because its"
		" subject name did not match the issuer name of the current"
		" certificate. Only displayed when the -issuer_checks option is set.";

		case X509_V_ERR_AKID_SKID_MISMATCH:
		return
		"the current candidate issuer certificate was rejected because its"
		" subject key identifier was present and did not match the authority"
		" key identifier current certificate. Only displayed when the"
		" -issuer_checks option is set.";

		case X509_V_ERR_AKID_ISSUER_SERIAL_MISMATCH:
		return 
		"the current candidate issuer certificate was rejected because its"
		" issuer name and serial number was present and did not match the"
		" authority key identifier of the current certificate. Only displayed"
		" when the -issuer_checks option is set.";

		case X509_V_ERR_KEYUSAGE_NO_CERTSIGN:
		return
			"the current candidate issuer certificate was rejected because its"
            " keyUsage extension does not permit certificate signing.";

		case X509_V_ERR_APPLICATION_VERIFICATION: 
		   return "server presented no certificate";
	}

	return "unknown verification error";
}
#endif

int main(int argc, char *argv[])
{
	cp_client *client;
    struct sigaction act;
	char request[0x100];
	char *hostbuf, *host, *uri, *pbuf;
	int port;
	cp_string *buf;
	char *CA_file;
	int use_ssl = 0;
	int rc;

	act.sa_handler = sigpipe_handler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;

	sigaction(SIGPIPE, &act, NULL);

	cp_log_init("test_client.log", LOG_LEVEL_DEBUG);

	if (argc < 2) 
	{
		printf("please specify url\n");
		exit(1);
	}

	if (argc < 3)
	{
		printf("no CA file specified - SSL disabled\n");
		CA_file = NULL;
	}
	else
	{
		use_ssl = 1;
		CA_file = argv[2];
		cp_client_ssl_init();
	}

	hostbuf = strdup(argv[1]);
	if ((host = strstr(hostbuf, "://")) != NULL)
		host += 3;
	else
		host = hostbuf;

	pbuf = strchr(host, ':');
	if (pbuf)
	{
		*pbuf = '\0';
		port = atoi(++pbuf);
	}
	else
	{
		port = 443;
		pbuf = host;
	}

	uri = strchr(pbuf, '/');
	if (uri == NULL) 
		uri = strdup(NOURI);
	else
	{
		char *tmp = uri;
		uri = strdup(tmp);
		*tmp = '\0';
	}

	if (CA_file)
		client = cp_client_create_ssl(host, port, CA_file, NULL, SSL_VERIFY_NONE);
	else
		client = cp_client_create(host, port);
	if (client == NULL)
	{
		printf("can\'t create client\n");
		exit(2);
	}

	if ((rc = cp_client_connect(client)) == -1)
	{
		printf("connect failed\n");
		exit(3);
	}
	else if (rc > 0)
	{
		printf("verification error: %s\n", ssl_verification_error_str(rc));
	}

	if (client->server_certificate)
		cp_client_verify_hostname(client);

	sprintf(request, request_fmt, uri, host);

	if ((rc = cp_client_write(client, request, strlen(request))) == -1)
		perror("write");

	buf = cp_string_create("", 0);
	rc = cp_client_read_string(client, buf, 0);

	printf("%s\n[%d bytes read]\n", cp_string_tocstr(buf), cp_string_len(buf));

	free(uri);
	free(hostbuf);
	cp_string_destroy(buf);
	cp_client_close(client);
	cp_client_destroy(client);

	if (use_ssl)
		cp_client_ssl_shutdown();

	cp_log_close();

	return 0;
}

