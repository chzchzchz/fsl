#include <iostream>
#include "evalctx.h"
#include "code_builder.h"
#include "util.h"
#include "cond.h"
#include "writepkt.h"
#include "type.h"
#include "symtab.h"
#include "struct_writer.h"
#include "runtime_interface.h"

extern CodeBuilder	*code_builder;
extern symtab_map	symtabs;
extern RTInterface	rt_glue;
extern writepkt_map	writepkts_map;

const VarScope* gen_vscope;

using namespace std;

void WritePktId::print(std::ostream& out) const
{
	out << "(writepkt-id ";
	id->print(out);
	out << ' ';
	e->print(out);
	out << ")";
}

void WritePktStruct::print(std::ostream& out) const
{
	out << "(writepkt-ids ";
	ids->print(out);
	out << ' ';
	e->print(out);
	out << ")";
}

void WritePktArray::print(std::ostream& out) const
{
	out << "(writepkt-array ";
	a->print(out);
	out << ' ';
	e->print(out);
	out << ")";
}

void WritePktBlk::print(std::ostream& out) const
{
	out << "(writepkt-blk ";
	for (const_iterator it = begin(); it != end(); it++) {
		(*it)->print(out);
		out << "\n";
	}
	out << ")";
}

void WritePkt::genCode(void) const
{
	/* for every stmt we want to extract the values from the
	 * passed in array..
	 */
	genStmtFuncCode();
}

void WritePktStmt::genCode(void) const
{
	cerr << "You should override the genCode in WritePktSmt.." << endl;
	assert (0 == 1);
}

void WritePkt::genProtos(void) const
{
	/* to generate:
	 * 	function proto for computing parameters
	 *	function proto for every pktblk line
	 *	No function proto per pktblk
	 */
	genStmtFuncProtos();
}

void WritePktStruct::genCode(void) const
{
	/* 0. create function / first bb */
	llvm::Function			*f;
	llvm::BasicBlock		*entry_bb, *bb_doit;
	llvm::IRBuilder<>		*builder;
	Expr				*lhs_loc, *lhs_size;
	Expr				*write_val;
	Expr				*write_call;
	const CondExpr			*ce;
	EvalCtx				ectx(
		getParent()->getParent()->getVarScope());

	f = code_builder->getModule()->getFunction(getFuncName());
	if (f == NULL) {
		cerr	<< "Could not find function prototype for "
			<< getFuncName() << endl;
		return;
	}

	entry_bb = llvm::BasicBlock::Create(llvm::getGlobalContext(), "entry", f);
	bb_doit = llvm::BasicBlock::Create(llvm::getGlobalContext(), "doit", f);

	builder = code_builder->getBuilder();
	builder->SetInsertPoint(entry_bb);

	/* 1. extract and name arguments from argument array */
	getParent()->getParent()->genLoadArgs(f);

	/* 2. if cond exists, generate conditional code */
	ce = getCond();
	if (ce != NULL){
		llvm::BasicBlock	*bb_else;
		llvm::Value		*cond_v;

		/* setup allocate BBs */
		bb_else = llvm::BasicBlock::Create(
			llvm::getGlobalContext(), "wpkt_else", f);
		builder->SetInsertPoint(entry_bb);

		/* generate conditional jump */
		cond_v = cond_codeGen(&ectx, ce);
		if (cond_v == NULL) {
			cerr <<  "WpktStruct: could not gen condition" << endl;
			return;
		}

		builder->CreateCondBr(cond_v, bb_doit, bb_else);
		builder->SetInsertPoint(bb_else);
		builder->CreateRetVoid();
	} else {
		builder->CreateBr(bb_doit);
	}

	/* either cond was true or no cond was present */
	builder->SetInsertPoint(bb_doit);

	/* 3. generate location for LHS */
	if (ectx.getType(ids) != NULL)  {
		cerr << "OOPS! WritePktStruct type ";
		ids->print(cerr);
		cerr << endl;
		assert (0 == 1 && "EXPECTED PRIMITIVE TYPE IN WPKT");
		return;
	}

	lhs_loc = ectx.resolveLoc(ids, lhs_size);
	if (lhs_loc == NULL) {
		cerr << "WritePkt: Could not resolve ";
		ids->print(cerr);
		cerr << endl;
		gen_vscope->print();
		assert (0 == 1 && "OOPS");
		return;
	}

	write_val = eval(ectx, e);

	/* 4. tell the runtime what we want to write */
	write_call = rt_glue.writeVal(
		lhs_loc,	/* location */
		lhs_size,	/* size of location */
		write_val	/* value */
	);

	delete write_val;
	delete lhs_loc;
	delete lhs_size;

	/* 5. and do it */
	write_call->codeGen();
	delete write_call;

	builder->CreateRetVoid();
}

