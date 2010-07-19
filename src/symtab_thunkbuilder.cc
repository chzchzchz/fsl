#include <iostream>
#include "symtab_thunkbuilder.h"
#include "cond.h"

using namespace std;
extern const_map		constants;
extern symtab_map		symtabs_thunked;

SymbolTable* SymTabThunkBuilder::getSymTab(
	const TypeBlock* tb, PhysicalType* &ptt)
{
	SymbolTable	*ret;

	assert (cur_symtab == NULL);
	assert (ret_pt == NULL);

	cur_symtab = new SymbolTable(NULL, NULL);
	cur_type = tb->getOwner();

	visit(tb);

	assert (weak_c == 0);

	ret = cur_symtab;
	cur_symtab = NULL;

	ptt = ret_pt;
	cur_type = NULL;

	return ret;
}

SymbolTable* SymTabThunkBuilder::getSymTab(const Type* t, PhysicalType* &ptt)
{
	SymbolTable	*ret;

	assert (cur_symtab == NULL);
	assert (ret_pt == NULL);

	cur_symtab = new SymbolTable(NULL, NULL);
	cur_type = t;

	apply(t);

	assert (weak_c == 0);

	ret = cur_symtab;
	cur_symtab = NULL;
	if (ret_pt != NULL) {
		delete ret_pt;
		ret_pt = NULL;
	}

	ptt = new PhysTypeThunk(
		cur_type,
		new ExprList(new Id("NEEDS_THUNK_VAL")));
	cur_type = NULL;

	return ret;
}

void SymTabThunkBuilder::retType(PhysicalType* pt)
{
	if (ret_pt != NULL)
		delete ret_pt;
	ret_pt = pt;
}

extern void dump_ptypes();

void SymTabThunkBuilder::addToCurrentSymTab(
	const std::string& 	type_str,
	const std::string&	field_name,
	const Id*		scalar,
	const IdArray*		array,
	ExprList*		type_args)
{
	const Type		*t;
	PhysicalType		*passed_pt;
	FCall			*passed_offset_fcall;
	ExprList		*exprs;
	const PhysicalType	*pt;


	t = tm.getUserType(type_str);
	pt = tm.getPT(type_str);

	if (t = tm.getUserType(type_str)) {
		if (type_args != NULL) {
			type_args->insert(type_args->begin(), new Id("XYZ_NEED_THUNK_V"));
		} else {
			type_args = new ExprList(new Id("XYZ_NEED_THUNK_V"));
		}
		passed_pt = new PhysTypeThunk(t, type_args);
		if (array != NULL) { 
			passed_pt = new PhysTypeArray(passed_pt, array->getIdx()->copy());
		}
	} else if (pt = tm.getPT(type_str)) {
		passed_pt = pt->copy();
		if (array != NULL) { 
			passed_pt = new PhysTypeArray(passed_pt, array->getIdx()->copy());
		}

		if (type_args != NULL) {
			cerr << "type args for a primitive type?? OOps" << endl;
			delete type_args;
		}
	} else {
		cout << "Couldn't resolve type string: " << type_str << endl;
		dump_ptypes();
		assert (0 == 1);
	}
	
	exprs = new ExprList(new Id("PT_THUNK_ARG"));
	for (int i = 0; i < cur_type->getNumArgs(); i++) {
		exprs->add(new Id("B_FILL_IN_THIS_ARG_ANTHONY"));
	}

	passed_offset_fcall = new FCall(
		new Id(typeOffThunkName(cur_type, field_name)),
		exprs);

	retType(passed_pt->copy());

	cur_symtab->add(field_name, passed_pt, passed_offset_fcall, (weak_c > 0));
}

void SymTabThunkBuilder::addToCurrentSymTabAnonType(
	const PhysicalType*	pt,
	const std::string&	field_name,
	const Id*		scalar,
	const IdArray*		array)
{
	PhysicalType		*passed_pt;
	FCall			*passed_offset_fcall;
	ExprList		*exprs;

	assert (pt != NULL);

	passed_pt = pt->copy();
	if (array != NULL) { 
		passed_pt = new PhysTypeArray(passed_pt, array->getIdx()->copy());
	}
	
	exprs = new ExprList();
	exprs->add(new Id("A_THIS_IS_THUNK_ARG"));
	for (int i = 0; i < cur_type->getNumArgs(); i++) {
		exprs->add(new Id("A_FILL_IN_THIS_ARG_ANTHONY"));
	}

	passed_offset_fcall = new FCall(
		new Id(typeOffThunkName(cur_type, field_name)),
		exprs);

	retType(passed_pt->copy());

	cur_symtab->add(field_name, passed_pt, passed_offset_fcall, (weak_c > 0));
}



void SymTabThunkBuilder::visit(const TypeDecl* td)
{
	addToCurrentSymTab(
		td->getType()->getName(),
		td->getName(), td->getScalar(), td->getArray(),
		NULL);
}

void SymTabThunkBuilder::visit(const TypeBlock* tb)
{
	TypeBlock::const_iterator	it;
	PhysTypeAggregate		*ret;

	ret = new PhysTypeAggregate("__block" + cur_type->getName());

	for (	TypeBlock::const_iterator it = tb->begin(); 
		it != tb->end(); 
		it++) 
	{
		(*it)->accept(this);
		if (ret_pt == NULL) {
			continue;
		}

		ret->add(ret_pt->copy());
	}

	/* but we should return the ptype.. */
	retType(ret);
}

void SymTabThunkBuilder::visit(const TypeCond* tc)
{
	PhysicalType	*pt_t, *pt_f;

	weak_c++;

	(tc->getTrueStmt())->accept(this);
	pt_t = ret_pt->copy();

	if (tc->getFalseStmt() != NULL) {
		tc->getFalseStmt()->accept(this);
		pt_f = ret_pt->copy();
	} else {
		pt_f = NULL;
	}

	weak_c--;

	retType(cond_resolve(tc->getCond(), tm, pt_t, pt_f));
}


void SymTabThunkBuilder::visit(const TypeUnion* tu)
{
	TypeBlock::const_iterator	it;
	const TypeBlock			*block;
	PhysTypeUnion			*pt_anon_union;

	block = tu->getBlock();
	pt_anon_union = new PhysTypeUnion(
		"__union_"+cur_type->getName()+"_"+tu->getName());

	weak_c++;

	for (it = block->begin(); it != block->end(); it++) {
		(*it)->accept(this);
		if (ret_pt != NULL)
			continue;
		pt_anon_union->add(ret_pt);
	}

	weak_c--;

	SymTabThunkBuilder	*stb = new SymTabThunkBuilder(tm);
	SymbolTable		*union_symtab = NULL;
	PhysicalType		*union_ptt = NULL;

	union_symtab = stb->getSymTab(block, union_ptt);
	union_symtab->setOwner(
		new PhysTypeThunk(
			cur_type, 
			new ExprList(new Id("NEEDS_UNION_THUNK_V"))));
	symtabs_thunked[pt_anon_union->getName()] = union_symtab;
	delete union_ptt;
	delete stb;

	addToCurrentSymTabAnonType(
		pt_anon_union, 
		tu->getName(),
		tu->getScalar(),
		tu->getArray());

	delete pt_anon_union;
}



void SymTabThunkBuilder::visit(const TypeParamDecl* tp)
{
	addToCurrentSymTab(
		tp->getType()->getName(),
		tp->getName(), tp->getScalar(), tp->getArray(),
		(tp->getType()->getExprs())->copy());
}

