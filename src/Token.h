#ifndef TOKEN_H
#define TOKEN_H

#include <assert.h>

#if 0
enum TokenType {
	TOKEN_NUM,
	TOKEN_TYPE,
	TOKEN_UNION,
	TOKEN_IF,
	TOKEN_ELSE,
	TOKEN_ENUM,
	TOKEN_LPAREN,
	TOKEN_RPAREN,
	TOKEN_LBRACE,
	TOKEN_RBRACE,
	TOKEN_LBRACK,
	TOKEN_RBRACK,
	TOKEN_ID,
	TOKEN_DOT,
	TOKEN_DIV,
	TOKEN_MUL,
	TOKEN_SUB,
	TOKEN_ADD,
	TOKEN_SEMI,
	TOKEN_ASSIGN,
	TOKEN_ASSIGNPLUS,
	TOKEN_ASSIGNMINUS,
	TOKEN_BITOR,
	TOKEN_LOGOR,
	TOKEN_BITAND,
	TOKEN_LOGAND,
	TOKEN_LSHIFT,
	TOKEN_RSHIFT,
	TOKEN_CMPEQ,
	TOKEN_CMPLE,
	TOKEN_CMPLT,
	TOKEN_CMPGE,
	TOKEN_CMPGT,
	TOKEN_CMPNE,
	TOKEN_RETURN,
	TOKEN_FOR,
	TOKEN_WHILE};
#endif

static unsigned long getLongHex(const char* s)
{
	unsigned long 	ret;
	int		i;

	ret = 0;
	for (i = 0; s[i]; i++) {
		char	c = s[i];
		ret <<= 4;
		ret |= (c >= 'a') ? (c - 'a') + 10 : c - '0';
	}
	return ret;
}

#endif
