#pragma once

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
		unsigned __int32	PC;

		// Registers
		union {
			Register reg[0x20];
			struct {
				Register R0;		// 0x00 // Zero Always
				Register R1;		// 0x01 // ??
				Register V[0x2];	// 0x02 :: 0x03 // Math-Return
				Register A[0x4];	// 0x04 :: 0x07 // Arguments
				Register T[0x8];	// 0x08 :: 0x0F // Temporary
				Register S[0x8];	// 0x10 :: 0x17 // Subroutine
				Register T8;		// 0x18 //
				Register T9;		// 0x19 //
				Register K[0x2];	// 0x1A :: 0x1B // Kernal
				
				Register GP;		// 0x1C	// Global Pointer
				Register SP;		// 0x1D	// Stack Pointer
				Register FP;		// 0x1E	// Frame Pointer
				Register RA;		// 0x1F // Return Address
			};
		};

	public:
		CPU(System * const);
		
		bool Cycle();
	};
}