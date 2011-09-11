#ifndef _PATTERN_H_
#define _PATTERN_H_


int read_pattern_file(FILE *fp_pattern, int verbose);
void disp_pattern_params(void);
char *pattern_name(void);
void pattern_params(FILE *out);

#endif /* _PATTERN_H_ */
