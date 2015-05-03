#include <fstream>

#include "System.h"
#include "Compiler/Compiler.h"
#include "Compiler/Linker.h"
#include "Compiler/Builder.h"

int main()
{
	
	VGS::Compiler::Builder builder("Language.txt");
	builder.Build("Test.asm", "Test.elf");

	const char * files[] = { "Test.elf", "3" };

	VGS::Compiler::Linker linker;
	linker.Link(files, 1, "Out.elf");

	VGS::Compiler::Compiler comp;

	char * pComp = nullptr;
	size_t cSize = 0;

	pComp = comp.Compile("Test.asm", cSize);

	VGS::System sys(pComp, cSize);
	sys.Run();

	//free(pData);
	return 0;
}