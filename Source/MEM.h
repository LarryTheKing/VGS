#pragma once

#include <cstdlib>

namespace VGS
{
	class MEM
	{
	private:

	public:
		unsigned __int32	size;	// Mem size
		__int8 *			pData;	// Internal Memory, please don't change
		
	public:
		MEM(unsigned __int32 const);
		MEM(void * const, unsigned __int32 const);
		~MEM(void);

		// Gets some data from memory at an ofset; Returns 0 on out of bounds
		template <typename T = __int32> inline T	Get(unsigned __int32 const);

		// Stores some data in memory at an offset
		template <typename T = __int32>	inline void Set(unsigned __int32 const, T const);
	};

	template <typename T = __int32> T MEM::Get(unsigned __int32 const offset)
	{
		return(offset < size ? *reinterpret_cast<T*>(pData + offset) : 0);
	}

	template <typename T = __int32> void MEM::Set(unsigned __int32 const offset, T const value)
	{
		if (offset < size)
			* reinterpret_cast<T*>(pData + offset) = value;
	}

}