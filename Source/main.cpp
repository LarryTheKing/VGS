#include <fstream>

#include "System.h"
#include "Compiler/Compiler.h"

int main()
{

	int t = strtol("cat", nullptr, 0);
	VGS::Compiler::Compiler comp;

	char * pComp = nullptr;
	size_t cSize = 0;

	pComp = comp.Compile("Test.asm", cSize);
	//
	//std::ifstream ifile("Test.vgs", std::ios::in | std::ios::binary | std::ios::ate);
	//if (!ifile.is_open())
	//	return 1;
	//
	//int fileSize = ifile.tellg();
	//char * pData = (char*)malloc(fileSize);
	//ifile.seekg(0, std::ios::beg);
	//ifile.read(pData, fileSize);
	//ifile.close();


	VGS::System sys(pComp, cSize);
	sys.Run();

	//free(pData);
	return 0;
}