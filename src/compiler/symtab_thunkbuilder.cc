#include <iostream>
#include <stdlib.h>
#include "expr.h"
#include "thunk_size.h"
#include "thunk_fieldoffset_cond.h"
#include "symtab_thunkbuilder.h"
#include "util.h"
#include "runtime_interface.h"
#include "thunk_params.h"

using namespace std;
extern type_map			types_map;
extern ctype_map		ctypes_map;
extern const_map		constants;
extern void			dump_ptypes();
map<string, ThunkBuilderFunc*>	tbf_map;
static FCall			from_base_fc(new Id("from_base_bits"), new ExprList());

extern RTInterface		rt_glue;

SymbolTable* SymTabThunkBuilder::getSymTab(
	const Type* t, list<SymbolTable*>& out_union_symtabs)
{
	SymbolTable	*ret;
	ThunkType	*thunk_type;
	ThunkSize	*thunk_size;

	assert (cur_symtab == NULL);

	thunk_type = new ThunkType(t);
	cur_thunk_type = thunk_type;

	cur_symtab = new SymbolTable(thunk_type);
	cur_type = t;

	union_symtabs.clear();
	field_count = 0;
	union_c = 0;
	weak_c = 0;
	last_tf_union_off = NULL;

	/* create base thunk field.. */
	setLastThunk(
		new ThunkField(
			*cur_thunk_type,
			"__base",
			new ThunkFieldOffset(rt_glue.getThunkArgOffset()),
			new ThunkFieldSize(0U),
			new ThunkElements(0U),
			ThunkParams::createNoParams(),
			TF_FIELDNUM_NONE)
	);

	apply(t);

	assert (union_c == 0);
	assert (weak_c == 0);

	/* create thunk size function */

	thunk_size = new ThunkSize(
		new AOPSub(
			last_tf->copyNextOffset(),	/* end of last elem */
			rt_glue.getThunkArgOffset()	/* offset of base */)
	);
	thunk_size->setOwner(thunk_type);
	thunk_type->setSize(thunk_size);

	ret = cur_symtab;
	out_union_symtabs = union_symtabs;

	cur_thunk_type = NULL;
	cur_symtab = NULL;
	cur_type = NULL;
	setLastThunk(NULL);

	return ret;
}

SymbolTable* SymTabThunkBuilder::getSymTab(
	const ThunkType* parent_tt,
	const TypeUnion* tu)
{
	SymbolTable	*ret;
	Type		*fake_type;
	ThunkType	*thunk_type;
	ThunkSize	*thunk_size;
	Expr		*size_expr;
	const ArgsList	*args_src;
	ArgsList	*args;

	assert (cur_symtab == NULL);

	/* create fake type-- inherit param fields from source type */
	args_src = tu->getOwner()->getArgs();
	args = (args_src != NULL) ? args_src->copy() : NULL;
	fake_type = new Type(
		new Id( "__union_" +
			tu->getOwner()->getName() + "_" +
			tu->getName()),
		args,
		NULL,
		new TypeBlock(),
		true);

	thunk_type = new ThunkType(fake_type);
	cur_thunk_type = thunk_type;
//	cur_thunk_type->setNumFields(parent_tt->getNumFields());

	cur_symtab = new SymbolTable(thunk_type);
	cur_type = fake_type;

	field_count = 0;
	union_c = 0;
	weak_c = 0;

	/* create base thunk field.. */
	/* XXX: this needs to take into account any conditions that may have
	 * piled up */
	setLastThunk(
		new ThunkField(
			*cur_thunk_type,
			"__base",
			new ThunkFieldOffset(rt_glue.getThunkArgOffset()),
			new ThunkFieldSize(0u),
			new ThunkElements(0u),
			ThunkParams::createNoParams(),
			TF_FIELDNUM_NONE)
	);

	last_tf_union_off = copyCurrentOffset();

	union_c = 1;
	weak_c = 1;

	TypeVisitAll::visit(tu->getBlock());

	delete last_tf_union_off;

	assert (weak_c == 1);

	{
	ExprList	*size_args = new ExprList();
	for (	sym_list::const_iterator it = cur_symtab->begin();
		it != cur_symtab->end();
		it++)
	{
		const ThunkField* tf = (*it)->getFieldThunk();
		size_args->add(tf->getSize()->copyFCall());
	}
	size_expr = rt_glue.maxValue(size_args);
	}

	/* create thunk size function */
	thunk_size = new ThunkSize(size_expr);
	thunk_size->setOwner(thunk_type);
	thunk_type->setSize(thunk_size);

	ret = cur_symtab;

	//cur_thunk_type = NULL;
	cur_symtab = NULL;
	cur_type = NULL;
	setLastThunk(NULL);

	return ret;
}

