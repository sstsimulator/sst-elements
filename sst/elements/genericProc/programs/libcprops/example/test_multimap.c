#include <stdio.h>
#include <string.h>       /* for strdup */
#include <stdlib.h>       /* for free */
#include <errno.h>
#include <ctype.h>
#include <cprops/multimap.h>

#ifdef CP_HAS_STRINGS_H
#include <strings.h>      /* for strcasecmp */
#else
#include <cprops/util.h>
#endif /* CP_HAS_STRINGS_H */

struct Employee
{
	char name[80];
	char title[80];
	char number[80];
};

void *Employee_name(void *employee)
{
	struct Employee *e = (struct Employee *) employee;
	return &e->name;
}

void *Employee_title(void *employee)
{
	struct Employee *e = (struct Employee *) employee;
	return &e->title;
}

void *Employee_number(void *employee)
{
	struct Employee *e = (struct Employee *) employee;
	return &e->number;
}

int numcmp(void *a, void *b)
{
	return atoi(a) - atoi(b);
}

struct Employee *Employee_dup(void *employee)
{
	struct Employee *e = (struct Employee *) malloc(sizeof(struct Employee));
	if (e) memcpy(e, employee, sizeof(struct Employee));
	return e;
}

void print_command_summary(char *executable)
{
	printf("%s - command summary\n", executable);
	printf("====================================\n"
		   "a [name, title, number]\t - add an entry\n"
		   "d [key]\t - delete entries matching [key] by current index\n"
		   "e [key, [name, title, number], new-value]\t - edit an entry\n"
	       "h\t - print this message\n"
		   "k [name, title, number]\t - switch to chosen index\n"
		   "l\t - list entries by current index\n"
		   "q\t - quit\n");
}		

cp_multimap *t;
cp_index *current = NULL;
cp_index *number_index;
cp_index *title_index;

char *chomp(char *str)
{
	char *p = str;
	char *last_cr = NULL;
	char *last_lf = NULL;

	while (*p)
	{
		if (*p == '\r') last_cr = p;
		if (*p == '\n') last_lf = p;
		p++;
	}

	if (last_cr == p - 2) *last_cr = '\0';
	else if (last_lf == p - 1) *last_lf = '\0';

	return str;
}

char *ltrim(char *str)
{
	char *p = str;
	while (isspace(*p)) p++;
	memmove(str, p, strlen(p) + 1);
	return str;
}

void input_field(char *msg, char *field)
{
	printf("enter %s > ", msg);
	fgets(field, 80, stdin);
	chomp(field);
}

void do_add(char *cmd)
{
	int err = 0;
	char *i, *f;
	struct Employee e;

	i = &cmd[1];
	while (*i && isspace(*i)) i++;
	if (*i)
		f = strchr(i, ',');
	else 
		f = NULL;

	if (f) 
	{
		strncpy(e.name, i, f - i);
		e.name[f - i] = '\0';
		i = f + 1;
		while (*i && isspace(*i)) i++;
		f = strchr(i, ',');
	}
	else
		input_field("name: ", e.name);

	if (f)
	{
		strncpy(e.title, i, f - i);
		e.title[f - i] = '\0';
		i = f + 1;
		while (*i && isspace(*i)) i++;
		f = i; 
		while (*f) f++;
	}
	else
		input_field("title: ", e.title);

	if (f)
	{
		strncpy(e.number, i, f - i);
		e.number[f - i] = '\0';
	}
	else
		input_field("number: ", e.number);

	if (cp_multimap_get(t, &e))
		printf("replacing ");
	else
		printf("adding ");

	printf("entry: [%s, %s, %s]\n", e.name, e.title, e.number);

	cp_multimap_insert(t, &e, &err);
	if (err == CP_UNIQUE_INDEX_VIOLATION)
	{
		printf("unique index violation, insert rejected\n");
	}
}

/* editing:
 * > a JOE, employee, 100
 * > k name
 *
 * > e JOE, number 200
 *
 * or
 *
 * > e
 * enter name to edit > JOE
 * edit record JOE, employee, 200
 * enter field to edit > number
 * enter new number > 200
 */
void do_edit(char *cmd)
{
	struct Employee e;
	struct Employee *curr;
	char *i, *f;
	char field[0x100];
	char value[0x100];
	char *key_field_name;
	char *dst;
	struct Employee *res;
	int before, after;

	memset(&e, 0, sizeof(struct Employee));

	if (current == NULL)
	{
		key_field_name = "name";
		dst = e.name;
	}
	else if (current == title_index)
	{
		key_field_name = "title";
		dst = e.title;
	}
	else if (current == number_index)
	{
		key_field_name = "number";
		dst = e.number;
	}
	else
		key_field_name = "MAYDAY";

	i = &cmd[1];
	while (*i && isspace(*i)) i++;
	if (*i)
		f = strchr(i, ',');
	else 
		f = NULL;

	if (f) 
	{
		strncpy(dst, i, f - i);
		dst[f - i] = '\0';
		i = f + 1;
		while (*i && isspace(*i)) i++;
		f = strchr(i, ' ');
	}
	else
	{
		printf("enter %s to edit > ", key_field_name);
		fgets(field, 0x100, stdin);
		strcpy(dst, field);
	}

	curr = cp_multimap_get_by_index(t, current, &e);
	if (curr == NULL)
	{
		printf("edit: no entry for %s %s\n", key_field_name, dst);
		return;
	}

	if (f)
	{
		strncpy(field, i, f - i);
		field[f - i] = '\0';
		i = f + 1;
		strcpy(value, i);
	}
	else
	{
		printf("enter field to edit > ");
		fgets(field, 0x100, stdin);
		if (strcmp(field, "name") && 
			strcmp(field, "number") && 
			strcmp(field, "title"))
		{
			printf("unrecognized field\n");
			return;
		}
			
		printf("enter new %s > ");
		fgets(value, 0x100, stdin);
	}
	
	memcpy(&e, curr, sizeof(struct Employee));
	if (strcmp(field, "name") == 0)
		strcpy(e.name, value);
	else if (strcmp(field, "title") == 0)
		strcpy(e.title, value);
	else if (strcmp(field, "number") == 0)
		strcpy(e.number, value);
	else
	{
		printf("unrecognized field\n");
		return;
	}

	cp_multimap_reindex(t, curr, &e);
	printf("edit: set %s to %s\n", field, value);
	printf("entry: [%s, %s, %s]\n", e.name, e.title, e.number);
}

