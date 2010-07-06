#ifndef TYPE_H
#define TYPE_H

#include <iostream>
#include <list>
#include <map>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "AST.h"
#include "collection.h"
#include "expr.h"

typedef std::map<std::string, class PhysicalType*>	ptype_map;
typedef std::list<class Type*>				type_list;
typedef std::map<std::string, class Type*>		type_map;

extern int yylineno;

class TypeDecl;
class TypeUnion;
class TypeParamDecl;
class TypeBlock;
class TypeCond;
class TypeFunc;

class TypeVisitor
{
public:
	virtual ~TypeVisitor() {}

	virtual void visit(const TypeDecl* td) = 0;
	virtual void visit(const TypeUnion* tu) = 0;
	virtual void visit(const TypeParamDecl* tpd) = 0;
	virtual void visit(const TypeBlock* tb) = 0;
	virtual void visit(const TypeCond* tb) = 0;
	virtual void visit(const TypeFunc* tf) = 0;
protected:
	TypeVisitor() {}
};

class TypeStmt
{
public:
	virtual ~TypeStmt() {}
	virtual void print(std::ostream& out) const = 0;
	unsigned int getLineNo() const { return lineno; }
	void setLineNo(unsigned int l) { lineno = l; }

	const Type* getOwner() const
	{
		/* do not try to get the owner when still anonymous */
		if (owner == NULL)
			assert (0 == 1);

		return owner;
	}

	virtual void setOwner(const Type* new_owner)
	{
		/* set once */
		assert (owner == NULL);
		owner = new_owner;
	}


	virtual void accept(TypeVisitor* tv) const = 0;
protected:
	TypeStmt() : lineno(yylineno), owner(NULL) {}
	const Type*	owner;

private:
	unsigned int	lineno;
};

static inline unsigned int tstmt_count_conds(const TypeStmt* t);

/* declaration */
class TypeDecl : public TypeStmt
{
public:
	TypeDecl(Id* in_type, Id* in_name)
	:	type(in_type),
		name(in_name),
		array(NULL)
	{
		assert (type != NULL);
		assert (name != NULL);
	}

	TypeDecl(Id* in_type, IdArray* in_array)
	:	type(in_type),
		name(NULL),
		array(in_array)
	{
		assert (type != NULL);
		assert (array != NULL);
	}

	virtual ~TypeDecl();

	void print(std::ostream& out) const;
	
	const std::string getName() const 
	{
		if (name != NULL)
			return name->getName();
		return array->getName();
	}

	virtual void accept(TypeVisitor* tv) const { tv->visit(this); }

	const Id* getType(void) const { return type; }
	const Id* getScalar(void) const { return name; }
	const IdArray* getArray(void) const { return array; }
private:
	Id*		type;
	Id*		name;
	IdArray*	array;
};

class TypeParamDecl : public TypeStmt
{
public:
	TypeParamDecl(FCall* in_type, Id* in_name)
	: name(in_name), array(NULL), type(in_type) 
	{
		assert (name != NULL);
		assert (type != NULL);
	}

	TypeParamDecl(FCall* in_type, IdArray* in_array) 
	: name(NULL), array(in_array), type(in_type)
	{
		assert (array != NULL);
		assert (type != NULL);
	}

	virtual ~TypeParamDecl() 
	{
		if (name != NULL) delete name;
		if (array != NULL) delete array;
		delete type;
	}

	void print(std::ostream& out) const;

	const std::string getName() const 
	{
		if (name != NULL)
			return name->getName();
		return array->getName();
	}

	virtual void accept(TypeVisitor* tv) const { tv->visit(this); }

	const FCall* getType(void) const { return type; }
	const Id* getScalar(void) const { return name; }
	const IdArray* getArray(void) const { return array; }
private:
	Id*		name;
	IdArray*	array;
	FCall*		type;
};


class TypeCond: public TypeStmt
{
public:
	TypeCond(CondExpr* ce, TypeStmt* taken, TypeStmt* nottaken = NULL) 
	: cond(ce), is_true(taken), is_false(nottaken)
	{
		assert (cond != NULL);
		assert (is_true != NULL);
	}

	virtual ~TypeCond(void)
	{
		delete cond;
		if (is_false != NULL) delete is_false;
		delete is_true;
	}

	void print(std::ostream& out) const { out << "TYPECOND"; }

	const TypeStmt* getTrueStmt() const { return is_true; }
	const TypeStmt* getFalseStmt() const { return is_false; }
	const CondExpr* getCond() const { return cond; }

	void setOwner(const Type* t) 
	{
		TypeStmt::setOwner(t);
		is_true->setOwner(t);
		if (is_false != NULL) is_false->setOwner(t);
	}	

	virtual void accept(TypeVisitor* tv) const { tv->visit(this); }
private:
	CondExpr	*cond;
	TypeStmt	*is_true;
	TypeStmt	*is_false;
};

class TypePreamble : public PtrList<FCall>
{
public:
	TypePreamble() {}
	virtual ~TypePreamble() {}

	void print(std::ostream& out) const { out << "PREAMBLE"; }
};

class TypeBlock : public TypeStmt, public PtrList<TypeStmt>
{
public:
	TypeBlock() {}
	virtual ~TypeBlock() {}

	void print(std::ostream& out) const;

	/* get number of conditions */
	unsigned int getNumConds(void) const
	{
		unsigned int	ret;

		ret = 0;
		for (const_iterator it = begin(); it != end(); it++) {
			ret += tstmt_count_conds(*it);
		}

		return ret;
	}

	virtual void setOwner(const Type* t)
	{
		for (iterator it = begin(); it != end(); it++) {
			(*it)->setOwner(t);
		}
	}

