
/**
 * @addtogroup cp_string
 */
/** @{ */
/**
 * @file
 * cp_string - 'safe' string implementation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(unix) || defined(__unix__) || defined(__MACH__)
#include <unistd.h>
#endif
#include <errno.h>
#ifdef _WINDOWS
#include <io.h>
#endif

#include "str.h"
#include "common.h"
#include "log.h"

cp_string *cp_string_create(char *data, int len)
{
	cp_string *str;
	
#ifdef DEBUG
	if (len < 0) 
	{
		cp_error(CP_INVALID_VALUE, "negative length (%d)", len);
		return NULL;
	}
#endif
	
	str = calloc(1, sizeof(cp_string));

	if (str)
	{
		str->len = len;
		str->size = str->len + 1;
		str->data = malloc(str->size * sizeof(char));
		if (str->data)
			memcpy(str->data, data, str->len);
		else
		{
			cp_error(CP_MEMORY_ALLOCATION_FAILURE, "can\'t create string (%d bytes)", str->len);
			free(str);
			str = NULL;
		}
	}

	return str;
}

cp_string *cp_string_create_empty(int initial_size)
{
	cp_string *str = calloc(1, sizeof(cp_string));

	if (str)
	{
		str->len = 0;
		str->size = initial_size + 1;
		str->data = malloc(str->size * sizeof(char));
		if (str->data == NULL)
		{
			free(str);
			str = NULL;
		}
	}

	return str;
}

void cp_string_delete(cp_string *str)
{
	if (str)
	{
		if (str->data)
			free(str->data);

		free(str);
	}
}

void cp_string_drop_wrap(cp_string *str)
{
	if (str) free(str);
}

void cp_string_drop_content(char *str)
{
	if (str) free(str);
}

void cp_string_destroy(cp_string *str)
{
	cp_string_delete(str);
}

void cp_string_clear(cp_string *str)
{
	if (str)
		memset(str, 0, sizeof(cp_string));
}

void cp_string_reset(cp_string *str)
{
	if (str)
	{
//		if (str->data)
//			free(str->data);
		str->len = 0;
//		str->size = 1;
//		str->data = malloc(str->size * sizeof(char));
		str->data[0] = '\0';
	}
}

	
cp_string *cp_string_cstrcpy(cp_string *str, char *cstr)
{
	if (str)
	{
		str->len = strlen(cstr);
		if (str->size < str->len + 1)
		{
			str->size = str->len + 1;
			str->data = malloc(str->size * sizeof(char));
		}
		if (str->data)
			memcpy(str->data, cstr, str->size * sizeof(char));
		else
		{
			free(str);
			str = NULL;
		}
	}

	return str;
}

cp_string *cp_string_cpy(cp_string *dst, cp_string *src)
{
	dst->len = src->len;
	if (dst->size < src->len + 1)
	{
		dst->size = src->len + 1;
		dst->data = malloc(dst->size * sizeof(char));
	}
	if (dst->data)
		memcpy(dst->data, src->data, src->len);
	else 
		return NULL;

	return dst;
}

cp_string *cp_string_dup(cp_string *src)
{
	cp_string *str = calloc(1, sizeof(cp_string));

	if (str)
	{
		*str = *src; /* bitwise copy */
		str->data = malloc((str->len + 1) * sizeof(char));
		if (str->data)
			memcpy(str->data, src->data, (str->len  + 1) * sizeof(char));
		else
		{
			free(str);
			str = NULL;
		}
	}

	return str;
}

cp_string *cp_string_cstrdup(char *src)
{
	cp_string *str = calloc(1, sizeof(cp_string));

	if (str)
	{
		str->len = strlen(src);
		str->size = str->len + 1;
		str->data = malloc(str->size * sizeof(char));
		if (str->data == NULL)
		{
			free(str);
			return NULL;
		}
		memcpy(str->data, src, str->size);
	}

	return str;
}
	
