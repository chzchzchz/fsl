#include <iostream>
#include "evalctx.h"
#include "code_builder.h"
#include "util.h"
#include "writepkt.h"
#include "type.h"
#include "symtab.h"
#include "struct_writer.h"
#include "runtime_interface.h"
#include "wpkt_inst.h"
#include "wpkt_struct.h"
#include "wpkt_call.h"

extern CodeBuilder	*code_builder;
extern symtab_map	symtabs;
extern RTInterface	rt_glue;
extern writepkt_map	writepkts_map;

const VarScope* gen_vscope;

using namespace std;

std::ostream& WritePktId::print(std::ostream& out) const
{
	out << "(writepkt-id ";
	id->print(out); out << ' '; e->print(out);
	out << ")";
	return out;
}

std::ostream& WritePktArray::print(std::ostream& out) const
{
	out << "(writepkt-array ";
	a->print(out); out << ' '; e->print(out);
	out << ")";
	return out;
}

std::ostream& WritePktBlk::print(std::ostream& out) const
{
	out << "(writepkt-blk ";
	for (const_iterator it = begin(); it != end(); it++) {
		(*it)->print(out);
		out << "\n";
	}
	out << ")";
	return out;
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

bool WritePktStmt::genCodeHeader(
	llvm::Function* f,
	llvm::BasicBlock* entry_bb,
	llvm::BasicBlock* bb_doit,
	llvm::BasicBlock* bb_else) const
{
	/* 0. create function / first bb */
	llvm::IRBuilder<>		*builder;
	const CondExpr			*ce;
	EvalCtx				ectx(
		getParent()->getParent()->getVarScope());

	if (f == NULL)
		f = code_builder->getModule()->getFunction(getFuncName());
	if (f == NULL) {
		cerr	<< "Could not find function prototype for "
			<< getFuncName() << endl;
		return false;
	}

	if (entry_bb == NULL)
		entry_bb = llvm::BasicBlock::Create(
			llvm::getGlobalContext(), "entry", f);
	if (bb_doit == NULL)
		bb_doit = llvm::BasicBlock::Create(
			llvm::getGlobalContext(), "doit", f);

	builder = code_builder->getBuilder();
	builder->SetInsertPoint(entry_bb);

	/* 1. extract and name arguments from argument array */
	getParent()->getParent()->genLoadArgs(f);

	/* 2. if cond exists, generate conditional code */
	ce = getCond();
	if (ce != NULL){
		llvm::Value		*cond_v;

		/* setup allocate BBs */
		if (bb_else == NULL) {
			bb_else = llvm::BasicBlock::Create(
				llvm::getGlobalContext(),
				"wpkt_else", f);
			builder->SetInsertPoint(bb_else);
			builder->CreateRetVoid();
		}
		builder->SetInsertPoint(entry_bb);

		/* generate conditional jump */
		cond_v = ce->codeGen(&ectx);
		if (cond_v == NULL) {
			cerr <<  "WpktStmt: could not gen condition" << endl;
			return false;
		}

		builder->CreateCondBr(cond_v, bb_doit, bb_else);
		builder->SetInsertPoint(bb_else);
	} else
		builder->CreateBr(bb_doit);

	/* either cond was true or no cond was present */
	builder->SetInsertPoint(bb_doit);
	return true;
}

void WritePktBlk::setParent(WritePkt* wp, unsigned int n)
{
	int	k;
	parent = wp;
	blk_num = n;
	k = 0;
	for (iterator it = begin(); it != end(); it++, k++) {
		WritePktStmt	*wps = *it;
		cerr << "ADDING: " << endl;
		wps->print(cerr);
		cerr << endl;
		wps->setParent(this, k);
		cerr << "FUNC NAME: " << wps->getFuncName() << endl << endl;
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

std::ostream& WritePktAnon::print(std::ostream& out) const
{
	return out << "(writepkt-anon " << wpb->print(out) << ")";
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

void WritePkt::genFuncTables(TableGen* tg) const
{
	const_iterator			it;
	unsigned int			n;
	ostream&			os(tg->getOS());

	/* generate dump of write stmts in a few tables */
	for (it = begin(), n = 0; it != end(); it++, n++) {
		const WritePktBlk	*wblk = *it;

		os << "static wpktf_t wpkt_funcs_" <<
			getName() << n << "[] = \n";
		{
		StructWriter			sw(os);
		for (	WritePktBlk::const_iterator it2 = wblk->begin();
			it2 != wblk->end();
			it2++)
		{
			const WritePktStruct	*wstmt;
			wstmt = dynamic_cast<const WritePktStruct*>((*it2));
			if (wstmt == NULL) continue;
			sw.write(wstmt->getFuncName());
		}
		}
		os << ";\n";
	}
}

void WritePkt::genCallsTables(TableGen* tg) const
{
	const_iterator			it;
	unsigned int			n;
	ostream&			os(tg->getOS());

	/* generate dump of write stmts in a few tables */
	for (it = begin(), n = 0; it != end(); it++, n++) {
		const WritePktBlk	*wblk = *it;

		os << "static struct fsl_rtt_wpkt2wpkt wpkt_calls_" <<
			getName() << n << "[] = \n";
		{
		StructWriter			sw(os);
		for (	WritePktBlk::const_iterator it2 = wblk->begin();
			it2 != wblk->end();
			it2++)
		{
			const WritePktCall	*wc;
			wc = dynamic_cast<const WritePktCall*>((*it2));
			if (wc == NULL) continue;
			sw.beginWrite();
			wc->genTableInstance(tg);
		}
		}
		os << ";\n";
	}
}

void WritePkt::genWpktStructs(TableGen* tg) const
{
	const_iterator			it;
	unsigned int			n, num_blks;
	ostream&			os(tg->getOS());
	list<WritePktBlk*>		l(*this);

	l.reverse();
	/* now generate writepkt structs.. */
	num_blks = size();
	for (it = l.begin(), n = num_blks-1; it != l.end(); it++, n--) {
		const WritePktBlk*	wblk = *it;
		StructWriter	sw(
			os,
			"fsl_rtt_wpkt",
			string("wpkt_") + getName() + int_to_string(n),
			false);
		sw.write("wpkt_param_c",
			getArgs()->getNumParamBufEntries());
		sw.write("wpkt_func_c", wblk->getNumFuncs());
		sw.write("wpkt_funcs",
			"wpkt_funcs_" + getName() + int_to_string(n));
		/* XXX need to support embedded blocks.. */
		sw.write("wpkt_blk_c", 0);
		sw.write("wpkt_blks", "NULL");

		sw.write("wpkt_call_c", wblk->getNumCalls());
		sw.write("wpkt_calls",
			"wpkt_calls_" + getName() + int_to_string(n));

		if (n != 0)
			sw.write(
				"wpkt_next",
				string("wpkt_")+getName()+int_to_string(n-1));
		else
			sw.write("wpkt_next", "NULL");
	}
}

void WritePkt::genTables(TableGen* tg) const
{
	genFuncTables(tg);
	genCallsTables(tg);
	genWpktStructs(tg);
}

void WritePktStmt::printExterns(class TableGen* tg) const
{
	assert (0 == 1 && "BASE IMPL");
}

void WritePkt::genExterns(TableGen* tg) const
{
	unsigned int	n;
	const_iterator	it;
	ostream&	os(tg->getOS());

	/* extern all stmt functions */
	for (it = begin(); it != end(); it++) {
		const WritePktBlk	*wblk = *it;

		for (	WritePktBlk::const_iterator it2 = wblk->begin();
			it2 != wblk->end();
			it2++) {
			(*it2)->printExterns(tg);
		}
	}

	/* extern all writepkt structs for outcalls */
	for (it = begin(), n = 0; it != end(); it++, n++) {
		os	<< "extern const struct fsl_rtt_wpkt "
			<< string("wpkt_")+getName()+int_to_string(n)<<";\n";
	}
}

unsigned int WritePktBlk::getNumFuncs(void) const
{
	unsigned int	ret = 0;
	for (const_iterator it = begin(); it != end(); it++)
		if (dynamic_cast<WritePktStruct*>((*it)) != NULL) ret++;
	return ret;
}

unsigned int WritePktBlk::getNumCalls(void) const
{
	unsigned int	ret = 0;
	for (const_iterator it = begin(); it != end(); it++) {
		if (dynamic_cast<WritePktCall*>((*it)) != NULL) ret++;
	}
	return ret;
}
