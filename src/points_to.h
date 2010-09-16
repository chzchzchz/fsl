#ifndef POINTS_TO_H
#define POINTS_TO_H

#include <list>
#include <map>
#include "collection.h"
#include "type.h"

typedef PtrList<class PointsRange>	pointsrange_list;
typedef pointsrange_list		pointsto_list;
typedef std::list<class Points*>	pointing_list;
typedef std::map<std::string, Points*>	pointing_map;

class Points
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

private:
	void loadPoints(void);
	void loadPointsRange(void);
	void loadPointsIf(void);

	void loadPointsInstance(
		const Expr* data_loc,
		const Id* as_name);
	void loadPointsIfInstance(
		const CondExpr* cond_expr,
		const Expr*	data_loc,
		const Id*	as_name);

	void loadPointsRangeInstance(
		const Id*	bound_var,
		const Expr*	first_val,
		const Expr*	last_val,
		const Expr*	data,
		const Id*	as_name);

	const Type*		src_type;
	pointsto_list		points_to_elems;
	pointsrange_list	points_range_elems;
};

class PointsRange 
{
public:
	PointsRange(
		const Type*	in_src_type, 
		const Type*	in_dst_type,
		Id*		in_binding,
		Expr*		in_min_expr,
		Expr*		in_max_expr,
		Expr*		in_points_expr,
		Id*		in_name,
		unsigned int	in_seq)
	: src_type(in_src_type),
	  dst_type(in_dst_type),
	  binding(in_binding),
	  max_expr(in_max_expr),
	  min_expr(in_min_expr),
	  points_expr(in_points_expr),
	  name(in_name),
	  seq(in_seq)
	{
		assert (src_type != NULL);
		assert (dst_type != NULL);
		assert (min_expr != NULL);
		assert (max_expr != NULL);
	}

	virtual ~PointsRange() 
	{
		delete binding;
		delete min_expr;
		delete max_expr;
		delete points_expr;
		if (name != NULL) delete name;
	}

	virtual void genCode(void) const;
	virtual void genProto(void) const;

	const Type* getSrcType(void) const { return src_type; }
	const Type* getDstType(void) const { return dst_type; }

	virtual const std::string getFCallName(void) const;
	virtual const std::string getMinFCallName(void) const;
	virtual const std::string getMaxFCallName(void) const;
	Id* getName(void) const { return name; }

private:
	void genCodeRange(void) const; 

	const Type*	src_type;
	const Type*	dst_type;
	Id*		binding;
	Expr*		max_expr;
	Expr*		min_expr;
	Expr*		points_expr;
	Id*		name;
	unsigned int	seq;
protected:
	unsigned int getSeqNum(void) const { return seq; }
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
