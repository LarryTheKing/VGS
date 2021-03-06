#include "System.h"
#include <stdio.h>
#include <iostream>

namespace VGS
{
	System::System(void * const pROM, unsigned __int32 const size)
		: cycle(0), bRun(true), vCPU(this), vGPU(this), vAPU(this),
		vROM(pROM, size), vRAM(RAM_SIZE), vGRAM(GRAM_SIZE), vGBUF(GBUF_SIZE), vABUF(ABUF_SIZE) 
	{
		vCPU.PC.u = MS_ROM + PC_START;
	}

	System::~System(void)
	{
		// Remember, the system doesn't own the ROM data
		vROM.pData = nullptr;
	}

	// Temporary System Calls
	void System::SystemCall(unsigned __int32 op)
	{
		switch (op)
		{
#pragma region TESTING
		case 1 :	// Print integer
			std::cout << vCPU.A[0].i;
			break;
		case 2 :	// Print float
			std::cout << vCPU.A[0].f;
			break;
		// case 3 : // Prints double ?
		case 4:		// Print string
			std::cout << GetPtr<char>(vCPU.A[0].u);
			break;
		case 5 :	// Get integer	
			std::cin >> vCPU.V[0].i;
			break;
		// case 6 :	// Gets float
		// case 7 :	// Gets double
		case 8 :	// Read string
			std::cin.getline(vRAM.pData + vCPU.A[0].u, vCPU.A[1].u);
			break;
		case 10:
			bRun = false;
			break;
#pragma endregion
#pragma region INPUT

#pragma endregion
		}
	}

	void System::Run()
	{
		while (bRun)
		{
			cycle++;
			vCPU.Cycle();		
		}
	}
}