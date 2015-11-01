#pragma once

#include "MEM.h"
#include "CPU.h"
#include "GPU.h"
#include "APU.h"
#define PC_START	0x00 // 0x200

#define RAM_SIZE	0x1000000
#define GRAM_SIZE	0x800000
#define GBUF_SIZE	0x200000
#define ABUF_SIZE	0x4000

#define MS_MASK	0xF0000000
#define MS_RAM	0x00000000
#define MS_ROM	0x80000000
#define MS_GRAM	0x40000000
#define MS_GBUF	0x50000000
#define MS_ABUF	0x20000000

namespace VGS
{
	class System
	{
		bool bRun;

	public: // PRETEND PRIVATE IN MOST CASES
		unsigned __int64 cycle;

		CPU	vCPU;	// Virtual CPU
		GPU	vGPU;	// Virtual GPU
		APU	vAPU;	// Virtual APU

		MEM vRAM;	// Random Access Memory (CPU)
		MEM vROM;	// Read Only Memory	
		MEM vGRAM;	// Random Access Memory (GPU)
		MEM vGBUF;	// GPU Frame buffer x2	(GPU)
		MEM vABUF;	// APU Frame buffer x2	(APU)
	public:
		System(void * const, unsigned __int32 const);
		~System();



		void Run();
	public:
		template <typename T = __int32> inline T	GetMem(unsigned __int32 const);
		template <typename T = __int32> inline T*	GetPtr(unsigned __int32 const);
		template <typename T = __int32> inline void	SetMem(unsigned __int32 const, T const);

		void Crash() {}
		void SystemCall(unsigned __int32);
	};

	
	// Get a value from some memory bank at an offset
	template <typename T> T* System::GetPtr(unsigned __int32 const offset)
	{
		switch (offset & MS_MASK)
		{
		case MS_ROM:
			return vROM.GetPtr<T>(offset & (~MS_MASK));
			break;
		case MS_RAM:
			return vRAM.GetPtr<T>(offset & (~MS_MASK));
			break;
		case MS_GRAM:
			return vGRAM.GetPtr<T>(offset & (~MS_MASK));
			break;
		case MS_GBUF:
			return vGBUF.GetPtr<T>(offset & (~MS_MASK));
			break;
		case MS_ABUF:
			return vABUF.GetPtr<T>(offset & (~MS_MASK));
			break;
		default:
			return 0; // Crash();	// Hmmm... you shouldn't be here
			break;
		}
	}

	// Get a value from some memory bank at an offset
	template <typename T> T System::GetMem(unsigned __int32 const offset)
	{
		switch (offset & MS_MASK)
		{
		case MS_ROM:
			return vROM.Get<T>(offset & (~MS_MASK));
			break;
		case MS_RAM:
			return vRAM.Get<T>(offset & (~MS_MASK));
			break;
		case MS_GRAM:
			return vGRAM.Get<T>(offset & (~MS_MASK));
			break;
		case MS_GBUF:
			return vGBUF.Get<T>(offset & (~MS_MASK));
			break;
		case MS_ABUF:
			return vABUF.Get<T>(offset & (~MS_MASK));
			break;
		default:
			return 0; // Crash();	// Hmmm... you shouldn't be here
			break;
		}
	}

	// Store a 32bit value in some memory bank at an offset
	template <typename T> void System::SetMem(unsigned __int32 const offset, T const value)
	{
		switch (offset & MS_MASK)
		{
		case MS_RAM:
			vRAM.Set<T>(offset & (~MS_MASK), value);
			break;
		case MS_ROM: // Don't think you should be writing to ROM, but OK
			vROM.Set<T>(offset & (~MS_MASK), value);
			break;	
		case MS_GRAM:
			vGRAM.Set<T>(offset & (~MS_MASK), value);
			break;
		case MS_GBUF:
			vGBUF.Set<T>(offset & (~MS_MASK), value);
			break;
		case MS_ABUF:
			vABUF.Set<T>(offset & (~MS_MASK), value);
			break;
		default:
			Crash(); // Hmmm... you shouldn't be here
			break;
		}
	}
}