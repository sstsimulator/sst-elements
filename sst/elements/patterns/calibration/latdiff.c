/*
** 
** Read <size,value> pairs from two files and show the idfference.
** We used this to measure router latency. Run pinpong_benchmark once.
** Then a second time going through one additional router. The increase
** in time required is due to the extra router in the path.
**
** The two files must match line for line. I.e., if the values for size 64 
** in file one are on line 10, then the values for size 64 in the second
** file must also be on line 10.
** 
** Rolf Riesen, September 2011, IBM Research, Ireland
*/
#include <stdio.h>
#include <stdlib.h>		/* For strtod() */
#include <unistd.h>		/* For getopt() */
#include <string.h>		/* For strerror() */
#include <errno.h>		/* For errno */
#include <math.h>		/* For fabs() */


#define FALSE				(0)
#define TRUE				(1)
#define MAX_LINE			(1024)



int
main(int argc, char *argv[])
{

int ch, error;

char *pos;
int rc;
char line[MAX_LINE];
char *fname1, *fname2;
FILE *fp1, *fp2;

double value1, size1;
double value2, size2;
double dummy;

int lineno;
int verbose;
double zero_diff;


    /* Defaults */
    error= FALSE;
    verbose= 0;
    fname1= NULL;
    fname2= NULL;


    /* check command line args */
    while ((ch= getopt(argc, argv, "vr:l:")) != EOF)   {
	switch (ch)   {
	    case 'v':
		verbose++;
		break;
	    case 'l':
		fname1= optarg;
		break;
	    case 'r':
		fname2= optarg;
		break;
	    default:
		error= TRUE;
		break;
	}
    }

    if (fname1 == NULL)   {
	error= TRUE;
	fprintf(stderr, "Must specify an input file using -l option!\n");
    }

    if (fname2 == NULL)   {
	error= TRUE;
	fprintf(stderr, "Must specify an input file using -r option!\n");
    }

    if (error)   {
	fprintf(stderr, "Usage: %s [-v {-v}] -l fname -r fname\n", argv[0]);
	fprintf(stderr, "    -v              Increase verbosity\n");
	fprintf(stderr, "    -l fname        First input file name\n");
	fprintf(stderr, "    -r fname        Second input file name\n");
	exit(-1);
    }

    /* Open the input files */
    fp1= fopen(fname1, "r");
    if (fp1 == NULL)   {
	fprintf(stderr, "Could not open the input file \"%s\": %s\n",
	    fname1, strerror(errno));
	exit(-1);
    }

    fp2= fopen(fname2, "r");
    if (fp2 == NULL)   {
	fprintf(stderr, "Could not open the input file \"%s\": %s\n",
	    fname2, strerror(errno));
	exit(-1);
    }



    lineno= 0;
    zero_diff= -1.0;

    printf("# Size     Diff     Per byte   Multiples      Compared to\n");
    printf("# bytes    us       in ns      of zero diff   size in %%\n");

    /* Process the input file */
    while (fgets(line, MAX_LINE, fp1) != NULL)   {
	lineno++;

	/* Get rid of comments */
	pos= strchr(line, '#');
	if (pos)   {
	    *pos= '\0';
	}

	/* Now scan it */
	rc= sscanf(line, "%lf %lf %lf %lf %lf %lf %lf # %lf",
		&size1, &dummy, &value1, &dummy, &dummy, &dummy, &dummy, &dummy);

	if (fgets(line, MAX_LINE, fp2) != NULL)   {
	    rc= sscanf(line, "%lf %lf %lf %lf %lf %lf %lf # %lf",
		    &size2, &dummy, &value2, &dummy, &dummy, &dummy, &dummy, &dummy);
	}

	if (rc < 3)   {
	    continue;
	}

	if (size1 != size2)   {
	    fprintf(stderr, "Values in the two files are not aligned starting at "
		"line %d!\n", lineno);
	    fprintf(stderr, "Value for size X in file 1 on line Y, must also be on "
		"line Y in second file\n");
	    exit(-1);
	}

	if (size1 == 0.0)   {
	    zero_diff= value2 - value1;
	}

	if (verbose > 1)   {
	    printf("Debug: Found size %f, value1 %f, value2 %f\n", size1, value1, value2);
	}

	printf("%9.0f  %7.3f   %7.3f   %7.3f   %7.3f\n",
	    size1, value2 - value1, (value2 - value1) * 1000.0 / size1, (value2 - value1) / zero_diff,
	    (value2 - value1) * 100.0 / value1);

    }

    return 0;

}  /* end of main() */
