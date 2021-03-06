%{
#include <string>
#include "util.h"
#include "AST.h"
#include "type.h"
#include "func.h"
#include "writepkt.h"
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
		return x;			\
	} while (0)

#define IN_TOKEN_CHR(x)				\
	do {					\
		yylval.val = yytext[1];		\
		return x;			\
	} while (0)

#define IN_TOKEN_HEX(x)					\
	do {						\
		yylval.val = getLongHex(yytext + 2);	\
		return x;				\
	} while (0)

extern "C" int yywrap();
int yywrap() {return 1;}


void yy_new_buf(void) { yypush_buffer_state(yy_create_buffer(yyin,YY_BUF_SIZE)); }
void yy_old_buf(void) { yypop_buffer_state(); }

%}

DIGIT	[0-9]
HEXDIGIT [0-9a-fA-F]
ID	[_a-zA-Z][_a-zA-Z0-9]*
SPACE	[ ]
TAB	[	]
WHITESPACE [ 	]
QSTR	\"([^"\n]|\\["\\n])*\"
CHRLITERAL	\'[ -z]\'
%x COMMENT
%x COMMENT2
%%

"0x"{HEXDIGIT}+	IN_TOKEN_HEX(TOKEN_NUM);

{DIGIT}+	IN_TOKEN_INT(TOKEN_NUM);
{CHRLITERAL}	IN_TOKEN_CHR(TOKEN_NUM);
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
"when"		IN_TOKEN(TOKEN_WHEN);
"typedef"	IN_TOKEN(TOKEN_TYPEDEF);
"as"		IN_TOKEN(TOKEN_AS);
"fixed"		IN_TOKEN(TOKEN_FIXED);
"nofollow"	IN_TOKEN(TOKEN_NOFOLLOW);
"write"		IN_TOKEN(TOKEN_WRITE);
"include"	IN_TOKEN(TOKEN_INCLUDE);
"<-"		IN_TOKEN(TOKEN_WRITEARROW);
"("		IN_TOKEN(TOKEN_LPAREN);
")"		IN_TOKEN(TOKEN_RPAREN);
"{"		IN_TOKEN(TOKEN_LBRACE);
"}"		IN_TOKEN(TOKEN_RBRACE);
"["		IN_TOKEN(TOKEN_LBRACK);
"]"		IN_TOKEN(TOKEN_RBRACK);
{ID}		IN_TOKEN_TEXT(TOKEN_ID);
{QSTR}		IN_TOKEN_TEXT(TOKEN_STR);
"."		IN_TOKEN(TOKEN_DOT);
"\n"		{ yyset_lineno(yyget_lineno() + 1); }
"//"		{ BEGIN(COMMENT); }
<COMMENT>[^\n]	{ BEGIN(COMMENT); }
<COMMENT>"\n"	{ BEGIN(INITIAL); yyset_lineno(yyget_lineno() + 1); }
"/*"		{ BEGIN(COMMENT2); }
<COMMENT2>"\n"	{ yyset_lineno( yyget_lineno() + 1); BEGIN(COMMENT2); }
<COMMENT2>[^*]	{ BEGIN(COMMENT2); }
<COMMENT2>"*/"	{ BEGIN(INITIAL);  }
"::"		IN_TOKEN(TOKEN_DOUBLECOLON);
"/"		IN_TOKEN(TOKEN_DIV);
"*"		IN_TOKEN(TOKEN_MUL);
"-"		IN_TOKEN(TOKEN_SUB);
"+"		IN_TOKEN(TOKEN_ADD);
"%"		IN_TOKEN(TOKEN_MOD);
";"		IN_TOKEN(TOKEN_SEMI);
"="		IN_TOKEN(TOKEN_ASSIGN);
"+="		IN_TOKEN(TOKEN_ASSIGNPLUS);
"-="		IN_TOKEN(TOKEN_ASSIGNMINUS);
"/="		IN_TOKEN(TOKEN_ASSIGNDIV);
"*="		IN_TOKEN(TOKEN_ASSIGNMUL);
"%="		IN_TOKEN(TOKEN_ASSIGNMOD);
"++"		IN_TOKEN(TOKEN_PLUSPLUS);
"--"		IN_TOKEN(TOKEN_SUBSUB);
"|"		IN_TOKEN(TOKEN_BITOR);
"!"		IN_TOKEN(TOKEN_EXCLAIM);
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
"\?"		IN_TOKEN(TOKEN_QUESTION);
"\@"		IN_TOKEN(TOKEN_AT);
.		{ printf("Couldn't tokenize: %s\n", yytext); }
%%
