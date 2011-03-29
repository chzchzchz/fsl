#ifndef REPAIRS_H
#define REPAIRS_H

#include "expr.h"
#include "writepkt.h"
#include "annotation.h"
#include "table_gen.h"

typedef PtrList<class Repairs>			repair_list;
typedef std::map<std::string, class Repairs*>	repair_map;
typedef PtrList<class RepairEnt>		repairent_list;

class Repairs : TableWriter, public Annotation
{
public:
	Repairs(const class Type* t);
	virtual ~Repairs() {}

	virtual void genExterns(TableGen* tg);
	virtual void genTables(TableGen* tg);

	unsigned int getNumRepairs(void) const { return repairs_l.size(); }
	virtual const anno_ent_list* getEntryList(void) const
		{ return (const anno_ent_list*)&repairs_l; }
protected:
	virtual void load(const Preamble* p);
private:
	repairent_list	repairs_l;
};

class RepairEnt : public AnnotationEntry
{
public:
	RepairEnt(
		Repairs* parent, const Preamble* in_pre,
		const CondExpr* ce, WritePktInstance* in_wpkt);
	virtual ~RepairEnt(void);

	void genCode(void);
	void genProto(void);

	void genExterns(TableGen*) const;
	void genTableInstance(TableGen*) const;
	const WritePktInstance* getRepair(void) const { return repair_wpkt; }
private:
	void genCondProto(void) const;
	void genCondCode(void) const;

	std::string getCondFuncName(void) const;
	const CondExpr		*cond_expr;
	WritePktInstance	*repair_wpkt;

};

#endif
