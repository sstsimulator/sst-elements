#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <cprops/thread.h>
#include <cprops/linked_list.h>
#include <cprops/hashtable.h>

#define COUNT 10
#define INSERTS 5000

int silent = 0;

cp_hashtable *t[COUNT];
cp_list *tl[COUNT];
cp_cond cond[COUNT];
cp_mutex lock[COUNT];
int running = 0;

cp_mutex start_mutex;
cp_cond start_cond;

void *writer(void *prm)
{
	int i, num;
	long count = (long) prm;
	char kbuf[30];
	char *entry;

	cp_mutex_lock(&start_mutex);
	while (!running)
		cp_cond_wait(&start_cond, &start_mutex);
	cp_mutex_unlock(&start_mutex);
	for (i = 0; i < count; i++)
	{
		sprintf(kbuf, "ENTRY %d", i);
		entry = strdup(kbuf);
		num = i % COUNT;
		if (!silent)
			printf("writing (%d): %s\n", num, entry);

		cp_mutex_lock(&lock[i % COUNT]);
		cp_hashtable_put(t[num], entry, entry);
		cp_list_append(tl[num], entry);
		cp_cond_broadcast(&cond[i % COUNT]);
		cp_mutex_unlock(&lock[i % COUNT]);
	}

	return NULL;
}

#if defined(unix) || defined(__unix__) || defined(__MACH__)
#define write_err perror
#endif

#ifdef _WINDOWS
void write_err(char *lpszFunction)
{
    LPVOID lpMsgBuf;
    LPVOID lpDisplayBuf;
    DWORD dw = GetLastError(); 

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL );

    lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT, 
        (lstrlen((LPCTSTR)lpMsgBuf)+lstrlen((LPCTSTR)lpszFunction)+40)*sizeof(TCHAR)); 
    sprintf((LPTSTR)lpDisplayBuf, 
        TEXT("%s failed with error %d: %s\n"), 
        lpszFunction, dw, lpMsgBuf); 
	printf(lpDisplayBuf);

    LocalFree(lpMsgBuf);
    LocalFree(lpDisplayBuf);
}
#endif /* _WINDOWS */

void *reader(void *prm)
{
	char *entry;
	int num = (int) prm;
	int total = 0;
	int rc;

	cp_mutex_lock(&start_mutex);
	while (!running)
		cp_cond_wait(&start_cond, &start_mutex);
	rc = cp_mutex_unlock(&start_mutex);
	if (rc == 0) write_err("reader");
	while (running || cp_hashtable_count(t[num]) > 0)
	{
		cp_mutex_lock(&lock[num]);
		while (running && cp_list_is_empty(tl[num]))
			cp_cond_wait(&cond[num], &lock[num]);

		while ((entry = (char *) cp_list_remove_head(tl[num])))
		{
			cp_hashtable_remove(t[num], entry);
			if (!silent)
				printf("[%d]: (%ld) entry: %s\n", num, cp_hashtable_count(t[num]), entry);
			total++;
			free(entry);
		}
		cp_mutex_unlock(&lock[num]);
	}

	printf("\n (-) reader %d: processed %d entries\n", num, total);
	return NULL;
}

int main(int argc, char *argv[])
{
	int i;
	cp_thread w[COUNT];
	cp_thread r[COUNT];
	long total;
	int rc;

	if (argc > 1) silent = atoi(argv[1]);

	for (i = 0; i < COUNT; i++)
	{
		rc = cp_mutex_init(&lock[i], NULL);
		cp_cond_init(&cond[i], NULL);
		t[i] = cp_hashtable_create(10, cp_hash_string, cp_hash_compare_string);
		tl[i] = cp_list_create();
	}


	rc = cp_mutex_init(&start_mutex, NULL);
	cp_cond_init(&start_cond, NULL);

	for (i = 0; i < COUNT; i++)
		cp_thread_create(r[i], NULL, reader, (void *) i);

	for (i = 0; i < COUNT; i++)
		cp_thread_create(w[i], NULL, writer, (void *) INSERTS);

	printf("press enter\n");
	getchar();
	cp_mutex_lock(&start_mutex);
	running = 1;
	total = time(NULL);
	cp_cond_broadcast(&start_cond);
	rc = cp_mutex_unlock(&start_mutex);
	if (rc == 0) write_err("MAIN");
	for (i = 0; i < COUNT; i++)
		cp_thread_join(w[i], NULL);
	running = 0;

	for (i = 0; i < COUNT; i++)
	{
		cp_mutex_lock(&lock[i]);
		cp_cond_broadcast(&cond[i]);
		cp_mutex_unlock(&lock[i]);
		cp_thread_join(r[i], NULL);
	}

	total = time(NULL) - total;

	printf("\ndone in %ld seconds. tables should be empty now. press enter.\n",
			total);
	getchar();

	for (i = 0; i < COUNT; i++)
	{
		printf("table %d: %ld items\n", i, cp_hashtable_count(t[i]));
		cp_hashtable_destroy(t[i]);
		printf("list %d: %ld items\n", i, cp_list_item_count(tl[i]));
		while (cp_list_item_count(tl[i]))
		{
			char *leftover = cp_list_remove_head(tl[i]);
			printf("       * %s\n", leftover);
		}
		cp_list_destroy(tl[i]);
	}

	printf("deleted them tables. press enter.\n");
	getchar();

	printf("bye.\n");
	return 0;
}

