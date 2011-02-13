#ifndef FSLTOOL_H
#define FSLTOOL_H

#ifndef __KERNEL__
#define TOOL_ENTRY(x)	int tool_entry(int argc, char* argv[])
#else
#define TOOL_ENTRY(x)	int x##_entry(int argc, char* argv[])
#endif

#endif
