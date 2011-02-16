#include "annotation.h"

Annotation::Annotation(const Type* t, const char* name)
: src_type(t), label(name), seq(0)
{
	assert (t != NULL);
}

void Annotation::genCode(void)
{
	const anno_ent_list	*l = getEntryList();

	if (l == NULL) return;
	iter_do(anno_ent_list, *l, genCode);
}

void Annotation::genProto(void)
{
	const anno_ent_list	*l = getEntryList();
	if (l == NULL) return;
	iter_do(anno_ent_list, *l, genProto);
}

void Annotation::loadByName(const char* n)
{
	preamble_list	l;

	if (n == NULL) n = label;

	l = src_type->getPreambles(n);
	for (preamble_list::const_iterator it = l.begin(); it != l.end(); it++)
		load(*it);
}

AnnotationEntry::AnnotationEntry(Annotation* in_parent, const Preamble* in_pre)
: parent(in_parent), pre(in_pre)
{
	seq = parent->getNewSeq();
}