void SymTabThunkBuilder::setLastThunk(ThunkField* tf)
{
	if (last_tf != NULL)
		delete last_tf;
	last_tf = tf;
}

void SymTabThunkBuilder::addToCurrentSymTab(
	const std::string&	type_str,
	const std::string&	field_name,
	const IdArray*		array,
	bool			no_follow,
	const ExprList*		exprs)
{
	ThunkField		*field_thunk;
	ThunkFieldOffset	*field_offset;
	ThunkFieldSize		*field_size;
	ThunkElements		*field_elems;
	ThunkParams		*field_params;
	const Type		*t;

	/* get field offset object */
	if (last_tf_union_off != NULL) {
		assert (cond_stack.size() == 0);
		field_offset = new ThunkFieldOffset(last_tf_union_off->copy());
	} else if (cond_stack.size() == 0) {
		field_offset = new ThunkFieldOffset(copyCurrentOffset());
	} else {
		field_offset = new ThunkFieldOffsetCond(
			getConds(), copyCurrentOffset(), rt_glue.fail());
	}

	/* get thunksize object */
	t = (types_map.count(type_str)) ?  types_map[type_str] : NULL;
	if (t != NULL) {
		field_size = new ThunkFieldSize(t);
	} else if (ctypes_map.count(type_str)) {
		field_size = new ThunkFieldSize(ctypes_map[type_str]);
	} else {
		cerr << "Could not get type size for " << type_str << endl;
		delete field_offset;
		return;
	}

	/* get number of elements object */
	if (array == NULL) {
		field_elems = new ThunkElements(1u, no_follow);
	} else {
		Expr	*num_elems;

		num_elems = Expr::rewriteReplace(
			array->getIdx()->simplify(),
			from_base_fc.copy(),
			copyCurrentOffset());

		field_elems = new ThunkElements(num_elems, array->isFixed(), no_follow);
	}

	if (t != NULL && t->getNumArgs() != 0) {
		if (exprs == NULL || t->getNumArgs() != exprs->size()) {
			cerr	<< "Wrong number of arguments for "
				<< t->getName() << endl;
			exit(-1);
		}
		field_params = new ThunkParams(exprs->copy());
	} else
		field_params = ThunkParams::createNoParams();

	field_thunk = new ThunkField(
		*cur_thunk_type, field_name,
		field_offset, field_size, field_elems,
		field_params);

	addToCurrentSymTab(type_str, field_name, field_thunk);
}

void SymTabThunkBuilder::addToCurrentSymTab(
	const std::string& type_name,
	const std::string& field_name,
	ThunkField* field_thunk)
{
	bool	was_added;

	setLastThunk(field_thunk->copy(*cur_thunk_type));
	was_added = cur_symtab->add(
		field_name,
		type_name,
		field_thunk,
		(weak_c > 0),
		(cond_stack.size() > 0));

	field_count++;
	if (union_c > 0)
		union_tf_list.add(field_thunk->copy(*cur_thunk_type));
}

const Type* SymTabThunkBuilder::getTypeFromUnionSyms(
	const std::string& field_name) const
{
	const Type *union_t = NULL;

	for (	list<SymbolTable*>::const_iterator it = union_symtabs.begin();
		it != union_symtabs.end();
		it++)
	{
		SymbolTable	*union_st = *it;
		string		name;

		union_t = union_st->getOwnerType();
		assert (union_t != NULL);

		name = "__union_"+cur_type->getName() +"_"+field_name;
		if (union_t->getName() == name) break;
		union_t = NULL;
	}

	return union_t;
}

