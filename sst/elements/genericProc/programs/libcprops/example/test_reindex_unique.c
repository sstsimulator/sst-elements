#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <cprops/multimap.h>

/* for getopt */
#if defined(unix) || defined(__unix__) || defined(__MACH__)
#include <unistd.h>
#else
#include <cprops/util.h>
#endif

/* test multimap reindexing
 *
 * a sequence has a starting point, an end point and an id. The test program
 * creates a bunch of sequences and checks the ordering is correct when sorting
 * by start point, end point and total length. Then a random sequence is chosen
 * and its start point, end point or both are changed. The ordering is checked
 * for all indices. This process is repeated as many times as necessary to 
 * convince the tester that reindexing actually works. 
 *
 * ./test_reindex [-n items] [-r repetitions]
 *
 * where items is the number of sequences to create, repetitions determines how
 * many times the test is to be repeated. Defaults are 0x10000 items and
 * 0x10000 repetitions.
 */

static int curr_sequence_id = 0;
int max_sequences = 0x10000;
int max_repeat = 0x10000;
int verbose = 0;

typedef struct _sequence
{
	int id;
	double x0;
	double x1;
} sequence;

void sequence_dump(sequence *q)
{
	printf("[%d, %.02lf -- %.02lf {%.02lf}]", q->id, q->x0, q->x1, q->x1 - q->x0);
}

void sequence_randomize(sequence *q)
{
	q->x0 = (double) (rand() % 10000) / 100;
	q->x1 = (double) (rand() % 10000) / 100;
	if (q->x1 < q->x0)
	{
		double tmp = q->x1;
		q->x1 = q->x0;
		q->x0 = tmp;
	}
}

void next_test_sequence(sequence *q)
{
	q->id = ++curr_sequence_id;
	sequence_randomize(q);
	if (verbose)
	{
		printf("created sequence %d: ", curr_sequence_id);
		sequence_dump(q);
		printf("\n");
	}
}

sequence *sequence_dup(sequence *s)
{
	sequence *q = (sequence *) malloc(sizeof(sequence));
	if (q == NULL) return NULL;
	memcpy(q, s, sizeof(sequence));
	return q;
}

static int dtoi_cmp(double d)
{
	if (d > 0) return 1;
	if (d < 0) return -1;
	return 0;
}

int sequence_start_cmp(void *s, void *q)
{
	double diff = ((sequence *)s )->x0 - ((sequence *) q)->x0;
	return dtoi_cmp(diff);
}

int sequence_end_cmp(void *s, void *q)
{
	double diff = ((sequence *)s )->x1 - ((sequence *) q)->x1;
	return dtoi_cmp(diff);
}

int sequence_id_cmp(void *s, void *q)
{
	double diff = ((sequence *) s)->id - ((sequence *) q)->id;
	return dtoi_cmp(diff);
}

int sequence_length_cmp(void *s, void *q)
{
	double diff =  
		   (((sequence *) s)->x1 - ((sequence *) s)->x0) - 
		   (((sequence *) q)->x1 - ((sequence *) q)->x0);
	return dtoi_cmp(diff);
}


int process_cmdline(int argc, char *argv[])
{
    int rc;
	int ret = 0;
    while ((rc = getopt(argc, argv, "n:r:vh")) != -1)
    {
        switch (rc)
        {
            case 'n': max_sequences = atoi(optarg); break;
            case 'r': max_repeat = atoi(optarg); break;
			case 'v': verbose = 1; break;
			case 'h': ret = -1; break;
            default:
                printf("unknown option -%c\n", rc);
				ret = -1;
        }
    }

    return ret;
}

void print_usage()
{
	printf("./test_reindex [-n items] [-r repetitions]\n\n"
		   "where items is the number of sequences to create, repetitions "
		   "determines how\n"
		   "many times the test is to be repeated. Defaults are 0x10000 "
		   "items, 0x10000\n"
		   "repetitions.\n\n");
	exit(1);
}

cp_multimap *t;
double d, n;
cp_index *current;
cp_index *start_index;
cp_index *end_index;
cp_index *length_index;
cp_index *indices[3];
int item_counts[3];
int *item_count;
char *index_name[] = { "length", "start", "end" };

