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
	unsigned int getNumVirts(void) const { return virts.size(); }
private:
	void loadVirtuals(bool conditional);
	VirtualType* loadVirtual(const Preamble* p, bool conditional);

	const Type*	src_type;
	virt_list	virts;
	unsigned int	seq;
};

class VirtualType : public PointsRange
{
public:
	VirtualType(
		InstanceIter	*in_iter,
		const Type	*in_virt_type,	/* type we map data to */
		Id*		in_name,
		unsigned int	seq)
	: PointsRange(in_iter, in_name, seq),
	  v_type(in_virt_type)
	{
		assert (v_type != NULL);
		iter->setPrefix(iter->getPrefix() + "_virt");
	}

	virtual ~VirtualType(void) {}
	const Type* getTargetType(void) const { return v_type; }

protected:
	const Type*	v_type;
};

class VirtualIf : public VirtualType
{
public:
	VirtualIf(
		InstanceIter	*in_iter,
		CondExpr	*in_cond,
		const Type	*in_virt_type,
		Id		*in_name,
		unsigned int	seq);

	virtual ~VirtualIf(void);
	virtual void genCode(void) const;
	virtual void genProto(void) const;

private:
	const std::string getWrapperFCallName(void) const;

	CondExpr	*cond;
	Expr		*true_min_expr;
	Expr		*false_min_expr;
};

#endif