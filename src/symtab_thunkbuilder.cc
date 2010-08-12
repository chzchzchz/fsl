#include <iostream>
#include "expr.h"
#include "thunk_size.h"
#include "thunk_fieldoffset_cond.h"
#include "symtab_thunkbuilder.h"
#include "util.h"
#include "runtime_interface.h"

using namespace std;
extern type_map			types_map;
extern ctype_map		ctypes_map;
extern const_map		constants;
extern void			dump_ptypes();
extern FCall			from_base_fc;
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
			new ThunkFieldOffset(thunk_type, "__base", 
				rt_glue.getThunkArg()),
			new ThunkFieldSize(thunk_type, "__base", 0U),
			new ThunkElements(thunk_type, "__base", 0U)));

	apply(t);

	assert (union_c == 0);
	assert (weak_c == 0);

	/* create thunk size function */

	thunk_size = new ThunkSize(
		thunk_type,
		new AOPSub(
			/* offset of last bit */
			new AOPAdd(
				new AOPMul(
					(last_tf->getSize())->copyFCall(),
					(last_tf->getElems())->copyFCall()),
				(last_tf->getOffset())->copyFCall()),
			/* offset of base */
			rt_glue.getThunkArg())
	);

	thunk_type->setSize(thunk_size);

	ret = cur_symtab;
	out_union_symtabs = union_symtabs;

	cur_thunk_type = NULL;
	cur_symtab = NULL;
	cur_type = NULL;
	setLastThunk(NULL);

	return ret;
}

SymbolTable* SymTabThunkBuilder::getSymTab(const TypeUnion* tu)
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
		new TypeBlock());

	thunk_type = new ThunkType(fake_type);
	cur_thunk_type = thunk_type;

	cur_symtab = new SymbolTable(thunk_type);
	cur_type = fake_type;

	field_count = 0;
	union_c = 0;
	weak_c = 0;

	/* create base thunk field.. */
	/* XXX: this needs to take into account any conditions that may have
	 * pilled up */
	setLastThunk(
		new ThunkField(
			*cur_thunk_type,
			new ThunkFieldOffset(thunk_type, "__base", 
				rt_glue.getThunkArg()),
			new ThunkFieldSize(thunk_type, "__base", 0u),
			new ThunkElements(thunk_type, "__base", 0u)));

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
	thunk_size = new ThunkSize(thunk_type, size_expr);
	thunk_type->setSize(thunk_size);

	ret = cur_symtab;

	cur_thunk_type = NULL;
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
	const IdArray*		array)
{
	ThunkField		*field_thunk;
	ThunkFieldOffset	*field_offset;
	ThunkFieldSize		*field_size;
	ThunkElements		*field_elems;
	const Type		*t;

	/* get field offset object */
	if (last_tf_union_off != NULL) {
		assert (cond_stack.size() == 0);
		field_offset = new ThunkFieldOffset(
			cur_thunk_type,
			field_name,
			last_tf_union_off->copy());
	} else if (cond_stack.size() == 0) {
		field_offset = new ThunkFieldOffset(
			cur_thunk_type, 
			field_name,
			copyCurrentOffset());
	} else {
		field_offset = new ThunkFieldOffsetCond(
			cur_thunk_type,
			field_name,
			getConds(),
			copyCurrentOffset(),
			rt_glue.fail());
	}

	/* get thunksize object */
	t = (types_map.count(type_str)) ?  types_map[type_str] : NULL;
	if (t != NULL) {
		field_size = new ThunkFieldSize(cur_thunk_type, field_name, t);
	} else if (ctypes_map.count(type_str)) {
		field_size = new ThunkFieldSize(
			cur_thunk_type, field_name, ctypes_map[type_str]);
	} else {
		cerr << "Could not get type size for " << type_str << endl;
		delete field_offset;
		return;
	}

	/* get number of elements object */
	if (array == NULL) {
		field_elems = new ThunkElements(
			cur_thunk_type, field_name, 1u);
	} else {
		Expr	*num_elems;

		num_elems = Expr::rewriteReplace(
			array->getIdx()->simplify(),
			from_base_fc.copy(),
			copyCurrentOffset());

		field_elems = new ThunkElements(
			cur_thunk_type, field_name, num_elems);
	}

	field_thunk = new ThunkField(
		*cur_thunk_type, field_offset, field_size, field_elems);

	addToCurrentSymTab(type_str, field_name, field_thunk);
}