int not_descending(void *entry, void *p)
{
	cp_index_map_node *node = (cp_index_map_node *) entry;
	cp_vector *items = (cp_vector *) node->entry;
	sequence *q = (sequence *) cp_vector_element_at(items, 0);

	if (current == length_index)
		n = q->x1 - q->x0;
	else if (current == start_index)
		n = q->x0;
	else if (current == end_index)
		n = q->x1;

	if (verbose) { sequence_dump(q); printf("\n"); }

	if (n >= d)
		d = n;
	else 
	{
		printf("order check failed, violating segmentation\n");
		*((char *) 0) = 0; // as a courtesy to tester, dump core
	}

	*item_count += cp_vector_size(items);
	return 0;
}

	
int check_order()
{
	int i;
	int len = sizeof(indices) / sizeof (cp_index *);
	for (i = 0; i <  len; i++)
	{
		if (verbose) printf("checking order for sequence %s\n", index_name[i]);
		d = 0;
		current = indices[i];
		item_count = &item_counts[i];
		*item_count = 0;
		if (cp_multimap_callback_by_index(t, current, not_descending, &d)) 
			return i + 1;
	}

	if (item_counts[0] != max_sequences ||
		item_counts[1] != max_sequences || 
		item_counts[2] != max_sequences)
	{
		printf("item counts: %d, %d, %d vs. %d\n", 
			item_counts[0], 
			item_counts[1], 
			item_counts[2], 
			max_sequences);
		return -1;
	}

	return 0;
}

int main(int argc, char *argv[])
{
	int count;
	int rc;
	sequence *s, dummy;
	int err;

	srand(time(NULL));
    if (process_cmdline(argc, argv)) print_usage();

	/* default index is id - a unique index. */
	t = cp_multimap_create_by_option(COLLECTION_MODE_COPY |
									 COLLECTION_MODE_DEEP | 
									 COLLECTION_MODE_NOSYNC,
									 NULL, 
								     (cp_compare_fn) sequence_id_cmp, 
									 (cp_copy_fn) sequence_dup, 
									 (cp_destructor_fn) free);
	if (t == NULL)
	{
		perror("create");
		exit(1);
	}

	start_index = 
		cp_multimap_create_index(t, CP_MULTIPLE, NULL, sequence_start_cmp, &rc);
	if (start_index == NULL)
	{
		fprintf(stderr, "received error %d\n", rc);
		exit(rc);
	}
	
	end_index = 
		cp_multimap_create_index(t, CP_MULTIPLE, NULL, sequence_end_cmp, &rc);
	if (end_index == NULL)
	{
		fprintf(stderr, "received error %d\n", rc);
		exit(rc);
	}
	
	length_index = 
		cp_multimap_create_index(t, CP_MULTIPLE, NULL, sequence_length_cmp, &rc);
	if (length_index == NULL)
	{
		fprintf(stderr, "received error %d\n", rc);
		exit(rc);
	}

	indices[0] = length_index;
	indices[1] = start_index;
	indices[2] = end_index;

	printf("creating %d entries\n", max_sequences);
	for (count = 0; count < max_sequences; count++)
	{
		next_test_sequence(&dummy);
		cp_multimap_insert(t, &dummy, &err);
	}
	
	for (count = 0; count < max_sequences; count++)
	{
		dummy.id = count + 1;
		s = cp_multimap_get(t, &dummy);
		if (s == NULL)
		{
			printf("error looking up sequence number %d of %d. sorry.\n", dummy.id, max_sequences);
			goto FAIL;
		}
	}
	printf("before reindexing: all sequences found\n");
	if ((rc = check_order()))
	{
		printf("order error encountered on index \"%s\" before reindexing test. sorry.\n", index_name[rc - 1], count);
		goto FAIL;
	}

	printf("testing %d times\n", max_repeat);
	for (count = 0; count < max_repeat; count++)
	{
		dummy.id = (rand() % max_sequences) + 1;
		s = cp_multimap_get(t, &dummy);
		if (s == NULL)
		{
			printf("error looking up sequence number %d of %d. sorry.\n", dummy.id, max_sequences);
			goto FAIL;
		}
		sequence_randomize(&dummy);
		if (verbose)
		{
			printf("reindexing: ");
			sequence_dump(s);
			printf(" => ");
			sequence_dump(&dummy);
			printf("\n\n");
		}
		if ((rc = cp_multimap_reindex(t, s, &dummy)))
		{
			printf("reindexing failed. sorry.\n");
			goto FAIL;
		}

		if ((rc = check_order()))
		{
			printf("order error encountered on index \"%s\" after %d iterations. sorry.\n", index_name[rc - 1], count);
			goto FAIL;
		}
	}

	cp_multimap_destroy(t);

	printf("no reindexing errors encountered\n");
	return 0;

FAIL:
	printf("for your convenience, dumping core (where available)\n");
	*((char *) 0) = 0;
	exit(1);
}

