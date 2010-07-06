%{
#include <iostream>
#include "AST.h"
#include "type.h"
#include "func.h"

GlobalBlock*	global_scope;
extern int yylex();
extern int yylineno;
void yyerror(const char* s)
{ 
	std::cerr <<  "Oops: " <<  s <<  ". Line: " << yylineno << std::endl; 
}

%}

%union {
	GlobalBlock		*g_scope;
	GlobalStmt		*g_stmt;
	TypeStmt		*t_stmt;
	FuncStmt		*f_stmt;
	FuncBlock		*f_block;
	ExprList		*expr_l;
	Expr			*e;
	EnumEnt			*enm_ent;
	Enum			*enm_enum;
	std::string		*text;
	long			val;
	int			token;
	Number			*number;
	Id			*id;
	IdStruct		*idstruct;
	TypePreamble		*preamb_l;
	ArgsList		*args;
	TypeBlock		*t_block;
	TypePreamble		*t_preamble;
	CondExpr		*c_expr;
}



//  braces
%token <token> TOKEN_LPAREN TOKEN_RPAREN TOKEN_LBRACE TOKEN_RBRACE 
%token <token> TOKEN_RBRACK TOKEN_LBRACK

// arith
%token <token> TOKEN_BITOR TOKEN_BITAND TOKEN_ADD TOKEN_SUB TOKEN_MUL TOKEN_DIV
%token <token> TOKEN_LSHIFT TOKEN_RSHIFT TOKEN_MOD


%token <token> TOKEN_SEMI

// comparison
%token <token> TOKEN_CMPEQ TOKEN_CMPLE TOKEN_CMPGE TOKEN_CMPNE TOKEN_CMPGT TOKEN_CMPLT
%token <token> TOKEN_COMMA TOKEN_DOT

%token <token> TOKEN_CONST

%token <token> TOKEN_ASSIGN TOKEN_ASSIGNPLUS TOKEN_ASSIGNMINUS

%token <text> TOKEN_ID
%token <val> TOKEN_NUM

// keywords
%token <token> TOKEN_TYPE TOKEN_UNION TOKEN_IF TOKEN_ELSE TOKEN_ENUM TOKEN_WHILE TOKEN_FOR TOKEN_RETURN 

// unused
%token <token> TOKEN_LOGOR TOKEN_LOGAND 

%type <expr_l> expr_list_ent expr_list
%type <e> expr expr_ident num array expr_id_struct fcall struct_type arith fcall_no_args fcall_args
%type <id> ident
%type <g_scope> program program_stmts
%type <g_stmt> program_stmt
%type <idstruct> ident_struct
%type <t_stmt> type_stmt
%type <args> type_args type_args_list
%type <t_block> type_block type_stmts
%type <t_preamble> type_preamble
%type <c_expr> cond_expr
%type <enm_enum> enum_ents
%type <enm_ent> enum_ent
%type <f_stmt> func_stmt
%type <f_block> func_block func_stmts

%start program

%%

program		: program_stmts { 
			global_scope = $1;
			$$ = $1; }
		;

program_stmts	: program_stmts program_stmt
		{
			$1->add($2);
			$$ = $1;
		}
		| program_stmt
		{
			$$ = new GlobalBlock();
			$$->add($1);
		}
		;

program_stmt	: TOKEN_CONST ident TOKEN_ASSIGN expr TOKEN_SEMI
		{
			$$ = new ConstVar($2, $4);
		}
		| TOKEN_CONST ident TOKEN_LBRACK TOKEN_RBRACK TOKEN_ASSIGN expr_list TOKEN_SEMI
		{
			$$ = new ConstArray($2, $6);
		}
		| TOKEN_TYPE ident type_args type_preamble type_block
		{
			$$ = new Type($2, $3, $4, $5);
		}
		| TOKEN_TYPE ident type_preamble type_block
		{
			$$ = new Type($2, NULL, $3, $4);
		}
		| TOKEN_TYPE ident type_block
		{
			$$ = new Type($2, NULL, NULL, $3);
		}
		| TOKEN_TYPE ident type_args type_block
		{
			$$ = new Type($2, $3, NULL, $4);
		}
		| TOKEN_ENUM ident TOKEN_ASSIGN TOKEN_LBRACE enum_ents TOKEN_RBRACE TOKEN_SEMI
		{
			$5->setName($2);
			$$ = $5;
		}
		| ident ident type_args func_block
		{
			$$ = new Func(
				(Id*)$1, (Id*)$2, 
				$3, (FuncBlock*)$4);
		}
		;

