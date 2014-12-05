#pragma once

//#include "System.h"

#define FLAG_H		0x1
#define FLAG_L		0x2
#define	FLAG_I		0x4
#define FLAG_ID		0x8

namespace VGS {

	class System;

	struct Register
	{
		union
		{
			__int32				i;	// Integer usage
			unsigned __int32	u;	// Unsigned integer usage
			float				f;	// Floating point usage
		};
	};

	class CPU
	{
	public:
		System * const		pSystem;
		unsigned __int32	cycle;

		// Registers
		union {
			Register reg[0x40];
			struct {
				Register T[0xF];	// 0x00 :: 0x0F	// Temporary	: Global, any use
				Register S[0xF];	// 0x10 :: 0x1F	// Subroutine	: Preserved for sub
				Register P[0x8];	// 0x20 :: 0x27	// Parameters	: Preserved for sub
				Register A[0x8];	// 0x28 :: 0x2F	// Math/Return	: Preserved for caller

				Register C;			// 0x30	// Program counter
				Register R;			// 0x31	// Interupt return
				Register F;			// 0x32	// Flags
			};
		};

	public:
		CPU(System * const pS) :
			pSystem(pS) {}
		bool Cycle();
	};
}