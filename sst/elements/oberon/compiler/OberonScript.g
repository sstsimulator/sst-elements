
grammar OberonScript;

options {
	language=Cpp;
}

@parser::namespace { OberonScript }
@lexer::namespace  { OberonScript }

program
	: 'program' '{' statement ( ';' ( statement ';' )* ) '}'
	;

statement
	:  
	;

assignment_statement
	:	IDENTIFIER '=' 
	;

expression
	:	(logical_or_expression)*
	;

logical_or_expression
	:	logical_and_expression ( '||' logical_and_expression )*
	;

logical_and_expression
	:	logical_eq_expression ( '&&' logical_eq_expression )*
	;

logical_eq_expression
	:	logical_lt_expression ( '==' logical_lt_expression )*
	;

logical_lt_expression
	:	logical_lte_expression ( '<' logical_lte_expression )*
	;

logical_lte_expression
	:	logical_gt_expression ( '<=' logical_gt_expression )*
	;

logical_gt_expression
	:	logical_gte_expression ( '>' logical_gte_expression )*
	;

logical_gte_expression
	:	add_expression ( '>=' add_expression )*
	;

add_expression
	:	sub_expression ( '+' sub_expression )*
	;

sub_expression
	:	mul_expression ( '-' mul_expression )*
	;

mul_expression
	:	div_expression ( '*' div_expression )*
	;

div_expression
	:	operand ( '/' operand )*
	;

operand
	:	IDENTIFIER
	|	INTEGER
	|	FLOATING_POINT
	;

IDENTIFIER
	:	LETTER (LETTER | '0'..'9')*
	;
	
fragment
LETTER
	:	'A'..'Z'
	|	'a'..'z'
	|	'_'
	;

INTEGER
	:	('0' | '1'..'9' '0'..'9'*)
	;

FLOATING_POINT
	:	('0'..'9')+ '.' ('0'..'9')*
	;

WS  	:  	(' '|'\r'|'\t'|'\n') { $channel=HIDDEN; }
    	;

COMMENT
    	:   	'/*' ( options {greedy=false;} : . )* '*/' { $channel=HIDDEN; }
    	;

LINE_COMMENT
    	: 	'//' ~('\n'|'\r')* '\r'? '\n' { $channel=HIDDEN; }
    	;
