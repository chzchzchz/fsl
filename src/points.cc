#include "code_builder.h"
#include "symtab.h"
#include "evalctx.h"
#include "func_args.h"
#include "util.h"

#include "points_to.h"

using namespace std;

typedef list<const FCall*>	point_list;

extern CodeBuilder		*code_builder;
extern symtab_map		symtabs;
extern const_map		constants;

Points::Points(const Type* t)
: src_type(t), seq(0)
{
	assert (src_type != NULL);
	loadPoints();
	loadPointsRange();
}

void Points::loadPoints(void)
{
	point_list	points_list;

	points_list = src_type->getPreambles("points");

	for (	point_list::iterator it = points_list.begin();
		it != points_list.end();
		it++)
	{
		const FCall	*fc;
		const Expr	*expr;
		/* takes the form points(expr) */
		fc = *it;
		expr = *(fc->getExprs()->begin());
		loadPointsInstance(expr);
	}
}

void Points::loadPointsRange(void)
{
	point_list	points_range_list;

	points_range_list = src_type->getPreambles("points_range");
	for (	point_list::iterator it = points_range_list.begin();
		it != points_range_list.end();
		it++)
	{
		ExprList::const_iterator	e_it;
		const FCall	*fc;
		const ExprList	*exprs;
		const Expr	*_bound_var, *first_val, *last_val, *data;
		const Id	*bound_var;

		/* takes the form
		 * points_range(bound_var, first_val, last_val, data_loc) */

		fc = *it;
		exprs = fc->getExprs();

		e_it = exprs->begin();
		_bound_var = *e_it; e_it++;
		first_val = *e_it; e_it++;
		last_val = *e_it; e_it++;
		data = *e_it;

		bound_var = dynamic_cast<const Id*>(_bound_var);
		if (bound_var == NULL) {
			cerr << "Expected identifier for bound variable in ";
			fc->print(cerr);
			cerr << endl;
			continue;
		}

		loadPointsRangeInstance(bound_var, first_val, last_val, data);
	}
}

void Points::loadPointsInstance(const Expr* data_loc)
{
	EvalCtx		ectx(symtabs[src_type->getName()], symtabs, constants);
	const Type	*dst_type;

	assert (data_loc != NULL);

	/* get the type of the expression so we know what we're pointing to */
	dst_type = ectx.getType(data_loc);
	if (dst_type == NULL) {
		cerr << "Could not resolve type for points-to: '";
		data_loc->print(cerr);
		cerr << "' in type " << src_type->getName() << endl;
		return;
	}

	/* convert the expression into code for the run-time tool */
	points_to_elems.add(new PointsTo(src_type, dst_type, data_loc, seq++));
}

void Points::loadPointsRangeInstance(
	const Id*	bound_var,
	const Expr*	first_val,
	const Expr*	last_val,
	const Expr*	data_loc)
{
	EvalCtx		ectx(symtabs[src_type->getName()], symtabs, constants);
	const Type	*dst_type;

	assert (bound_var != NULL);
	assert (first_val != NULL);
	assert (last_val != NULL);
	assert (data_loc != NULL);

	dst_type = ectx.getType(data_loc);
	if (dst_type == NULL) {
		cerr << "Could not resolve type for points-to: '";
		data_loc->print(cerr);
		cerr << "' in type " << src_type->getName() << endl;
		return;
	}

	points_range_elems.add(new PointsRange(
		src_type, dst_type,
		bound_var, first_val, last_val,
		data_loc,
		seq++));
}


void PointsTo::genCode(void) const
{
	code_builder->genCode(src_type, getFCallName(), points_expr);
}

void PointsTo::genProto(void) const
{
	const ThunkType	*tt;
	tt = symtabs[src_type->getName()]->getThunkType();
	code_builder->genProto(getFCallName(), tt->getThunkArgCount());
}

const std::string PointsTo::getFCallName(void) const
{
	return "__pointsto_" + src_type->getName() + "_" + int_to_string(seq);
}

void PointsRange::genCode(void) const
{
	ArgsList	al;

	al.add(new Id("u64"), binding->copy());

	FuncArgs	extra_args(&al);

	/* function(<thunk-args>, <binding-var>) */
	code_builder->genCode(
		src_type, getFCallName(), points_expr, &extra_args);

	/* function(<thunk-args>) */
	code_builder->genCode(src_type, getMinFCallName(), min_expr);
	code_builder->genCode(src_type, getMaxFCallName(), max_expr);
}

void PointsRange::genProto(void) const
{
	const ThunkType	*tt;

	tt = symtabs[src_type->getName()]->getThunkType();
	code_builder->genProto(
		getFCallName(), tt->getThunkArgCount() + 1 /*binding sym*/);
	code_builder->genProto(getMinFCallName(), tt->getThunkArgCount());
	code_builder->genProto(getMaxFCallName(), tt->getThunkArgCount());
}

const std::string PointsRange::getFCallName(void) const
{
	return	"__pointsrange_" + src_type->getName() + 
		"_" + int_to_string(seq);
}

const std::string PointsRange::getMinFCallName(void) const
{
	return	"__pointsrangeMin_" + src_type->getName() + 
		"_" + int_to_string(seq);
}

const std::string PointsRange::getMaxFCallName(void) const
{
	return	"__pointsrangeMax_" + src_type->getName() + 
		"_" + int_to_string(seq);
}


void Points::genCode(void)
{

	for (	pointsto_list::const_iterator it = points_to_elems.begin();
		it != points_to_elems.end();
		it++)
	{
		(*it)->genCode();
	}

	for (	pointsrange_list::const_iterator it = points_range_elems.begin();
		it != points_range_elems.end();
		it++)
	{
		(*it)->genCode();
	}
}

void Points::genProtos(void)
{

	for (	pointsto_list::const_iterator it = points_to_elems.begin();
		it != points_to_elems.end();
		it++)
	{
		(*it)->genProto();
	}

	for (	pointsrange_list::const_iterator it = points_range_elems.begin();
		it != points_range_elems.end();
		it++)
	{
		(*it)->genProto();
	}
}
