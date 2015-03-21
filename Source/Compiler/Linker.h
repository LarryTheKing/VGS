#pragma once

#include "Stack.h"
#include "ELF.h"
#include "Offset.h"

namespace VGS
{
	namespace Compiler
	{
		class Linker
		{
		private:
			DynamicStackAlloc	s_text;		// .text		a.k.a ROM
			DynamicStackAlloc	s_data;		// .data		a.k.a RAM

			DynamicStackAlloc	s_strtab;	// .strtab		For all the strings
			DynamicStackAlloc	s_symtab;	// .symtab		For global symbols
			DynamicStackAlloc	s_rela;		// .rela.text	For non-local references

		public:
			bool Link(char const * const [], unsigned __int32, char const * const);
			bool AddObject(const char * const);

		private:
			bool AddSections(char * const);
			bool LinkObject(char * const);
		};
	}
}