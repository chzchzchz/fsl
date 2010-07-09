#ifndef SYMTAB_H
#define SYMTAB_H

#include <iostream>
#include <map>
#include "phys_type.h"

typedef std::pair<PhysicalType*, class Expr*>		sym_binding;
typedef std::map<std::string, sym_binding>		sym_map;
typedef std::map<std::string, SymbolTable*>		symtab_map;

#define symbind_make(x,y)	(sym_binding(x,y))
#define symbind_phys(x)		((x).first)
#define symbind_off(x)		((x).second)

/**
 * TODO: GENERATE TABLE THAT CAN BE USED AT RUN-TIME TO RESOLVE TYPE 
 * NAMES!
 */
class SymbolTable
{
public:
	SymbolTable(
		const SymbolTable*	in_backref, 
		PhysicalType*		in_owner)
	: backref(in_backref),
	  owner(in_owner)
	{
	}

	virtual ~SymbolTable()
	{
		freeData();
	}

	void setOwner(PhysicalType* new_owner) 
	{
		assert (owner == NULL);
		owner = new_owner;
	}

	const PhysicalType* getOwner(void) const { return owner; }

	const Type* getOwnerType(void) const 
	{ 
		const PhysTypeUser	*ptu;
		const PhysTypeThunk	*ptt;

		ptu = dynamic_cast<const PhysTypeUser*>(getOwner());
		if (ptu != NULL) {
			return ptu->getType();
		}

		ptt = dynamic_cast<const PhysTypeThunk*>(getOwner());
		if (ptt != NULL ) {
			return ptt->getType();
		}

		return NULL;
	}

	bool add(const std::string& name, PhysicalType* pt, Expr* offset_bits)
	{
		if (sm.count(name) != 0) {
			/* exists */
			return false;
		}

		sm[name] = symbind_make(pt,offset_bits);
		return true;
	}

	bool lookup(const std::string& name, sym_binding& sb) const
	{
		sym_map::const_iterator	it;

		if (sm.count(name) == 0) {
			if (backref == NULL)
				return false;

			return backref->lookup(name, sb);
		}

		it = sm.find(name);
		if (it == sm.end()) return false;

		sb = ((*it).second);
		return true;
	}

	void print(std::ostream& out) const
	{
		sym_map::const_iterator	it;

		out << "SymTable for " << owner->getName() << std::endl;
		for (it = sm.begin(); it != sm.end(); it++) {
			sym_binding	sb((*it).second);
			out 	<< "Symbol: \"" <<  (*it).first << "\" = ";
			out << symbind_phys(sb)->getName();
			out << '@';
			symbind_off(sb)->print(out);
			out << std::endl;
		}
	}

	SymbolTable& operator=(const SymbolTable& st)
	{
		copyInto(st);
		return *this;
	}

	void copyInto(const SymbolTable& st, Expr* new_base = NULL)
	{
		sym_map::const_iterator	it;

		if (&st == this)
			return;

		freeData();

		owner = (st.owner)->copy();
		backref = st.backref;
	
		for (it = st.sm.begin(); it != st.sm.end(); it++) {
			if (new_base == NULL) {
				add(	(*it).first,
					symbind_phys(((*it).second))->copy(),
					symbind_off((*it).second)->copy());
			} else {
				add(	(*it).first,
					symbind_phys(((*it).second))->copy(),
					new AOPAdd(
					new_base->copy(), 
					symbind_off((*it).second)->copy()));
			}
		}
	}

	bool loadArgs(const ptype_map& tm, const ArgsList* args);
	bool loadTypeBlock(
		const ptype_map& tm, 
		const Expr* base_expr,
		const TypeBlock* tb);

	const SymbolTable* getBackRef(void) const { return backref; }

	sym_map::const_iterator	begin(void) const { return sm.begin(); }
	sym_map::const_iterator end(void) const { return sm.end(); }
private:
	bool loadTypeStmt(
		const ptype_map&	tm,
		const Expr*		base_expr,
		const TypeStmt*		stmt);

	bool loadTypeCond(
		const ptype_map&	tm,
		const Expr*		base_expr,
		const TypeCond*		tc);



	void freeData(void)
	{
		sym_map::iterator	it;

		for (it = sm.begin(); it != sm.end(); it++) {
			sym_binding	sb((*it).second);

			delete symbind_phys(sb);
			delete symbind_off(sb);
		}

		sm.clear();

		if (owner != NULL) {
			delete owner;
			owner = NULL;
		}
	}

	const SymbolTable*	backref;
	PhysicalType*		owner;
	sym_map			sm;
};

class SymTabThunkBuilder : public TypeVisitAll
{
public:
	SymTabThunkBuilder(const ptype_map& ptypes) 
	: cur_symtab(NULL),
	  cur_type(NULL),
	  tm(ptypes),
	  ret_pt(NULL) {}

	virtual ~SymTabThunkBuilder() {}
	SymbolTable* getSymTab(const Type*, PhysicalType * &ptt);
	virtual void visit(const TypeDecl* td);
	virtual void visit(const TypeUnion* tu);
	virtual void visit(const TypeParamDecl*);
	virtual void visit(const TypeBlock* tb);
	virtual void visit(const TypeCond* tb);

private:
	void retType(PhysicalType* pt);
	void addToCurrentSymTabAnonType(
		const PhysicalType*	pt,
		const std::string&	field_name,
		const Id*		scalar,
		const IdArray*		array);

	void addToCurrentSymTab(
		const std::string& 	type_str,
		const std::string&	field_name,
		const Id*		scalar,
		const IdArray*		array,
		ExprList*		type_args);

	SymbolTable		*cur_symtab;
	const Type		*cur_type;
	const ptype_map		&tm;
	PhysicalType		*ret_pt;
};

class SymTabBuilder : public TypeVisitAll
{
public:
	SymTabBuilder(const ptype_map& ptypes) 
	: cur_symtab(NULL),
	  tm(ptypes),
	  off_thunked(NULL),
	  off_pure(NULL),
	  union_c(0),
	  ret_pt(NULL) {}

	virtual ~SymTabBuilder() {}
	SymbolTable* getSymTab(const Type*, PhysTypeUser* &ptt);
	virtual void visit(const TypeDecl* td);
	virtual void visit(const TypeUnion* tu);
	virtual void visit(const TypeParamDecl*);
	virtual void visit(const TypeBlock* tb);
	virtual void visit(const TypeCond* tb);
	virtual void visit(const TypeFunc* tf);

private:
	void updateOffset(Expr* e);
	void retType(PhysicalType* pt);

	SymbolTable		*cur_symtab;
	const Type		*cur_type;
	const ptype_map		&tm;
	Expr			*off_thunked;	/* uses thunk_off_arg */
	Expr			*off_pure;	/* does not use off_arg */
	unsigned int		union_c;
	PhysicalType		*ret_pt;
};


#endif
