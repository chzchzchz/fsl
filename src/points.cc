#include "code_builder.h"
#include "symtab.h"
#include "evalctx.h"
#include "func_args.h"
#include "util.h"
#include "runtime_interface.h"
#include "eval.h"

#include "points_to.h"

using namespace std;

typedef list<const Preamble*>	point_list;

extern CodeBuilder		*code_builder;
extern symtab_map		symtabs;
extern RTInterface		rt_glue;

Points::Points(const Type* t)
: src_type(t), seq(0)
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

		loadPointsIfInstance(cexpr, data_loc);
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

		loadPointsInstance(data_loc);
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

		loadPointsRangeInstance(bound_var, first_val, last_val, data);
	}
}

void Points::loadPointsIfInstance(
	const CondExpr* ce, const Expr* data_loc)
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
		src_type, dst_type, ce->copy(), data_loc->copy(), seq++));
}

void Points::loadPointsInstance(const Expr* data_loc)
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
		src_type,
		dst_type,
		new Id("__nobinding"),
		new Number(1),
		new Number(1),
		data_loc->copy(),
		seq++));
}

void Points::loadPointsRangeInstance(
	const Id*	bound_var,
	const Expr*	first_val,
	const Expr*	last_val,
	const Expr*	data_loc)
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
		src_type, dst_type,
		bound_var->copy(), first_val->copy(), last_val->copy(),
		data_loc->copy(),
		seq++));
}


void PointsRange::genCode(void) const
{
	/* function(<thunk-args>, <binding-var>) */
	genCodeRange();

	/* function(<thunk-args>) */
	code_builder->genCode(src_type, getMinFCallName(), min_expr);
	code_builder->genCode(src_type, getMaxFCallName(), max_expr);
}

void PointsRange::genCodeRange(void) const
{
//	code_builder->genCode(
//		src_type, getFCallName(), points_expr, &extra_args);

	llvm::Function		*f;
	llvm::BasicBlock	*bb_bits;
	llvm::IRBuilder<>	*builder;
	llvm::Value		*ret_typepass, *ret_off, *ret_params;
	string			fcall_bits;
	Expr			*expr_eval_bits;
	SymbolTable		*local_syms;
	string			fname;
	Expr			*raw_expr = points_expr;
	llvm::Function::arg_iterator	ai;
	llvm::Module		*mod;


	mod = code_builder->getModule();
	fname = getFCallName();
	builder = code_builder->getBuilder();

	assert (raw_expr != NULL);

	f = mod->getFunction(fname);
	local_syms = symtabs[src_type->getName()];
	assert (local_syms != NULL);

	expr_eval_bits = eval(EvalCtx(local_syms), raw_expr);
	assert (expr_eval_bits != NULL);

	bb_bits = llvm::BasicBlock::Create(
		llvm::getGlobalContext(), "entry", f);
	builder->SetInsertPoint(bb_bits);
	code_builder->genThunkHeaderArgs(f, src_type);

	ai = f->arg_begin();	/* diskoff */
	ai++;			/* parambuf */
	ai++;			/* idx */

	/* gen s for idx and output parambuf */
	builder->CreateStore(
		ai,
		code_builder->createTmpI64(binding->getName()));

	ret_typepass = expr_eval_bits->codeGen();
	assert (ret_typepass != NULL);
	assert (ret_typepass->getType() == code_builder->getTypePassStruct());
	/* output value should be a typepass struct */

	ret_params = builder->CreateExtractValue(ret_typepass, 1, "params");
	ai++;	/* parambuf output */
	code_builder->emitMemcpy64(ai, ret_params, dst_type->getNumArgs());

	ret_off = builder->CreateExtractValue(ret_typepass, 0, "offset");
	builder->CreateRet(ret_off);
}

void PointsRange::genProto(void) const
{
	const ThunkType	*tt;

	tt = symtabs[src_type->getName()]->getThunkType();
//	code_builder->genProto(
//		getFCallName(), tt->getThunkArgCount() + 1 /*binding sym*/);
	{
	llvm::Function			*f;
	llvm::FunctionType		*ft;
	vector<const llvm::Type*>	args;

	/* diskoff_t */
	args.push_back(llvm::Type::getInt64Ty(llvm::getGlobalContext()));

	/* parambuf_t */
	args.push_back(llvm::Type::getInt64PtrTy(llvm::getGlobalContext()));

	/* binding sym */
	args.push_back(llvm::Type::getInt64Ty(llvm::getGlobalContext()));

	/* parambuf_t out */
	args.push_back(llvm::Type::getInt64PtrTy(llvm::getGlobalContext()));

	ft = llvm::FunctionType::get(
		llvm::Type::getInt64Ty(llvm::getGlobalContext()), args, false);
	f = llvm::Function::Create(
		ft,
		llvm::Function::ExternalLinkage, 
		getFCallName(),
		code_builder->getModule());

	/* should not be redefinitions.. */
	if (f->getName() != getFCallName()) {
		cerr << "Expected name " << getFCallName() <<" got " <<
		f->getNameStr() << endl;
	}

	assert (f->getName() == getFCallName());
	assert (f->arg_size() == args.size());
	}

	code_builder->genThunkProto(getMinFCallName());
	code_builder->genThunkProto(getMaxFCallName());
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

PointsIf::PointsIf(
	const Type*	in_src_type,
	const Type*	in_dst_type,
	CondExpr	*in_cond_expr,
	Expr		*in_points_expr,
	unsigned int	in_seq)
: PointsRange(
	in_src_type, in_dst_type,
	new Id("__no_binding"), 
	new Number(1),
	new FCall(
		new Id(getWrapperFCallName(in_src_type->getName(), in_seq)),
		new ExprList(rt_glue.getThunkArgOffset())),
	in_points_expr,
	in_seq),
	cond_expr(in_cond_expr)
{
	assert (0 == 1 && "NEED TO PASS PARAMS");
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
		getSrcType(),
		getWrapperFCallName(
			getSrcType()->getName(),
			getSeqNum()),
		cond_expr,
		new Number(1),
		new Number(0));

	PointsRange::genCode();
}

void PointsIf::genProto(void) const
{
	const ThunkType	*tt;
	tt = symtabs[getSrcType()->getName()]->getThunkType();
	code_builder->genProto(
		getWrapperFCallName(getSrcType()->getName(), getSeqNum()), 
		tt->getThunkArgCount());
	PointsRange::genProto();
}


