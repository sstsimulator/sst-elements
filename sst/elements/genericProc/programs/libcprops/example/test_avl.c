#include <stdio.h>
#include <string.h>       /* for strdup */
#include <stdlib.h>       /* for free */
#include <errno.h>
#include <cprops/avl.h>

#ifdef CP_HAS_STRINGS_H
#include <strings.h>      /* for strcasecmp */
#else
#include <cprops/util.h>
#endif /* CP_HAS_STRINGS_H */

#ifdef DEBUG
int check_tree(cp_avltree *t);
int node_count(cp_avltree *t);
#endif

int main()
{
	int opcount = 0;
	int insert_count = 0;
	int delete_count = 0;
	int ncount;
	cp_avltree *t =
		cp_avltree_create_by_option(COLLECTION_MODE_NOSYNC | 
									COLLECTION_MODE_COPY |
									COLLECTION_MODE_DEEP, 
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
//		char number[80];

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

		cp_avltree_insert(t, strdup(name), strdup(number));
#endif
		x = cp_avltree_get(t, name);
		if (x == NULL)
		{
			printf("INSERT %s\n", name);
			cp_avltree_insert(t, name, name);
			insert_count++;
		}
		else
		{
			printf("DELETE %s\n", name);
			cp_avltree_delete(t, name);
			delete_count++;
		}

#ifdef DEBUG
		ncount = node_count(t);

		printf("\nsize: %d +%d -%d\n", cp_avltree_count(t), insert_count, delete_count);
		if (check_tree(t)) exit(-1);
		if (cp_avltree_count(t) < 0) 
		{
			printf("INVALID SIZE\n");
			exit(-2);
		}
		if (ncount != cp_avltree_count(t))
		{
			printf("SIZE ERROR - reported %d actual %d\n",  cp_avltree_count(t), ncount);
			exit(-3);
		}
#else 
		cp_avltree_dump(t);
#endif /* DEBUG */
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

		printf("contains: %d\n", cp_avltree_contains(t, name));

		if ((number = cp_avltree_get(t, name)) != NULL)
			printf("%s: %s\n", name, number);
		else
			printf("no entry for %s\n", name);
	}

	cp_avltree_destroy(t);
	return 0;
}

#ifdef DEBUG

#define MAX(a, b) ((a) > (b) ? (a) : (b))

void dump_heights(cp_avlnode *node, int level)
{
	int i;
	if (node->right) dump_heights(node->right, level + 1);
	for (i = 0; i < level; i++)
		printf("  . ");
	printf("(%d <%d>) [%s => %s]\n", node->balance, node->height, (char *) node->key, (char *) node->value);
	if (node->left) dump_heights(node->left, level + 1);
}

		
int scan_for_max(cp_avlnode *node)
{
	int left, right, balance;
	if (node == NULL) return 0;
	left = scan_for_max(node->left);
	if (left == -1) return -1;
	right = scan_for_max(node->right);
	if (right == -1) return -1;
	node->height = MAX(left, right) + 1;
	
	balance = + (node->right ? node->right->height : 0)  
			  - (node->left ? node->left->height : 0);
	if (balance != node->balance)
	{
		printf("IMBALANCE IN NODE [%s => %s] calc: %d actual: %d\n", (char *) node->key, (char *) node->value, node->balance, balance);
		
		return -1;
	}
	if (node->balance > 1 || node->balance < -1)
	{
		printf("IMBALANCE IN NODE [%s => %s]: overflow (%d)\n", (char *) node->key, (char *) node->value, node->balance);
		
		return -1;
	}

	return node->height;
}

int check_tree(cp_avltree *t)
{
	int max;
	
	max = scan_for_max(t->root);
//	dump_heights(t->root, 0);
//	if (max == -1)	
	cp_avltree_dump(t);
	if (max >= 0) return 0;
	return -1;
}

void count_node(cp_avlnode *node, int *count)
{
	if (node)
	{
		(*count)++;
		if (node->right) count_node(node->right, count);
		if (node->left) count_node(node->left, count);
	}
}

int node_count(cp_avltree *t)
{
	int res = 0;
	count_node(t->root, &res);
	return res;
}

#endif /* DEBUG */