	virtual void accept(TypeVisitor* tv) const { tv->visit(this); }
};

class TypeUnion : public TypeStmt
{
public:
	TypeUnion(TypeBlock* in_block, Id* in_name)
	: block(in_block),
	  name(in_name),
	  array(NULL)
	{
		assert (block != NULL);
		assert (name != NULL);
	}

	TypeUnion(TypeBlock* in_block, IdArray* in_array)
	: block(in_block),
	  name(NULL),
	  array(in_array)
	{
		assert (block != NULL);
		assert (array != NULL);
	}

	virtual ~TypeUnion()
	{
		if (name != NULL) delete name;
		if (array != NULL) delete array;
		delete block;
	}

	void print(std::ostream& out) const { 
		out << "UNION: "; 
		block->print(out);
	}

	virtual void setOwner(const Type* t) 
	{
		TypeStmt::setOwner(t);
		block->setOwner(t);
	}

	const TypeBlock* getBlock(void) const { return block; }

	virtual void accept(TypeVisitor* tv) const { tv->visit(this); }
private:
	TypeBlock	*block;
	Id		*name;
	IdArray		*array;
};


class TypeFunc : public TypeStmt
{
public:
	TypeFunc(FCall* in_fcall) 
		: fcall(in_fcall)
	{
		assert (fcall != NULL);
	} 

	virtual ~TypeFunc() 
	{
		delete fcall;
	}

	void print(std::ostream& out) const { out << "TYPEFUNC"; }

	const std::string getName(void) const 
	{
		std::string	s(fcall->getName());
		int		tmp_str_bufsz = s.size() + 32;
		char		tmp_str[tmp_str_bufsz];

		snprintf(tmp_str, tmp_str_bufsz, "%s_%p", s.c_str(), this);

		return std::string(tmp_str);
	}

	virtual void accept(TypeVisitor* tv) const { tv->visit(this); }

	const FCall* getFCall(void) const { return fcall; }
private:
	FCall*	fcall;
};


class Type : public GlobalStmt
{
public:

	Type(	Id* in_name, ArgsList* in_args, TypePreamble* in_preamble, 
		TypeBlock* in_block)
	: name(in_name),
	  args(in_args),
	  preamble(in_preamble),
	  block(in_block),
	  type_num(-1)
	{
		assert (in_name != NULL);
		assert (in_block != NULL);
		in_block->setOwner(this);
	}

	void print(std::ostream& out) const;

	virtual ~Type(void);

	const std::string& getName(void) const { return name->getName(); }

	PhysicalType* resolve(const ptype_map& tm) const;
	const ArgsList* getArgs(void) const { return args; }
	class SymbolTable* getSyms(const ptype_map& tm) const;


	std::list<const FCall*> getPreambles(const std::string& name) const;

	void setTypeNum(int new_type_num)
	{
		/* do not set type number more than once since we may happen
		 * to rely on a stale value */
		assert (type_num == -1);
		assert (new_type_num >= 0);
		type_num = new_type_num;
	}

	int getTypeNum(void) const 
	{
		/* do not allow get if it hasn't been set yet! */
		assert (type_num != -1);
		return type_num;
	}

	unsigned int getNumArgs(void) const
	{
		if (args == NULL) return 0;
		return args->size();
	}

	const TypeBlock* getBlock() const { return block; }

private:
	Id		*name;
	ArgsList	*args;
	TypePreamble	*preamble;
	TypeBlock	*block;
	int		type_num;
	
};

std::ostream& operator<<(std::ostream& in, const Type& t);

static unsigned int tstmt_count_conds(const TypeStmt* t)
{
	const TypeBlock	*b;
	const TypeCond	*c;

	if (t == NULL) return 0;

	b = dynamic_cast<const TypeBlock*>(t);
	if (b != NULL)
		return b->getNumConds();

	c = dynamic_cast<const TypeCond*>(t);
	if (c != NULL) {
		return	1 + 
			tstmt_count_conds(c->getTrueStmt()) + 
			tstmt_count_conds(c->getFalseStmt());
	}


	return 0;
}


class TypeVisitAll : public TypeVisitor
{
public:
	virtual ~TypeVisitAll() {}

	virtual void apply(const Type* t) 
	{
		visit(t->getBlock());
	}

	virtual void visit(const TypeDecl* td) { return; }

	virtual void visit(const TypeUnion* tu)
	{
		const TypeBlock	*tb = tu->getBlock();
		for (	TypeBlock::const_iterator it = tb->begin();
			it != tb->end();
			it++)
		{
			(*it)->accept(this);
		}
	}
	virtual void visit(const TypeParamDecl* tpd) { return; }


	virtual void visit(const TypeBlock* tb)
	{	
		for (	TypeBlock::const_iterator it = tb->begin();
			it != tb->end();
			it++)
		{
			(*it)->accept(this);
		}
	}

	virtual void visit(const TypeCond* tc) 
	{
		tc->getTrueStmt()->accept(this);
		if (tc->getFalseStmt() != NULL) {
			tc->getFalseStmt()->accept(this);
		}
	}

	virtual void visit(const TypeFunc* tf) { return; }

protected:
	TypeVisitAll() {}
};


std::ostream& operator<<(std::ostream& in, const Type& t);

inline static std::string typeThunkName(const Type* t)
{
	return (std::string("__thunk_") + t->getName()) + "_bits";
}

inline static std::string typeOffThunkName(
	const Type* t, const std::string& varname)
{
	return std::string("__thunkoff_")+t->getName()+"_"+varname+"_bits";
}


#endif
