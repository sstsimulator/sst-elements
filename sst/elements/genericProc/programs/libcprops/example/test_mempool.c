#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(unix) || defined(__unix__) || defined(__MACH__)
#include <unistd.h>
#else
#include <cprops/util.h>
#endif
#include <time.h>
#include <cprops/mempool.h>

typedef struct _node
{
	char *item;
	struct _node *next;
} node;

int use_general_pool = 0;
int general_pool_small = 0;
int use_mempool = 1;
int item_size = 32;
int item_variation = 0;
int items = 1000000;
int multiple = 10;
int skip_free = 0;

int process_cmdline(int argc, char *argv[])
{
	int rc;
	while ((rc = getopt(argc, argv, "n:s:m:v:gGuhe")) != -1)
	{
		switch (rc)
		{
			case 'n': items = atoi(optarg); break;
			case 's': item_size = atoi(optarg); break;
			case 'm': multiple = atoi(optarg); break;
			case 'v': item_variation = atoi(optarg); break;
			case 'u': use_mempool = 0; break;
			case 'g': use_general_pool = 1; break;
			case 'G': { use_general_pool = 1; general_pool_small = 1; } break;
			case 'e': skip_free = 1; break;
			case 'h': return -1;
			default:
				printf("unknown option -%c\n", rc);
		}
	}

	return 0;
}

int print_usage()
{
	printf("usage: test_mempool [-n item-count] [-s item-size] [-m multiple] [options]\n"
		   "where options are\n"
		   "-u :   use plain free/malloc rather than a memory pool\n"
		   "-g :   use shared mempool with ``faster\'\' (type 2) bit\n"
		   "-G :   use shared mempool with ``smaller\'\' (type 1) bit\n"
		   "-e :   when pooling, only release memory when done\n"
		   "-h :   print this message and exit\n"
		   "\nif neither one of -[ugG] is set mempools are used\n\n");
	exit(1);
}

#ifdef _WINDOWS /* msvc liketh not long stringes */
static char *content_buf = 
	"SONNET 1\n\
FROM fairest creatures we desire increase,\n\
That thereby beauty\'s rose might never die,\n\
But as the riper should by time decease,\n\
His tender heir might bear his memory:\n\
But thou, contracted to thine own bright eyes,\n\
Feed\'st thy light\'st flame with self-substantial fuel,\n\
Making a famine where abundance lies,\n\
Thyself thy foe, to thy sweet self too cruel.\n\
Thou that art now the world\'s fresh ornament\n\
And only herald to the gaudy spring,\n\
Within thine own bud buriest thy content\n\
And, tender churl, makest waste in niggarding.\n\
Pity the world, or else this glutton be,\n\
To eat the world\'s due, by the grave and thee.\n";
#else 
static char *content_buf = 
	"SONNET 1\n\
FROM fairest creatures we desire increase,\n\
That thereby beauty\'s rose might never die,\n\
But as the riper should by time decease,\n\
His tender heir might bear his memory:\n\
But thou, contracted to thine own bright eyes,\n\
Feed\'st thy light\'st flame with self-substantial fuel,\n\
Making a famine where abundance lies,\n\
Thyself thy foe, to thy sweet self too cruel.\n\
Thou that art now the world\'s fresh ornament\n\
And only herald to the gaudy spring,\n\
Within thine own bud buriest thy content\n\
And, tender churl, makest waste in niggarding.\n\
Pity the world, or else this glutton be,\n\
To eat the world\'s due, by the grave and thee.\n\
\n\
SONNET 2\n\
When forty winters shall beseige thy brow,\n\
And dig deep trenches in thy beauty\'s field,\n\
Thy youth\'s proud livery, so gazed on now,\n\
Will be a tatter\'d weed, of small worth held:\n\
Then being ask\'d where all thy beauty lies,\n\
Where all the treasure of thy lusty days,\n\
To say, within thine own deep-sunken eyes,\n\
Were an all-eating shame and thriftless praise.\n\
How much more praise deserved thy beauty\'s use,\n\
If thou couldst answer \'This fair child of mine\n\
Shall sum my count and make my old excuse,\n\
Proving his beauty by succession thine!\n\
This were to be new made when thou art old,\n\
And see thy blood warm when thou feel\'st it cold.\n\
\n\
SONNET 3\n\
Look in thy glass, and tell the face thou viewest\n\
Now is the time that face should form another;\n\
Whose fresh repair if now thou not renewest,\n\
Thou dost beguile the world, unbless some mother.\n\
For where is she so fair whose unear\'d womb\n\
Disdains the tillage of thy husbandry?\n\
Or who is he so fond will be the tomb\n\
Of his self-love, to stop posterity?\n\
Thou art thy mother\'s glass, and she in thee\n\
Calls back the lovely April of her prime:\n\
So thou through windows of thine age shall see\n\
Despite of wrinkles this thy golden time.\n\
But if thou live, remember\'d not to be,\n\
Die single, and thine image dies with thee.\n\
\n\
SONNET 4\n\
Unthrifty loveliness, why dost thou spend\n\
Upon thyself thy beauty\'s legacy?\n\
Nature\'s bequest gives nothing but doth lend,\n\
And being frank she lends to those are free.\n\
Then, beauteous niggard, why dost thou abuse\n\
The bounteous largess given thee to give?\n\
Profitless usurer, why dost thou use\n\
So great a sum of sums, yet canst not live?\n\
For having traffic with thyself alone,\n\
Thou of thyself thy sweet self dost deceive.\n\
Then how, when nature calls thee to be gone,\n\
What acceptable audit canst thou leave?\n\
Thy unused beauty must be tomb\'d with thee,\n\
Which, used, lives th\' executor to be.\n\
\n\
SONNET 5\n\
Those hours, that with gentle work did frame\n\
The lovely gaze where every eye doth dwell,\n\
Will play the tyrants to the very same\n\
And that unfair which fairly doth excel:\n\
For never-resting time leads summer on\n\
To hideous winter and confounds him there;\n\
Sap cheque\'d with frost and lusty leaves quite gone,\n\
Beauty o\'ersnow\'d and bareness every where:\n\
Then, were not summer\'s distillation left,\n\
A liquid prisoner pent in walls of glass,\n\
Beauty\'s effect with beauty were bereft,\n\
Nor it nor no remembrance what it was:\n\
But flowers distill\'d though they with winter meet,\n\
Leese but their show; their substance still lives sweet.\n\
\n\
SONNET 6\n\
Then let not winter\'s ragged hand deface\n\
In thee thy summer, ere thou be distill\'d:\n\
Make sweet some vial; treasure thou some place\n\
With beauty\'s treasure, ere it be self-kill\'d.\n\
That use is not forbidden usury,\n\
Which happies those that pay the willing loan;\n\
That\'s for thyself to breed another thee,\n\
Or ten times happier, be it ten for one;\n\
Ten times thyself were happier than thou art,\n\
If ten of thine ten times refigured thee:\n\
Then what could death do, if thou shouldst depart,\n\
Leaving thee living in posterity?\n\
Be not self-will\'d, for thou art much too fair\n\
To be death\'s conquest and make worms thine heir.\n\
\n\
SONNET 7\n\
Lo! in the orient when the gracious light\n\
Lifts up his burning head, each under eye\n\
Doth homage to his new-appearing sight,\n\
Serving with looks his sacred majesty;\n\
And having climb\'d the steep-up heavenly hill,\n\
Resembling strong youth in his middle age,\n\
yet mortal looks adore his beauty still,\n\
Attending on his golden pilgrimage;\n\
But when from highmost pitch, with weary car,\n\
Like feeble age, he reeleth from the day,\n\
The eyes, \'fore duteous, now converted are\n\
From his low tract and look another way:\n\
So thou, thyself out-going in thy noon,\n\
Unlook\'d on diest, unless thou get a son.\n";
#endif

