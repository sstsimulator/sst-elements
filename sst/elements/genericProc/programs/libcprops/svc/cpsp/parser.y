%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define YYSTYPE char *

#include "cprops/vector.h"
#include "cprops/str.h"

void yyerror(const char *str)
{
	fprintf(stderr, "error: %s\n",str);
}
		  
int yywrap()
{
	return 1;
} 
				    
static cp_vector *global;
static cp_vector *service;
static cp_vector *codepack;

static int entered_service = 0;
static int consumed;
static cp_string *curr_code;
					
char *strdup_escape(char *src)
{
	char *res, *dup;
	int len = strlen(src) * 2 + 1;
	dup = res = malloc(len * sizeof(char));

	while (*src)
	{
		switch (*src)
		{
			case '\n' : 
				*res++ = '\\';
				*res++ = 'n';
				src++;
				break;

			case '\\' : 
			case '\'' :
			case '\"' :
				*res++ = '\\';
				

			default :
				*res++ = *src++;
		}
	}
	*res = '\0';

	return dup;
}
				
static char *global_header = 
"#include <stdio.h>\n"
"#include \"cprops/http.h\"\n"
	;

static char *service_header = 
#ifdef _WINDOWS
"__declspec(dllexport)\n"
#endif /* _WINDOWS */
"int cpsp_service_function(cp_http_request *request, cp_http_response *response)\n"
"{\n"
"\tcp_http_session *session = cp_http_request_get_session(request, 1);\n"
"\tcp_string *content = cp_string_create(\"\", 0);\n"
"\tresponse->content = content;\n"
"\tresponse->content_type = HTML;\n"
"\t{\n"
    ;

static char *service_footer = 
"\t}\n"
"\treturn HTTP_CONNECTION_POLICY_KEEP_ALIVE;\n"
"}\n"
    ;

/* the following lines are to silence warnings */
int yylex();
extern int yyparse(void);

int main(int argc, char *argv[])
{
	int i, n;
	global = cp_vector_create(10);
	service = cp_vector_create(10);
	codepack = cp_vector_create(10);
	curr_code = cp_string_create("", 0);
	yyparse();

	printf("%s\n", global_header);
	n = cp_vector_size(global);
	for (i = 0; i < n; i++)
		printf("%s", (char *) cp_vector_element_at(global, i));

	printf("\n%s\n", service_header);
	n = cp_vector_size(service);
	for (i = 0; i < n; i++)
		printf("%s", (char *) cp_vector_element_at(service, i));

	printf("%s\n", service_footer);

	cp_vector_destroy_custom(global, free);
	cp_vector_destroy_custom(service, free);
	cp_vector_destroy_custom(codepack, free);

	return 0;
} 

%}

%token OPENPAGEBLOCK OPENGLOBALBLOCK OPENSVCBLOCK OPENVALBLOCK CLOSEBLOCK CODE

%%

statements:
	| statements statement;

statement:
	pageblock | globalblock | svcblock | valblock | code
	{
		if (!consumed)
		{
			char *curr_code_ptr = cp_string_tocstr(curr_code);
			if (!entered_service)
			{
				while (*curr_code_ptr == '\n') curr_code_ptr++;
				if (*curr_code_ptr) entered_service = 1;
			}
				
			if (*curr_code_ptr)
			{
				char *escaped = strdup_escape(curr_code_ptr);
				cp_string *g = cp_string_cstrdup("\tcp_string_cstrcat(content, \"");
				cp_string_cstrcat(g, escaped);
				free(escaped);
				cp_string_cstrcat(g, "\");\n");
				cp_vector_add_element(service, strdup(cp_string_tocstr(g)));
				cp_string_delete(g);
			}

			cp_string_reset(curr_code);
			consumed = 5;
		}
	}
	;

pageblock: 
	OPENPAGEBLOCK code CLOSEBLOCK
	{
		cp_string *g = cp_string_cstrdup("//%@");
		cp_string_cat(g, curr_code);
		cp_string_cstrcat(g, "\n");
		cp_vector_add_element(global, strdup(cp_string_tocstr(g)));
		cp_string_delete(g);
		cp_string_reset(curr_code);
		consumed = 1;
	}
	;
	

globalblock: 
	OPENGLOBALBLOCK code CLOSEBLOCK
	{
		cp_string_cstrcat(curr_code, "\n");
		cp_vector_add_element(global, strdup(cp_string_tocstr(curr_code)));
		cp_string_reset(curr_code);
		consumed = 2;
	}
	;
	
svcblock: 
	OPENSVCBLOCK code CLOSEBLOCK
	{
		cp_string_cstrcat(curr_code, "\n");
		cp_vector_add_element(service, strdup(cp_string_tocstr(curr_code)));
		cp_vector_add_element(service, strdup("\n"));
		cp_string_reset(curr_code);
		consumed = 3;
	}
	;
	
valblock: 
	OPENVALBLOCK code CLOSEBLOCK
	{
		cp_string *g = cp_string_cstrdup("\tcp_string_cstrcat(content, ");
		cp_string_cat(g, curr_code);
		cp_string_cstrcat(g, ");\n");
		cp_vector_add_element(service, strdup(cp_string_tocstr(g)));
		cp_string_delete(g);
		cp_string_reset(curr_code);
		consumed = 4;
	}
	;
	
code:
	| code raw
	{
		$$ = $2;
	}
	;

raw: 
	CODE
	{
		consumed = 0;
		if ($1)
			cp_string_cstrcat(curr_code, $1);
	}
	;