void WritePktBlk::setParent(WritePkt* wp, unsigned int n)
{
	int	k;
	parent = wp;
	blk_num = n;
	k = 0;
	for (iterator it = begin(); it != end(); it++, k++) {
		WritePktStmt	*wps = *it;
		wps->setParent(this, k);
	}
}

void WritePkt::genStmtFuncCode(void) const
{
	unsigned int	i = 0;
	gen_vscope = NULL;
	for (const_iterator it = begin(); it != end(); it++) {
		WritePktBlk*	wpb = *it;
		for (	WritePktBlk::const_iterator it2 = wpb->begin();
			it2 != wpb->end(); it2++, i++)
		{
			WritePktStmt	*wps = *it2;
			wps->genCode();
		}
	}
	gen_vscope = NULL;
}

void WritePkt::genStmtFuncProtos(void) const
{
	unsigned int	i = 0;
	for (const_iterator it = begin(); it != end(); it++) {
		WritePktBlk*	wpb = *it;
		for (	WritePktBlk::const_iterator it2 = wpb->begin();
			it2 != wpb->end(); it2++, i++)
		{
			WritePktStmt	*wps = *it2;
			wps->genProto();
		}
	}
}

string WritePktStmt::getFuncName(void) const
{
	return  "__wpkt_"+parent->getParent()->getName() +
		"_b" + int_to_string(parent->getBlkNum()) +
		"_s" + int_to_string(stmt_num);
}

void WritePktStmt::genProto(void) const
{
	llvm::Function		*f;
	llvm::FunctionType	*ft;
	std::string		f_name;

	f_name = getFuncName();

	/* write pkt function calls take a single argument, a pointer */
	vector<const llvm::Type*>	args(
		1,
		llvm::Type::getInt64PtrTy(llvm::getGlobalContext()));

	ft = llvm::FunctionType::get(
		llvm::Type::getVoidTy(llvm::getGlobalContext()),
		args,
		false);

	f = llvm::Function::Create(
		ft,
		llvm::Function::ExternalLinkage,
		f_name,
		code_builder->getModule());

	/* should not be redefinitions.. */
	if (f->getName() != f_name) {
		cerr << "Expected name " << f_name <<" got " <<
		f->getNameStr() << endl;
	}

	assert (f->getName() == f_name);
	assert (f->arg_size() == 1);
}

void WritePktAnon::print(std::ostream& out) const
{
	out << "(writepkt-anon ";
	wpb->print(out);
	out << ")";
}

void WritePktCall::print(std::ostream& out) const
{
	out << "(writepkt-call ";
	name->print(out);
	out << " ";
	exprs->print(out);
	out << ")";
}

WritePkt::WritePkt(Id* in_name, ArgsList* in_args, const wblk_list& wbs)
	: name(in_name), args(in_args)
{
	unsigned int	blk_num;

	assert (name != NULL);
	assert (args != NULL);
	blk_num = 0;
	for (	wblk_list::const_iterator it =  wbs.begin();
		it != wbs.end();
		it++, blk_num++)
	{
		WritePktBlk	*wpb;
		wpb = *it;
		wpb->setParent(this, blk_num);
		add(wpb);
	}
}

void WritePkt::genLoadArgs(llvm::Function* f)
{
	llvm::Function::arg_iterator		arg_it;
	llvm::AllocaInst			*arg_array_ptr;
	llvm::IRBuilder<>			*builder;

	builder = code_builder->getBuilder();
	assert (f->arg_begin() != f->arg_end() && "Must have one argument");
	arg_it = f->arg_begin();

	arg_array_ptr = code_builder->createTmpI64Ptr();
	builder->CreateStore(arg_it, arg_array_ptr);

	/* arguments are passed to wpkt functions by array of 64-bit ints */
	/* extract arguments based on argslist */
	vscope.clear();
	vscope.loadArgsFromArray(arg_array_ptr, args);
	gen_vscope = &vscope;
}

WritePktInstance* WritePkt::getInstance(
	const std::string& protoname,
	const class Type* clo_type,
	const ExprList* exprs) const
{
	return new WritePktInstance(this, clo_type, protoname, exprs);
}

WritePktInstance::WritePktInstance(
	const WritePkt* in_parent,
	const class Type* in_t,
	const std::string& in_fname,
	const ExprList*	in_exprs)
: parent(in_parent), t(in_t), funcname(in_fname)
{
	exprs = in_exprs->copy();
}

unsigned int WritePktInstance::getParamBufEntries(void) const
{
	return getParent()->getArgs()->getNumParamBufEntries();
}

