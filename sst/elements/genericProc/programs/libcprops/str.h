#ifndef _CP_STRING_H
#define _CP_STRING_H

/** @{ */
/**
 * @file
 * cp_string - 'safe' string allowing binary content 
 */

#include "common.h"

__BEGIN_DECLS

#include "config.h"

/** cp_string definition */
typedef CPROPS_DLL struct _cp_string
{
	int size;    /**< size allocated   */
	int len;     /**< size used        */
	char *data;  /**< internal buffer  */
} cp_string;

/** allocate a new cp_string */
CPROPS_DLL
cp_string *cp_string_create(char *data, int len);
/** allocate an empty cp_string with a given buffer size */
CPROPS_DLL
cp_string *cp_string_create_empty(int initial_size);
/** deallocate a cp_string */
CPROPS_DLL
void cp_string_delete(cp_string *str);
/* on windows, you can't just free memory allocated by code in a dll. If you
 * want to set up a string, then delete the string object wrapper and just use
 * the internal char *, you have to free the wrap with cp_string_drop_wrap() 
 * and free the char * with cp_string_drop_content.
 */
CPROPS_DLL
void cp_string_drop_wrap(cp_string *str);
CPROPS_DLL 
void cp_string_drop_content(char *content);

/** synonym for cp_string_delete */
CPROPS_DLL
void cp_string_destroy(cp_string *str);
/** sets string to 0 */
CPROPS_DLL
void cp_string_clear(cp_string *str);
/** releases existing string and sets string to empty string */
CPROPS_DLL
void cp_string_reset(cp_string *str);
/** copies the content of a null terminated c string */
CPROPS_DLL
cp_string *cp_string_cstrcpy(cp_string *str, char *cstr);
/** copies the content of a cp_string */
CPROPS_DLL
cp_string *cp_string_cpy(cp_string *dst, cp_string *src);
/** creates a copy of src string. internal buffer is duplicated. */
CPROPS_DLL
cp_string *cp_string_dup(cp_string *src);
/** creates a cp_string with src as its content */
CPROPS_DLL
cp_string *cp_string_cstrdup(char *src);
/** concatenate cp_strings */
CPROPS_DLL
cp_string *cp_string_cat(cp_string *str, cp_string *appendum);
/** append data from a buffer */
CPROPS_DLL
cp_string *cp_string_cat_bin(cp_string *str, void *bin, int len);
/** append data from a null terminated c string */
CPROPS_DLL
cp_string *cp_string_cstrcat(cp_string *str, char *cstr);
/** append a character to a string */
CPROPS_DLL
cp_string *cp_string_append_char(cp_string *str, char ch);
/** compare cp_strings */
CPROPS_DLL
int cp_string_cmp(cp_string *s1, cp_string *s2);
/** return a pointer to the internal buffer */
CPROPS_DLL
char *cp_string_tocstr(cp_string *str);
/** return the length of the internal buffer */
CPROPS_DLL
int cp_string_len(cp_string *s);
/** return the internal buffer */
CPROPS_DLL
char *cp_string_data(cp_string *s);

/** read len bytes from an open file descriptor (blocking) */
CPROPS_DLL
cp_string *cp_string_read(int fd, int len);
/** write the content of a cp_string to a file descriptor (blocking) */
CPROPS_DLL
int cp_string_write(cp_string *str, int fd);
/** read the contents of a file into a cp_string */
CPROPS_DLL
cp_string *cp_string_read_file(char *filename);
/** write the contents of a cp_string to a file */
CPROPS_DLL
int cp_string_write_file(cp_string *str, char *filename);

/** flip the contents of a cp_string */
CPROPS_DLL
void cp_string_flip(cp_string *str);
/** remove all occurrences of letters from str */
CPROPS_DLL
cp_string *cp_string_filter(cp_string *str, char *letters);

/** dump a cp_string to stdout */
CPROPS_DLL
void cp_string_dump(cp_string *str);

__END_DECLS

/** @} */

#endif

