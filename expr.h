#ifndef EXPR_H
#define EXPR_H

class Expr
{
};

class ExprList
{
public:
	ExprList() {}
	~ExprList() 
	{
		for (int i = 0; i < exprs.size(); i++)
			delete exprs[i];
	}

	void add(const Expr* expr) { exprs.push_back(expr); }
	const std::vector<const Expr*>&	get() const { return exprs; }
private:
	std::vector<const Expr*>	exprs;
};

class Id  : public Expr
{
public: 
	Id(const std::string& s) : id_name(s) {}
	virtual ~Id() {}
	const std::string& getName() const { return id_name; }
private:
	const std::string id_name;
};


class FCall  : public Expr
{
public:
	FCall(const Id* in_id, const ExprList* in_exprs) 
	:	id(in_id),
		exprs(in_exprs)
	{ 
	}
	virtual ~FCall() {}
	const ExprList* getExprs(void) const { return exprs; }
	const std::string& getName(void) const { return id->getName(); }
private:
	const Id*		id;
	const ExprList*		exprs;
};

class IdStruct : public Expr 
{
public:
	IdStruct() { }
	virtual ~IdStruct() {} 
	void add(const Expr* e) { ids.push_back(e); }
private:
	std::list<const Expr*>	ids;
};


class IdArray  : public Expr
{
public: 
	IdArray(const Id *in_id, const Expr* expr) : 
		id(in_id), idx(expr) {}

	virtual ~IdArray() {}
	const std::string& getName() const { return id->getName(); }
private:
	const Id		*id;
	const Expr		*idx;
};


class Number : public Expr
{
public:
	Number(unsigned long v) : n(v) {}
	virtual ~Number() {}

	unsigned long getValue() const { return n; }
private:
	unsigned long n;
};

class BinArithOp : public Expr
{
public:
	virtual ~BinArithOp() {}
protected:
	BinArithOp(const Expr* e1, const Expr* e2)
		: e_lhs(e1), e_rhs(e2) {}
	const Expr	*e_lhs;
	const Expr	*e_rhs;
private:
	BinArithOp() {}
};

class AOPNOP : public BinArithOp
{
public:
	AOPNOP(const Expr* e1, const Expr* e2)
		: BinArithOp(e1, e2)
	{
	}
	virtual ~AOPNOP() {}
private:
};


class AOPOr : public BinArithOp
{
public:
	AOPOr(const Expr* e1, const Expr* e2)
		: BinArithOp(e1, e2)
	{
	}
	virtual ~AOPOr() {}
private:
};

class AOPAnd : public BinArithOp
{
public:
	AOPAnd(const Expr* e1, const Expr* e2)
		: BinArithOp(e1, e2)
	{
	}
	virtual ~AOPAnd() {}
private:
};



class AOPAdd : public BinArithOp
{
public:
	AOPAdd(const Expr* e1, const Expr* e2)
		: BinArithOp(e1, e2)
	{
		e_lhs = e1;
	}
	virtual ~AOPAdd() {}
private:
};

class AOPSub : public BinArithOp
{
public:
	AOPSub(const Expr* e1, const Expr* e2) 
		: BinArithOp(e1, e2)
	{
	}

	virtual ~AOPSub() {} 
private:
};

class AOPDiv : public BinArithOp
{
public:
	AOPDiv(const Expr* e1, const Expr* e2) 
		: BinArithOp(e1, e2)
	{
	}

	virtual ~AOPDiv() {}
private:
};

class AOPMul : public BinArithOp
{
public:
	AOPMul(const Expr* e1, const Expr* e2)
		: BinArithOp(e1, e2)
	{
	}

	virtual ~AOPMul() {}
private:
};

class AOPLShift : public BinArithOp
{
public:
	AOPLShift(const Expr* e1, const Expr* e2)
		: BinArithOp(e1, e2)
	{
	}

	virtual ~AOPLShift() {}
private:
};

class AOPRShift : public BinArithOp
{
public:
	AOPRShift(const Expr* e1, const Expr* e2) 
		: BinArithOp(e1, e2)
	{
	}

	virtual ~AOPRShift() {}
private:
};

#endif
