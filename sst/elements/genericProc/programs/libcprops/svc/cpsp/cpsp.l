%{
#include <stdio.h>
#include <string.h>
#define YYSTYPE char *
#include "parser.h"

extern YYSTYPE yylval;

%}
%%
\r								/* suppress CR */
"<%@"							return OPENPAGEBLOCK;
"<%!"							return OPENGLOBALBLOCK;
"<%"							return OPENSVCBLOCK;
"<%=" 							return OPENVALBLOCK;
"%>"							return CLOSEBLOCK;
\n          					yylval = strdup(yytext); return CODE;
\%/[^>]     					yylval = strdup(yytext); return CODE;
\</[^%]     					yylval = strdup(yytext); return CODE;
[^%<\n\r]*						yylval = strdup(yytext); return CODE;
%%


