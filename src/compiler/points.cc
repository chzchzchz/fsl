#include "code_builder.h"
#include "symtab.h"
#include "evalctx.h"
#include "util.h"
#include "runtime_interface.h"
#include "struct_writer.h"
#include "eval.h"

#include "points.h"

using namespace std;

extern CodeBuilder		*code_builder;
extern symtab_map		symtabs;
extern type_map			types_map;
extern RTInterface		rt_glue;

unsigned int			points_seq = 0;

Points::Points(const Type* t)
 : Annotation(t, "points")
{
	loadByName();
	loadByName("points_range");
	loadByName("points_range_cast");
	loadByName("points_cast");
	loadByName("points_if");
}

void Points::load(const Preamble* p)
{
	const string	n(p->getName());

	if (n == "points") loadPoints(p);
	else if (n == "points_range") loadPointsRange(p);
	else if (n == "points_range_cast") loadPointsRangeCast(p);
	else if (n == "points_if") loadPointsIf(p);
	else if (n == "points_cast") loadPointsCast(p);
	else assert (0 == 1 && "BAD POINTS NAME");
}

void Points::loadPointsIf(const Preamble* p)
{
	const preamble_args		*args;
	preamble_args::const_iterator	args_it;
	const Expr			*data_loc;
	const CondExpr			*cexpr;

	args = p->getArgsList();
	if (args == NULL || args->size() != 2) {
		cerr << "points_if: expects two args" << endl;
		return;
	}

	args_it = args->begin();

	cexpr = (*args_it)->getCondExpr();
	args_it++;
	data_loc = (*args_it)->getExpr();

	if (cexpr == NULL || data_loc == NULL) {
		cerr << "points_if: Unexpected argtype" << endl;
		return;
	}

	loadPointsIfInstance(p, cexpr, data_loc);
}

void Points::loadPointsCast(const Preamble* p)
{
	const preamble_args		*args;
	preamble_args::const_iterator	args_it;
	const Expr			*data_loc, *_type_name;
	std::string			type_name_str;
	const Id			*type_name_id;
	const FCall			*type_name_fc;
	ExprList*			dst_params;
	const Type			*dst_type;

	args = p->getArgsList();
	if (args == NULL ||  args->size() != 2) {
		cerr << "points_cast: expects two args" << endl;
		return;
	}

	args_it = args->begin();
	_type_name = (*args_it)->getExpr();
	if (_type_name == NULL) {
		cerr << "points_cast: Unexpected argtype" << endl;
		return;
	}

	type_name_id = dynamic_cast<const Id*>(_type_name);
	type_name_fc = dynamic_cast<const FCall*>(_type_name);
	if (type_name_id == NULL && type_name_fc == NULL) {
		cerr << "points_cast : Wanted type name in first arg. ";
		cerr << "Got: ";
		_type_name->print(cerr);
		cerr << endl;
		return;
	}

	if (type_name_id != NULL) {
		type_name_str = type_name_id->getName();
		dst_params = NULL;
	} else {
		assert (type_name_fc != NULL);
		type_name_str = type_name_fc->getName();
		dst_params = type_name_fc->getExprs()->copy();
	}

	if (types_map.count(type_name_str) == 0) {
		cerr << "points_cast: invalid type " << type_name_str  << endl;
		return;
	}
	dst_type = types_map[type_name_str];

	args_it++;
	data_loc = (*args_it)->getExpr();
	if (data_loc == NULL) {
		cerr << "points_cast: Unexpected argtype" << endl;
		return;
	}

	loadPointsInstance(p, dst_type, dst_params, data_loc);
}

void Points::loadPoints(const Preamble *p)
{
	const preamble_args	*args;
	const Expr		*data_loc;

	args = p->getArgsList();
	if (args == NULL ||  args->size() != 1) {
		cerr << "points: expects one arg" << endl;
		return;
	}

	data_loc = (args->front())->getExpr();
	if (data_loc == NULL) {
		cerr << "points: Unexpected argtype" << endl;
		return;
	}

	loadPointsInstance(p, data_loc);
}

void Points::loadPointsRangeCast(const Preamble* p)
{
	const preamble_args		*args;
	preamble_args::const_iterator	args_it;
	InstanceIter			*inst_iter;

	/* takes the form
	 * points_range(bound_var, first_val, last_val, data_loc, cast_type) */

	args = p->getArgsList();
	if (args == NULL || args->size() != 5) {
		cerr << "points_range_cast: expects 5 args" << endl;
		return;
	}

	args_it = args->begin();
	inst_iter = new InstanceIter();
	if (inst_iter->loadCast(src_type, args_it) == false) {
		cerr << "points_range_cast: Could not init inst iter"<<endl;
		delete inst_iter;
		return;
	}

	loadPointsRangeInstance(p, inst_iter);
}

