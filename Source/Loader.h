#pragma once

#include "System.h"

namespace VGS
{
	class Loader
	{
	public:
		bool LoadELF(System * const, char const * const);
	};
	
}