#include <fstream>

#include "System.h"
#include "Compiler/Builder.h"
#include "Compiler/Linker.h"


int main()
{
	
	VGS::Compiler::Builder builder;
	builder.Build("Test.asm", "Test.elf");
	builder.Build("pread.asm", "pread.elf");

	const char * files[] = { "Test.elf", "pread.elf" };

	VGS::Compiler::Linker linker;
	linker.Link(files, 2, "Out.elf");

	return 0;
}