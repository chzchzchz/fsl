#ifndef COND_H
#define COND_H

#include "eval.h"

/* returns an i1 */
class llvm::Value* cond_codeGen(const EvalCtx* ctx, const CondExpr* cond);

#endif
