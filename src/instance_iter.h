#ifndef INSTANCEITER_H
#define INSTANCEITER_H

#include <string>
#include "table_gen.h"
#include "expr.h"
#include "type.h"

/* represents an iterator you'd find in preambles (e.g. virt, pointsto) */
class InstanceIter
{
public:
	InstanceIter(void);
	bool load(
		const Type* in_src_type,
		preamble_args::const_iterator& arg_it);

	InstanceIter(
		const Type*	in_src_type,
		const Type*	in_dst_type,
		Id*		in_binding,
		Expr*		in_min_expr,
		Expr*		in_max_expr,
		Expr*		in_lookup_expr);

	virtual ~InstanceIter(void);
	const Type* getSrcType(void) const { return src_type; }
	const Type* getDstType(void) const { return dst_type; }

	void setPrefix(const std::string& s) { fc_name_prefix = s; }
	const std::string& getPrefix(void) const { return fc_name_prefix; }

	std::string getLookupFCallName(void) const;
	std::string getMinFCallName(void) const;
	std::string getMaxFCallName(void) const;

	const Id* getBinding(void) const { return binding; }
	const Expr* getMinExpr(void) const { return min_expr; }
	const Expr* getMaxExpr(void) const { return max_expr; }
	void setMinExpr(const Expr* e);

	void genCode(void) const;
	void genProto(void) const;
	void printExterns(TableGen* tg) const;
private:
	void genCodeLookup(void) const;

	const Type*	src_type;
	const Type*	dst_type;	/* as given by lookup_expr */
	Id		*binding;
	Expr		*min_expr;
	Expr		*max_expr;
	Expr		*lookup_expr;
	std::string	fc_name_prefix;
};

#endif
