#pragma once
namespace VGS
{
	// Generic 32 bit register
	struct Register
	{
		union
		{
			__int32				i;	// Integer usage
			unsigned __int32	u;	// Unsigned integer usage
			float				f;	// Floating point usage
		};
	};
}