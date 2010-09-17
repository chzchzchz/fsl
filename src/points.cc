#include "code_builder.h"
#include "symtab.h"
#include "evalctx.h"
#include "util.h"
#include "runtime_interface.h"
#include "eval.h"

#include "points_to.h"

using namespace std;

typedef list<const Preamble*>	point_list;

extern CodeBuilder		*code_builder;
extern symtab_map		symtabs;
extern RTInterface		rt_glue;

unsigned int			points_seq = 0;

Points::Points(const Type* t)
: src_type(t)
{
	assert (src_type != NULL);
	loadPoints();
	loadPointsRange();
	loadPointsIf();
}

void Points::loadPointsIf(void)
{
	point_list	pointsif_list;

	pointsif_list = src_type->getPreambles("points_if");
	for (	point_list::const_iterator it = pointsif_list.begin();
		it != pointsif_list.end();
		it++)
	{
		const preamble_args		*args;
		preamble_args::const_iterator	args_it;
		const Expr			*data_loc;
		const CondExpr			*cexpr;

		args = (*it)->getArgsList();
		if (args == NULL || args->size() != 2) {
			cerr << "points_if: expects two args" << endl;
			continue;
		}

		args_it = args->begin();

		cexpr = (*args_it)->getCondExpr();
		args_it++;
		data_loc = (*args_it)->getExpr();

		if (cexpr == NULL || data_loc == NULL) {
			cerr << "points_if: Unexpected argtype" << endl;
			continue;
		}

		loadPointsIfInstance(
			cexpr, data_loc, (*it)->getAddressableName());
	}
}

void Points::loadPoints(void)
{
	point_list	points_list;

	points_list = src_type->getPreambles("points");

	for (	point_list::const_iterator it = points_list.begin();
		it != points_list.end();
		it++)
	{
		const preamble_args	*args;
		const Expr		*data_loc;

		args = (*it)->getArgsList();
		if (args == NULL ||  args->size() != 1) {
			cerr << "points: expects one arg" << endl;
			continue;
		}

		data_loc = (args->front())->getExpr();
		if (data_loc == NULL) {
			cerr << "points: Unexpected argtype" << endl;
			continue;
		}

		loadPointsInstance(data_loc, (*it)->getAddressableName());
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
		const preamble_args		*args;
		preamble_args::const_iterator	args_it;
		const Expr	*_bound_var, *first_val, *last_val, *data;
		const Id	*bound_var;

		/* takes the form
		 * points_range(bound_var, first_val, last_val, data_loc) */

		args = (*it)->getArgsList();
		if (args == NULL || args->size() != 4) {
			cerr << "points_range: expects 4 args" << endl;
			continue;
		}

		args_it = args->begin();
		_bound_var = (*args_it)->getExpr(); args_it++;
		first_val = (*args_it)->getExpr(); args_it++;
		last_val = (*args_it)->getExpr(); args_it++;
		data = (*args_it)->getExpr();

		bound_var = dynamic_cast<const Id*>(_bound_var);
		if (bound_var == NULL) {
			cerr << "Expected identifier for bound variable in ";
			if (_bound_var == NULL) 
				cerr << " bad argtype";
			 else
				_bound_var->print(cerr);
			cerr << endl;
			continue;
		}

		if (first_val == NULL || last_val == NULL || data == NULL) {
			cerr << "points_range: Unexpected argtype" << endl;
			continue;
		}

		loadPointsRangeInstance(
			bound_var, first_val, last_val, data,
			(*it)->getAddressableName());
	}
}

void Points::loadPointsIfInstance(
	const CondExpr* ce, const Expr* data_loc, const Id* as_name)
{
	EvalCtx		ectx(symtabs[src_type->getName()]);
	const Type	*dst_type;

	assert (ce != NULL);
	assert (data_loc != NULL);

	dst_type = ectx.getType(data_loc);
	if (dst_type == NULL) {
		cerr << "Could not resolve type for pointsif: '";
		data_loc->print(cerr);
		cerr << "' in type " << src_type->getName() << endl;
		return;
	}

	points_range_elems.add(new PointsIf(
		src_type, dst_type,
		ce->copy(), data_loc->copy(),
		(as_name != NULL) ? as_name->copy() : NULL,
		points_seq++));
}

void Points::loadPointsInstance(const Expr* data_loc, const Id* as_name)
{
	EvalCtx		ectx(symtabs[src_type->getName()]);
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
	points_to_elems.add(new PointsRange(
		new InstanceIter(
			src_type,
			dst_type,
			new Id("__nobinding"),
			new Number(1),
			new Number(1),
			data_loc->copy()),
		(as_name != NULL) ? as_name->copy() : NULL,
		points_seq++));
}

void Points::loadPointsRangeInstance(
	const Id*	bound_var,
	const Expr*	first_val,
	const Expr*	last_val,
	const Expr*	data_loc,
	const Id*	as_name)
{
	EvalCtx		ectx(symtabs[src_type->getName()]);
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
		new InstanceIter(
			src_type, dst_type,
			bound_var->copy(), first_val->copy(), last_val->copy(),
			data_loc->copy()),
		(as_name != NULL) ? as_name->copy() : NULL,
		points_seq++));
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

PointsRange::PointsRange(
	InstanceIter*	in_iter,
	Id*		in_name,
	unsigned int	in_seq)
: iter(in_iter), name(in_name), seq(in_seq)
{
	assert (iter != NULL);
	iter->setPrefix(
		"__pointsrange_" +
		iter->getSrcType()->getName() + "_" +
		int_to_string(seq));
}

PointsIf::PointsIf(
	const Type*	in_src_type,
	const Type*	in_dst_type,
	CondExpr	*in_cond_expr,
	Expr		*in_points_expr,
	Id		*in_name,
	unsigned int	in_seq)
: PointsRange(
	new InstanceIter(
		in_src_type, in_dst_type,
		new Id("__no_binding"),
		new Number(1),
		new FCall(
			new Id(getWrapperFCallName(
				in_src_type->getName(), in_seq)),
			new ExprList(rt_glue.getThunkClosure())),
		in_points_expr),
	in_name,
	in_seq),
	cond_expr(in_cond_expr)
{
	assert (cond_expr != NULL);
}


PointsIf::~PointsIf(void)
{
	delete cond_expr;
}

const string PointsIf::getWrapperFCallName(
	const string& type_name, unsigned int s) const
{
	return 	"__pointsif_condwrap_" + type_name + "_" + 
		int_to_string(s);
}

void PointsIf::genCode(void) const
{
	code_builder->genCodeCond(
		iter->getSrcType(),
		getWrapperFCallName(iter->getSrcType()->getName(), getSeqNum()),
		cond_expr,
		new Number(1),
		new Number(0));

	PointsRange::genCode();
}

void PointsIf::genProto(void) const
{
	code_builder->genThunkProto(
		getWrapperFCallName(
			iter->getSrcType()->getName(), getSeqNum()));
	PointsRange::genProto();
}

