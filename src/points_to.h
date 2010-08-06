#ifndef POINTS_TO_H
#define POINTS_TO_H

#include <list>
#include <map>
#include "collection.h"
#include "type.h"

typedef PtrList<class PointsTo>		pointsto_list;
typedef PtrList<class PointsRange>	pointsrange_list;
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

	void loadPointsInstance(const Expr* data_loc);
	void loadPointsRangeInstance(
		const Id*	bound_var,
		const Expr*	first_val,
		const Expr*	last_val,
		const Expr*	data);

	const Type*		src_type;
	pointsto_list		points_to_elems;
	pointsrange_list	points_range_elems;
	unsigned int		seq;
};

class PointsTo
{
public:
	PointsTo(
		const Type* in_src_type, 
		const Type* in_dst_type,
		const Expr* in_points_expr,
		unsigned int in_seq)
	: src_type(in_src_type),
	  dst_type(in_dst_type),
	  points_expr(in_points_expr->copy()),
	  seq(in_seq)
	{
		assert (src_type != NULL);
		assert (dst_type != NULL);
	}

	virtual ~PointsTo() 
	{
		delete points_expr;
	}

	virtual void genCode(void) const;
	virtual void genProto(void) const;

	const Type* getSrcType(void) const { return src_type; }
	const Type* getDstType(void) const { return dst_type; }

	const std::string getFCallName(void) const;

private:
	const Type*	src_type;
	const Type*	dst_type;
	Expr*		points_expr;
	unsigned int	seq;
};

class PointsRange 
{
public:
	PointsRange(
		const Type*	in_src_type, 
		const Type*	in_dst_type,
		const Id*	in_binding,
		const Expr*	in_min_expr,
		const Expr*	in_max_expr,
		const Expr*	in_points_expr,
		unsigned int	in_seq)
	: src_type(in_src_type),
	  dst_type(in_dst_type),
	  binding(in_binding->copy()),
	  min_expr(in_min_expr->copy()),
	  max_expr(in_max_expr->copy()),
	  points_expr(in_points_expr->copy()),
	  seq(in_seq)
	{
		assert (src_type != NULL);
		assert (dst_type != NULL);
	}

	virtual ~PointsRange() 
	{
		delete binding;
		delete min_expr;
		delete max_expr;
		delete points_expr;
	}

	virtual void genCode(void) const;
	virtual void genProto(void) const;

	const Type* getSrcType(void) const { return src_type; }
	const Type* getDstType(void) const { return dst_type; }

	const std::string getFCallName(void) const;
	const std::string getMinFCallName(void) const;
	const std::string getMaxFCallName(void) const;
private:

	const Type*	src_type;
	const Type*	dst_type;
	Id*		binding;
	Expr*		points_expr;
	Expr*		max_expr;
	Expr*		min_expr;
	unsigned int	seq;
};

#endif