void SymTabThunkBuilder::addUnionToSymTab(
	const std::string&	field_name,
	const IdArray*		array)
{
	ThunkField		*field_thunk;
	ThunkFieldOffset	*field_offset;
	ThunkFieldSize		*field_size;
	ThunkElements		*field_elems;
	const Type		*union_t;

	assert (last_tf_union_off != NULL);
	/* get field offset object */
	field_offset = new ThunkFieldOffset(last_tf_union_off->copy());

	/* get field size object.. */
	union_t = getTypeFromUnionSyms(field_name);
	assert (union_t != NULL);
	field_size = new ThunkFieldSize(union_t);

	/* get number of elements object */
	if (array == NULL) {
		field_elems = new ThunkElements(1);
	} else {
		Expr	*num_elems;

		num_elems = Expr::rewriteReplace(
			array->getIdx()->simplify(),
			from_base_fc.copy(),
			last_tf_union_off->copy());

		field_elems = new ThunkElements(num_elems, true);
	}

	assert (
		union_t->getNumArgs() == 0 &&
		"Parameterized union types not supported yet");

	/* XXX: should this be a copyParams?-- need clearer semantics
	 * for union types */
	field_thunk = new ThunkField(
		*cur_thunk_type, field_name,
		field_offset, field_size, field_elems,
		ThunkParams::createNoParams());

	addToCurrentSymTab(union_t->getName(), field_name, field_thunk);
}

void SymTabThunkBuilder::visit(const TypeDecl* td)
{
	addToCurrentSymTab(
		td->getType()->getName(),
		td->getName(),
		td->getArray(),
		td->isNoFollow());
}

void SymTabThunkBuilder::visit(const TypeCond* tc)
{
	const TypeStmt	*t_stmt, *f_stmt;
	ThunkField	*last_tf_true, *last_tf_false;
	ThunkField	*initial_tf;
	const CondExpr	*cond_expr;
	Expr		*initial_last_off;

	initial_last_off = copyCurrentOffset();
	initial_tf = last_tf->copy(*cur_thunk_type);

	weak_c++;

	t_stmt = tc->getTrueStmt();
	cond_expr = tc->getCond();

	cond_stack.push_back(cond_expr);
	t_stmt->accept(this);
	cond_stack.pop_back();

	last_tf_true = last_tf->copy(*cur_thunk_type);

	if ((f_stmt = tc->getFalseStmt()) != NULL) {
		CondExpr*	cond_neg;

		/* reset offset */
		setLastThunk(initial_tf);

		cond_neg = new CondNot(cond_expr->copy());

		cond_stack.push_back(cond_neg);
		f_stmt->accept(this);
		cond_stack.pop_back();

		delete cond_neg;

		last_tf_false = last_tf->copy(*cur_thunk_type);
	} else {
		delete initial_tf;
		last_tf_false = NULL;
	}

	weak_c--;

	rebaseByCond(initial_last_off, cond_expr, last_tf_true, last_tf_false);

	delete initial_last_off;
	delete last_tf_true;
	if (last_tf_false != NULL) delete last_tf_false;
}

void SymTabThunkBuilder::rebaseByCond(
	const Expr* 		initial_off,
	const CondExpr*		cond_expr,
	const ThunkField*	last_tf_true,
	const ThunkField*	last_tf_false)
{
	CondExpr		*rebase_cond_expr;
	Expr			*next_off_true, *next_off_false;
	ThunkField		*rebase_tf;
	ThunkFieldSize		*field_size;
	ThunkFieldOffset	*field_offset;
	string			fieldname;

	fieldname = "__rebase_" +
		cur_thunk_type->getType()->getName() + "_" +
		int_to_string(field_count);
	field_count++;

	/* get condition for determining rebase value */
	cond_stack.push_back(cond_expr);
	rebase_cond_expr = getConds();
	cond_stack.pop_back();

	/* TODO: these are going to be checked.. FIX!! */
	next_off_true = last_tf_true->copyNextOffset();
	if (last_tf_false == NULL)
		next_off_false = initial_off->copy();
	else
		next_off_false = last_tf_false->copyNextOffset();

	field_size = new ThunkFieldSize(0u);
	field_offset = 	new ThunkFieldOffsetCond(
		rebase_cond_expr,
		next_off_true,
		next_off_false);

	/* create a field which will do a 'rebase', use it as the last tf */
	rebase_tf = ThunkField::createInvisible(
		*cur_thunk_type, fieldname,
		field_offset, field_size);

	setLastThunk(rebase_tf);
}