func_block	: TOKEN_LBRACE func_stmts TOKEN_RBRACE { $$ = $2; }
		;

func_stmts	: func_stmts func_stmt { $1->add($2); }
		| func_stmt
		{
			$$ = new FuncBlock();
			$$->add($1);
		}
		;

func_stmt	: ident ident TOKEN_SEMI { $$ = new FuncDecl($1, (Id*)$2); } 
		| ident array TOKEN_SEMI { $$ = new FuncDecl($1, (IdArray*)$2); }
		| ident TOKEN_ASSIGN expr TOKEN_SEMI 
		{
			$$ = new FuncAssign((Id*)$1, $3);
		}
		| ident TOKEN_ASSIGNPLUS expr TOKEN_SEMI 
		{
			$$ = new FuncAssign((Id*)$1, new AOPAdd((Id*)$1, $3));
		}
		| ident TOKEN_ASSIGNMINUS expr TOKEN_SEMI 
		{
			$$ = new FuncAssign((Id*)$1, new AOPSub((Id*)$1, $3));
		}
		| array TOKEN_ASSIGN expr TOKEN_SEMI 
		{
			$$ = new FuncAssign((IdArray*)$1, $3);
		}
		| array TOKEN_ASSIGNPLUS expr TOKEN_SEMI 
		{
			$$ = new FuncAssign((IdArray*)$1, new AOPAdd((IdArray*)$1, $3));
		}
		| array TOKEN_ASSIGNMINUS expr TOKEN_SEMI 
		{
			$$ = new FuncAssign((IdArray*)$1, new AOPSub((IdArray*)$1, $3));
		}
		| TOKEN_RETURN expr TOKEN_SEMI { $$ = new FuncRet($2); }
		| TOKEN_IF TOKEN_LPAREN cond_expr TOKEN_RPAREN func_stmt
		{
			$$ = new FuncCondStmt($3, $5, NULL);
		}
		| TOKEN_IF TOKEN_LPAREN cond_expr TOKEN_RPAREN func_stmt TOKEN_ELSE func_stmt
		{
			$$ = new FuncCondStmt($3, $5, $7);
		}
		| func_block { $$ = $1; }
		;

enum_ents	: enum_ents TOKEN_COMMA enum_ent  { $1->add($3); }
		| enum_ent
		{
			$$ = new Enum();
			$$->add($1);
		}
		;

enum_ent	: ident { $$ = new EnumEnt($1); } 
		| ident TOKEN_ASSIGN num 
		{ 
			$$ = new EnumEnt((Id*)$1, (Number*)$3);  
		} 
		;

	
type_args	: TOKEN_LPAREN type_args_list TOKEN_RPAREN
		{
			$$ = $2;
		}
		;
type_args_list	: type_args_list TOKEN_COMMA ident ident
		{
			$1->add($3, $4);
		}
		| ident ident
		{
			$$ = new ArgsList();
			$$->add($1, $2);
		}
		;

type_preamble 	: type_preamble fcall
		{
			$1->add((FCall*)$2);
		}
		| fcall 
		{
			$$ = new TypePreamble();
			$$->add((FCall*)$1);
		}
		;

type_block	: TOKEN_LBRACE type_stmts TOKEN_RBRACE
		{
			$$ = $2;
		}
		;

type_stmts	: type_stmts type_stmt
		{
			$1->add($2);
		}
		| type_stmt
		{
			$$ = new TypeBlock();
			$$->add($1);
		}
		;

type_stmt	: ident ident TOKEN_SEMI
		{
			$$ = new TypeDecl((Id*)$1, (Id*)$2);
		}
		| ident array TOKEN_SEMI
		{
			$$ = new TypeDecl((Id*)$1, (IdArray*)$2);
		}
		| fcall TOKEN_SEMI
		{
			$$ = new TypeFunc((FCall*)$1);
		}
		| fcall_args ident TOKEN_SEMI
		{
			$$ = new TypeParamDecl((FCall*)$1, (Id*)$2);
		}
		| fcall_args array TOKEN_SEMI
		{
			$$ = new TypeParamDecl((FCall*)$1, (IdArray*)$2);
		}
		| TOKEN_UNION type_block ident TOKEN_SEMI
		{
			$$ = new TypeUnion($2, (Id*)$3);
		}
		| TOKEN_UNION type_block array TOKEN_SEMI
		{
			$$ = new TypeUnion($2, (IdArray*)$3);
		}
		| TOKEN_IF TOKEN_LPAREN cond_expr TOKEN_RPAREN type_stmt
		{
			$$ = new TypeCond($3, $5);
		}
		| TOKEN_IF TOKEN_LPAREN cond_expr TOKEN_RPAREN type_stmt TOKEN_ELSE type_stmt
		{
			$$ = new TypeCond($3, $5, $7);
		}
		| type_block { $$ = $1; } 
		;

