#include "MEM.h"

namespace VGS
{
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

}