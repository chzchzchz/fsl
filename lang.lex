%{
#include <string>
#include "Token.h"
#include "AST.h"
#include "type.h"
#include "parser.hh"

#define IN_TOKEN(x)			\
	do {				\
		yylval.token = (int)x;	\
		return x;		\
	} while (0) 

#define IN_TOKEN_TEXT(x)  				\
	do {						\
		yylval.text = new std::string(yytext);	\
		return x;				\
	} while (0)

#define IN_TOKEN_INT(x)				\
	do {					\
		yylval.val = atol(yytext);	\
		IN_TOKEN(x);			\
	} while (0)

#define IN_TOKEN_HEX(x)					\
	do {						\
		yylval.val = getLongHex(yytext);	\
		IN_TOKEN(x);				\
	} while (0)

extern "C" int yywrap();
int yywrap() {return 1;} 
%}

DIGIT	[0-9]
HEXDIGIT [0-9a-f]
ID	[_a-zA-Z][_a-zA-Z0-9]*
SPACE	[ ]
TAB	[	]
WHITESPACE [ 	]
%x COMMENT
%x COMMENT2

%%

"0x"{HEXDIGIT}+	IN_TOKEN_HEX(TOKEN_NUM);

{DIGIT}+	IN_TOKEN_INT(TOKEN_NUM);
{WHITESPACE}
"type"		IN_TOKEN(TOKEN_TYPE);
"union"		IN_TOKEN(TOKEN_UNION);
"if"		IN_TOKEN(TOKEN_IF);
"else"		IN_TOKEN(TOKEN_ELSE);
"enum"		IN_TOKEN(TOKEN_ENUM);
"while"		IN_TOKEN(TOKEN_WHILE);
"for"		IN_TOKEN(TOKEN_FOR);
"return"	IN_TOKEN(TOKEN_RETURN);
"const"		IN_TOKEN(TOKEN_CONST);
"("		IN_TOKEN(TOKEN_LPAREN);
")"		IN_TOKEN(TOKEN_RPAREN);
"{"		IN_TOKEN(TOKEN_LBRACE);
"}"		IN_TOKEN(TOKEN_RBRACE);
"["		IN_TOKEN(TOKEN_LBRACK);
"]"		IN_TOKEN(TOKEN_RBRACK);
{ID}		IN_TOKEN_TEXT(TOKEN_ID);
"."		IN_TOKEN(TOKEN_DOT);
"//"		{ BEGIN(COMMENT); }
<COMMENT>[^\n]	
<COMMENT>"\n"	{ BEGIN(INITIAL); }
"/*"		{ BEGIN(COMMENT2); }
<COMMENT2>[^*]
<COMMENT2>"*/"	{ BEGIN(INITIAL); }
"/"		IN_TOKEN(TOKEN_DIV);
"*"		IN_TOKEN(TOKEN_MUL);
"-"		IN_TOKEN(TOKEN_SUB);
"+"		IN_TOKEN(TOKEN_ADD);
";"		IN_TOKEN(TOKEN_SEMI);
"="		IN_TOKEN(TOKEN_ASSIGN);
"+="		IN_TOKEN(TOKEN_ASSIGNPLUS);
"-="		IN_TOKEN(TOKEN_ASSIGNMINUS);
"|"		IN_TOKEN(TOKEN_BITOR);
"||"		IN_TOKEN(TOKEN_LOGOR);
"&&"		IN_TOKEN(TOKEN_LOGAND);
"&"		IN_TOKEN(TOKEN_BITAND);
"<<"		IN_TOKEN(TOKEN_LSHIFT);
">>"		IN_TOKEN(TOKEN_RSHIFT);
"=="		IN_TOKEN(TOKEN_CMPEQ);
">="		IN_TOKEN(TOKEN_CMPGE);
"<="		IN_TOKEN(TOKEN_CMPLE);
"!="		IN_TOKEN(TOKEN_CMPNE);
">"		IN_TOKEN(TOKEN_CMPGT);
"<"		IN_TOKEN(TOKEN_CMPLT);
","		IN_TOKEN(TOKEN_COMMA);
.		printf("Couldn't tokenize: %s\n", yytext);
%%



