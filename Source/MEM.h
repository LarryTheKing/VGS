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

		template <typename T = __int32> T	Get(unsigned __int32 const offset)
		{	return(offset < size ? *reinterpret_cast<T*>(pData + offset) : 0);	}

		template <typename T = __int32>	T*	Set(unsigned __int32 const offset)
		{	if (offset < size)
				return reinterpret_cast<T*>(pData + offset)	}
		
		__int8	Get8	(unsigned __int32 const);
		__int16 Get16	(unsigned __int32 const);
		__int32	Get32	(unsigned __int32 const);

		void	Set8	(unsigned __int32 const, __int8 const);
		void	Set16	(unsigned __int32 const, __int16 const);
		void	Set32	(unsigned __int32 const, __int32 const);
	};

	// Allocates a block of memory s bytes in size
	MEM::MEM(unsigned __int32 const s)	
		:size(s), pData(nullptr)
	{
		if (size)
			pData = reinterpret_cast<char*>(malloc(size));
	}

	// Wraps a block of memory, pD, s bytes in size
	MEM::MEM(void * const pD, unsigned __int32 const s)
		:size(s), pData(reinterpret_cast<__int8*>(pD))	{}

	// Frees the previously allocated memory
	MEM::~MEM(void)
	{
		free(pData);
	}

	// Returns the int8 stored at an offset; Returns 0 on out of bounds
	__int8 MEM::Get8(unsigned __int32 const offset)
	{
		return(offset < size ? pData[offset] : 0);
	}

	// Returns the int16 stored at an offset; Returns 0 on out of bounds
	__int16 MEM::Get16(unsigned __int32 const offset)
	{
		return(offset < size ? *reinterpret_cast<__int16*>(pData + offset) : 0);
	}

	// Returns the int32 stored at an offset; Returns 0 on out of bounds
	__int32 MEM::Get32(unsigned __int32 const offset)
	{
		return(offset < size ? * reinterpret_cast<__int32*>(pData + offset) : 0);
	}

	// Stores an int32 in memory at an offset
	void	MEM::Set32(unsigned __int32 const offset, __int32 const value)
	{
		if (offset < size)
			* reinterpret_cast<__int32*>(pData + offset) = value;
	}
}