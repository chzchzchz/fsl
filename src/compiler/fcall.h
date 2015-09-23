#ifndef FCALL_H
#define FCALL_H

class FCall  : public Expr
{
public:
	FCall(Id* in_id, ExprList* in_exprs)
	:	id(in_id),
		exprs(in_exprs)
	{
		assert (id != NULL);
		assert (exprs != NULL);
		exprs = in_exprs->simplify();
		delete in_exprs;
	}
	virtual ~FCall()
	{
		delete id;
		delete exprs;
	}

	const ExprList* getExprs(void) const { return exprs; }
	ExprList* getExprs(void) { return exprs; }
	void setExprs(ExprList* new_exprs)
	{
		assert (new_exprs != NULL);
		assert (new_exprs->size() == exprs->size());

		if (new_exprs == exprs) return;

		delete exprs;
		exprs = new_exprs->simplify();
		delete new_exprs;
	}

	const std::string& getName(void) const { return id->getName(); }
	void print(std::ostream& out) const
	{
		out << "(fcall " << getName() << " (";
		exprs->print(out);
		out << ")";
	}

	FCall* copy(void) const { return new FCall(id->copy(), exprs->copy()); }

	bool operator==(const Expr* e) const
	{
		const FCall	*in_fc;

		in_fc = dynamic_cast<const FCall*>(e);
		if (in_fc == NULL) return false;

		return (*(in_fc->id) == id) && (*exprs == in_fc->exprs);
	}

	virtual Expr* rewrite(const Expr* to_rewrite, const Expr* new_expr)
	{
		assert (to_rewrite && "no target expression");
		assert (new_expr && "no replacement expression");
		exprs->rewrite(to_rewrite, new_expr);
		return Expr::rewrite(to_rewrite, new_expr);
	}

	llvm::Value* codeGen(void) const;

	virtual Expr* accept(ExprVisitor* ev) const { return ev->visit(this); }

	static Expr* mkClosure(Expr* diskoff, Expr* params, Expr* virt);
	static Expr* mkBaseClosure(const class Type* t);
	static Expr* extractCloOff(const Expr* expr);
	static Expr* extractCloParam(const Expr* expr);
	static Expr* extractCloVirt(const Expr* expr);
	static Expr* extractParamVal(const Expr* expr, const Expr* off);
private:
	bool handleSpecialForms(llvm::Value* &ret) const;

	llvm::Value*	codeGenParams(std::vector<llvm::Value*>& args) const;
	llvm::Value* 	codeGenExtractParamVal(void) const;
	llvm::Value*	codeGen(std::vector<llvm::Value*>& args) const;
	llvm::Value*	codeGenNormalFunc() const;
	llvm::Value*	codeGenTypeFunc() const;
	llvm::Value*	codeGenLet(void) const;
	llvm::Value*	codeGenMkClosure(void) const;
	llvm::Value* 	codeGenParamsAllocaByCount(void) const;
	llvm::Value*	codeGenClosureRetCall(
		std::vector<llvm::Value*>& args) const;
	llvm::Value*	codeGenPrimitiveRetCall(
		std::vector<llvm::Value*>& args) const;

	llvm::Value* 	codeGenExtractOff(void) const;
	llvm::Value* 	codeGenExtractParam(void) const;
	llvm::Value*	codeGenExtractVirt(void) const;
	Id*		id;
	ExprList*	exprs;
};

#endif
