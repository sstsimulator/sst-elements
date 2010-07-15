#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <errno.h>

#include <cprops/btree.h>

#define ORDER 5

static char cmpbuf[0x400];

/* break if the current entry is smaller than the previous */
int compare_entry(cp_mapping *m, void *prm)
{
	int rc;
	cp_string *p = (cp_string *) m->key;
	if ((rc = strcmp(cp_string_tocstr(p), cmpbuf)) < 0)
	{
		printf("btest: key [%s] precedes key [%s] (%d)\n",
			cmpbuf, cp_string_tocstr(p), rc);
		{ int i; for (i = 0; i < 30; i++) printf("\n"); }
		*(char *) 0 = 1;
		return rc;
	}

	strcpy(cmpbuf, cp_string_tocstr(p));

	return 0;
}

int check_order(cp_btree_file *bf)
{
	cmpbuf[0] = '\0';
	return cp_btree_file_callback(bf, compare_entry, NULL);
}

int main(int argc, char *argv[])
{
    int rc;
	int opcount = 0;
	int insert_count = 0;
	int delete_count = 0;
    cp_btree_file *bf = NULL;
	cp_btreenode *p, *c1; //, *c2, *cc1, *cc2;
	cp_string *name, *number;

	cp_log_init("btest.log", 0);

    if (argc < 2) 
    {
        fprintf(stderr, "usage: %s filename\n", argv[0]);
        exit(1);
    }

    bf = cp_btree_file_open(argv[1], &rc);
    if (rc == CP_FILE_NOT_FOUND)
    {
        bf = cp_btree_file_create(argv[1], ORDER, 
							 	  COLLECTION_MODE_NOSYNC, // | COLLECTION_MODE_DEEP,
								  NULL, "cp_string_cmp", 
								  NULL, "cp_string_destroy", 
								  NULL, "cp_string_destroy", 
								  "cp_string_serialize", 
							      "cp_string_deserialize", 
								  "cp_string_serialize", 
								  "cp_string_deserialize", &rc);
    }

    if (rc) 
    {
        fprintf(stderr, "can\'t read file [%s]\n", argv[1]);
        exit(2);
    }

	name = cp_string_create_empty(81);
	number = cp_string_create_empty(81);

	while (1)
	{
		char *x;
		char *_name = name->data;
		char *_number = number->data;

		printf("\n\n\n\n\n[== %d ==] enter name [next]: ", opcount);
		_name[0] = '\0';
		fgets(_name, 80, stdin);
		_name[strlen(_name) - 1] = '\0'; /* chop newline */
		if (_name[0] == '\0') break;
		name->len = strlen(_name);

		x = cp_btree_file_get(bf, name);
		if (x == NULL)
		{
			printf("enter number [next]: ");
			_number[0] = '\0';
			fgets(_number, 80, stdin);
			_number[strlen(_number) - 1] = '\0'; /* chop newline */
			if (_number[0] == '\0') break;

			printf("INSERT %s => %s\n", _name, _number);
			cp_btree_file_insert(bf, cp_string_cstrdup(_name), cp_string_cstrdup(_number));
			insert_count++;
		}
		else
		{
			printf("DELETE %s\n", _name);
			cp_btree_file_delete(bf, name);
			delete_count++;
		}

		printf("\nsize: %d +%d -%d\n", cp_btree_file_count(bf), insert_count, delete_count);

		if (check_order(bf))
		{
			printf("invalid order - dumping core\n");
			*(char *) 0 = 1;
		}

		cp_btree_file_print(bf);
	}

	while (1)
	{
		char *_name = name->data;
		cp_string *_number;

		printf("enter name [quit]: ");
		_name[0] = '\0';
		fgets(_name, 80, stdin);
		_name[strlen(_name) - 1] = '\0'; /* chop newline */
		name->len = strlen(_name);
		if (_name[0] == '\0') break;

		printf("contains: %d\n", cp_btree_file_get(bf, name) != NULL);

		if ((_number = cp_btree_file_get(bf, name)) != NULL)
			printf("%s: %s\n", _name, cp_string_tocstr(_number));
		else
			printf("no entry for %s\n", _name);
	}

    cp_btree_file_print(bf);
    cp_btree_file_close(bf);
	cp_log_close();
    return 0;
}