void WritePktInstance::genCode(const ArgsList* args_in /* bindings */) const
{
	/* 0. create function / first bb */
	llvm::Function			*f;
	llvm::BasicBlock		*entry_bb;
	llvm::IRBuilder<>		*builder;
	llvm::Function::arg_iterator	arg_it;
	llvm::AllocaInst		*pb_out_ptr;
	const ArgsList			*args_out;
	unsigned int			pb_idx, arg_idx;
	EvalCtx				ectx(symtabs[t->getName()]);

	args_out = getParent()->getArgs();
	assert (args_out != NULL);
	assert (exprs->size() == args_out->size());

	cerr << "Generating code for wpktinstance " << funcname << endl;

	f = code_builder->getModule()->getFunction(funcname);
	assert (f->arg_size() == 3);
	assert (f != NULL && "Missing wpktinst prototype");

	entry_bb = llvm::BasicBlock::Create(llvm::getGlobalContext(), "entry", f);
	builder = code_builder->getBuilder();
	builder->SetInsertPoint(entry_bb);

	/* 1. pull in type scope */
	code_builder->genThunkHeaderArgs(f, t);

	/* 1.5. pull in input parambuf */
	arg_it = f->arg_begin();
	arg_it++;
	code_builder->loadArgsFromParamBuf(arg_it, args_in);

	/* 2. load up output parambuf */
	arg_it++;
	pb_out_ptr = code_builder->createTmpI64Ptr();
	builder->CreateStore(arg_it, pb_out_ptr);

	/* 3. dump expressions into parambuf */
	pb_idx = 0;
	arg_idx = 0;
	for (	ExprList::const_iterator it = exprs->begin();
		it != exprs->end();
		it++, arg_idx++)
	{
		const Expr	*cur_expr = *it;
		Expr		*evaled_cexpr;
		int		elems_stored;

		evaled_cexpr = eval(ectx, cur_expr);
		elems_stored = code_builder->storeExprIntoParamBuf(
			args_out->get(arg_idx), evaled_cexpr,
			pb_out_ptr, pb_idx);
		delete evaled_cexpr;
		if (elems_stored <= 0) {
			assert (0 == 1 && "FAILED TO STORE");
			return;
		}
		pb_idx += elems_stored;
	}

	/* 4. return */
	builder->CreateRetVoid();
}

void WritePktInstance::genProto(void) const
{
	llvm::Function			*f;
	llvm::FunctionType		*ft;
	vector<const llvm::Type*>	args;


	args.push_back(code_builder->getClosureTyPtr());
	args.push_back(
		llvm::Type::getInt64PtrTy(llvm::getGlobalContext()));
	args.push_back(
		llvm::Type::getInt64PtrTy(llvm::getGlobalContext()));

	/* writepkt inst function calls take three arguments:
	 * 	1. source type closure
	 * 	2. parambuf for input
	 * 	3. parambuf for output
	 */
	ft = llvm::FunctionType::get(
		llvm::Type::getVoidTy(llvm::getGlobalContext()),
		args,
		false);

	f = llvm::Function::Create(
		ft,
		llvm::Function::ExternalLinkage,
		funcname,
		code_builder->getModule());

	/* should not be redefinitions.. */
	if (f->getName() != funcname) {
		cerr << "Expected name " << funcname <<" got " <<
		f->getNameStr() << endl;
	}

	assert (f->getName() == funcname);
	assert (f->arg_size() == 3);
}

WritePktInstance* WritePkt::getInstance(
	const Expr* wpkt_call,
	const std::string& protoname,
	const class Type* clo_type)
{
	WritePkt	*wpkt;
	const FCall	*wpkt_fc;

	assert (wpkt_call != NULL);
	assert (clo_type != NULL);

	if ((wpkt_fc = dynamic_cast<const FCall*>(wpkt_call)) == NULL)
		return NULL;

	if (writepkts_map.count(wpkt_fc->getName()) == 0)
		return NULL;

	wpkt = writepkts_map[wpkt_fc->getName()];
	return wpkt->getInstance(protoname, clo_type, wpkt_fc->getExprs());
}

void WritePktInstance::genExterns(TableGen* tg) const
{
	string	args_pr[] = {
		"const struct fsl_rt_closure*", "uint64_t*", "uint64_t*"};
	tg->printExternFunc(
		funcname,
		vector<string>(args_pr,args_pr+3),
		"void");
}

void WritePktInstance::genTableInstance(TableGen* tg) const
{
	StructWriter	sw(tg->getOS());

	sw.write("wpi_params", funcname);
	sw.write(	"wpi_wpkt",
			string("&wpkt_")+parent->getName()+int_to_string(0));
}
