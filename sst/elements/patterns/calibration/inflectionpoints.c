/*
** 
** From a stream of <size,value> pairs on stdin, compute the inflection
** points. I.e., places where the slope changes by more than some
** threshold.
** Rolf Riesen, September 2011, IBM Research, Ireland
*/
#include <stdio.h>
#include <stdlib.h>		/* For strtod() */
#include <unistd.h>		/* For getopt() */
#include <math.h>		/* For fabs() */


#define FALSE		(0)
#define TRUE		(1)
#define CHANGE		(1.0)
#define SUPRESS_THRESHOLD	(1)



int
main(int argc, char *argv[])
{

int ch, error;

double value, size, slope;
double dummy;
double value_diff, size_diff;
double previous_size;
double previous_value;
double previous_slope;
double last_value;
double last_size;
double p;
int supress;

int cnt;
int verbose;
int rc;


    /* Defaults */
    error= FALSE;
    verbose= 0;


    /* check command line args */
    while ((ch= getopt(argc, argv, "v")) != EOF)   {
	switch (ch)   {
	    case 'v':
		verbose++;
		break;
	    default:
		error= TRUE;
		break;
	}
    }

    if (error)   {
	fprintf(stderr, "Usage: %s [-v {-v}]\n", argv[0]);
	fprintf(stderr, "    -v       Increase verbosity\n");
	exit(-1);
    }


    cnt= 0;
    previous_size= 0.0;
    previous_value= 0.0;
    previous_slope= 0.0;
    supress= 10;


    /* Read numbers until EOF */
#define rrr (1)
#if rrr
    while ((rc= scanf("%lf %lf %lf %lf %lf %lf %lf # %lf", &size, &dummy, &value, &dummy,
		    &dummy, &dummy, &dummy, &dummy)) != EOF)   {
	if (rc != 8)   {
#else
    while ((rc= scanf("%lf %lf %lf %lf %lf %lf %lf", &size, &dummy, &value, &dummy,
		    &dummy, &dummy, &dummy)) != EOF)   {
	if (rc != 7)   {
#endif
	    fprintf(stderr, "%s   Error reading data on line %d\n", argv[0], cnt);
	    exit(-1);
	}

	size_diff= size - previous_size;
	value_diff= value - previous_value;
	slope= fabs(value_diff / size_diff);

	if (cnt == 0)   {
	    /* Start the list */
	    printf("NetNICparams = %12.0f %12.0f\n", size, value * 1000.0);
	}

	previous_slope= (value - last_value) / (size - last_size);
	p= fabs(slope * 100.0 / previous_slope - 100.0);
	if (p > CHANGE)   {
	    /* We've reached an inflection point */
	    if (cnt > 1)   {
		if (supress > SUPRESS_THRESHOLD)   {
		    printf("NetNICparams = %12.0f %12.0f\n", previous_size, previous_value * 1000.0);
		    last_size= size;
		    last_value= value;
		    supress= 0;
		} else   {
		    supress++;
		}
	    }
	}

	previous_size= size;
	previous_value= value;

	cnt++;
    }

    printf("NetNICparams = %12.0f %12.0f\n", size, value * 1000.0);

    return 0;

}  /* end of main() */
