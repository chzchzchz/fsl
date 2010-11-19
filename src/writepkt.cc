#include "writepkt.h"

WritePktStmt::~WritePktStmt()
{
	if (e != NULL) delete e;
	if (wpb != NULL) delete wpb;
}

void WritePktStmt::printParams(std::ostream& out) const
{
	if (e != NULL) e->print(out);
	if (wpb != NULL) wpb->print(out);
}

void WritePktId::print(std::ostream& out) const
{
	out << "(writepkt-id ";
	id->print(out);
	out << ' ';
	printParams(out);
	out << ")";
}

void WritePktStruct::print(std::ostream& out) const
{
	out << "(writepkt-ids ";
	ids->print(out);
	out << ' ';
	printParams(out);
	out << ")";
}

void WritePktArray::print(std::ostream& out) const
{
	out << "(writepkt-array ";
	a->print(out);
	out << ' ';
	printParams(out);
	out << ")";
}


void WritePktBlk::print(std::ostream& out) const
{
	out << "(writepkt-blk ";
	for (const_iterator it = begin(); it != end(); it++) {
		(*it)->print(out);
		out << "\n";
	}
	out << ")";
}
