#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(unix) || defined(__unix__) || defined(__MACH__)
#include <unistd.h>
#endif
#include <cprops/str.h>
#include <cprops/log.h>
#include <cprops/client.h>

char *NOURI = "/";

char *request_fmt = "GET %s HTTP/1.1\nHost: %s\nConnection: close\n\n";

int main(int argc, char *argv[])
{
	cp_client *client;
	char request[0x100];
	char *hostbuf, *host, *uri, *pbuf;
	int port;
	cp_string *buf = cp_string_create_empty(0x100);

	cp_log_init("test_client.log", LOG_LEVEL_DEBUG);

	if (argc < 2) 
	{
		printf("please specify url\n");
		exit(1);
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
		port = 80;
		pbuf = host;
	}

	uri = strchr(pbuf, '/');
	if (uri == NULL) uri = NOURI;

	cp_client_init();

	client = cp_client_create(host, port);
	if (client == NULL)
	{
		printf("can\'t create client\n");
		exit(2);
	}

	if (cp_client_connect(client) == -1)
	{
		printf("connect failed\n");
		exit(3);
	}

	sprintf(request, request_fmt, uri, host);

	if (cp_client_write(client, request, strlen(request)) == -1)
//	if (write(client->fd, request, strlen(request)) == -1)
		perror("write");
	
//	buf = cp_string_read(client->fd, 0);
	cp_client_read_string(client, buf, 0);
	printf("%s\n[%d bytes read]\n", cp_string_tocstr(buf), cp_string_len(buf));
	cp_string_destroy(buf);

	cp_client_close(client);
	cp_client_destroy(client);

	cp_client_shutdown();

	cp_log_close();

	free(hostbuf);

	return 0;
}

