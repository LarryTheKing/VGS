#include "CPU.h"
#include "System.h"

namespace VGS
{
	CPU::CPU(System * const pS) :	pSystem(pS) 
	{
		// Zero registers
		for (int i = 0; i < 0x20; i++)
			reg[i].u = 0u;

		// Set SP and FP to the address of last 4 bytes of RAM
		SP.u = FP.u= pS->vRAM.size - 4;
	}

	bool CPU::Cycle()
	{
		return true;
	}
}