#pragma once

#include "Stack.h"

namespace VGS
{
	namespace Compiler
	{
		template <class T> struct Offset
		{
		public:
			size_t				offset;
			DynamicStackAlloc *	pStack;

		public:
			Offset() : offset(0), pStack(nullptr) {}
			Offset(T* pAbs, DynamicStackAlloc *	pStack)
				:pStack(pStack)
			{
				offset = reinterpret_cast<char*>(pAbs)-pStack->Begin();
			}

			T* get(void){ return reinterpret_cast<T*>(pStack->Begin() + offset); }

			T&			operator*()			{ return *get(); }
			const T&	operator*() const	{ return *get(); }
			T*			operator->()		{ return get(); }
			const T*	operator->()		{ return get(); }
		};
	}
}