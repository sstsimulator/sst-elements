#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include <cprops/config.h>

#ifdef CP_HAS_REGEX_H
#include <regex.h>
#else
#include <pcreposix.h>
#endif /* CP_HAS_REGEX_H */

#include <cprops/log.h>
#include <cprops/str.h>
#include <cprops/httpclient.h>

#ifdef CP_USE_SSL
#include <openssl/ssl.h>
#endif /* CP_USE_SSL */

static char *url_re_lit = 
	"^(http(s)?:\\/\\/)?([^:\\/]+)(\\:[0-9]{1,5})?(\\/.*)?$";

int main(int argc, char *argv[])
{
	int rcc;
	cp_httpclient *client;
	regex_t url_re;
	regmatch_t rm[6];
	
	cp_string *host;
	int ssl;
	int port;
	cp_string *uri;
	
	cp_httpclient_result *result;
	cp_http_response *res;

	if (argc < 2)
	{
		printf("do this: %s \"url\"\n", argv[0]);
		return 1;
	}

	if ((rcc = regcomp(&url_re, url_re_lit, REG_EXTENDED | REG_ICASE)) != 0)
	{
		char errbuf[0x100];
		regerror(rcc, &url_re, errbuf, 0x100);
		printf("regex error: %s\n", errbuf);
		return 2;
	}
	
	if (regexec(&url_re, argv[1], 6, rm, 0))
	{
		printf("can\'t parse url: %s\n", argv[1]);
		return 3;
	}
	
	ssl = rm[2].rm_so != -1;
	host = cp_string_create(&argv[1][rm[3].rm_so], rm[3].rm_eo - rm[3].rm_so);
	if (rm[4].rm_so != -1)
		port = atoi(&argv[1][rm[4].rm_so + 1]);
	else
		port = ssl ? 443 : 80;
	if (rm[5].rm_so != -1)
		uri = cp_string_create(&argv[1][rm[5].rm_so], rm[5].rm_eo - rm[5].rm_so);
	else
		uri = cp_string_create("/", 1);

	printf("host: %s\n", cp_string_tocstr(host));
	printf("port: %d\n", port);
	printf("uri:  %s\n", cp_string_tocstr(uri));
#ifdef CP_USE_SSL
	printf("ssl:  %s\n", ssl ? "yes" : "no");
#else
	if (ssl) 
	{
		fprintf(stderr, "can\'t fetch: cprops configured with --disable-ssl\n");
		exit(1);
	}
#endif /* CP_USE_SSL */
	
	cp_log_init("test_httpclient.log", 0); //LOG_LEVEL_DEBUG);
	cp_httpclient_init();
	
#ifdef CP_USE_SSL
	if (ssl)
		client = cp_httpclient_create_ssl(cp_string_tocstr(host), port, "../test/cacert.pem", NULL, SSL_VERIFY_NONE);
	else
#endif /* CP_USE_SSL */
		client = cp_httpclient_create(cp_string_tocstr(host), port);

	if (client)
	{
		result = cp_httpclient_fetch(client, cp_string_tocstr(uri));
		if (result)
		{
			res = cp_httpclient_result_get_response(result);
			if (res && res->content) 
			{
				struct timeval done;
				unsigned long span;
				
				gettimeofday(&done, NULL);
				span = done.tv_sec * 1000000 + done.tv_usec - 
						(client->socket->created.tv_sec * 1000000 + 
						 client->socket->created.tv_usec);
				
				printf("%s\n", cp_string_tocstr(res->content));
				printf("%ld bytes read (content: %ld bytes) in %ld.%03ld sec (%03.3f kb/sec)\n",
						client->socket->bytes_read, cp_string_len(res->content), 
						(span / 1000000), (span % 1000000) / 1000,
						1000.0 * (float) client->socket->bytes_read / (float) span);
				cp_httpclient_result_destroy(result);
			}
		}
		cp_httpclient_destroy(client);
	}
	else
		printf("can\'t make connection - cp_httpclient_create failed\n");

	cp_string_destroy(host);
	cp_string_destroy(uri);

	cp_httpclient_shutdown();
	cp_log_close();

	regfree(&url_re);
	return 0;
}

