#ifndef VIRTTYPE_H
#define VIRTTYPE_H

#include <list>
#include <map>
#include "collection.h"
#include "type.h"
#include "points.h"

typedef PtrList<class VirtualType>		virt_list;
typedef std::list<class VirtualTypes*>		typevirt_list;
typedef std::map<std::string, VirtualTypes*>	typevirt_map;

class VirtualTypes : public TableWriter, public Annotation
{
public:
	VirtualTypes(const Type* t);
	virtual ~VirtualTypes(void) {}

	unsigned int getNumVirts(void) const { return virts.size(); }
	virtual const anno_ent_list* getEntryList(void) const
		{ return (const anno_ent_list*)&virts; }

	virtual void genExterns(TableGen* tg);
	virtual void genTables(TableGen* tg);
protected:
	void load(const Preamble* pre);
private:
	VirtualType* loadVirtual(const Preamble* p, bool conditional);

	virt_list	virts;
};

class VirtualType : public PointsRange
{
public:
	VirtualType(
		Annotation	*in_parent,
		const Preamble	*in_pre,
		InstanceIter	*in_iter,
		const Type	*in_virt_type /* type we map data to */)
	: PointsRange(in_parent, in_pre, in_iter),
	  v_type(in_virt_type)
	{
		assert (v_type != NULL);
		iter->setPrefix(iter->getPrefix() + "_virt");
	}

	virtual ~VirtualType(void) {}
	const Type* getTargetType(void) const { return v_type; }
	void genInstance(TableGen* tg) const;
protected:
	const Type*	v_type;
};

class VirtualIf : public VirtualType
{
public:
	VirtualIf(
		Annotation	*in_parent,
		const Preamble	*in_pre,
		InstanceIter	*in_iter,
		CondExpr	*in_cond,
		const Type	*in_virt_type);

	virtual ~VirtualIf(void);
	virtual void genCode(void);
	virtual void genProto(void);

private:
	const std::string getWrapperFCallName(void) const;

	CondExpr	*cond;
	Expr		*true_min_expr;
	Expr		*false_min_expr;
};

#endif