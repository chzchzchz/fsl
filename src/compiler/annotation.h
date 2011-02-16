#ifndef ANNOTATION_H
#define ANNOTATION_H

#include <string>

#include "type.h"
#include "expr.h"
#include "preamble.h"
#include "collection.h"
#include "util.h"

typedef PtrList<class AnnotationEntry>		anno_ent_list;

class Annotation
{
public:
	Annotation(const Type* t, const char* name);
	virtual ~Annotation() {}
	const Type* getType(void) const { return src_type; }
	virtual void genCode(void);
	virtual void genProto(void);
	virtual const anno_ent_list* getEntryList(void) const { return NULL; }
	unsigned int getNewSeq(void) { return seq++; }
	unsigned int getSeq(void) const { return seq; }
	const char* getLabel(void) const { return label; }
protected:
	void loadByName(const char* c = NULL);
	virtual void load(const Preamble* p) = 0;
	const Type	*src_type;
	const char	*label;		/* (e.g. asserts, points_to, ...) */
private:
	Annotation(void) {}
	unsigned int		seq;

};

class AnnotationEntry
{
public:
	AnnotationEntry(Annotation* parent, const Preamble* p);
	virtual ~AnnotationEntry() {}
	virtual void genCode(void) = 0;
	virtual void genProto(void) = 0;
	const Id* getName(void) const { return pre->getAddressableName(); }
	unsigned int getSeqNum(void) const { return seq; }

	virtual const std::string getFCallName(void) const
	{
		return 	"__" + std::string(parent->getLabel()) + "_" +
			parent->getType()->getName() + "_" +
			int_to_string(seq);
	}

protected:
	AnnotationEntry();

	Annotation	*parent;
	const Preamble	*pre;
	unsigned int	seq;
};

#endif