void SymTabThunkBuilder::addToCurrentSymTab(
	const std::string& type_name,
	const std::string& field_name,
	ThunkField* field_thunk)
{
	setLastThunk(field_thunk->copy(*cur_thunk_type));
	cur_symtab->add(
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
		if (union_t->getName() == name) {
			break;
		}
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
	field_offset = new ThunkFieldOffset(
		cur_thunk_type, field_name, last_tf_union_off->copy());

	/* get field size object.. */
	union_t = getTypeFromUnionSyms(field_name);
	assert (union_t != NULL);
	field_size = new ThunkFieldSize(cur_thunk_type, field_name, union_t);

	/* get number of elements object */
	if (array == NULL) {
		field_elems = new ThunkElements(
			cur_thunk_type, field_name, 1);
	} else {
		Expr	*num_elems;

		num_elems = Expr::rewriteReplace(
			array->getIdx()->simplify(),
			from_base_fc.copy(),
			last_tf_union_off->copy());


		field_elems = new ThunkElements(
			cur_thunk_type, field_name, num_elems);
	}

	field_thunk = new ThunkField(
		*cur_thunk_type, field_offset, field_size, field_elems);

	addToCurrentSymTab(
		union_t->getName(),
		field_name,
		field_thunk);
}

void SymTabThunkBuilder::visit(const TypeDecl* td)
{
	addToCurrentSymTab(
		td->getType()->getName(),
		td->getName(), 
		td->getArray());
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
	ThunkElements		*field_elements;
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

	field_size = new ThunkFieldSize(cur_thunk_type, fieldname, 0u);
	field_elements = new ThunkElements(cur_thunk_type, fieldname, 1);
	field_offset = 	new ThunkFieldOffsetCond(
		cur_thunk_type,
		fieldname,
		rebase_cond_expr,
		next_off_true,
		next_off_false);

	/* create a field which will do a 'rebase', use it as the last tf */
	rebase_tf = new ThunkField(
		*cur_thunk_type,
		field_offset, field_size, field_elements);

	setLastThunk(rebase_tf);
}

void SymTabThunkBuilder::visit(const TypeUnion* tu)
{
	SymTabThunkBuilder	*sttb;

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
	sttb = new SymTabThunkBuilder();
	union_symtabs.push_back(sttb->getSymTab(tu));
	delete sttb;


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
		tp->getArray());
}

void SymTabThunkBuilder::visit(const TypeFunc* tf)
{
	string			fcall_name;
	ExprList		*args;
	ThunkField		*thunkf;

	fcall_name = tf->getFCall()->getName();

	thunkf = NULL;
	if (fcall_name == "align_bits") {
		thunkf = alignBits(tf);
	} else if (fcall_name == "align_bytes") {
		thunkf = alignBytes(tf);
	} else if (fcall_name == "skip_bits") {
		thunkf = skipBits(tf);
	} else if (fcall_name == "skip_bytes") {
		thunkf = skipBytes(tf);
	} else if (fcall_name == "assert_eq") {
		cerr << "XXX: ASSERT_EQ" << endl;
		return;
	} else if (fcall_name == "assert_le") {
		cerr << "XXX: ASSERT_LE" << endl;
		return;
	} else {
		cerr << "Symtab: Unknown function call: ";
		tf->print(cerr);
		cerr << endl;
		cerr << "((" << fcall_name << "))" << endl;
		return;
	}

	if (thunkf == NULL)
		return;

	setLastThunk(thunkf);

	field_count++;
}