void Points::loadPointsRange(const Preamble* p)
{
	const preamble_args		*args;
	preamble_args::const_iterator	args_it;
	InstanceIter			*inst_iter;

	/* takes the form
	 * points_range(bound_var, first_val, last_val, data_loc) */

	args = p->getArgsList();
	if (args == NULL || args->size() != 4) {
		cerr << "points_range: expects 4 args" << endl;
		return;
	}

	args_it = args->begin();
	inst_iter = new InstanceIter();
	if (inst_iter->load(src_type, args_it) == false) {
		cerr << "points_range: Could not init inst iter" << endl;
		delete inst_iter;
		return;
	}

	loadPointsRangeInstance(p, inst_iter);
}

void Points::loadPointsIfInstance(
	const Preamble* pre, const CondExpr* ce, const Expr* data_loc)
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
		this, pre, dst_type, ce->copy(), data_loc->copy());
	p_elems_all.add(new_pif);
}

void Points::loadPointsInstance(const Preamble* pre, const Expr* data_loc)
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

	loadPointsInstance(pre, dst_type, NULL, data_loc);
}

void Points::loadPointsInstance(
	const Preamble* pre,
	const Type* dst_type, ExprList* dst_params,
	const Expr* data_loc)
{
	PointsRange	*new_pr;

	assert (dst_type != NULL);
	assert (data_loc != NULL);

	/* convert the expression into code for the run-time tool */
	new_pr = new PointsRange(
		this,
		pre,
		new InstanceIter(
			src_type,
			dst_type,
			dst_params,
			new Id("__nobinding"),
			new Number(1),
			new Number(1),
			data_loc->copy()));
	p_elems_all.add(new_pr);
}

void Points::loadPointsRangeInstance(
	const Preamble* pre, InstanceIter* inst_iter)
{
	PointsRange	*new_pr;
	assert (inst_iter != NULL);
	new_pr = new PointsRange(this, pre, inst_iter);
	p_elems_all.add(new_pr);
}

PointsRange::PointsRange(
	Annotation* in_parent, const Preamble* in_pre, InstanceIter* in_iter)
: AnnotationEntry(in_parent, in_pre), iter(NULL)
{
	setIter(in_iter);
}

PointsRange::PointsRange(Annotation* in_parent, const Preamble* in_pre)
: AnnotationEntry(in_parent, in_pre), iter(NULL)
{ }

void PointsRange::setIter(InstanceIter* in_iter)
{
	assert (iter == NULL);
	assert (in_iter != NULL);

	iter = in_iter;
	iter->setPrefix(
		"__pointsrange_" + iter->getSrcType()->getName() + "_" +
		int_to_string(seq));
}

PointsIf::PointsIf(
	Annotation*	in_p,
	const Preamble	*in_pre,
	const Type*	in_dst_type,
	CondExpr	*in_cond_expr,
	Expr		*in_points_expr)
: PointsRange(in_p, in_pre), cond_expr(in_cond_expr)
{
	setIter(
		new InstanceIter(
			in_p->getType(), in_dst_type,
			new Id("__no_binding"),
			new Number(1),
			new FCall(
				new Id(getWrapperFCallName()),
				new ExprList(rt_glue.getThunkClosure())),
			in_points_expr));

	assert (cond_expr != NULL);
}

PointsIf::~PointsIf(void) { delete cond_expr; }

const string PointsIf::getWrapperFCallName(void) const
{
	return 	"__pointsif_condwrap_" +
		parent->getType()->getName() + "_" +
		int_to_string(getSeqNum());
}

void PointsIf::genCode(void)
{
	Number	*one, *zero;

	one = new Number(1);
	zero = new Number(0);
	code_builder->genCodeCond(
		iter->getSrcType(), getWrapperFCallName(),
		cond_expr, one, zero);

	PointsRange::genCode();
}

void PointsIf::genProto(void)
{
	code_builder->genThunkProto(getWrapperFCallName());
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
	const Id		*name(getName());

	sw.write(".pt_iter = ");
	iter->genTableInstance(tg);

	if (name != NULL)	sw.writeStr("pt_name", name->getName());
	else			sw.write("pt_name", "NULL");
}

void PointsRange::genExterns(TableGen* tg) const { iter->printExterns(tg); }