void do_delete(char *cmd)
{
	struct Employee e;
	char *i;
	char field[0x100];
	char *key_field_name;
	char *dst;
	struct Employee *res;
	int before, after;

	if (current == NULL)
	{
		key_field_name = "name";
		dst = e.name;
	}
	else if (current == title_index)
	{
		key_field_name = "title";
		dst = e.title;
	}
	else if (current == number_index)
	{
		key_field_name = "number";
		dst = e.number;
	}
	else
		key_field_name = "MAYDAY";

	i = cmd + 1;
	while (*i && isspace(*i)) i++;
	if (*i == '\0')
	{
		printf("enter %s to look up: >", key_field_name);
		fgets(field, 0x100, stdin);
		i = field;
	}

	memset(&e, 0, sizeof(struct Employee));
	strcpy(dst, i);

	before = cp_multimap_size(t);
	res = cp_multimap_remove_by_index(t, current, &e);
	after = cp_multimap_size(t);
	if (res)
		printf("%s: %d entr%s deleted\n", i, before - after, (before - after) > 1 ? "ies" : "y");
	else
		printf("no entries found to match %s %s\n", key_field_name, i);
}

void do_key(char *cmd)
{
	char *i, *f;
	char buf[0x100];

	i = &cmd[1];
	while (*i && isspace(*i)) i++;
	if (*i == '\0')
	{
		printf("enter one of [name, title, number] > ");
		fgets(buf, 0x100, stdin);
		i = buf;
	}

	if (strcmp(i, "name") == 0)
		current = NULL;
	else if (strcmp(i, "title") == 0)
		current = title_index;
	else if (strcmp(i, "number") == 0)
		current = number_index;
	else
		printf("unavailable index %s\n", i);
}

void Employee_dump(struct Employee *e)
{
	printf("Name: %s Title: %s Number: %s\n", e->name, e->title, e->number);
}

int print_Employee_entry(void *entry, void *multiple)
{
	cp_index_map_node *node = entry;

	if (multiple)
	{
		int i;
		cp_vector *v = node->entry;
		for (i = 0; i < cp_vector_size(v); i++)
			Employee_dump(cp_vector_element_at(v, i));
	}
	else
		Employee_dump(node->entry);

	return 0;
}

void do_list(char *cmd)
{
	void *multiple = 
		current == NULL || current->type == CP_UNIQUE ? NULL : (void *) 1;
	cp_multimap_callback_by_index(t, 
	                              current, 
								  print_Employee_entry, 
								  multiple); 
}

#define COMMAND 0x100

int main(int argc, char *argv[])
{
	int rc;
	int opcount = 0;
	int insert_count = 0;
	int delete_count = 0;
	int ncount;
	int done = 0;
	char command[COMMAND + 1];
	int error;

	t = cp_multimap_create_by_option(COLLECTION_MODE_NOSYNC | 
								     COLLECTION_MODE_COPY |
					                 COLLECTION_MODE_DEEP, 
									 Employee_name, 
								     (cp_compare_fn) strcmp, 
									 (cp_copy_fn) Employee_dup, free);
	if (t == NULL)
	{
		perror("create");
		exit(1);
	}

	number_index = 
		cp_multimap_create_index(t, CP_UNIQUE, Employee_number, numcmp, &rc);
	if (number_index == NULL)
	{
		fprintf(stderr, "received error %d\n", rc);
		exit(rc);
	}

	title_index = 
		cp_multimap_create_index(t, CP_MULTIPLE, Employee_title, 
		                         (cp_compare_fn) strcmp, &rc);
	if (title_index == NULL)
	{
		fprintf(stderr, "received error %d\n", rc);
		exit(rc);
	}

	print_command_summary(argv[0]);
	while (!done)
	{
		error = 1;
		printf("> ");
		fgets(command, COMMAND, stdin);
		ltrim(command);
		chomp(command);
		if (strlen(command) == 0) continue;
		if (strcmp(command, "q") == 0 || strcmp(command, "Q") == 0) break;
		if (command[1] == 0x20 || command[1] == '\0')
		{
			error = 0;
			switch (tolower(command[0]))
			{
				case 'a': do_add(command); break;
				case 'd': do_delete(command); break;
				case 'e': do_edit(command); break;
				case 'h': print_command_summary(argv[0]); break;
				case 'k': do_key(command); break;
				case 'l': do_list(command); break;
				default: error = 1;
			}
		}
		if (error)
			printf("unrecognized command: [%s]. Enter 'h' for a list of commands or 'q' to quit.\n", command);
	}

	cp_multimap_destroy(t);
	return 0;
}