cp_mempool *node_pool;
cp_mempool *item_pool;
cp_shared_mempool *papa_pool;

node *list = NULL;

size_t get_item_size()
{
	return item_size + rand() % (item_variation - item_size);
}

void add_item(node **root)
{
	node *n;
	size_t item_size_local = item_variation ? get_item_size() : item_size;

	if (use_general_pool)
	{
		n = (node *) cp_shared_mempool_alloc(papa_pool, sizeof(node));
		n->item = (char *) cp_shared_mempool_alloc(papa_pool, item_size_local);
	}
	else if (use_mempool)
	{
		n = (node *) cp_mempool_alloc(node_pool);
		if (item_variation)
			n->item = (char *) malloc (item_size_local);
		else
			n->item = (char *) cp_mempool_alloc(item_pool);
//		*((int *) n->item) = 5;
	}
	else
	{
		n = (node *) malloc(sizeof(node));
		n->item = (char *) malloc(item_size_local);
	}

	strncpy(n->item, content_buf, item_size_local - 1);

	n->next = *root;
	*root = n;
}

void drop_item(node **root)
{
	if (*root)
	{
		node *n = *root;
		*root = (*root)->next;
		if (use_general_pool)
		{
			cp_shared_mempool_free(papa_pool, n->item);
			cp_shared_mempool_free(papa_pool, n);
		}
		else if (use_mempool)
		{
//			*((int *) n->item) = 10;
			if (item_variation)
				free(n->item);
			else
				cp_mempool_free(item_pool, n->item);

			cp_mempool_free(node_pool, n);
		}
		else
		{
			free(n->item);
			free(n);
		}
	}
}

int main(int argc, char *argv[])
{
	int i;

	if (process_cmdline(argc, argv)) print_usage();

	if (item_variation)
	{
		srand(time(NULL));

		if (item_variation < item_size)
		{
			size_t swap = item_size;
			item_size = item_variation;
			item_variation = swap;
		}
	}
		
	if (use_general_pool)
	{
		papa_pool = 
			cp_shared_mempool_create_by_option(COLLECTION_MODE_NOSYNC, 
					0, 0, multiple);
		if (!general_pool_small)
			papa_pool->gm_mode = CP_SHARED_MEMPOOL_TYPE_2;
		else
			papa_pool->gm_mode = CP_SHARED_MEMPOOL_TYPE_1;

		cp_shared_mempool_register(papa_pool, sizeof(node));
		cp_shared_mempool_register(papa_pool, item_size);
		printf("using generalized mpool\n");
	}
	else if (!use_mempool) 
		printf("not pooling memory\n");
	else
	{
		node_pool = 
			cp_mempool_create_by_option(COLLECTION_MODE_NOSYNC, 
                                      sizeof(node), multiple);
		if (!item_variation)
		{
			item_pool = 
				cp_mempool_create_by_option(COLLECTION_MODE_NOSYNC, 
        	                              item_size, multiple);
		}
		printf("using mpool\n");
	}

	fflush(stdout);

	for (i = 0; i < items; i++) add_item(&list);
	if (!skip_free)
		for (i = 0; i < items; i++) drop_item(&list);

	if (use_general_pool)
		cp_shared_mempool_destroy(papa_pool);
	else if (use_mempool)
	{
		if (!item_variation)
			cp_mempool_destroy(item_pool);
		cp_mempool_destroy(node_pool);
	}

	return 0;
}

