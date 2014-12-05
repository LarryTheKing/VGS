#pragma once

#include "CPU.h"
#include "GPU.h"
#include "APU.h"
#include "MEM.h"

#define RAM_SIZE	0xFFFFFF
#define GRAM_SIZE	0x7FFFFF
#define GBUF_SIZE	0x1FFFFF
#define ABUF_SIZE	0x4000

#define MS_MASK	0xF0000000
#define MS_ROM	0x00000000
#define MS_RAM	0x80000000
#define MS_GRAM	0x40000000
#define MS_GBUF	0x50000000
#define MS_ABUF	0x20000000

namespace VGS
{
	class System
	{
	private:
		unsigned __int32 cycle;

		CPU	vCPU;	// Virtual CPU
		GPU	vGPU;	// Virtual GPU
		APU	vAPU;	// Virtual APU

		MEM vROM;	// Read Only Memory
		MEM vRAM;	// Random Access Memory (CPU)
		MEM vGRAM;	// Random Access Memory (GPU)
		MEM vGBUF;	// GPU Frame buffer x2	(GPU)
		MEM vABUF;	// APU Frame buffer x2	(APU)
	public:
		System(void * const, unsigned __int32 const);
		~System();

		void Run();
	public:
		__int32 GetMem(unsigned __int32 const);
		void SetMem(unsigned __int32 const, __int32 const);

		void Crash() {}
	};

	System::System(void * const pROM, unsigned __int32 const size)
		: vCPU(this), vGPU(this), vAPU(this),
		vROM(pROM, size), vRAM(RAM_SIZE), vGRAM(GRAM_SIZE), vGBUF(GBUF_SIZE), vABUF(ABUF_SIZE) {}

	// Get a 32bit value from some memory bank at an offset
	__int32	System::GetMem(unsigned __int32 const offset)
	{
		switch (offset & MS_MASK)
		{
		case MS_ROM:
			return vROM.Get32(offset & (~MS_MASK));
			break;
		case MS_RAM:
			return vRAM.Get32(offset & (~MS_MASK));
			break;
		case MS_GRAM:
			return vGRAM.Get32(offset & (~MS_MASK));
			break;
		case MS_GBUF:
			return vGBUF.Get32(offset & (~MS_MASK));
			break;
		case MS_ABUF:
			return vABUF.Get32(offset & (~MS_MASK));
			break;
		default:
			return 0; // Crash();	// Hmmm... you shouldn't be here
			break;
		}
	}

	// Store a 32bit value in some memory bank at an offset
	void	System::SetMem(unsigned __int32 const offset, __int32 const value)
	{
		switch (offset & MS_MASK)
		{
		case MS_ROM:
			vROM.Set32(offset & (~MS_MASK), value);
			break;
		case MS_RAM:
			vRAM.Set32(offset & (~MS_MASK), value);
			break;
		case MS_GRAM:
			vGRAM.Set32(offset & (~MS_MASK), value);
			break;
		case MS_GBUF:
			vGBUF.Set32(offset & (~MS_MASK), value);
			break;
		case MS_ABUF:
			vABUF.Set32(offset & (~MS_MASK), value);
			break;
		default:
			Crash(); // Hmmm... you shouldn't be here
			break;
		}
	}
}