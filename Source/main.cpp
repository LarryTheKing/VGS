#include <fstream>

#include "System.h"
#include "Compiler/Compiler.h"

int main()
{
	VGS::Compiler::Compiler comp;
	comp.Compile("Test.asm");
	
	std::ifstream ifile("Test.vgs", std::ios::in | std::ios::binary | std::ios::ate);
	if (!ifile.is_open())
		return 1;
	
	int fileSize = ifile.tellg();
	char * pData = (char*)malloc(fileSize);
	ifile.seekg(0, std::ios::beg);
	ifile.read(pData, fileSize);
	ifile.close();


	VGS::System sys(pData, fileSize);
	sys.Run();

	free(pData);
	return 0;
}