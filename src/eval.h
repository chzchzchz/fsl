#ifndef EVAL_H
#define EVAL_H

#include "symtab.h"
#include "phys_type.h"
#include "type.h"
#include "expr.h"
#include "evalctx.h"


Expr* expr_resolve_consts(const const_map& consts, Expr* cur_expr);
Expr* eval(const EvalCtx& ec, const Expr*);
llvm::Value* evalAndGen(const EvalCtx& ectx, const Expr* expr);



#endif
