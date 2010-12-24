#include "code_builder.h"
#include "symtab.h"
#include "evalctx.h"
#include "util.h"
#include "runtime_interface.h"
#include "struct_writer.h"
#include "eval.h"

#include "points.h"

using namespace std;

typedef list<const Preamble*>	point_list;

extern CodeBuilder		*code_builder;
extern symtab_map		symtabs;
extern type_map			types_map;
extern RTInterface		rt_glue;

unsigned int			points_seq = 0;

Points::Points(const Type* t)
: src_type(t)
{
	assert (src_type != NULL);
	loadPoints();
	loadPointsCast();
	loadPointsRange();
	loadPointsRangeCast();
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

void Points::loadPointsCast(void)
{
	point_list	points_list;

	points_list = src_type->getPreambles("points_cast");
	for (	point_list::const_iterator it = points_list.begin();
		it != points_list.end();
		it++)
	{
		const preamble_args		*args;
		preamble_args::const_iterator	args_it;
		const Expr			*data_loc, *_type_name;
		const Id			*type_name;
		const Type			*dst_type;

		args = (*it)->getArgsList();
		if (args == NULL ||  args->size() != 2) {
			cerr << "points_cast: expects two args" << endl;
			continue;
		}

		args_it = args->begin();
		_type_name = (*args_it)->getExpr();
		if (_type_name == NULL) {
			cerr << "points_cast: Unexpected argtype" << endl;
			continue;
		}
		type_name = dynamic_cast<const Id*>(_type_name);
		if (type_name == NULL) {
			cerr << "points_cast : Wanted type name in first arg. ";
			cerr << "Got: ";
			_type_name->print(cerr);
			cerr << endl;
			continue;
		}

		if (types_map.count(type_name->getName()) == 0) {
			cerr << "points_cast: invalid type " <<
				type_name->getName()  << endl;
			continue;
		}
		dst_type = types_map[type_name->getName()];

		args_it++;
		data_loc = (*args_it)->getExpr();
		if (data_loc == NULL) {
			cerr << "points_cast: Unexpected argtype" << endl;
			continue;
		}

		loadPointsInstance(dst_type, data_loc, (*it)->getAddressableName());
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

void Points::loadPointsRangeCast(void)
{
	point_list	points_range_list;

	points_range_list = src_type->getPreambles("points_range_cast");
	for (	point_list::iterator it = points_range_list.begin();
		it != points_range_list.end();
		it++)
	{
		const preamble_args		*args;
		preamble_args::const_iterator	args_it;
		InstanceIter			*inst_iter;

		/* takes the form
		 * points_range(bound_var, first_val, last_val, data_loc, cast_type) */

		args = (*it)->getArgsList();
		if (args == NULL || args->size() != 5) {
			cerr << "points_range_cast: expects 5 args" << endl;
			continue;
		}

		args_it = args->begin();
		inst_iter = new InstanceIter();
		if (inst_iter->loadCast(src_type, args_it) == false) {
			cerr << "points_range_cast: Could not init inst iter"<<endl;
			delete inst_iter;
			continue;
		}

		loadPointsRangeInstance(inst_iter, (*it)->getAddressableName());
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
		InstanceIter			*inst_iter;

		/* takes the form
		 * points_range(bound_var, first_val, last_val, data_loc) */

		args = (*it)->getArgsList();
		if (args == NULL || args->size() != 4) {
			cerr << "points_range: expects 4 args" << endl;
			continue;
		}

		args_it = args->begin();
		inst_iter = new InstanceIter();
		if (inst_iter->load(src_type, args_it) == false) {
			cerr << "points_range: Could not init inst iter"<<endl;
			delete inst_iter;
			continue;
		}

		loadPointsRangeInstance(inst_iter, (*it)->getAddressableName());
	}
}

void Points::loadPointsIfInstance(
	const CondExpr* ce, const Expr* data_loc, const Id* as_name)
{
	EvalCtx		ectx(symtabs[src_type->getName()]);
	const Type	*dst_type;
	PointsIf	*new_pif;

	assert (ce != NULL);
	assert (data_loc != NULL);

	dst_type = ectx.getType(data_loc);
	if (dst_type == NULL) {
		cerr << "Could not resolve type for pointsif: '";
		data_loc->print(cerr);
		cerr << "' in type " << src_type->getName() << endl;
		return;
	}

	new_pif = new PointsIf(
		src_type, dst_type,
		ce->copy(), data_loc->copy(),
		(as_name != NULL) ? as_name->copy() : NULL,
		points_seq++);
	points_range_elems.add(new_pif);
	p_elems_all.push_back(new_pif);
}

void Points::loadPointsInstance(const Expr* data_loc, const Id* as_name)
{
	EvalCtx		ectx(symtabs[src_type->getName()]);
	const Type	*dst_type;

	/* get the type of the expression so we know what we're pointing to */
	dst_type = ectx.getType(data_loc);
	if (dst_type == NULL) {
		cerr << "Could not resolve type for points-to: '";
		data_loc->print(cerr);
		cerr << "' in type " << src_type->getName() << endl;
		return;
	}

	loadPointsInstance(dst_type, data_loc, as_name);
}

void Points::loadPointsInstance(
	const Type* dst_type, const Expr* data_loc, const Id* as_name)
{
	PointsRange	*new_pr;

	assert (dst_type != NULL);
	assert (data_loc != NULL);

	/* convert the expression into code for the run-time tool */
	new_pr = new PointsRange(
		new InstanceIter(
			src_type,
			dst_type,
			new Id("__nobinding"),
			new Number(1),
			new Number(1),
			data_loc->copy()),
		(as_name != NULL) ? as_name->copy() : NULL,
		points_seq++);
	points_to_elems.add(new_pr);
	p_elems_all.push_back(new_pr);
}

void Points::loadPointsRangeInstance(
	InstanceIter* inst_iter, const Id* as_name)
{
	PointsRange	*new_pr;

	assert (inst_iter != NULL);

	new_pr = new PointsRange(
		inst_iter,
		(as_name != NULL) ? as_name->copy() : NULL,
		points_seq++);
	points_range_elems.add(new_pr);
	p_elems_all.push_back(new_pr);
}

void Points::genCode(void)
{

	for (	pr_list::const_iterator it = p_elems_all.begin();
		it != p_elems_all.end();
		it++)
	{
		(*it)->genCode();
	}
}

void Points::genProtos(void)
{

	for (	pr_list::const_iterator it = p_elems_all.begin();
		it != p_elems_all.end();
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
	Number	*one, *zero;

	one = new Number(1);
	zero = new Number(0);
	code_builder->genCodeCond(
		iter->getSrcType(),
		getWrapperFCallName(iter->getSrcType()->getName(), getSeqNum()),
		cond_expr,
		one,
		zero);

	PointsRange::genCode();
}

void PointsIf::genProto(void) const
{
	code_builder->genThunkProto(
		getWrapperFCallName(
			iter->getSrcType()->getName(), getSeqNum()));
	PointsRange::genProto();
}

void Points::genExterns(TableGen* tg)
{
	for (	pr_list::const_iterator it = p_elems_all.begin();
		it != p_elems_all.end();
		it++)
	{
		(*it)->genExterns(tg);
	}
}

void Points::genTables(TableGen* tg)
{
	StructWriter	sw(
		tg->getOS(),
		"fsl_rtt_pointsto",
		"__rt_tab_pointsto_" + getType()->getName() + "[]",
		true);

	for (	pr_list::const_iterator it = p_elems_all.begin();
		it != p_elems_all.end();
		it++)
	{
		sw.beginWrite();
		(*it)->genTableInstance(tg);
	}
}

void PointsRange::genTableInstance(TableGen* tg) const
{
	StructWriter		sw(tg->getOS());

	sw.write("pt_type_dst", iter->getDstType()->getTypeNum());
	sw.write("pt_range", iter->getLookupFCallName());
	sw.write("pt_min", iter->getMinFCallName());
	sw.write("pt_max", iter->getMaxFCallName());

	if (name != NULL)	sw.writeStr("pt_name", name->getName());
	else			sw.write("pt_name", "NULL");
}

void PointsRange::genExterns(TableGen* tg) const
{
	iter->printExterns(tg);
}
