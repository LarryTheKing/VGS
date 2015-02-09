#pragma once

#include "Stack.h"
#include "ELF.h"

namespace VGS
{
	namespace Compiler
	{
		class Linker
		{
		private:
			DynamicStackAlloc	s_text;		// .text
			DynamicStackAlloc	s_data;		// .data
			DynamicStackAlloc	s_rodata;	// .rodata
			unsigned __int32	s_bss;		// .bss

			DynamicStackAlloc	s_strtab;	// .strtab
			DynamicStackAlloc	s_symtab;	// .symtab
			DynamicStackAlloc	s_rela;		// .rela.text

		public:
			bool Link(char const * const [], unsigned __int32, char const * const);
			bool AddObject(const char * const);
		};
	}
}