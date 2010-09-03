#ifndef VIRTTYPE_H
#define VIRTTYPE_H

#include <list>
#include <map>
#include "collection.h"
#include "type.h"
#include "points_to.h"

typedef PtrList<class VirtualType>		virt_list;
typedef std::list<class VirtualTypes*>		typevirt_list;
typedef std::map<std::string, VirtualTypes*>	typevirt_map;

class VirtualTypes
{
public:
	VirtualTypes(const Type* t);
	virtual ~VirtualTypes(void) {}

	void genCode(void);
	void genProtos(void);

	const virt_list* getVirts(void) const { return &virts; }
	const Type* getType(void) const { return src_type; }
private:
	void loadVirtuals(void);
	VirtualType* loadVirtual(const Preamble* p);

	const Type*	src_type;
	virt_list	virts;
	unsigned int	seq;
};

class VirtualType : public PointsRange {
public:
	VirtualType(
		const Type*	in_src_type,	/* thunk context*/
		const Type*	in_dst_type,	/* type returned by type-expr */
		const Type	*in_virt_type,	/* type we map data to */
		Id*		in_binding,
		Expr*		in_min_expr,
		Expr*		in_max_expr,
		Expr*		in_lookup_expr,
		unsigned int	seq)
	: PointsRange(
		in_src_type,
		in_dst_type,
		in_binding,
		in_min_expr,
		in_max_expr,
		in_lookup_expr,
		seq),
	  v_type(in_virt_type)
	{
		assert (v_type != NULL);
	}

	virtual ~VirtualType(void) {}
	const Type* getTargetType(void) const { return v_type; }

	virtual const std::string getFCallName(void) const;
	virtual const std::string getMinFCallName(void) const;
	virtual const std::string getMaxFCallName(void) const;

private:
	const Type*	v_type;
};

#endif