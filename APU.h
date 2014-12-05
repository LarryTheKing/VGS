#pragma once

//#include "System.h"

namespace VGS
{
	class System;

	class APU
	{
		System *			pSystem;
		unsigned __int32	cycle;

	public:
		APU(System * const pS) :
			pSystem(pS) {}
	};
}