cond_expr	: TOKEN_LPAREN cond_expr TOKEN_RPAREN { $$ = $2; }
		| fcall { $$ = new FuncCond((FCall*)$1); }
		| expr TOKEN_CMPEQ expr { $$ = new CmpEQ($1, $3); }
		| expr TOKEN_CMPNE expr { $$ = new CmpNE($1, $3); }
		| expr TOKEN_CMPLE expr { $$ = new CmpLE($1, $3); }
		| expr TOKEN_CMPGE expr { $$ = new CmpGE($1, $3); }
		| expr TOKEN_CMPLT expr { $$ = new CmpLT($1, $3); }
		| expr TOKEN_CMPGT expr { $$ = new CmpGT($1, $3); }
		| cond_expr TOKEN_LOGAND cond_expr 
			{ $$ = new BOPAnd($1, $3); }
		| cond_expr TOKEN_LOGOR cond_expr 
			{ $$ = new BOPOr($1, $3); } 
		;


expr_list	: TOKEN_LBRACE expr_list_ent TOKEN_RBRACE { $$ = $2; }
		;



expr_list_ent	: expr_list_ent TOKEN_COMMA expr
		{
			$1->add($3);
		}
		| expr
		{
			$$ = new ExprList();
			$$->add($1);
		}
		;

fcall_no_args	: ident TOKEN_LPAREN TOKEN_RPAREN
		{
			$$ = new FCall($1, new ExprList());
		}
fcall_args	: ident TOKEN_LPAREN expr_list_ent TOKEN_RPAREN 
		{ 
			$$ = new FCall($1, $3);
		}

fcall	: fcall_no_args { $$ = $1; }
	| fcall_args { $$ = $1; }
	;

expr	:	TOKEN_LPAREN expr TOKEN_RPAREN	{ $$ = new ExprParens($2); }
	|	num				{ $$ = $1; }
	|	expr_ident			{ $$ = $1; }
	|	expr_id_struct			{ $$ = $1; }
	|	array				{ $$ = $1; }
	|	arith				{ $$ = $1; }
	|	fcall				{ $$ = $1; }
	;

expr_ident 	: ident { $$ = $1 }
		;
	
expr_id_struct	: ident_struct { $$ = $1; }
		;

ident_struct	:	ident_struct TOKEN_DOT struct_type { $1->add($3); } 
		|	struct_type { $$ = new IdStruct(); $$->add($1); }
		;

struct_type	: ident {$$ = $1; }
		| array {$$ = $1; }
		;

array	:	ident TOKEN_LBRACK expr TOKEN_RBRACK
	{
		$$ = new IdArray($1, $3);
	}
	;	


ident	:	TOKEN_ID { 
			$$ = new Id(*yylval.text); 
			delete $1; }
	;

num	:	TOKEN_NUM { $$ = new Number($1); }
	;

arith		: 	expr TOKEN_BITOR expr  	{ $$ = new AOPOr($1, $3); }
		|	expr TOKEN_BITAND expr	{ $$ = new AOPAnd($1, $3); }
		|	expr TOKEN_ADD expr	{ $$ = new AOPAdd($1, $3); }
		|	expr TOKEN_SUB expr	{ $$ = new AOPSub($1, $3); }
		|	expr TOKEN_DIV expr	{ $$ = new AOPDiv($1, $3); }
		|	expr TOKEN_MUL expr	{ $$ = new AOPMul($1, $3); }
		| 	expr TOKEN_LSHIFT expr	{ $$ = new AOPLShift($1, $3); }
		|	expr TOKEN_RSHIFT expr 	{ $$ = new AOPRShift($1, $3); }
		|	expr TOKEN_MOD expr	{ $$ = new AOPMod($1, $3); }
		;
