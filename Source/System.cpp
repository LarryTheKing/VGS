#include "System.h"

namespace VGS
{
	System::System(void * const pROM, unsigned __int32 const size)
		: vCPU(this), vGPU(this), vAPU(this),
		vROM(pROM, size), vRAM(RAM_SIZE), vGRAM(GRAM_SIZE), vGBUF(GBUF_SIZE), vABUF(ABUF_SIZE) 
	{
		vCPU.PC.u = MS_ROM + PC_START;
	}
}