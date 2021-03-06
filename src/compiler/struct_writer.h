#ifndef STRUCTWRITER_H
#define STRUCTWRITER_H
#include <iostream>
#include <string>
#include <assert.h>
#include <stdint.h>

class StructWriter
{
public:
	StructWriter(std::ostream& in_out)
	: out(in_out), first_elem(true), is_toplevel(false)
	{
		assert (out.good() == true);
		out << '{';
	}

	StructWriter(
		std::ostream& in_out,
		const std::string& type_name,
		const std::string& name,
		bool isStatic = false)
	: out(in_out), first_elem(true), is_toplevel(true)
	{
		assert (out.good() == true);
		if (isStatic) out << "static ";
		out << "const struct " << type_name << " " << name << " = {\n";
	}

	virtual ~StructWriter(void) { out << ((is_toplevel) ? "};\n" : "}\n"); }

	void writeStr(const std::string& fieldname, const std::string& val)
	{
		writeComma();
		out << '.' << fieldname << " = \"" << val << "\"";
	}

	void write32(const std::string& fieldname, uint32_t val)
	{
		writeComma();
		out << '.' << fieldname << "=(uint32_t)" << val;
	}

	void write(const std::string& fieldname, uint64_t val)
	{
		writeComma();
		out << '.' << fieldname << '=' << val << "UL";
	}

	void write(const std::string& fieldname, const std::string& val)
	{
		writeComma();
		out << '.' << fieldname << '=' << val;
	}

	void writeB(const std::string& fieldname, const bool is_true)
	{
		writeComma();
		out << '.' << fieldname << '=' << ((is_true) ? "true" : "false");
	}

	void write(const std::string& name)
	{
		writeComma();
		out << name;
	}

	void beginWrite(void) { writeComma(); }

private:
	void writeComma(void)
	{
		if (first_elem == false) out << ",\n";
		first_elem = false;
	}

	std::ostream&	out;
	bool		first_elem;
	bool		is_toplevel;
};

#endif
