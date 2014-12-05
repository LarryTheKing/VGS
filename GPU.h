#pragma once

//#include "System.h"

namespace VGS
{
	class System;

	class GPU
	{
		System *			pSystem;
		unsigned __int32	cycle;

		__int32 stage;

	public:
		GPU(System * const pS) :
			pSystem(pS) {}
	};

}