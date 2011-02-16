#ifndef POINTS_TO_H
#define POINTS_TO_H

#include <list>
#include <map>
#include "collection.h"
#include "type.h"
#include "table_gen.h"
#include "instance_iter.h"
#include "annotation.h"

typedef std::list<class Points*>	pointing_list;
typedef std::map<std::string, Points*>	pointing_map;
typedef PtrList<class PointsRange>	pr_list;

class Points : public TableWriter, public Annotation
{
public:
	Points(const Type* t);
	virtual ~Points() {}

	unsigned int getNumPointing(void) const { return p_elems_all.size(); }
	virtual const anno_ent_list* getEntryList(void) const
		{ return (const anno_ent_list*)&p_elems_all; }

	virtual void genExterns(TableGen* tg);
	virtual void genTables(TableGen* tg);

protected:
	virtual void load(const Preamble* p);

private:
	void loadPoints(const Preamble*);
	void loadPointsRange(const Preamble*);
	void loadPointsIf(const Preamble*);
	void loadPointsCast(const Preamble*);
	void loadPointsRangeCast(const Preamble*);

	void loadPointsInstance(
		const Preamble* pre,
		const Type* dst_type,
		ExprList* dst_params,
		const Expr* data_loc);
	void loadPointsInstance(const Preamble* pre, const Expr* data_loc);
	void loadPointsIfInstance(
		const Preamble* pre,
		const CondExpr* cond_expr,
		const Expr*	data_loc);
	void loadPointsRangeInstance(const Preamble* pre, InstanceIter*	iter);

	pr_list			p_elems_all;
};

class PointsRange : public AnnotationEntry
{
public:
	PointsRange(Annotation* p, const Preamble* pre, InstanceIter* in_iter);

	virtual ~PointsRange() { delete iter; }

	virtual void genCode(void) { return iter->genCode(); }
	virtual void genProto(void) { return iter->genProto(); }

	const InstanceIter* getInstanceIter(void) const { return iter; }
	void genTableInstance(TableGen* tg) const;
	void genExterns(TableGen* tg) const;
protected:
	void setIter(InstanceIter* iter);
	PointsRange(Annotation* p, const Preamble* pre);
	InstanceIter	*iter;
};

class PointsIf : public PointsRange
{
public:
	PointsIf(
		Annotation*	in_parent,
		const Preamble	*in_pre,
		const Type*	in_dst_type,
		CondExpr*	in_cond_expr,
		Expr*		in_points_expr);
	virtual ~PointsIf();

	virtual void genCode(void);
	virtual void genProto(void);
private:
	const std::string getWrapperFCallName(void) const;
	CondExpr	*cond_expr;
};

#endif