void SymTabThunkBuilder::visit(const TypeUnion* tu)
{
	SymTabThunkBuilder	*sb;

	/* TODO support nested unions */
	assert (union_c <= 1);

	if (union_c == 0) {
		last_tf_union_off = copyCurrentOffset();
		union_tf_list.clear();
	}

	weak_c++;
	union_c++;
	TypeVisitAll::visit(tu);
	union_c--;
	weak_c--;

	/* make 'union' type available for symtab reference */
	sb = new SymTabThunkBuilder();
	union_symtabs.push_back(sb->getSymTab(cur_thunk_type, tu));
	delete sb;

	addUnionToSymTab(tu->getName(), tu->getArray());

	if (union_c == 0) {
		delete last_tf_union_off;
		last_tf_union_off = NULL;
		union_tf_list.clear();
	}
}

void SymTabThunkBuilder::visit(const TypeParamDecl* tp)
{
	addToCurrentSymTab(
		tp->getType()->getName(),
		tp->getName(),
		tp->getArray(),
		false,
		tp->getType()->getExprs());
}

Expr* SymTabThunkBuilder::copyFromBase(void) const
{
	return new AOPSub(copyCurrentOffset(), rt_glue.getThunkArgOffset());
}

Expr* SymTabThunkBuilder::copyCurrentOffset(void) const
{
	Expr	*ret;
	if (union_c > 0) {
		assert (last_tf_union_off != NULL);
		ret = last_tf_union_off;
	} else {
		assert (last_tf != NULL);
		ret = last_tf->copyNextOffset();
	}

	return ret;
}

CondExpr* SymTabThunkBuilder::getConds(void) const
{
	CondExpr			*ret;
	cond_list::const_iterator	it;

	if (cond_stack.size() == 0) return NULL;

	it = cond_stack.begin();
	ret = (*it)->copy();
	for (++it; it != cond_stack.end(); it++)
		ret = new BOPAnd(ret, (*it)->copy());

	return ret;
}


void SymTabThunkBuilder::visit(const TypeFunc* tf)
{
	string			fcall_name;
	ThunkBuilderFunc	*tbf;
	ThunkField		*thunkf;

	tbf = tbf_map[tf->getFCall()->getName()];
	if (tbf == NULL) {
		cerr << "Symtab: Unknown function call: ";
		tf->print(cerr);
		cerr << endl;
		return;
	}

	thunkf = tbf->apply(this, tf);
	if (thunkf == NULL) return;
	setLastThunk(thunkf);

	field_count++;
}

ThunkField* TBFSetBits::doIt(
	const SymTabThunkBuilder* sb, const ExprList* args) const
{
	assert (sb->getCondDepth() == 0);

	return ThunkField::createInvisible(
		*sb->getCurThunkType(),
		getFuncName(sb),
		new ThunkFieldOffset(
			new AOPAdd(
				rt_glue.getThunkArgOffset(),
				(args->front())->simplify())),
		new ThunkFieldSize(new Number(0)));
}

ThunkField* TBFSetBytes::doIt(
	const SymTabThunkBuilder* sb, const ExprList* args) const
{
	assert (sb->getCondDepth() == 0);

	return ThunkField::createInvisible(
		*sb->getCurThunkType(),
		getFuncName(sb),
		new ThunkFieldOffset(
			new AOPAdd(
				rt_glue.getThunkArgOffset(),
				new AOPMul(
					(args->front())->simplify(),
					new Number(8)))),
		new ThunkFieldSize(new Number(0)));
}

ThunkField* TBFAlignBits::doIt(
	const SymTabThunkBuilder* sb, const ExprList* args) const
{
	Expr	*align_bits_pad;

	align_bits_pad = new AOPMod(
		new AOPSub((args->front())->simplify(), sb->copyFromBase()),
		(args->front())->simplify());

	assert (sb->getCondDepth() == 0);

	return ThunkField::createInvisible(
		*sb->getCurThunkType(),
		getFuncName(sb),
		new ThunkFieldOffset(sb->copyCurrentOffset()),
		new ThunkFieldSize(align_bits_pad));
}