Expr* SymTabThunkBuilder::copyFromBase(void) const
{
	return new AOPSub(
		copyCurrentOffset(), 
		rt_glue.getThunkArg());
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

ThunkField* SymTabThunkBuilder::alignBits(const TypeFunc* tf)
{
	ThunkField*	thunkf = NULL;
	ExprList*	args;
	Expr*		from_base;
	Expr*		align_bits_pad;

	from_base = copyFromBase();
	args = (tf->getFCall()->getExprs())->simplify();
	args->rewrite(&from_base_fc, from_base);


	if (args->size() != 1) {
		cerr << "align_bits takes one arg" << endl;
		goto done;
	}

	align_bits_pad = new AOPMod(
		new AOPSub(
			(args->front())->simplify(),
			from_base->simplify()),
		(args->front())->simplify());

	assert (cond_stack.size() == 0);

	thunkf = new ThunkField(
		*cur_thunk_type,
		new ThunkFieldOffset(
			cur_thunk_type,
			"__align_bits_"+int_to_string(field_count),
			copyCurrentOffset()),
		new ThunkFieldSize(
			cur_thunk_type,
			"__align_bits_"+int_to_string(field_count),
			align_bits_pad),
		new ThunkElements(
			cur_thunk_type,
			"__align_bits_"+int_to_string(field_count),
			1)
	);		

done:
	delete args;
	delete from_base;

	return thunkf;
}

ThunkField* SymTabThunkBuilder::alignBytes(const TypeFunc* tf)
{
	ThunkField*	thunkf = NULL;
	ExprList*	args;
	Expr*		from_base;
	Expr*		align_bytes_pad;

	from_base = copyFromBase();
	args = (tf->getFCall()->getExprs())->simplify();
	args->rewrite(&from_base_fc, from_base);


	if (args->size() != 1) {
		cerr << "align_bytes takes one arg" << endl;
		goto done;
	}

	align_bytes_pad = new AOPMod(
		new AOPSub(
			new AOPMul(
				(args->front())->simplify(),
				new Number(8)),
			from_base->simplify()),
		new AOPMul(
			(args->front())->simplify(),
			new Number(8)));

	assert (cond_stack.size() == 0);

	thunkf = new ThunkField(
		*cur_thunk_type,
		new ThunkFieldOffset(
			cur_thunk_type,
			"__align_bytes_"+int_to_string(field_count),
			copyCurrentOffset()),
		new ThunkFieldSize(
			cur_thunk_type,
			"__align_bytes_"+int_to_string(field_count),
			align_bytes_pad),
		new ThunkElements(
			cur_thunk_type,
			"__align_bytes_"+int_to_string(field_count),
			1)
	);		

done:
	delete args;
	delete from_base;

	return thunkf;
}

ThunkField* SymTabThunkBuilder::skipBits(const TypeFunc* tf)
{
	ThunkField*	thunkf = NULL;
	ExprList*	args;
	Expr*		from_base;

	from_base = copyFromBase();
	args = (tf->getFCall()->getExprs())->simplify();
	args->rewrite(&from_base_fc, from_base);

	if (args->size() != 1) {
		cerr << "skip_bits takes one arg" << endl;
		goto done;
	}

	assert (cond_stack.size() == 0);

	thunkf = new ThunkField(
		*cur_thunk_type,
		new ThunkFieldOffset(
			cur_thunk_type,
			"__skip_bits_"+int_to_string(field_count),
			copyCurrentOffset()),
		new ThunkFieldSize(
			cur_thunk_type,
			"__skip_bits_"+int_to_string(field_count),
			(args->front())->simplify()),
		new ThunkElements(
			cur_thunk_type,
			"__skip_bits_"+int_to_string(field_count),
			1)
	);		

done:
	delete args;
	delete from_base;

	return thunkf;
}

ThunkField* SymTabThunkBuilder::skipBytes(const TypeFunc* tf)
{
	ThunkField*	thunkf = NULL;
	ExprList*	args;
	Expr*		from_base;

	from_base = copyFromBase();
	args = (tf->getFCall()->getExprs())->simplify();
	args->rewrite(&from_base_fc, from_base);

	if (args->size() != 1) {
		cerr << "skip_bytes takes one arg" << endl;
		goto done;
	}

	assert (cond_stack.size() == 0);

	thunkf = new ThunkField(
		*cur_thunk_type,
		new ThunkFieldOffset(
			cur_thunk_type,
			"__skip_bytes_"+int_to_string(field_count),
			copyCurrentOffset()),
		new ThunkFieldSize(
			cur_thunk_type,
			"__skip_bytes_"+int_to_string(field_count),
			new AOPMul(
				(args->front())->simplify(),
				new Number(8))),
		new ThunkElements(
			cur_thunk_type,
			"__skip_bytes_"+int_to_string(field_count),
			1)
	);		

done:
	delete args;
	delete from_base;

	return thunkf;
}

CondExpr* SymTabThunkBuilder::getConds(void) const
{
	CondExpr			*ret;
	cond_list::const_iterator	it;

	if (cond_stack.size() == 0)
		return NULL;

	it = cond_stack.begin();
	ret = (*it)->copy();
	for (	++it;
		it != cond_stack.end();
		it++)
	{
		ret = new BOPAnd(ret, (*it)->copy());
	}

	return ret;
}
