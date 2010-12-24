#ifndef POINTS_TO_H
#define POINTS_TO_H

#include <list>
#include <map>
#include "collection.h"
#include "type.h"
#include "table_gen.h"
#include "instance_iter.h"

typedef PtrList<class PointsRange>	pointsrange_list;
typedef pointsrange_list		pointsto_list;
typedef std::list<class Points*>	pointing_list;
typedef std::map<std::string, Points*>	pointing_map;
typedef std::list<class PointsRange*>	pr_list;

class Points : public TableWriter
{
public:
	Points(const Type* t);
	virtual ~Points() {}

	const pointsto_list* getPointsTo(void) const
	{
		return &points_to_elems;
	}

	const pointsrange_list* getPointsRange(void) const
	{
		return &points_range_elems;
	}

	const Type* getType(void) const { return src_type; }

	unsigned int getNumPointing(void) const
	{
		return	points_to_elems.size() +
			points_range_elems.size();
	}

	void genCode(void);
	void genProtos(void);
	virtual void genExterns(TableGen* tg);
	virtual void genTables(TableGen* tg);

private:
	void loadPoints(void);
	void loadPointsRange(void);
	void loadPointsIf(void);
	void loadPointsCast(void);
	void loadPointsRangeCast(void);

	void loadPointsInstance(
		const Type* dst_type,
		const Expr* data_loc,
		const Id* as_name);
	void loadPointsInstance(
		const Expr* data_loc,
		const Id* as_name);
	void loadPointsIfInstance(
		const CondExpr* cond_expr,
		const Expr*	data_loc,
		const Id*	as_name);
	void loadPointsRangeInstance(
		InstanceIter*	iter,
		const Id*	as_name);

	const Type*		src_type;
	pointsto_list		points_to_elems;
	pointsrange_list	points_range_elems;
	pr_list			p_elems_all;
};

class PointsRange
{
public:
	PointsRange(InstanceIter* in_iter, Id* in_name, unsigned int in_seq);

	virtual ~PointsRange()
	{
		delete iter;
		if (name != NULL) delete name;
	}

	virtual void genCode(void) const { return iter->genCode(); }
	virtual void genProto(void) const { return iter->genProto(); }

	const InstanceIter* getInstanceIter(void) const { return iter; }
	Id* getName(void) const { return name; }
	void genTableInstance(TableGen* tg) const;
	void genExterns(TableGen* tg) const;
protected:
	InstanceIter	*iter;
	unsigned int getSeqNum(void) const { return seq; }

private:
	Id		*name;
	unsigned int	seq;
};

class PointsIf : public PointsRange
{
public:
	PointsIf(
		const Type*	in_src_type,
		const Type*	in_dst_type,
		CondExpr*	in_cond_expr,
		Expr*		in_points_expr,
		Id*		in_name,
		unsigned int	in_seq);
	virtual ~PointsIf();

	virtual void genCode(void) const;
	virtual void genProto(void) const;
private:
	const std::string getWrapperFCallName(
		const std::string& type_name,
		unsigned int s) const;

	CondExpr	*cond_expr;
};

#endif