cp_string *cp_string_cat(cp_string *str, cp_string *appendum)
{
	int len = str->len;
	str->len += appendum->len;
	if (str->len + 1 > str->size)
	{
		str->size = str->len + 1;
		str->data = realloc(str->data, str->size * sizeof(char));
	}
	if (str->data)
		memcpy(str->data + len * sizeof(char), appendum->data, 
			appendum->len * sizeof(char));

	return str;
}

cp_string *cp_string_cstrcat(cp_string *str, char *cstr)
{
	int len = str->len;
	int clen = strlen(cstr);

	str->len += clen * sizeof(char);
	if (str->len + 1 > str->size)
	{
//		str->size = str->len + 0x400 - (str->len % 0x400); /* align to 1kb block */
		str->size = str->len + 1;
		str->data = realloc(str->data, str->size * sizeof(char));
	}
	if (str->data)
		memcpy(str->data + len * sizeof(char), cstr, clen);

	return str;
}

cp_string *cp_string_append_char(cp_string *str, char ch)
{
	if (str->len + 1 > str->size)
	{
		str->size = str->len + 0x100;
		str->data = realloc(str->data, str->size * sizeof(char));
		if (str->data == NULL) return NULL;
	}
	str->data[str->len++] = ch;

	return str;
}

cp_string *cp_string_cat_bin(cp_string *str, void *bin, int len)
{
	int olen = str->len;
	str->len += len;

	if (str->len > str->size)
	{
		str->size = str->len + 0x400 - (str->len % 0x400); /* align to 1kb block */
		str->data = realloc(str->data, str->size * sizeof(char));
	}
	memcpy(&str->data[olen], bin, len);

	return str;
}

int cp_string_cmp(cp_string *s1, cp_string *s2)
{
	if (s1 == s2) return 0; //~~ implies cp_string_cmp(NULL, NULL) == 0

	if (s1 == NULL) return -1;
	if (s2 == NULL) return 1;

	if (s1->len == s2->len)
		return memcmp(s1->data, s2->data, s1->len);
	else
	{
		int p = (s1->len > s2->len) ? s2->len : s1->len;
		int rc = memcmp(s1->data, s2->data, p);
		if (rc == 0) 
			return s1->len - s2->len;
		return rc;
	}
}

char *cp_string_tocstr(cp_string *str)
{
	char *cstr = NULL;
	
	if (str)
	{
		str->data[str->len * sizeof(char)] = '\0';
//		str->data[str->len * sizeof(char) + 1] = '\0';
		cstr = str->data;
	}

	return cstr;
}

int cp_string_len(cp_string *s)
{
	return s->len;
}

char *cp_string_data(cp_string *s)
{
	return s->data;
}

#define CHUNK 0x1000
cp_string *cp_string_read(int fd, int len)
{
	char buf[CHUNK];
	int read_len;
	cp_string *res = NULL;
	
	if (len == 0)
		read_len = CHUNK;
	else
		read_len = len < CHUNK ? len : CHUNK;

	while (len == 0 || res == NULL || res->len < len)
	{
		int rc = 
#ifdef HAVE_READ
			read(fd, buf, read_len);
#else
			_read(fd, buf, read_len);
#endif /* HAVE_READ */
		if (rc <= 0) break;
		if (res == NULL)
		{
			res = cp_string_create(buf, rc);
			if (res == NULL) return NULL;
		}
		else
			cp_string_cat_bin(res, buf, rc);
	}

	return res;
}

int cp_string_write(cp_string *str, int fd)
{
	int rc;
	int total = 0;

	while (total < str->len)
	{
		rc =
#ifdef HAVE_WRITE
			write(fd, &str->data[total], str->len - total);
#else
			_write(fd, &str->data[total], str->len - total);
#endif
		/* write sets EAGAIN when a socket is marked non-blocking and the
		 * write would block. trying to write again could result in spinning
		 * on the write call.
		 */
		if (rc == -1) 
		{
			if (errno == EINTR /* || errno == EAGAIN */) /* try again */ 
				continue;
			else 
				break; 
		}
		total += rc;
	}

	return total;
}

