#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include <cprops/config.h>

#ifdef HAVE_REGEX_H
#include <regex.h>
#else
#include <pcreposix.h>
#endif

#include <cprops/log.h>
#include <cprops/str.h>
#include <cprops/httpclient.h>

#ifdef CP_USE_SSL
#include <openssl/ssl.h>
#endif /* CP_USE_SSL */

#ifdef HAVE_POLL
#include <sys/poll.h>
#else
#ifdef CP_HAS_SYS_SELECT_H
#include <sys/select.h>
#endif /* CP_HAS_SYS_SELECT_H */
#endif /* HAVE_POLL */

typedef struct _invoke_spec
{
	cp_httpclient *client;
	char *uri;
} invoke_spec;

static char *url_re_lit = 
	"^(http(s)?:\\/\\/)?([^:\\/]+)(\\:[0-9]{1,5})?(\\/.*)?$";

regex_t url_re;

invoke_spec *parse_prm(char *prm)
{
	cp_httpclient *client;
	regmatch_t rm[6];
	invoke_spec *spec = NULL;
	
	cp_string *host;
	int ssl;
	int port;
	cp_string *uri;

	if (regexec(&url_re, prm, 6, rm, 0))
	{
		printf("can\'t parse url: %s\n", prm);
		return NULL;
	}
	
	ssl = rm[2].rm_so != -1;
	host = cp_string_create(&prm[rm[3].rm_so], rm[3].rm_eo - rm[3].rm_so);
	if (rm[4].rm_so != -1)
		port = atoi(&prm[rm[4].rm_so + 1]);
	else
		port = ssl ? 443 : 80;
	if (rm[5].rm_so != -1)
		uri = cp_string_create(&prm[rm[5].rm_so], rm[5].rm_eo - rm[5].rm_so);
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
	
#ifdef CP_USE_SSL
	if (ssl)
		client = cp_httpclient_create_ssl(cp_string_tocstr(host), port, "../test/cacert.pem", "test", SSL_VERIFY_NONE);
	else
#endif /* CP_USE_SSL */
		client = cp_httpclient_create(cp_string_tocstr(host), port);
	
	cp_string_destroy(host);

	if (client == NULL)
	{
		free(uri);
		return NULL;
	}
	
	spec = calloc(1, sizeof(invoke_spec));
	spec->client = client;
	spec->uri = uri->data;
	cp_string_drop_wrap(uri);

	return spec;
}

int *ids;
int results = 0;

void write_result(cp_httpclient_result *res)
{
	char filename[80];
	int id;

	id = *(int *) res->id;
	if (res->result != CP_HTTP_RESULT_SUCCESS)
	{
		fprintf(stderr, "request %d: error\n", id);
	}
	else
	{
		sprintf(filename, "tha-%d.html", id);
		cp_string_write_file(res->response->content, filename);
	}

	results++;
	
}

int main(int argc, char *argv[])
{
	int i, rcc;
	invoke_spec **spec;
	int num = argc - 1;
#ifndef HAVE_POLL
	struct timeval delay;
	delay.tv_sec = 0;
	delay.tv_usec = 50000;
#endif

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
	
	cp_log_init("test_httpclient.log", LOG_LEVEL_DEBUG);
	cp_httpclient_init();
	
	spec = malloc(num * sizeof(invoke_spec));
	ids = malloc(num * sizeof(int));
	for (i = 0; i < num; i++)
	{
		spec[i] = parse_prm(argv[i + 1]);
		ids[i] = i;
	}

	for (i = 0; i < num; i++)
		cp_httpclient_fetch_nb(spec[i]->client, spec[i]->uri, &ids[i], 
				write_result, 1 /* run in backgroun thread */);
				
	/* wait for transfers to exit */
#ifdef HAVE_POLL
	while (results < num) poll(NULL, 0, 200);
#else
	while (results < num) select(0, NULL, NULL, NULL, &delay);
#endif

	for (i = 0; i < num; i++)
	{
		cp_httpclient_destroy(spec[i]->client);
		cp_string_drop_content(spec[i]->uri);
		free(spec[i]);
	}
	free(spec);
	free(ids);

	cp_httpclient_shutdown();
	cp_log_close();

	regfree(&url_re);
	return 0;
}

