#include "wpkt_struct.h"
#include "code_builder.h"
#include "evalctx.h"
#include "eval.h"
#include "util.h"
#include "runtime_interface.h"

using namespace std;
extern CodeBuilder	*code_builder;
extern RTInterface	rt_glue;
extern const VarScope	*gen_vscope;

std::ostream& WritePktStruct::print(std::ostream& out) const
{
	out << "(writepkt-ids ";
	ids->print(out);
	out << ' ';
	e->print(out);
	out << ")";
	return out;
}

void WritePktStruct::genCode(void) const
{
	Expr				*write_val;
	Expr				*lhs_loc, *lhs_size;
	Expr				*write_call;
	llvm::IRBuilder<>		*builder;
	EvalCtx				ectx(
		getParent()->getParent()->getVarScope());

	if (WritePktStmt::genCodeHeader() == false) return;

	builder = code_builder->getBuilder();

	/* 1. generate location for LHS */
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

	/* 2. tell the runtime what we want to write */
	write_call = rt_glue.writeVal(
		lhs_loc,	/* location */
		lhs_size,	/* size of location */
		write_val	/* value */
	);

	delete write_val;
	delete lhs_loc;
	delete lhs_size;

	/* 3. and do it */
	write_call->codeGen();
	delete write_call;

	builder->CreateRetVoid();
}

void WritePktStruct::genProto(void) const
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

void WritePktStruct::printExterns(class TableGen* tg) const
{
	const string args[] = {"const uint64_t*"};
	tg->printExternFunc(
		getFuncName(),
		vector<string>(args, args+1),
		"void");
}