cp_string *cp_string_read_file(char *filename)
{
	cp_string *res;
	FILE *fp = fopen(filename, "rb");
	if (fp == NULL) return NULL;

	res = cp_string_read(fileno(fp), 0);
	fclose(fp);

	return res;
}

int cp_string_write_file(cp_string *str, char *filename)
{
	int rc;
	FILE *fp = fopen(filename, "wb");
	if (fp == NULL) return 0;

	rc = cp_string_write(str, fileno(fp));
	fclose(fp);

	return rc;
}

#define LINELEN 81
#define CHARS_PER_LINE 16

static char *print_char = 
	"                "
	"                "
	" !\"#$%&'()*+,-./"
	"0123456789:;<=>?"
	"@ABCDEFGHIJKLMNO"
	"PQRSTUVWXYZ[\\]^_"
	"`abcdefghijklmno"
	"pqrstuvwxyz{|}~ "
	"                "
	"                "
	" ¡¢£¤¥¦§¨©ª«¬­®¯"
	"°±²³´µ¶·¸¹º»¼½¾¿"
	"ÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏ"
	"ÐÑÒÓÔÕÖ×ØÙÚÛÜÝÞß"
	"àáâãäåæçèéêëìíîï"
	"ðñòóôõö÷øùúûüýþÿ";

void cp_string_dump(cp_string *str)
{
	int rc;
	int idx;
	char prn[LINELEN];
	char lit[CHARS_PER_LINE + 1];
	char hc[4];
	short line_done = 1;

	rc = str->len;
	idx = 0;
	lit[CHARS_PER_LINE] = '\0';
	while (rc > 0)
	{
		if (line_done) 
#ifdef HAVE_SNPRINTF
			snprintf(prn, LINELEN, "%08X: ", idx);
#else
			sprintf(prn, "%08X: ", idx);
#endif /* HAVE_SNPRINTF */
		do
		{
			unsigned char c = str->data[idx];
#ifdef HAVE_SNPRINTF
			snprintf(hc, 4, "%02X ", c);
#else
			sprintf(hc, "%02X ", c);
#endif /* HAVE_SNPRINTF */
#ifdef HAVE_STRLCAT
			strlcat(prn, hc, 4);
#else
			strcat(prn, hc);
#endif /* HAVE_STRLCAT */
			lit[idx % CHARS_PER_LINE] = print_char[c];
			++idx;
		} while (--rc > 0 && (idx % CHARS_PER_LINE != 0));
		line_done = (idx % CHARS_PER_LINE) == 0;
		if (line_done) 
			printf("%s  %s\n", prn, lit);
		else if (rc == 0)
#ifdef HAVE_STRLCAT
			strlcat(prn, "   ", LINELEN);
#else
			strcat(prn, "   ");
#endif /* HAVE_STRLCAT */
	}
	if (!line_done)
	{
		lit[(idx % CHARS_PER_LINE)] = '\0';
		while ((++idx % CHARS_PER_LINE) != 0) 
#ifdef HAVE_STRLCAT
			strlcat(prn, "   ", LINELEN);
#else
			strcat(prn, "   ");
#endif /* HAVE_STRLCAT */
		printf("%s  %s\n", prn, lit);

	}
}

/** flip the contents of a cp_string */
void cp_string_flip(cp_string *str)
{
	if (str->len)
	{
		char *i, *f, ch;
		f = &str->data[str->len - 1];
		i = str->data;
		while (i < f)
		{
			ch = *i;
			*i = *f;
			*f = ch;
			i++;
			f--;
		}
	}
}

/* remove all occurrences of letters from str */
cp_string *cp_string_filter(cp_string *str, char *letters)
{
	char *i;
	char *f;

	str->data[str->len] = '\0';
	i = str->data;
	while ((f = strpbrk(i, letters)))
	{
		i = f;
		while (*f && strchr(letters, *f)) f++;
		if (*f)
		{
			memmove(i, f, str->len - (f - str->data));
			str->len -= f - i;
			str->data[str->len] = '\0';
		}
		else
		{
			*i = '\0';
			str->len -= str->len - (i - str->data);
			break;
		}
	}

	return str;
}

/** @} */


