#include <stdio.h>
#include <string.h>     /* for strdup */
#include <stdlib.h>     /* for free */
#include <errno.h>
#include <cprops/rb.h>

#ifdef CP_HAS_STRINGS_H
#include <strings.h>    /* for strcasecmp */
#else
#include <cprops/util.h>
#endif /* CP_HAS_STRINGS_H */

#ifdef DEBUG
int check_tree(cp_rbtree *t);
int node_count(cp_rbtree *t);
#endif /* DEBUG */

int main()
{
	int opcount = 0;
	int insert_count = 0;
	int delete_count = 0;
	int ncount;
	cp_rbtree *t =
	    cp_rbtree_create_by_option(COLLECTION_MODE_COPY | 
				                   COLLECTION_MODE_DEEP | 
								   COLLECTION_MODE_NOSYNC,
				                   (cp_compare_fn) strcmp,
								   (cp_copy_fn) strdup, free, 
								   (cp_copy_fn) strdup, free);
	
	if (t == NULL)
	{
		perror("create");
		exit(1);
	}

	while (++opcount)
   	{
		char *x;
		char name[80];
		char number[80];

		printf("[== %d ==] enter name [next]: ", opcount);
		name[0] = '\0';
		fgets(name, 80, stdin);
		name[strlen(name) - 1] = '\0'; /* chop newline */
		if (name[0] == '\0') break;
#if 0
		printf("enter number [next]: ");
		number[0] = '\0';
		fgets(number, 80, stdin);
		number[strlen(number) - 1] = '\0'; /* chop newline */
		if (number[0] == '\0') break;

		cp_rbtree_insert(t, strdup(name), strdup(number));
#endif
		x = cp_rbtree_get(t, name);
		if (x == NULL)
		{
			printf("INSERT %s\n", name);
			cp_rbtree_insert(t, name, name);
			insert_count++;
		}
		else
		{
			printf("DELETE %s\n", name);
			cp_rbtree_delete(t, name);
			delete_count++;
		}


		printf("\nsize: %d +%d -%d\n", cp_rbtree_count(t), insert_count, delete_count);

#ifdef DEBUG
		if (check_tree(t)) exit(-1);
#endif
		cp_rbtree_dump(t);
	}

	while (1)
	{
		char name[80];
		char *number;

		printf("enter name [quit]: ");
		name[0] = '\0';
		fgets(name, 80, stdin);
		name[strlen(name) - 1] = '\0'; /* chop newline */
		if (name[0] == '\0') break;

		printf("contains: %d\n", cp_rbtree_contains(t, name));

		if ((number = cp_rbtree_get(t, name)) != NULL)
			printf("%s: %s\n", name, number);
		else
			printf("no entry for %s\n", name);
	}

	cp_rbtree_destroy(t);
	return 0;
}

#ifdef DEBUG

int scan_rbtree(cp_rbnode *node, int black_count, int *max_count)
{
	int rc;
	if (node == NULL)
	{
		if (*max_count == 0) 
		{
			*max_count = black_count;
			return 1;
		}
		else 
		{
			if (*max_count != black_count)
			{
				printf("IMBALANCE: max_count %d, black_count %d\n", 
						*max_count, black_count);
				return -1;
			}
			
			return 1;
		}
	}

	if (node->color == RB_BLACK) black_count++;
	else 
	{
		if ((node->right && node->right->color == RB_RED) ||
			(node->left && node->left->color == RB_RED))
		{
			printf("IMBALANCE: in node [%s]: red node with red child/ren\n",
				   (char *) node->key);
			return 0;
		}
	}
			
	rc = scan_rbtree(node->left, black_count, max_count);
	if (rc == -1) 
	{
		printf("offending node: %s\n", node->key);
		return 0;
	}
	else if (rc == 0) return 0;
	
	rc = scan_rbtree(node->right, black_count, max_count);
	if (rc == -1)
	{
		printf("offending node: %s\n", node->key);
		return 0;
	}
	else if (rc == 0) return 0;
	return 1;
}

int check_tree(cp_rbtree *t)
{
	int valid;
	int black_count = 0;
	int max_count = 0;
	
	if (t->root && t->root->color == RB_RED) 
	{
		printf("IMBALANCE: root is RED\n");
		cp_rbtree_dump(t);
		return -1;
	}

	valid = scan_rbtree(t->root, black_count, &max_count);
	if (!valid)	
		cp_rbtree_dump(t);
	if (valid) return 0;
	return -1;
}

void count_node(cp_rbnode *node, int *count)
{
	if (node)
	{
		(*count)++;
		if (node->right) count_node(node->right, count);
		if (node->left) count_node(node->left, count);
	}
}

int node_count(cp_rbtree *t)
{
	int res = 0;
	count_node(t->root, &res);
	return res;
}

#endif /* DEBUG */