ThunkField* TBFAlignBytes::doIt(
	const SymTabThunkBuilder* sb, const ExprList* args) const
{
	Expr*		align_bytes_pad;

	align_bytes_pad = new AOPMod(
		new AOPSub(
			new AOPMul((args->front())->simplify(), new Number(8)),
			sb->copyFromBase()),
		new AOPMul((args->front())->simplify(), new Number(8)));

	assert (sb->getCondDepth() == 0);

	return ThunkField::createInvisible(
		*sb->getCurThunkType(),
		getFuncName(sb),
		new ThunkFieldOffset(sb->copyCurrentOffset()),
		new ThunkFieldSize(align_bytes_pad));
}

ThunkField* TBFSkipBits::doIt(
	const SymTabThunkBuilder* sb, const ExprList* args) const
{
	assert (sb->getCondDepth() == 0);
	return ThunkField::createInvisible(
		*sb->getCurThunkType(),
		getFuncName(sb),
		new ThunkFieldOffset(sb->copyCurrentOffset()),
		new ThunkFieldSize((args->front())->simplify())
	);
}

ThunkField* TBFSkipBytes::doIt(
	const SymTabThunkBuilder* sb, const ExprList* args) const
{
	assert (sb->getCondDepth() == 0);
	return ThunkField::createInvisible(
		*sb->getCurThunkType(),
		getFuncName(sb),
		new ThunkFieldOffset(sb->copyCurrentOffset()),
		new ThunkFieldSize(
			new AOPMul(
				(args->front())->simplify(),
				new Number(8)))
	);
}

ThunkField* TBFPadPhysBytes::doIt(
	const SymTabThunkBuilder* sb, const ExprList* args) const
{
	Expr		*pad_bits, *rem_space;
	Expr		*align_sz, *min_space;

	assert (sb->getCondDepth() == 0);

	/* arg1 = alignment size (e.g. a sector) */
	/* arg2 = min space before no padding */
	/* remaining = (arg1 - (cur_loc % arg1));
	 * pad = (remaining < arg2) ? remaining : 0; */
	align_sz = new AOPMul(args->getNth(0)->simplify(), new Number(8));
	min_space = new AOPMul(args->getNth(1)->simplify(), new Number(8));
	rem_space = new AOPSub(
		align_sz->copy(),
		new AOPMod(
			rt_glue.toPhys(
				rt_glue.getThunkClosure(),
				sb->copyCurrentOffset()),
			align_sz));

	pad_bits= new CondVal(
		new CmpLT(rem_space->copy(), min_space->copy()),
		rem_space,
		new Number(0));

	return ThunkField::createInvisible(
		*sb->getCurThunkType(),
		getFuncName(sb),
		new ThunkFieldOffset(sb->copyCurrentOffset()),
		new ThunkFieldSize(pad_bits));
}

ThunkField* ThunkBuilderFunc::apply(
	const SymTabThunkBuilder* sb, const TypeFunc* tf) const
{
	ExprList*	args = setupArgs(sb, tf);
	ThunkField*	ret;

	if (args == NULL) return NULL;
	ret = doIt(sb, args);
	delete args;

	return ret;
}

ExprList* ThunkBuilderFunc::setupArgs(
	const SymTabThunkBuilder* sb, const TypeFunc* tf) const
{
	ExprList*	args;
	Expr*		from_base;

	from_base = sb->copyFromBase();
	args = (tf->getFCall()->getExprs())->simplify();
	args->rewrite(&from_base_fc, from_base);
	delete from_base;

	if (args->size() != num_args) {
		cerr << "Takes " << num_args << " args." << endl;
		delete args;
		args = NULL;
	}

	return args;
}

string ThunkBuilderFunc::getFuncName(const SymTabThunkBuilder* sb) const
{
	return string("__")+string(func_prefix)+string("_") +
		int_to_string(sb->getFieldCount());
}

void thunkbuilder_init_funcmap(void)
{
	tbf_map["set_bytes"] = new TBFSetBytes();
	tbf_map["set_bits"] = new TBFSetBits();
	tbf_map["skip_bytes"] = new TBFSkipBytes();
	tbf_map["skip_bits"] = new TBFSkipBits();
	tbf_map["align_bits"] = new TBFAlignBits();
	tbf_map["align_bytes"] = new TBFAlignBytes();
	tbf_map["pad_phys_bytes"] = new TBFPadPhysBytes();
}
