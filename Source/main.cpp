#include <fstream>
#include <iostream>

#include "System.h"
#include "Loader.h"
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

	VGS::System * pSys = new VGS::System(nullptr, 0);
	VGS::Loader loader;

	if (loader.LoadELF(pSys, "Out.elf"))
	{
		std::cout << std::endl << "running..." << std::endl;
		pSys->Run();
	}

	return 0;
}