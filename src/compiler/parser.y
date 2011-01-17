%{
#include <iostream>
#include "AST.h"
#include "type.h"
#include "func.h"
#include "writepkt.h"
#include "deftype.h"
#include "detached_preamble.h"

GlobalBlock*	global_scope;
extern int yylex();
extern int yylineno;
extern char* yytext;
extern FILE* yyin;
extern const char* fsl_src_fname;
extern void yy_new_buf(void);
extern void yy_old_buf(void);
void yyerror(const char* s)
{
	std::cerr	<<  "Oops: " <<  s <<  ". File: " << fsl_src_fname <<
			". Line: " <<  yylineno
			<<". Text: '" << yytext << "'. " << std::endl;
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
	PtrList<Id>		*id_l;
	PtrList<CondOrExpr>	*c_or_e_l;
	ArgsList		*args;
	TypeBlock		*t_block;
	TypePreamble		*t_preamble;
	CondExpr		*c_expr;
	Preamble		*p_preamble;
	CondOrExpr		*c_or_e;
	WritePkt		*w_wp;
	WritePktStmt		*w_ws;
	WritePktBlk		*w_wb;
	wblk_list		*w_wbs;
}

//  braces
%token <token> TOKEN_LPAREN TOKEN_RPAREN TOKEN_LBRACE TOKEN_RBRACE
%token <token> TOKEN_RBRACK TOKEN_LBRACK

// arith
%token <token> TOKEN_BITOR TOKEN_BITAND TOKEN_ADD TOKEN_SUB TOKEN_MUL TOKEN_DIV
%token <token> TOKEN_LSHIFT TOKEN_RSHIFT TOKEN_MOD
%token <token> TOKEN_SEMI TOKEN_QUESTION TOKEN_FIXED TOKEN_NOFOLLOW TOKEN_WRITE TOKEN_EXCLAIM

// comparison
%token <token> TOKEN_CMPEQ TOKEN_CMPLE TOKEN_CMPGE TOKEN_CMPNE TOKEN_CMPGT TOKEN_CMPLT

%token <token> TOKEN_COMMA TOKEN_DOT TOKEN_AT

%token <token> TOKEN_CONST

%token <token> TOKEN_ASSIGN TOKEN_ASSIGNPLUS TOKEN_ASSIGNMINUS TOKEN_ASSIGNDIV TOKEN_ASSIGNMUL TOKEN_ASSIGNMOD TOKEN_PLUSPLUS TOKEN_SUBSUB TOKEN_WRITEARROW TOKEN_INCLUDE

%token <token> TOKEN_WHEN TOKEN_TYPEDEF TOKEN_DOUBLECOLON TOKEN_AS

%token <text> TOKEN_ID TOKEN_STR
%token <val> TOKEN_NUM

// keywords
%token <token> TOKEN_TYPE TOKEN_UNION TOKEN_IF TOKEN_ELSE TOKEN_ENUM TOKEN_WHILE TOKEN_FOR TOKEN_RETURN

%token <token> TOKEN_LOGOR TOKEN_LOGAND

%type <c_or_e> cond_or_expr
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
%type <w_ws> write_stmt
%type <w_wb> write_block write_stmts

%type <c_expr> cond_expr
%type <enm_enum> enum_ents
%type <enm_ent> enum_ent
%type <f_stmt> func_stmt
%type <f_block> func_block func_stmts
%type <p_preamble> preamble
%type <id_l> id_list
%type <c_or_e_l> preamble_args_list
%type <w_wbs> write_blocks

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
		| program_stmts TOKEN_INCLUDE TOKEN_STR
		{
			// INCLUDE DIRECTIVE TAKE CONTROL.
			GlobalBlock	*old_gblk;
			FILE		*old_yyin;
			int		old_yylineno;
			const char	*old_src_fname;
			char		*cur_fname;
			std::string	s(*$3);

			/* save */
			old_yylineno = yylineno;
			old_yyin = yyin;
			old_gblk = $1;
			old_src_fname = fsl_src_fname;

			/* parse included file */
			cur_fname = strdup(s.substr(1,s.size()-2).c_str());
			yyin = fopen(cur_fname, "r");
			if (yyin == NULL) {
				free(cur_fname);
				cur_fname = strdup(
					(std::string("../../fs/")+
					s.substr(1,s.size()-2)).c_str());
				yyin = fopen(cur_fname, "r");
			}
			if (yyin == NULL) {
				free(cur_fname);
				cur_fname = strdup(
					(std::string("../fs/")+
					s.substr(1,s.size()-2)).c_str());
				yyin = fopen(cur_fname, "r");
			}
			assert (yyin != NULL || "BAD INCLUDE FILE");
			fsl_src_fname = cur_fname;
			yylineno = 1;
			yy_new_buf();
			yyparse();
			fclose(yyin);
			yy_old_buf();

			/* merge parsed global scope with saved global scope */
			/* restore */
			yyin = old_yyin;
			yylineno = old_yylineno;
			free(cur_fname);
			fsl_src_fname = old_src_fname;
			old_gblk->splice(old_gblk->begin(), *global_scope);
			global_scope->clear_nofree();
			delete global_scope;
			$$ = old_gblk;
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
		| TOKEN_ENUM ident TOKEN_ASSIGN TOKEN_LBRACE enum_ents TOKEN_RBRACE ident TOKEN_SEMI
		{
			$5->setName($2);
			$5->setType($7);
			$$ = $5;
		}
		| ident ident type_args func_block
		{
			$$ = new Func((Id*)$1, (Id*)$2, $3, (FuncBlock*)$4);
		}
		| TOKEN_WRITE ident type_args write_blocks
		{
			wblk_list	*wblist = $4;
			$$ = new WritePkt((Id*)$2, $3, *wblist);
			delete wblist;
		}
		| TOKEN_TYPEDEF ident TOKEN_ASSIGN ident TOKEN_SEMI
		{
			$$ = new DefType((Id*)$2, (Id*)$4);
		}
		| ident TOKEN_DOUBLECOLON  preamble TOKEN_SEMI
		{
			$$ = new DetachedPreamble((Id*)$1, $3);
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
		| ident TOKEN_PLUSPLUS TOKEN_SEMI
		{
			$$ = new FuncAssign((Id*)$1, new AOPAdd(
				((Id*)$1)->copy(), new Number(1)));
		}
		| ident TOKEN_SUBSUB TOKEN_SEMI
		{
			$$ = new FuncAssign((Id*)$1, new AOPSub(
				((Id*)$1)->copy(), new Number(1)));
		}
		| ident TOKEN_ASSIGNPLUS expr TOKEN_SEMI
		{
			$$ = new FuncAssign((Id*)$1, new AOPAdd(((Id*)$1)->copy(), $3));
		}
		| ident TOKEN_ASSIGNMINUS expr TOKEN_SEMI
		{
			$$ = new FuncAssign((Id*)$1, new AOPSub(((Id*)$1)->copy(), $3));
		}
		| ident TOKEN_ASSIGNMUL expr TOKEN_SEMI
		{
			$$ = new FuncAssign((Id*)$1, new AOPMul(((Id*)$1)->copy(), $3));
		}
		| ident TOKEN_ASSIGNDIV expr TOKEN_SEMI
		{
			$$ = new FuncAssign((Id*)$1, new AOPDiv(((Id*)$1)->copy(), $3));
		}
		| ident TOKEN_ASSIGNMOD expr TOKEN_SEMI
		{
			$$ = new FuncAssign((Id*)$1, new AOPMod(((Id*)$1)->copy(), $3));
		}
		| array TOKEN_ASSIGN expr TOKEN_SEMI
		{
			$$ = new FuncAssign((IdArray*)$1, $3);
		}
		| array TOKEN_PLUSPLUS expr TOKEN_SEMI
		{
			assert (0 == 1 && "STUB: ArrayPlusPlus");
			$$ = new FuncAssign((IdArray*)$1, new AOPAdd((IdArray*)$1, $3));
		}
		| array TOKEN_SUBSUB expr TOKEN_SEMI
		{
			assert (0 == 1 && "STUB: ArraySubSub");
			$$ = new FuncAssign((IdArray*)$1, new AOPAdd((IdArray*)$1, $3));
		}
		| array TOKEN_ASSIGNPLUS expr TOKEN_SEMI
		{
			assert (0 == 1 && "STUB: ArrayAssignPlus");
			$$ = new FuncAssign((IdArray*)$1, new AOPAdd((IdArray*)$1, $3));
		}
		| array TOKEN_ASSIGNMINUS expr TOKEN_SEMI
		{
			assert (0 == 1 && "STUB: ArrayAssignPlus");
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
		| TOKEN_WHILE TOKEN_LPAREN cond_expr TOKEN_RPAREN func_stmt
		{
			$$ = new FuncWhileStmt($3, $5);
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

type_args	: TOKEN_LPAREN type_args_list TOKEN_RPAREN { $$ = $2; }
		| TOKEN_LPAREN TOKEN_RPAREN { $$ = new ArgsList(); }
		;
type_args_list	: type_args_list TOKEN_COMMA ident ident { $1->add($3, $4); }
		| ident ident
		{
			$$ = new ArgsList();
			$$->add($1, $2);
		}
		;

type_preamble 	: type_preamble preamble { $1->add($2); }
		| preamble
		{
			$$ = new TypePreamble();
			$$->add($1);
		}
		;

id_list		: id_list ident  { $1->add($2); }
		| ident
		{
			$$ = new PtrList<Id>();
			$$->add($1);
		}
		;

preamble_args_list	: preamble_args_list TOKEN_COMMA cond_or_expr
			{
				$1->add($3);
				$$ = $1;
			}
			| cond_or_expr
			{
				$$ = new PtrList<CondOrExpr>();
				$$->add($1);
			}

preamble	: ident TOKEN_LPAREN preamble_args_list TOKEN_RPAREN
		{
			$$ = new Preamble($1, $3, NULL);
		}
		| ident TOKEN_LPAREN preamble_args_list TOKEN_RPAREN TOKEN_AS ident
		{
			$$ = new Preamble($1, $3, $6);
		}
		| ident TOKEN_LPAREN preamble_args_list TOKEN_RPAREN TOKEN_WHEN id_list
		{
			$$ = new Preamble($1, $3, NULL, $6);
		}
		| ident { $$ = new Preamble($1); }
		| ident TOKEN_WHEN id_list { $$ = new Preamble($1, NULL, $3); }
		;


type_block	: TOKEN_LBRACE type_stmts TOKEN_RBRACE { $$ = $2; } ;

type_stmts	: type_stmts type_stmt { $1->add($2); }
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
			$$ = new TypeDecl((Id*)$1, (IdArray*)$2, false, false);
		}
		| ident ident TOKEN_NOFOLLOW TOKEN_SEMI
		{
			$$ = new TypeDecl((Id*)$1, (Id*)$2, true);
		}
		| ident array TOKEN_FIXED TOKEN_SEMI
		{
			$$ = new TypeDecl((Id*)$1, (IdArray*)$2, true, false);
		}
		| ident array TOKEN_FIXED TOKEN_NOFOLLOW TOKEN_SEMI
		{
			$$ = new TypeDecl((Id*)$1, (IdArray*)$2, true, true);
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
			$$ = new TypeParamDecl((FCall*)$1, (IdArray*)$2, false);
		}
		| fcall_args array TOKEN_FIXED TOKEN_SEMI
		{
			$$ = new TypeParamDecl((FCall*)$1, (IdArray*)$2, true);
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

cond_or_expr	: TOKEN_QUESTION cond_expr { $$ = new CondOrExpr($2); }
		| expr { $$ = new CondOrExpr($1); }

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

expr_list_ent	: expr_list_ent TOKEN_COMMA expr { $1->add($3); }
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
	|	TOKEN_AT			{ $$ = new Id("@"); }
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
		| fcall { $$ = $1; }
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

write_stmts	: write_stmts write_stmt { $1->add($2); }
		| write_stmt { $$ = new WritePktBlk(); $$->add($1) }
		;
write_stmt	: ident_struct TOKEN_WRITEARROW expr TOKEN_EXCLAIM
		{ $$ = new WritePktStruct($1, $3); }
		| ident_struct TOKEN_WRITEARROW expr TOKEN_QUESTION cond_expr TOKEN_EXCLAIM
		{ $$ = new WritePktStruct($1, $3); $$->setCond($5); }
		| write_block TOKEN_EXCLAIM
		{ $$ = new WritePktAnon($1);}
		| write_block TOKEN_QUESTION cond_expr TOKEN_EXCLAIM
		{ $$ = new WritePktAnon($1); $$->setCond($3); }
		| array TOKEN_WRITEARROW expr TOKEN_EXCLAIM
		{ $$ = new WritePktArray((IdArray*)$1, $3);}
		| ident TOKEN_WRITEARROW expr TOKEN_EXCLAIM
		{ $$ = new WritePktId((Id*)$1, $3);}
		| ident expr_list TOKEN_EXCLAIM
		{ $$ = new WritePktCall((Id*)$1, $2); }
		| ident expr_list TOKEN_QUESTION cond_expr TOKEN_EXCLAIM
		{ $$ = new WritePktCall((Id*)$1, $2); $$->setCond($4); }

		;
write_block	: TOKEN_LBRACE write_stmts TOKEN_RBRACE { $$ = $2; }
write_blocks	: write_blocks TOKEN_WRITEARROW write_block
		{
			$1->push_back($3);
			$$ = $1;
		}
		| write_block
		{
			wblk_list	*wbl;
			wbl = new wblk_list();
			wbl->push_back($1);
			$$ = wbl;
		}
