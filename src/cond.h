#ifndef COND_H
#define COND_H

#include "eval.h"

PhysicalType* cond_resolve(
	const CondExpr* cond, 
	const ptype_map& tm,
	PhysicalType* t, PhysicalType* f);

/* returns an i1 */
class llvm::Value* cond_codeGen(const EvalCtx* ctx, const CondExpr* cond);


#endif
