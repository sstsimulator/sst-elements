/*
** $Id: minavgmax.c,v 1.1 2010-10-27 23:42:57 rolf Exp $
** From a stream of numbers on stdin, compute the minimum, average, and maximum
** Rolf Riesen, September 2009, Sandia National Laboratories
*/
#include <stdio.h>
#include <stdlib.h>		/* For strtod() */
#include <unistd.h>		/* For getopt() */
#include <math.h>


#define FALSE		(0)
#define TRUE		(1)
#define DOUBLE_MAX	(999999999999999.0)
#define DOUBLE_MIN	(-999999999999999.0)


typedef struct value_t   {
    double value;
    struct value_t *next;
} value_t;

value_t *alloc_value(double value, value_t *prev);



value_t *
alloc_value(double value, value_t *prev)
{

value_t *ptr;


    ptr= (value_t *)malloc(sizeof(value_t));
    if (ptr == NULL)   {
	fprintf(stderr, "Out of memory!\n");
	exit(-1);
    }
    ptr->next= NULL;
    ptr->value= value;

    if (prev != NULL)   {
	prev->next= ptr;
    }

    return ptr;

}  /* end of alloc_value() */



int
main(int argc, char *argv[])
{

int ch, error;

double value, total;
double min, avg, max, median;
double prev, sd;
double offset;
int cnt;
int verbose;
int rc;
int i;
int zero;
int comma;
value_t *first_value;
value_t *last_value;
value_t *current;


    /* Defaults */
    error= FALSE;
    verbose= 0;
    zero= FALSE;
    comma= FALSE;


    /* check command line args */
    while ((ch= getopt(argc, argv, "vcz")) != EOF)   {
	switch (ch)   {
	    case 'v':
		verbose++;
		break;
	    case 'c':
		comma= TRUE;
		break;
	    case 'z':
		zero= TRUE;
		break;
	    default:
		error= TRUE;
		break;
	}
    }

    if (error)   {
	fprintf(stderr, "Usage: %s [-z] [-v {-v}]\n", argv[0]);
	fprintf(stderr, "    -v       Increase verbosity\n");
	fprintf(stderr, "    -z       Zero: normalize to min value\n");
	fprintf(stderr, "    -c       Output as comma seperated values (csv)\n");
	exit(-1);
    }


    /* Read numbers until EOF, count them, and find min, avg, and max */
    cnt= 0;
    min= DOUBLE_MAX;
    max= DOUBLE_MIN;
    prev= DOUBLE_MIN - 1.0;
    total= 0.0;
    offset= 0;

    /* Create a dummy header */
    first_value= alloc_value(0.0, NULL);
    last_value= first_value;

    while ((rc= scanf("%lf", &value)) != EOF)   {
	if (rc != 1)   {
	    fprintf(stderr, "%s   Value %f may be invalid\n", argv[0], value);
	    exit(-1);
	}
	if (prev < DOUBLE_MIN)   {
	    /* First time */
	    prev= value;
	    if (zero)   {
		offset= value;
		prev= value - offset;
		if (verbose)   {
		    printf("# Using offset %f to normalize to smallest value\n", offset);
		}
	    }
	} else if (prev > value)   {
	    fprintf(stderr, "Input data must be sorted to find median!\n");
	    exit(-1);
	} else   {
	    prev= value - offset;
	}

	value= value - offset;
	last_value= alloc_value(value, last_value);
	if (value < DOUBLE_MIN)   {
	    fprintf(stderr, "Can't handle values less than %f\n", DOUBLE_MIN);
	}
	if (value > DOUBLE_MAX)   {
	    fprintf(stderr, "Can't handle values more than %f\n", DOUBLE_MAX);
	}
	if (value > max)   {
	    max= value;
	}
	if (value < min)   {
	    min= value;
	}
	total= total + value;
	cnt++;
    }
    avg= total / cnt;

    /* Start after dummy header */
    current= first_value->next;

    /* Find the median and calculate the standard deviation */
    i= (cnt + 1) / 2;
    sd= 0.0;
    median= 0.0;
    while (current)   {
	if (--i == 0)   {
	    median= current->value;
	}
	sd= sd + (current->value - avg) * (current->value - avg);
	current= current->next;
    }

    sd= sqrt(sd / cnt);

    if (verbose)   {
	if (comma)   {
	    printf("# min,avg,max,median,sd\t(of %d numbers read)\n", cnt);
	} else   {
	    printf("# min\t\tavg\t\tmax\t\tmedian\t\tsd\t\t(of %d numbers read)\n", cnt);
	}
    }
    if (comma)   {
	printf("%f,%f,%f,%f,%f\n", min, avg, max, median, sd);
    } else   {
	printf("%f\t%f\t%f\t%f\t%f\n", min, avg, max, median, sd);
    }

    return 0;

}  /* end of main() */
