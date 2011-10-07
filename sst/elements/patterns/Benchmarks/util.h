/*
** Some commonly used functions by the benchmarks
*/

#ifndef _UTIL_H_
#define _UTIL_H_

void disp_cmd_line(int argc, char **argv);

#define DEFAULT_CMD_LINE_ERR_CHECK \
    /* Command line error checking */ \
    case '?': \
	if (my_rank == 0)   { \
	    fprintf(stderr, "Unknown option \"%s\"\n", argv[optind - 1]); \
	} \
	error= TRUE; \
	break; \
    case ':': \
	if (my_rank == 0)   { \
		fprintf(stderr, "Missing option argument to \"%s\"\n", argv[optind - 1]); \
	} \
	error= TRUE; \
	break; \
    default: \
	error= TRUE; \
	break;

#endif  /* _UTIL_H_ */
